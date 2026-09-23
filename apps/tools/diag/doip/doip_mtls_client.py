# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""Connect to a DoIP server with no TLS, TLS or mTLS, and optionally exchange UDS.

Pick the security mode with --mode:

    none    plain DoIP, TCP port 13400, nothing encrypted
    tls     TLS on port 3496; the ECU is authenticated, we are not
    mtls     TLS on port 3496; both sides present certificates (the default)

Uses the certificates in ./certs (see generate_certs.py) and defaults to the DoIP
server at 192.168.225.1, so a first connection is just:

    python doip_mtls_client.py                       # mTLS
    python doip_mtls_client.py --mode tls            # TLS only
    python doip_mtls_client.py --mode none           # plain DoIP on 13400

Add --uds to send diagnostic requests once the session is up, and --ecu-ip to
reach a different server:

    python doip_mtls_client.py --uds 1003 --uds 22F190
    python doip_mtls_client.py --ecu-ip 127.0.0.1 --uds 22F190

In mtls mode the ECU must trust ca.crt for client certificates, and (unless
--insecure is used) present a certificate signed by that same CA.
"""

import argparse
import logging
import pathlib
import socket
import ssl
import sys

# Allow running straight from a checkout, without pip-installing doipclient
_REPO_ROOT = pathlib.Path(__file__).resolve().parent.parent
if str(_REPO_ROOT) not in sys.path:
    sys.path.insert(0, str(_REPO_ROOT))

from doipclient import DoIPClient
from doipclient.constants import TCP_DATA_SECURED, TCP_DATA_UNSECURED
from doipclient.messages import RoutingActivationRequest

from doip_mtls import (
    DEFAULT_CA_CERT,
    DEFAULT_CLIENT_CERT,
    DEFAULT_CLIENT_KEY,
    SecureDoIPClient,
    build_tls_context,
)

ACTIVATION_TYPES = {
    "default": RoutingActivationRequest.ActivationType.Default,
    "regulatory": RoutingActivationRequest.ActivationType.DiagnosticRequiredByRegulation,
    "central-security": RoutingActivationRequest.ActivationType.CentralSecurity,
    "none": None,
}

SECURITY_MODES = ("none", "tls", "mtls")
DEFAULT_MODE = "mtls"

# The DoIP server this was set up for; override with --ecu-ip
DEFAULT_ECU_IP = "192.168.225.1"

EXIT_OK = 0
EXIT_FAILURE = 1

# ISO 14229-1: requestCorrectlyReceived-ResponsePending. Not a final answer - the
# ECU is still working and will send the real response later, so it must not be
# handed back to the caller as-is.
NRC_RESPONSE_PENDING = 0x78
DEFAULT_MAX_PENDING = 50


def _hex_payload(text):
    """Parse "22F190", "22 f1 90" or "0x22,0xF1,0x90" into a bytearray."""
    cleaned = (
        text.lower().replace("0x", "").replace(",", " ").replace("-", " ").replace(" ", "")
    )
    try:
        return bytearray.fromhex(cleaned)
    except ValueError:
        raise argparse.ArgumentTypeError(
            "{!r} is not a hex byte string, e.g. 22F190".format(text)
        )


def _address(text):
    """Parse a logical address given as 0x0201 or 513."""
    try:
        return int(text, 0)
    except ValueError:
        raise argparse.ArgumentTypeError(
            "{!r} is not a logical address, e.g. 0x0201".format(text)
        )


def default_port(mode):
    """Table 39: 13400 for unsecured DoIP, 3496 once TLS is in play."""
    return TCP_DATA_UNSECURED if mode == "none" else TCP_DATA_SECURED


def parse_args(argv=None):
    parser = argparse.ArgumentParser(
        description=__doc__.splitlines()[0],
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument("--mode", choices=SECURITY_MODES, default=DEFAULT_MODE,
                        help="security mode: 'none' for plain DoIP, 'tls' to "
                             "authenticate only the ECU, 'mtls' for both sides "
                             "(default: %(default)s)")

    connection = parser.add_argument_group("connection")
    connection.add_argument("--ecu-ip", default=DEFAULT_ECU_IP, metavar="IP",
                            help="IP address of the DoIP server, IPv4 or IPv6 "
                                 "(default: %(default)s)")
    connection.add_argument("--port", type=int, default=None,
                            help="TCP port (default: {} with TLS, {} without)".format(
                                TCP_DATA_SECURED, TCP_DATA_UNSECURED))
    connection.add_argument("--ecu-logical-address", type=_address, default=0x0201,
                            metavar="ADDR",
                            help="logical address of the ECU (default: 0x0201)")
    connection.add_argument("--client-logical-address", type=_address, default=0x0E00,
                            metavar="ADDR",
                            help="our own logical address (default: 0x0E00)")
    connection.add_argument("--client-ip", metavar="IP",
                            help="local address to bind, if you have several adapters")
    connection.add_argument("--activation-type", choices=sorted(ACTIVATION_TYPES),
                            default="default",
                            help="routing activation to request, or 'none' to skip "
                                 "(default: %(default)s)")
    connection.add_argument("--protocol-version", type=_address, default=0x02,
                            metavar="VER",
                            help="DoIP protocol version (default: 0x02)")

    tls = parser.add_argument_group("TLS (ignored with --mode none)")
    tls.add_argument("--cert", default=DEFAULT_CLIENT_CERT, metavar="PATH",
                     help="our client certificate, mtls mode only "
                          "(default: certs/client.crt)")
    tls.add_argument("--key", default=DEFAULT_CLIENT_KEY, metavar="PATH",
                     help="private key for --cert (default: certs/client.key)")
    tls.add_argument("--key-password", metavar="PW",
                     help="password for --key, if it is encrypted")
    tls.add_argument("--ca", default=DEFAULT_CA_CERT, metavar="PATH",
                     help="CA that signed the ECU certificate (default: certs/ca.crt)")
    tls.add_argument("--check-hostname", action="store_true",
                     help="also require the ECU certificate to match the address "
                          "connected to; needs that address in its subjectAltName")
    tls.add_argument("--server-hostname", metavar="NAME",
                     help="name to send for SNI and match against the certificate "
                          "(default: the value of --ecu-ip)")
    tls.add_argument("--insecure", action="store_true",
                     help="do not verify the ECU certificate at all; for bring-up only")
    tls.add_argument("--min-tls", choices=["1.2", "1.3"], default="1.3",
                     help="lowest TLS version to accept; the ECU is configured for "
                          "TLSv1.3 (default: %(default)s)")
    tls.add_argument("--handshake-timeout", type=float, default=10, metavar="SEC",
                     help="seconds allowed for the TLS handshake (default: %(default)s)")
    tls.add_argument("--keylog", metavar="PATH",
                     help="write TLS session keys here so Wireshark can decrypt the "
                          "capture; anyone with this file can read the traffic")

    diagnostics = parser.add_argument_group("diagnostics")
    diagnostics.add_argument("--uds", action="append", type=_hex_payload, default=[],
                             metavar="HEX",
                             help="UDS request to send once connected, e.g. 22F190; "
                                  "repeatable, sent in order")
    diagnostics.add_argument("--timeout", type=float, default=5, metavar="SEC",
                             help="seconds to wait for each UDS response, reset on "
                                  "every NRC 0x78 (ResponsePending) (default: %(default)s)")
    diagnostics.add_argument("--max-pending", type=int, default=DEFAULT_MAX_PENDING,
                             metavar="N",
                             help="give up after this many consecutive NRC 0x78 "
                                  "(ResponsePending) replies to one request "
                                  "(default: %(default)s)")
    parser.add_argument("--verbose", action="store_true",
                        help="enable doipclient debug logging")

    args = parser.parse_args(argv)
    if args.port is None:
        args.port = default_port(args.mode)
    return args


def connect(args):
    """Open the DoIP session in the requested security mode."""
    print("Connecting to {}:{} (ECU 0x{:04X}) as 0x{:04X}, mode {}".format(
        args.ecu_ip, args.port, args.ecu_logical_address,
        args.client_logical_address, args.mode,
    ))
    common = dict(
        tcp_port=args.port,
        client_logical_address=args.client_logical_address,
        client_ip_address=args.client_ip,
        activation_type=ACTIVATION_TYPES[args.activation_type],
        protocol_version=args.protocol_version,
    )

    if args.mode == "none":
        # Plain DoIP: the library handles this fine, no context involved
        return DoIPClient(args.ecu_ip, args.ecu_logical_address, **common)

    if args.insecure:
        print("WARNING: --insecure, the ECU certificate is not being verified")
    context = build_tls_context(
        # tls mode deliberately sends no client certificate
        client_cert=args.cert if args.mode == "mtls" else None,
        client_key=args.key if args.mode == "mtls" else None,
        ca_cert=args.ca,
        key_password=args.key_password,
        check_hostname=args.check_hostname,
        verify_ecu=not args.insecure,
        minimum_version=(
            ssl.TLSVersion.TLSv1_3 if args.min_tls == "1.3" else ssl.TLSVersion.TLSv1_2
        ),
        keylog_filename=args.keylog,
    )
    return SecureDoIPClient(
        args.ecu_ip,
        args.ecu_logical_address,
        ssl_context=context,
        server_hostname=args.server_hostname,
        handshake_timeout=args.handshake_timeout,
        **common
    )


def report_session(client, args):
    print("Connected")
    if args.mode == "none":
        print("  Security      : none, plain DoIP (nothing is encrypted)")
    else:
        tls = client.describe_tls()
        print("  TLS version   : {}{}".format(
            tls["version"], "" if tls["version_ok"] else "  <- ECU expects TLSv1.3"
        ))
        print("  Cipher        : {}{}".format(
            tls["cipher"],
            "" if tls["cipher_ok"] else "  <- outside the ECU's tls_cipher_suites"
        ))
        if tls["ecu_cert_verified"]:
            print("  ECU cert      : CN={} (issued by CN={})".format(
                tls["ecu_common_name"], tls["ecu_issuer"]
            ))
        else:
            print("  ECU cert      : not verified (--insecure)")
        if args.mode == "mtls":
            # There is no client-side API to confirm the certificate was actually
            # transmitted; the ECU only asks for one if it is configured for mTLS.
            print("  Client cert   : {} (sent if the ECU asks for one)".format(
                args.cert))
        else:
            print("  Client cert   : none loaded (--mode tls)")
    if args.activation_type == "none":
        print("  Activation    : skipped (--activation-type none)")
    else:
        print("  Activation    : succeeded ({})".format(args.activation_type))


class TooManyPendingResponses(Exception):
    """The ECU sent NRC 0x78 more times in a row than --max-pending allows."""


def _receive_final_response(client, request, timeout, max_pending):
    """Send `request` and wait for the UDS response that is not NRC 0x78.

    NRC 0x78 (requestCorrectlyReceived-ResponsePending) means the ECU is still
    working and will send the real answer later. Per ISO 14229-1 the tester must
    keep waiting for it rather than treat the 0x78 itself as the answer, and its
    wait timer restarts on every 0x78 received. `max_pending` is a safety net for
    an ECU that never stops sending it.
    """
    client.send_diagnostic(request)
    pending_count = 0
    while True:
        response = client.receive_diagnostic(timeout=timeout)
        if (
            len(response) >= 3
            and response[0] == 0x7F
            and response[2] == NRC_RESPONSE_PENDING
        ):
            pending_count += 1
            if pending_count > max_pending:
                raise TooManyPendingResponses(
                    "gave up after {} consecutive NRC 0x78 (ResponsePending)".format(
                        max_pending
                    )
                )
            print("<- {}   (NRC 0x78, ResponsePending - still waiting)".format(
                bytes(response).hex(" ")))
            continue
        return response


def exchange_uds(client, requests, timeout, max_pending=DEFAULT_MAX_PENDING):
    """Send each UDS request and print the response. Returns True if all succeeded."""
    all_ok = True
    for request in requests:
        print("\n-> {}".format(request.hex(" ")))
        try:
            response = _receive_final_response(client, request, timeout, max_pending)
        except TimeoutError as exc:
            print("<- timeout: {}".format(exc))
            all_ok = False
            continue
        except TooManyPendingResponses as exc:
            print("<- {}".format(exc))
            all_ok = False
            continue
        except IOError as exc:  # DoIP negative acknowledgement
            print("<- rejected by DoIP layer: {}".format(exc))
            all_ok = False
            continue
        print("<- {}".format(bytes(response).hex(" ")))
        if len(response) >= 3 and response[0] == 0x7F:
            print("   (UDS negative response, NRC 0x{:02X})".format(response[2]))
    return all_ok


def explain(exc, mode):
    """Turn the common failure modes into something actionable."""
    if isinstance(exc, ssl.SSLCertVerificationError):
        return (
            "The ECU's certificate was not accepted.\n"
            "  - Is --ca the CA that signed it? Use the ECU's own CA if it has one.\n"
            "  - Hostname mismatch? Drop --check-hostname, or reissue the ECU\n"
            "    certificate with its address in the subjectAltName.\n"
            "  - To get past this temporarily during bring-up, add --insecure."
        )
    if isinstance(exc, ssl.SSLError):
        text = str(exc).lower()
        if "certificate required" in text or "certificate_required" in text:
            if mode == "tls":
                return ("The ECU requires a client certificate, so plain TLS is not "
                        "enough - use --mode mtls.")
            return ("The ECU demanded a client certificate but did not accept ours - "
                    "check that --cert/--key were loaded and are what it expects.")
        if "unknown ca" in text or "unknown_ca" in text or "decrypt error" in text:
            return ("The ECU rejected our client certificate: it does not trust the CA "
                    "that signed it. Install certs/ca.crt as a trusted client CA on "
                    "the ECU.")
        # Check this before anything matching "version", since it is also a version error
        if "wrong version number" in text or "record layer" in text:
            return ("That port is not speaking TLS. Port {} is the unsecured DoIP port "
                    "- either use --mode none, or connect to {}.".format(
                        TCP_DATA_UNSECURED, TCP_DATA_SECURED))
        if ("unsupported protocol" in text or "protocol version" in text
                or "no protocols available" in text):
            return ("No shared TLS version. The ECU may only speak TLS 1.2 - drop "
                    "--min-tls 1.3.")
        if "handshake failure" in text:
            return ("The ECU rejected the handshake parameters. Most likely no shared "
                    "cipher suite or signature algorithm: it expects TLSv1.3 with "
                    "TLS_AES_128_GCM_SHA256/TLS_AES_256_GCM_SHA384 and an "
                    "ecdsa_secp256r1_sha256 (EC P-256) certificate. If OPENSSL_CONF "
                    "points at openssl_doip.cnf, try without it.")
        if "eof" in text or "handshake" in text:
            if mode == "tls":
                return ("The ECU closed the connection during the handshake. It most "
                        "likely requires a client certificate - use --mode mtls.")
            return ("The ECU closed the connection during the handshake. Common causes: "
                    "it requires a client certificate from a CA it trusts, or the port "
                    "is not the TLS one.")
        return "TLS handshake failed."
    if isinstance(exc, ConnectionRefusedError):
        if "Activation Request failed" in str(exc):
            hint = ("TLS succeeded but DoIP routing activation was denied. Check the "
                    "response code above: 0x04 means the ECU wants authentication, "
                    "0x06 that it requires TLS, 0x02/0x03 that the logical address is "
                    "wrong or already registered. Try --client-logical-address.")
            if mode == "none":
                hint = ("DoIP routing activation was denied. Response code 0x06 means "
                        "the ECU requires TLS - use --mode tls or --mode mtls. 0x04 "
                        "means it wants authentication, 0x02/0x03 that the logical "
                        "address is wrong or already registered.")
            return hint
        return ("Nothing is listening there. Check the port - secured DoIP is {}, "
                "unsecured {}.".format(TCP_DATA_SECURED, TCP_DATA_UNSECURED))
    if isinstance(exc, ConnectionResetError):
        if mode == "none":
            return ("The ECU closed the connection immediately. Port {} normally "
                    "requires TLS - try --mode mtls, or --mode none against port "
                    "{}.".format(TCP_DATA_SECURED, TCP_DATA_UNSECURED))
        return ("The ECU closed the connection. It may have rejected the handshake or "
                "the DoIP header - try --verbose.")
    if isinstance(exc, (socket.timeout, TimeoutError)):
        if "handshake" in str(exc).lower():
            return ("The TLS handshake timed out, so that port may not be speaking TLS "
                    "at all. Port {} is the unsecured DoIP port - use --mode none for "
                    "it, or connect to {}.".format(
                        TCP_DATA_UNSECURED, TCP_DATA_SECURED))
        if mode == "none":
            return ("Timed out. If the ECU only accepts secured DoIP it may be ignoring "
                    "us on port {} - try --mode mtls. Otherwise the address may be "
                    "unreachable or firewalled.".format(TCP_DATA_UNSECURED))
        return ("Timed out. The address may be unreachable or a firewall may be "
                "dropping the traffic; verify with a plain TCP connection first.")
    if isinstance(exc, ValueError) and "check_hostname" in str(exc):
        return ("This is the library bug SecureDoIPClient exists to avoid - it means a "
                "plain DoIPClient was used instead of SecureDoIPClient.")
    if isinstance(exc, FileNotFoundError):
        return "Certificate file missing - run: python generate_certs.py"
    if isinstance(exc, OSError):
        return "Connection failed at the socket level."
    return None


def main(argv=None):
    args = parse_args(argv)
    logging.basicConfig(
        level=logging.DEBUG if args.verbose else logging.WARNING,
        format="%(levelname)s %(name)s: %(message)s",
    )

    try:
        client = connect(args)
    except Exception as exc:
        print("FAILED: {}: {}".format(type(exc).__name__, exc), file=sys.stderr)
        hint = explain(exc, args.mode)
        if hint:
            print("\n{}".format(hint), file=sys.stderr)
        return EXIT_FAILURE

    try:
        report_session(client, args)
        succeeded = (
            exchange_uds(client, args.uds, args.timeout, args.max_pending)
            if args.uds else True
        )
    finally:
        client.close()
    return EXIT_OK if succeeded else EXIT_FAILURE


if __name__ == "__main__":
    sys.exit(main())
