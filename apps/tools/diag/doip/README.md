# DoIP client with no TLS / TLS / mTLS

A standalone DoIP client for talking to a DoIP server on another device, in any of
three security modes, plus the certificates it uses. No test framework — just
scripts you run.

| `--mode` | Port | Who authenticates |
| --- | --- | --- |
| `none` | 13400 | nobody, plain DoIP |
| `tls` | 3496 | the ECU only |
| `mtls` | 3496 | both sides (default) |

## Files

| File | Purpose |
| --- | --- |
| `doip_mtls.py` | The implementation: `build_tls_context()`, `build_mtls_context()` and `SecureDoIPClient`. Import this from your own code. |
| `doip_mtls_client.py` | Command line client: connects, does routing activation, optionally sends UDS requests. |
| `certs/` | Pre-generated CA, client and server material — PEM, EC P-256. Already created, nothing to do. |
| `generate_certs.py` | Reissues `certs/` (needed only to change the address, key type or key format). |
| `openssl_doip.cnf` | Optional strict TLS profile, see [the cipher suite limitation](#the-one-limitation). |
| `fake_secure_ecu.py` | Optional local DoIP server, serving any of the three modes, to check the client before using the real device. |

Requires Python 3.7+ (for `ssl.TLSVersion`). No third-party packages. Certificate
generation additionally needs the `openssl` CLI.

## Quick start

The DoIP server address defaults to **192.168.225.1** and the mode to **mtls**, so
no arguments are needed:

```
cd tls_testcode
python doip_mtls_client.py                 # mTLS on port 3496
python doip_mtls_client.py --mode tls      # TLS on 3496, no client certificate
python doip_mtls_client.py --mode none     # plain DoIP on 13400
```

The port follows the mode automatically; `--port` overrides it. The default run
presents `certs/client.crt`, verifies the ECU against `certs/ca.crt`, runs DoIP
routing activation and prints what was negotiated:

```
Connecting to 192.168.225.1:3496 (ECU 0x0201) as 0x0E00, mode mtls
Connected
  TLS version   : TLSv1.3
  Cipher        : TLS_AES_256_GCM_SHA384
  ECU cert      : CN=doip-ecu (issued by CN=DoIP-Test-CA)
  Client cert   : …/certs/client.crt (sent if the ECU asks for one)
  Activation    : succeeded (default)
```

With `--mode none` there is no handshake to report:

```
Connecting to 192.168.225.1:13400 (ECU 0x0201) as 0x0E00, mode none
Connected
  Security      : none, plain DoIP (nothing is encrypted)
  Activation    : succeeded (default)
```

Send UDS requests with `--uds` (repeatable, sent in order), in any mode:

```
python doip_mtls_client.py --uds 1003 --uds 22F190
```

```
-> 10 03
<- 50 03

-> 22 f1 90
<- 62 f1 90
```

The addresses default to the ECU at `0x0201` and this client at `0x0E00`. Override
with `--ecu-logical-address` / `--client-logical-address`. `--help` lists
everything.

### NRC 0x78 (ResponsePending)

Per ISO 14229-1, `7F <SID> 78` is not a final answer - it just means the ECU is
still working and will send the real response later. The client keeps waiting for
it instead of returning the 0x78 itself, restarting the `--timeout` wait on every
occurrence:

```
-> 22 f1 90
<- 7f 22 78   (NRC 0x78, ResponsePending - still waiting)
<- 7f 22 78   (NRC 0x78, ResponsePending - still waiting)
<- 62 f1 90
```

`--max-pending` (default 50) caps how many consecutive 0x78 replies to accept
before giving up, so a misbehaving ECU that never stops sending it cannot hang the
client forever.


## Matching the ECU's TLS configuration

The certificates and defaults here are built for a DoIP entity configured as:

```json
"tls_min_version":      "TLSv1.3",
"tls_cipher_suites":    "TLS_AES_128_GCM_SHA256:TLS_AES_256_GCM_SHA384",
"signature_algorithms": "ecdsa_secp256r1_sha256"
```

| Setting | How it is met |
| --- | --- |
| `tls_min_version` | `build_tls_context()` sets `minimum_version = TLSv1_3`; the CLI's `--min-tls` defaults to `1.3`. |
| `signature_algorithms` | `certs/` hold EC keys on prime256v1 (secp256r1) signed with `ecdsa-with-SHA256`, so that is the algorithm actually used. |
| `tls_cipher_suites` | Both suites negotiate successfully. Python cannot *restrict* the offer — see below. |

### The one limitation

Python's `ssl` module cannot set TLS 1.3 cipher suites or signature algorithms from
code: `set_ciphers()` applies only to TLS 1.2 and below (it rejects TLS 1.3 suite
names with `No cipher can be selected`), and there is no `set_sigalgs()`. So the
client offers OpenSSL's default TLS 1.3 list — `TLS_AES_256_GCM_SHA384`,
`TLS_CHACHA20_POLY1305_SHA256`, `TLS_AES_128_GCM_SHA256` — a superset of what the
ECU accepts. The ECU does the choosing, so the session always lands on a permitted
suite, and the client flags it if that is somehow not the case:

```
  Cipher        : TLS_CHACHA20_POLY1305_SHA256  <- outside the ECU's tls_cipher_suites
```

If the *offer itself* has to be restricted, point `OPENSSL_CONF` at the supplied
profile before starting Python. It applies OpenSSL-level system defaults
(`MinProtocol`, `CipherSuites`, `SignatureAlgorithms`) process-wide:

```
OPENSSL_CONF=openssl_doip.cnf python doip_mtls_client.py            # bash
set OPENSSL_CONF=openssl_doip.cnf && python doip_mtls_client.py     # cmd.exe
```

## Certificate format

Everything in `certs/` is PEM:

| File | Contents |
| --- | --- |
| `ca.crt`, `client.crt`, `server.crt` | `-----BEGIN CERTIFICATE-----`, X.509 |
| `ca.key`, `client.key`, `server.key` | `-----BEGIN PRIVATE KEY-----`, unencrypted PKCS#8, EC prime256v1 |

Note that the `file` command misreports these keys as "OpenSSH private key"; use
`openssl pkey -in certs/client.key -noout -text` to inspect them properly.

EC private keys have two PEM encodings. If the ECU's TLS stack only parses the
traditional form (`-----BEGIN EC PRIVATE KEY-----`), reissue with
`--key-format sec1`. `--key-type rsa` goes back to RSA 2048 if you ever need it:

```
python generate_certs.py --force --key-format sec1
python generate_certs.py --force --key-type rsa
```

## Which mode does my ECU want?

If you do not know, work down the list:

1. `--mode mtls` — if it connects, you are done.
2. `--mode tls` — if mtls fails but this works, the ECU does not ask for a client
   certificate.
3. `--mode none` — if routing activation comes back with code `0x06`
   (`DeniedRequiresTLS`), the ECU insists on TLS, so go back to step 1.

The client prints a hint naming the likely mode for each failure.

**Do not use `--activation-type none` to probe mTLS.** In TLS 1.3 the server's
rejection of a missing or untrusted client certificate arrives *after* the client
considers the handshake complete, so with no data exchanged the client reports
`Connected` while the ECU has actually refused it. Let routing activation run —
that round trip is what surfaces the alert.

## Setting up the server device

The generated CA signs both sides, so the DoIP server has to be given the matching
half. Copy to the server device:

| File | Used for |
| --- | --- |
| `certs/server.crt`, `certs/server.key` | the server's own TLS identity |
| `certs/ca.crt` | the trusted CA for **client** certificates, so it accepts `client.crt` |

If the server already has its own PKI, do it the other way round: point `--ca` at
the server's CA certificate, and pass a client certificate/key that the server
trusts:

```
python doip_mtls_client.py --ecu-ip 192.168.225.1 \
    --ca /path/to/ecu_ca.crt --cert /path/to/my_tester.crt --key /path/to/my_tester.key
```

`--check-hostname` is off by default, because ECU certificates rarely carry the IP
you connect to in a subjectAltName. The ECU certificate is still verified against
`--ca` either way. The generated `server.crt` *does* carry
`IP:192.168.225.1, IP:127.0.0.1, DNS:localhost, DNS:doip-ecu`, so if the server
device uses that certificate you can turn the check on right away:

```
python doip_mtls_client.py --check-hostname
```

For a different address, reissue first so the SAN covers it:

```
python generate_certs.py --force --ecu-ip 10.0.0.5
python doip_mtls_client.py --ecu-ip 10.0.0.5 --check-hostname
```

The keys in `certs/` are throwaway test material with no passphrase, valid for ten
years. Do not reuse them anywhere that matters.

## Using it from your own code

mTLS — both sides authenticate:

```python
from doipclient.constants import TCP_DATA_SECURED
from doip_mtls import SecureDoIPClient, build_mtls_context

client = SecureDoIPClient(
    "192.168.225.1",             # ECU IP
    0x0201,                      # ECU logical address
    tcp_port=TCP_DATA_SECURED,   # 3496, not the unsecured 13400
    ssl_context=build_mtls_context(),   # defaults to the files in ./certs
)
print(client.describe_tls())
client.send_diagnostic(bytearray([0x22, 0xF1, 0x90]))
print(client.receive_diagnostic(timeout=5))
client.close()
```

TLS only — leave the client certificate out:

```python
from doip_mtls import SecureDoIPClient, build_tls_context

client = SecureDoIPClient(
    "192.168.225.1", 0x0201,
    tcp_port=TCP_DATA_SECURED,
    ssl_context=build_tls_context(ca_cert="certs/ca.crt"),  # no client_cert
)
```

No TLS — nothing from this module is needed:

```python
from doipclient import DoIPClient
from doipclient.constants import TCP_DATA_UNSECURED

client = DoIPClient("192.168.225.1", 0x0201, tcp_port=TCP_DATA_UNSECURED)  # 13400
```

`build_tls_context()` takes `client_cert`, `client_key`, `ca_cert`, `key_password`,
`check_hostname`, `verify_ecu`, `minimum_version` and `keylog_filename`;
`build_mtls_context()` is the same thing with `client_cert`/`client_key` defaulted
to `certs/client.*`. `describe_tls()` reports the negotiated version and cipher plus
`version_ok` / `cipher_ok` against the ECU's settings. `MutualTlsDoIPClient` remains
as an alias of `SecureDoIPClient`. For use with udsoncan, hand the client to
`doipclient.connectors.DoIPClientUDSConnector` as usual.

## Checking the client locally

The fake ECU serves whichever mode you ask for, on TLS 1.3 with the same
certificates. Terminal 1:

```
python fake_secure_ecu.py                # mTLS on 3496
python fake_secure_ecu.py --mode tls     # TLS on 3496
python fake_secure_ecu.py --mode none    # plain DoIP on 13400
```

Terminal 2, with a matching `--mode`:

```
python doip_mtls_client.py --ecu-ip 127.0.0.1 --uds 22F190
python doip_mtls_client.py --ecu-ip 127.0.0.1 --mode tls --uds 22F190
python doip_mtls_client.py --ecu-ip 127.0.0.1 --mode none --uds 22F190
```

It logs each connection with the client's common name, and answers every UDS request
with the positive response SID (request SID + 0x40).

To check against a strictly configured peer instead, `openssl s_server` can be held
to exactly the ECU's settings:

```
openssl s_server -accept 4433 -www -cert certs/server.crt -key certs/server.key \
    -CAfile certs/ca.crt -Verify 1 -tls1_3 \
    -ciphersuites TLS_AES_128_GCM_SHA256:TLS_AES_256_GCM_SHA384 \
    -sigalgs ecdsa_secp256r1_sha256 -client_sigalgs ecdsa_secp256r1_sha256

python doip_mtls_client.py --ecu-ip 127.0.0.1 --port 4433 --activation-type none
```

`s_server` speaks HTTP rather than DoIP, hence `--activation-type none`; this checks
the handshake only. Note it must be started with `-www` (or a live stdin) — when
backgrounded without it, `s_server` sees EOF on stdin and exits on first connect.

## Troubleshooting

The client prints a hint for each common failure. What they usually mean:

| Message | Cause |
| --- | --- |
| `TLSV1_ALERT_UNKNOWN_CA` | The ECU does not trust the CA that signed our client certificate. Install `certs/ca.crt` on the ECU as a trusted client CA. |
| `CERTIFICATE_VERIFY_FAILED` | We do not trust the ECU's certificate. Point `--ca` at the ECU's CA, or use `--insecure` for bring-up only. |
| `Hostname mismatch` | The address is not in the ECU certificate's subjectAltName. Drop `--check-hostname` or reissue with `--ecu-ip`. |
| `certificate required` | The ECU wants a client certificate. In `--mode tls`, switch to `--mode mtls`. |
| `EOF occurred in violation of protocol` | Usually the same thing seen from TLS 1.3: the ECU wanted a client certificate. Try `--mode mtls`. |
| `SSLV3_ALERT_HANDSHAKE_FAILURE` | No shared cipher suite or signature algorithm. The ECU expects TLS 1.3 with the two AES-GCM suites and an EC P-256 certificate. If `OPENSSL_CONF` is set, try without it. |
| `The handshake operation timed out` | That port is probably not speaking TLS at all — use `--mode none` for 13400, or connect to 3496. |
| `ConnectionResetError` in `--mode none` | The ECU dropped a plaintext connection; it likely requires TLS. |
| `Activation Request failed with code` | The transport worked; DoIP refused. `0x04` wants authentication, `0x06` requires TLS (so use `--mode tls`/`mtls`), `0x02`/`0x03` mean the logical address is wrong or already registered. |
| `Nothing is listening there` | Wrong port. Secured DoIP is 3496, unsecured 13400. |
| UDS request times out but activation succeeded | `--ecu-logical-address` is probably wrong. Activation carries only *our* address, so a wrong ECU address is not caught there. |

To read the traffic in Wireshark, dump the session keys and load the file under
*TLS → (Pre)-Master-Secret log filename*:

```
python doip_mtls_client.py --ecu-ip 192.168.225.1 --keylog tls_keys.log
```

## Why `SecureDoIPClient` exists

`DoIPClient` documents `use_secure=<ssl.SSLContext>`, but that path is broken in
v1.1.0 — for both TLS and mTLS — and both bugs have to be worked around:

* `client.py:777` checks the context with `isinstance(x, type(ssl.SSLContext))`.
  `type(ssl.SSLContext)` is just `type`, and a context *instance* is not a class,
  so the check is always False — your context, and with it your client
  certificate, is silently replaced by `ssl.create_default_context()`.
* `client.py:785` calls `wrap_socket()` without `server_hostname`, so that default
  context (`check_hostname=True`) raises
  `ValueError: check_hostname requires server_hostname`.

So both `use_secure=True` and `use_secure=<your context>` fail immediately with
that `ValueError`, and no client certificate is ever sent. `SecureDoIPClient`
overrides `_wrap_socket()` and is unaffected; it also gives the handshake its own
timeout instead of inheriting the library's 2 s `A_PROCESSING_TIME`. If you would
rather fix the library, it is two lines:

```python
# client.py:777
if isinstance(self._use_secure, ssl.SSLContext):

# client.py:785
self._tcp_sock = ssl_context.wrap_socket(
    self._tcp_sock, server_hostname=self._ecu_ip_address
)
```

`SecureDoIPClient` keeps working either way, since it passes the context through
`use_secure` and the override uses that same object.
