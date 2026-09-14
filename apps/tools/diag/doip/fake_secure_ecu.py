# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""A minimal DoIP server for checking the client without the real ECU.

Optional - the actual deliverable is the client. This exists so you can confirm
the certificates and each security mode work on one machine before pointing the
client at another device. It supports the same three modes:

    python fake_secure_ecu.py                    # mTLS, port 3496 (default)
    python fake_secure_ecu.py --mode tls         # TLS, no client certificate
    python fake_secure_ecu.py --mode none        # plain DoIP, port 13400

Then, in another terminal:

    python doip_mtls_client.py --ecu-ip 127.0.0.1 --mode mtls --uds 22F190

It implements only what a client needs during a session: routing activation and
diagnostic messages, answering each UDS request with the positive response SID
(request SID + 0x40) and the request's remaining bytes echoed back.
"""

import argparse
import pathlib
import socket
import ssl
import sys
import threading

# Allow running straight from a checkout, without pip-installing doipclient
_REPO_ROOT = pathlib.Path(__file__).resolve().parent.parent
if str(_REPO_ROOT) not in sys.path:
    sys.path.insert(0, str(_REPO_ROOT))

from doipclient.constants import TCP_DATA_SECURED, TCP_DATA_UNSECURED
from doipclient.messages import (
    DiagnosticMessage,
    DiagnosticMessagePositiveAcknowledgement,
    RoutingActivationRequest,
    RoutingActivationResponse,
)

CERT_DIR = pathlib.Path(__file__).resolve().parent / "certs"
DEFAULT_CA_CERT = str(CERT_DIR / "ca.crt")
DEFAULT_SERVER_CERT = str(CERT_DIR / "server.crt")
DEFAULT_SERVER_KEY = str(CERT_DIR / "server.key")

SECURITY_MODES = ("none", "tls", "mtls")
MODE_DESCRIPTIONS = {
    "none": "plain DoIP, no TLS",
    "tls": "TLS, no client certificate requested",
    "mtls": "mTLS, client certificate required",
}

PROTOCOL_VERSION = 0x02
DOIP_HEADER_SIZE = 8
ACCEPT_POLL_TIMEOUT = 0.2  # keeps stop() responsive
HANDSHAKE_TIMEOUT = 10
SERVE_POLL_TIMEOUT = 1
UDS_POSITIVE_RESPONSE_OFFSET = 0x40


def pack_doip(payload_type, payload, protocol_version=PROTOCOL_VERSION):
    """Prepend the 8 byte DoIP header (Table 16) to an already packed payload."""
    return (
        bytes([protocol_version, ~protocol_version & 0xFF])
        + payload_type.to_bytes(2, "big")
        + len(payload).to_bytes(4, "big")
        + bytes(payload)
    )


def default_port(mode):
    """Table 39: 13400 for unsecured DoIP, 3496 once TLS is in play."""
    return TCP_DATA_UNSECURED if mode == "none" else TCP_DATA_SECURED


class FakeSecureEcu:
    """A DoIP entity that answers routing activation and diagnostic requests.

    :param mode: 'none' for plain DoIP, 'tls' to present a certificate without
        asking for one, 'mtls' to also require and verify a client certificate.
    :type mode: str, optional
    :param server_cert: Path to this entity's certificate. Unused when mode is 'none'.
    :type server_cert: str, optional
    :param server_key: Path to the private key for `server_cert`.
    :type server_key: str, optional
    :param ca_cert: CA to verify client certificates against, for 'mtls'.
    :type ca_cert: str, optional
    :param host: Address to bind.
    :type host: str, optional
    :param port: TCP port to bind. None picks the default for `mode`, 0 an
        ephemeral port.
    :type port: int, optional
    :param logical_address: Logical address this entity claims to be.
    :type logical_address: int, optional
    :param activation_response_code: Response code to answer routing activation with.
    :type activation_response_code: RoutingActivationResponse.ResponseCode, optional
    :param verbose: Print a line per connection and per message.
    :type verbose: bool, optional
    """

    def __init__(
        self,
        mode="mtls",
        server_cert=DEFAULT_SERVER_CERT,
        server_key=DEFAULT_SERVER_KEY,
        ca_cert=DEFAULT_CA_CERT,
        host="0.0.0.0",
        port=None,
        logical_address=0x0201,
        activation_response_code=RoutingActivationResponse.ResponseCode.Success,
        verbose=True,
    ):
        if mode not in SECURITY_MODES:
            raise ValueError("mode must be one of {}".format(", ".join(SECURITY_MODES)))
        self.mode = mode
        self.host = host
        self.port = default_port(mode) if port is None else port
        self.logical_address = logical_address
        self.client_common_names = []
        self.handshake_errors = []
        self._server_cert = server_cert
        self._server_key = server_key
        self._ca_cert = ca_cert
        self._activation_response_code = activation_response_code
        self._verbose = verbose
        self._listener = None
        self._threads = []
        self._stop = threading.Event()
        self._ready = threading.Event()

    def __enter__(self):
        return self.start()

    def __exit__(self, exc_type, exc_value, traceback):
        self.stop()

    def _log(self, message):
        if self._verbose:
            print(message, flush=True)

    def _build_context(self):
        if self.mode == "none":
            return None  # plain DoIP, nothing to wrap the socket with
        context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
        # Matches the ECU's "tls_min_version": "TLSv1.3"
        context.minimum_version = ssl.TLSVersion.TLSv1_3
        context.load_cert_chain(self._server_cert, self._server_key)
        if self.mode == "mtls":
            context.verify_mode = ssl.CERT_REQUIRED  # demand a client certificate
            context.load_verify_locations(cafile=self._ca_cert)
        return context

    def start(self):
        """Bind, listen and start serving in the background."""
        context = self._build_context()
        self._listener = socket.socket()
        self._listener.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self._listener.bind((self.host, self.port))
        self._listener.listen(5)
        self._listener.settimeout(ACCEPT_POLL_TIMEOUT)
        self.port = self._listener.getsockname()[1]
        self._spawn(self._accept_loop, context)
        if not self._ready.wait(5):
            raise RuntimeError("failed to start")
        self._log("Listening on {}:{} as ECU 0x{:04X} ({})".format(
            self.host, self.port, self.logical_address, MODE_DESCRIPTIONS[self.mode],
        ))
        return self

    def stop(self):
        """Stop serving and wait for the worker threads to finish."""
        self._stop.set()
        if self._listener is not None:
            self._listener.close()
        for thread in self._threads:
            thread.join(timeout=5)

    def serve_forever(self):
        """Block until interrupted, then shut down."""
        try:
            while not self._stop.is_set():
                self._stop.wait(0.5)
        except KeyboardInterrupt:
            self._log("\nShutting down")
        finally:
            self.stop()

    def _spawn(self, target, *args):
        thread = threading.Thread(target=target, args=args, daemon=True)
        thread.start()
        self._threads.append(thread)

    def _accept_loop(self, context):
        self._ready.set()
        while not self._stop.is_set():
            try:
                connection, peer = self._listener.accept()
            except socket.timeout:
                continue
            except OSError:
                break  # listener closed by stop()
            connection.settimeout(HANDSHAKE_TIMEOUT)
            # One thread per connection, so a peer that abandons a handshake
            # cannot wedge the accept loop.
            self._spawn(self._serve, context, connection, peer)

    def _serve(self, context, connection, peer):
        if context is None:
            stream = connection
            self._log("{}: connected, plain DoIP".format(peer[0]))
        else:
            try:
                stream = context.wrap_socket(connection, server_side=True)
            except OSError as exc:  # ssl.SSLError and friends
                self.handshake_errors.append("{}: {}".format(type(exc).__name__, exc))
                self._log("{}: handshake failed: {}".format(peer[0], exc))
                connection.close()
                return
            certificate = stream.getpeercert()
            common_name = None
            if certificate:
                common_name = dict(
                    item for rdn in certificate["subject"] for item in rdn
                ).get("commonName")
                self.client_common_names.append(common_name)
            self._log("{}: {}, client cert: {}".format(
                peer[0], stream.version(), common_name or "none"
            ))
        stream.settimeout(SERVE_POLL_TIMEOUT)
        try:
            self._serve_doip(stream, peer)
        except OSError:
            pass  # client hung up
        finally:
            stream.close()
            self._log("{}: disconnected".format(peer[0]))

    def _serve_doip(self, stream, peer):
        buffer = b""
        while not self._stop.is_set():
            try:
                data = stream.recv(4096)
            except socket.timeout:
                continue
            if not data:
                return
            buffer += data
            while len(buffer) >= DOIP_HEADER_SIZE:
                payload_type = int.from_bytes(buffer[2:4], "big")
                payload_length = int.from_bytes(buffer[4:8], "big")
                if len(buffer) < DOIP_HEADER_SIZE + payload_length:
                    break  # wait for the rest of the message
                end = DOIP_HEADER_SIZE + payload_length
                payload = buffer[DOIP_HEADER_SIZE:end]
                buffer = buffer[end:]
                self._respond(stream, peer, payload_type, payload)

    def _send(self, stream, message):
        stream.sendall(pack_doip(message.payload_type, message.pack()))

    def _respond(self, stream, peer, payload_type, payload):
        if payload_type == RoutingActivationRequest.payload_type:
            request = RoutingActivationRequest.unpack(payload, len(payload))
            self._log("{}: routing activation from 0x{:04X} -> 0x{:02X}".format(
                peer[0], request.source_address, self._activation_response_code
            ))
            self._send(
                stream,
                RoutingActivationResponse(
                    request.source_address,
                    self.logical_address,
                    self._activation_response_code,
                ),
            )
        elif payload_type == DiagnosticMessage.payload_type:
            request = DiagnosticMessage.unpack(payload, len(payload))
            user_data = bytes(request.user_data)
            self._log("{}: UDS request {}".format(peer[0], user_data.hex(" ")))
            self._send(
                stream,
                DiagnosticMessagePositiveAcknowledgement(
                    self.logical_address, request.source_address, 0x00
                ),
            )
            response = (
                bytes([user_data[0] + UDS_POSITIVE_RESPONSE_OFFSET]) + user_data[1:]
            )
            self._send(
                stream,
                DiagnosticMessage(
                    self.logical_address, request.source_address, response
                ),
            )
        else:
            self._log("{}: ignoring payload type 0x{:04X}".format(peer[0], payload_type))


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--mode", choices=SECURITY_MODES, default="mtls",
                        help="security mode to serve (default: %(default)s)")
    parser.add_argument("--host", default="0.0.0.0", help="address to bind")
    parser.add_argument("--port", type=int, default=None,
                        help="TCP port to bind (default: {} with TLS, {} without)".format(
                            TCP_DATA_SECURED, TCP_DATA_UNSECURED))
    parser.add_argument("--cert", default=DEFAULT_SERVER_CERT,
                        help="server certificate (default: certs/server.crt)")
    parser.add_argument("--key", default=DEFAULT_SERVER_KEY,
                        help="server private key (default: certs/server.key)")
    parser.add_argument("--ca", default=DEFAULT_CA_CERT,
                        help="CA to verify client certificates against, mtls mode "
                             "(default: certs/ca.crt)")
    parser.add_argument("--logical-address", type=lambda x: int(x, 0), default=0x0201,
                        help="logical address to claim (default: 0x0201)")
    args = parser.parse_args(argv)

    ecu = FakeSecureEcu(
        mode=args.mode,
        server_cert=args.cert,
        server_key=args.key,
        ca_cert=args.ca,
        host=args.host,
        port=args.port,
        logical_address=args.logical_address,
    )
    ecu.start()
    ecu.serve_forever()


if __name__ == "__main__":
    main()
