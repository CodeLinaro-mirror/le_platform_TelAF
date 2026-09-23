# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""TLS and mutual TLS (mTLS) support for DoIPClient.

Three security modes are possible against a DoIP entity:

* **none** - plain DoIP on TCP port 13400. Use ``doipclient.DoIPClient`` directly,
  nothing in this module is needed.
* **tls** - TLS on port 3496 (``TCP_DATA_SECURED``). The ECU authenticates itself
  with a certificate; the client does not. Use `build_tls_context()` without a
  client certificate, together with `SecureDoIPClient`.
* **mtls** - as above, but the client also presents a certificate, which is what
  ``load_cert_chain()`` does. Use `build_mtls_context()` with `SecureDoIPClient`.

ISO 13400-2:2019 requires TLS 1.2 or newer for the secured port.

The certificate paths default to the files in ``./certs`` produced by
``generate_certs.py``, so nothing has to be passed in for a local test.

``DoIPClient`` documents a ``use_secure`` parameter that accepts a preconfigured
context, but in v1.1.0 that path does not work - for either TLS or mTLS:

* ``client.py`` tests the context with ``isinstance(x, type(ssl.SSLContext))``.
  ``type(ssl.SSLContext)`` is just ``type``, and a context *instance* is not a
  class, so the check is always False and the caller's context - including any
  client certificate - is silently replaced with ``ssl.create_default_context()``.
* ``_wrap_socket()`` does not pass ``server_hostname``, so that default context
  (``check_hostname=True``) raises ``ValueError: check_hostname requires
  server_hostname``.

Both ``use_secure=True`` and ``use_secure=<your context>`` therefore fail with
that ``ValueError``. ``SecureDoIPClient`` below overrides ``_wrap_socket()`` to
sidestep the problem without patching the library, and stays correct if the
library is fixed, because the override uses the same context that was handed to
``use_secure``.
"""

import pathlib
import ssl
import sys

# Allow running straight from a checkout, without pip-installing doipclient
_REPO_ROOT = pathlib.Path(__file__).resolve().parent.parent
if str(_REPO_ROOT) not in sys.path:
    sys.path.insert(0, str(_REPO_ROOT))

from doipclient import DoIPClient
from doipclient.constants import A_PROCESSING_TIME

CERT_DIR = pathlib.Path(__file__).resolve().parent / "certs"
DEFAULT_CA_CERT = str(CERT_DIR / "ca.crt")
DEFAULT_CLIENT_CERT = str(CERT_DIR / "client.crt")
DEFAULT_CLIENT_KEY = str(CERT_DIR / "client.key")

# The ECU is configured with:
#   "tls_min_version":      "TLSv1.3"
#   "tls_cipher_suites":    "TLS_AES_128_GCM_SHA256:TLS_AES_256_GCM_SHA384"
#   "signature_algorithms": "ecdsa_secp256r1_sha256"
# so TLS 1.3 is the floor here, and certs/ are EC prime256v1 signed with SHA-256
# (see generate_certs.py).
DEFAULT_MINIMUM_VERSION = ssl.TLSVersion.TLSv1_3
ALLOWED_CIPHER_SUITES = ("TLS_AES_128_GCM_SHA256", "TLS_AES_256_GCM_SHA384")


def build_tls_context(
    client_cert=None,
    client_key=None,
    ca_cert=DEFAULT_CA_CERT,
    key_password=None,
    check_hostname=False,
    verify_ecu=True,
    minimum_version=DEFAULT_MINIMUM_VERSION,
    keylog_filename=None,
):
    """Build an SSL context for a DoIP connection.

    With `client_cert`/`client_key` the handshake becomes mutually authenticated
    (mTLS); without them only the ECU is authenticated (plain TLS).

    Note on the ECU's ``tls_cipher_suites`` and ``signature_algorithms`` settings:
    Python's ``ssl`` module cannot restrict either from code. ``set_ciphers()``
    only applies to TLS 1.2 and below (it rejects TLS 1.3 suite names outright),
    and there is no ``set_sigalgs()`` at all. OpenSSL's built-in TLS 1.3 offer is
    ``TLS_AES_256_GCM_SHA384:TLS_CHACHA20_POLY1305_SHA256:TLS_AES_128_GCM_SHA256``,
    a superset of what the ECU accepts, so the handshake still settles on one of
    the two permitted AES-GCM suites - the ECU picks. Likewise our EC P-256
    certificate is what makes ``ecdsa_secp256r1_sha256`` the algorithm actually
    used. To narrow the *offer* itself, see openssl_doip.cnf in this directory.

    :param client_cert: Path to this client's certificate, in PEM format. Pass None
        for server-authentication-only TLS.
    :type client_cert: str, optional
    :param client_key: Path to the private key for `client_cert`. Required when
        `client_cert` is given, unless the key is in that same file.
    :type client_key: str, optional
    :param ca_cert: Path to the CA certificate (bundle) that signed the ECU's
        certificate. Ignored when `verify_ecu` is False.
    :type ca_cert: str
    :param key_password: Password for `client_key`, if it is encrypted.
    :type key_password: str, optional
    :param check_hostname: Verify that the ECU certificate matches the address being
        connected to. Requires the ECU certificate to carry that IP (or DNS name) in a
        subjectAltName, which is often not the case, so this defaults to False. The ECU
        certificate is still verified against `ca_cert` either way.
    :type check_hostname: bool, optional
    :param verify_ecu: Verify the ECU's certificate against `ca_cert`. Setting this to
        False accepts any certificate the ECU offers - useful for bring-up when you do
        not yet have the ECU's CA, but it leaves the connection open to interception, so
        do not leave it off.
    :type verify_ecu: bool, optional
    :param minimum_version: Lowest acceptable TLS version, TLS 1.3 by default to match
        the ECU. ISO 13400-2:2019 itself only requires TLS 1.2.
    :type minimum_version: ssl.TLSVersion, optional
    :param keylog_filename: Write TLS session keys here so the capture can be decrypted
        in Wireshark. Anyone with this file can read the traffic.
    :type keylog_filename: str, optional
    :return: A context suitable for `SecureDoIPClient`
    :rtype: ssl.SSLContext
    """
    context = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
    context.minimum_version = minimum_version
    if verify_ecu:
        context.check_hostname = check_hostname
        context.verify_mode = ssl.CERT_REQUIRED
        context.load_verify_locations(cafile=ca_cert)
    else:
        # check_hostname must go first: it cannot stay enabled with CERT_NONE
        context.check_hostname = False
        context.verify_mode = ssl.CERT_NONE
    if client_cert:
        # Our identity. This is the half that makes the handshake mutual.
        context.load_cert_chain(
            certfile=client_cert, keyfile=client_key, password=key_password
        )
    if keylog_filename:
        context.keylog_filename = keylog_filename
    return context


def build_mtls_context(
    client_cert=DEFAULT_CLIENT_CERT, client_key=DEFAULT_CLIENT_KEY, **kwargs
):
    """Build a mutually authenticated context, defaulting to the files in ./certs.

    Convenience wrapper around `build_tls_context()`; takes the same keyword
    arguments.
    """
    return build_tls_context(
        client_cert=client_cert, client_key=client_key, **kwargs
    )


class SecureDoIPClient(DoIPClient):
    """A DoIPClient that reliably uses a caller-supplied SSL context.

    Works for both TLS and mTLS - which one you get depends on whether the context
    carries a client certificate. Accepts every ``DoIPClient`` argument, plus the
    keyword-only arguments below. Remember to pass ``tcp_port=TCP_DATA_SECURED``
    (3496), since the library defaults to the unsecured port 13400.

    :param ssl_context: The context to wrap the TCP socket with, e.g. from
        `build_tls_context()` or `build_mtls_context()`. Overrides any `use_secure`
        value.
    :type ssl_context: ssl.SSLContext
    :param server_hostname: Name sent for SNI and matched against the ECU certificate
        when the context has `check_hostname` enabled. Defaults to the ECU IP address.
    :type server_hostname: str, optional
    :param handshake_timeout: Seconds allowed for the TLS handshake. The library arms the
        socket with A_PROCESSING_TIME (2s) before wrapping, which is tight for ECUs with
        slow crypto, so the handshake gets its own budget here.
    :type handshake_timeout: float, optional
    """

    def __init__(
        self,
        *args,
        ssl_context,
        server_hostname=None,
        handshake_timeout=10,
        **kwargs
    ):
        self._ssl_context = ssl_context
        self._server_hostname = server_hostname
        self._handshake_timeout = handshake_timeout
        # Truthy, so _connect() calls _wrap_socket(). Passing the context itself (rather
        # than True) means a fixed library hands us back this same object.
        kwargs["use_secure"] = ssl_context
        super().__init__(*args, **kwargs)

    def _wrap_socket(self, ssl_context):
        """Wrap the TCP socket, ignoring the context the library picked for us."""
        server_hostname = self._server_hostname or self._ecu_ip_address
        original_timeout = self._tcp_sock.gettimeout()
        self._tcp_sock.settimeout(self._handshake_timeout)
        # A failed handshake detaches the underlying socket, so there is nothing to
        # restore the timeout on - let the SSLError propagate untouched.
        wrapped = self._ssl_context.wrap_socket(
            self._tcp_sock, server_hostname=server_hostname
        )
        # Restore whatever _connect() armed the socket with, so read timeouts behave
        # the same as on an unencrypted connection.
        wrapped.settimeout(
            original_timeout if original_timeout is not None else A_PROCESSING_TIME
        )
        self._tcp_sock = wrapped

    def describe_tls(self):
        """Summarise the negotiated session, for logging during bring-up.

        :return: Negotiated version, cipher, the ECU certificate subject/issuer
            (empty when the ECU certificate was not verified) and whether the
            session matches the ECU's documented TLS settings.
        :rtype: dict
        """
        socket_ = self._tcp_sock
        cipher = socket_.cipher()
        certificate = socket_.getpeercert() or {}
        version = socket_.version()
        suite = cipher[0] if cipher else None

        def _name(field):
            return dict(
                item for rdn in certificate.get(field, ()) for item in rdn
            ).get("commonName")

        return {
            "version": version,
            "cipher": suite,
            "ecu_common_name": _name("subject"),
            "ecu_issuer": _name("issuer"),
            "ecu_cert_verified": bool(certificate),
            # Against the ECU's tls_min_version / tls_cipher_suites settings
            "version_ok": version == "TLSv1.3",
            "cipher_ok": suite in ALLOWED_CIPHER_SUITES,
        }


# Previous name, kept so existing code and notes keep working
MutualTlsDoIPClient = SecureDoIPClient
