# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""Generate the certificates and keys used by the DoIP client.

Run once (the files are committed alongside this script, so normally you do not
need to). Re-run with --force to reissue, e.g. to bake your ECU's real IP address
into the server certificate so hostname checking can be enabled.

    python generate_certs.py --force --ecu-ip 192.168.225.1

Defaults match a DoIP entity configured for:

    "tls_min_version":        "TLSv1.3"
    "tls_cipher_suites":      "TLS_AES_128_GCM_SHA256:TLS_AES_256_GCM_SHA384"
    "signature_algorithms":   "ecdsa_secp256r1_sha256"

so keys are EC on the NIST P-256 curve (prime256v1 / secp256r1) and every
certificate is signed with ECDSA-SHA256. Pass --key-type rsa if you need the old
RSA material instead.

Everything is written as PEM: certificates between -----BEGIN CERTIFICATE----- lines,
private keys unencrypted as PKCS#8 (-----BEGIN PRIVATE KEY-----). Use
--key-format sec1 if the ECU's TLS stack only parses -----BEGIN EC PRIVATE KEY-----.

Produces, in ./certs:

    ca.crt / ca.key          the test CA that signs both sides
    client.crt / client.key  the tester identity -> presented by the DoIP client
    server.crt / server.key  the ECU identity -> install these on the DoIP server

Requires the openssl CLI on PATH. These are throwaway test keys with no
passphrase; do not reuse them anywhere that matters.
"""

import argparse
import pathlib
import shutil
import subprocess
import sys

HERE = pathlib.Path(__file__).resolve().parent
DEFAULT_CERT_DIR = HERE / "certs"

# ecdsa_secp256r1_sha256 means an EC key on prime256v1, signed with SHA-256
KEY_ARGUMENTS = {
    "ec": ["-newkey", "ec", "-pkeyopt", "ec_paramgen_curve:prime256v1"],
    "rsa": ["-newkey", "rsa:2048"],
}
DEFAULT_KEY_TYPE = "ec"
DIGEST = "-sha256"
# Certificates are written as PEM (base64 between -----BEGIN CERTIFICATE----- lines),
# pinned explicitly rather than relying on openssl's compiled-in default. Keys written
# via -keyout are always PEM, as unencrypted PKCS#8 ("BEGIN PRIVATE KEY");
# --key-format sec1 rewrites them as traditional "BEGIN EC PRIVATE KEY" instead, for
# TLS stacks that only parse that form.
PEM_OUTPUT = ["-outform", "PEM"]
KEY_FORMATS = ("pkcs8", "sec1")
DEFAULT_KEY_FORMAT = "pkcs8"

DEFAULT_DAYS = "3650"
CA_COMMON_NAME = "DoIP-Test-CA"
CLIENT_COMMON_NAME = "doip-tester"
SERVER_COMMON_NAME = "doip-ecu"
# The DoIP server's address, plus loopback so the local fake_secure_ecu.py check
# also works with hostname verification enabled.
DEFAULT_ECU_IPS = ["192.168.225.1", "127.0.0.1"]
DEFAULT_ECU_DNS = ["localhost", SERVER_COMMON_NAME]

GENERATED_FILES = [
    "ca.crt", "ca.key",
    "client.crt", "client.key",
    "server.crt", "server.key",
]


def _openssl(openssl, *args):
    """Run openssl, surfacing its stderr if it fails.

    Arguments are passed as a list rather than through a shell, which also keeps
    Git Bash / MSYS from rewriting the "/CN=..." subjects into Windows paths.
    """
    result = subprocess.run([openssl, *args], capture_output=True, text=True)
    if result.returncode != 0:
        sys.exit(
            "openssl {} failed:\n{}".format(args[0], result.stderr.strip())
        )


def _to_sec1(openssl, key, key_type):
    """Rewrite an EC key from PKCS#8 to the traditional SEC1 PEM form, in place."""
    if key_type != "ec":
        return  # only EC keys have the "BEGIN EC PRIVATE KEY" variant
    temporary = key.with_suffix(".sec1")
    _openssl(openssl, "ec", "-in", str(key), "-out", str(temporary), *PEM_OUTPUT)
    temporary.replace(key)


def _self_signed_ca(openssl, out_dir, days, key_arguments):
    key = out_dir / "ca.key"
    crt = out_dir / "ca.crt"
    _openssl(
        openssl, "req", "-x509", *key_arguments, "-nodes", DIGEST, "-days", days,
        *PEM_OUTPUT, "-keyout", str(key), "-out", str(crt),
        "-subj", "/CN=" + CA_COMMON_NAME,
    )
    return key, crt


def _leaf_certificate(
    openssl, out_dir, name, common_name, ca_key, ca_crt, days, extensions,
    key_arguments,
):
    key = out_dir / (name + ".key")
    csr = out_dir / (name + ".csr")
    crt = out_dir / (name + ".crt")
    ext = out_dir / (name + ".ext")
    ext.write_text(extensions)
    _openssl(
        openssl, "req", *key_arguments, "-nodes", DIGEST, *PEM_OUTPUT,
        "-keyout", str(key), "-out", str(csr), "-subj", "/CN=" + common_name,
    )
    _openssl(
        openssl, "x509", "-req", DIGEST, "-days", days, *PEM_OUTPUT, "-in", str(csr),
        "-CA", str(ca_crt), "-CAkey", str(ca_key), "-out", str(crt),
        "-extfile", str(ext),
    )
    csr.unlink()  # intermediate artifacts, not needed at runtime
    ext.unlink()
    return key, crt


def generate(out_dir, ecu_ips, ecu_dns, days=DEFAULT_DAYS, force=False,
             key_type=DEFAULT_KEY_TYPE, key_format=DEFAULT_KEY_FORMAT):
    """Generate the CA, client and server material into `out_dir`, all as PEM."""
    openssl = shutil.which("openssl")
    if openssl is None:
        sys.exit("openssl CLI not found on PATH")
    key_arguments = KEY_ARGUMENTS[key_type]

    out_dir = pathlib.Path(out_dir)
    existing = [name for name in GENERATED_FILES if (out_dir / name).exists()]
    if existing and not force:
        sys.exit(
            "refusing to overwrite existing files in {} ({}); pass --force".format(
                out_dir, ", ".join(existing)
            )
        )
    out_dir.mkdir(parents=True, exist_ok=True)

    ca_key, ca_crt = _self_signed_ca(openssl, out_dir, days, key_arguments)

    # The SAN is what makes check_hostname=True usable; without the ECU's real
    # address in here, verification of the hostname cannot succeed.
    san = ",".join(
        ["IP:" + ip for ip in ecu_ips] + ["DNS:" + name for name in ecu_dns]
    )
    _leaf_certificate(
        openssl, out_dir, "server", SERVER_COMMON_NAME, ca_key, ca_crt, days,
        "subjectAltName={}\nextendedKeyUsage=serverAuth\n".format(san),
        key_arguments,
    )
    _leaf_certificate(
        openssl, out_dir, "client", CLIENT_COMMON_NAME, ca_key, ca_crt, days,
        "extendedKeyUsage=clientAuth\n",
        key_arguments,
    )

    if key_format == "sec1":
        for name in ("ca", "server", "client"):
            _to_sec1(openssl, out_dir / (name + ".key"), key_type)

    print("Wrote to {}:".format(out_dir))
    for name in GENERATED_FILES:
        print("  {}".format(name))
    print("\nFormat  : PEM ({} certificates, {} private keys)".format(
        "X.509", "PKCS#8" if key_format == "pkcs8" else "SEC1",
    ))
    print("Key type: {}".format(
        "EC prime256v1 (secp256r1), signed with ECDSA-SHA256"
        if key_type == "ec" else "RSA 2048, signed with SHA-256"
    ))
    print("ECU certificate covers: {}".format(san))
    print(
        "\nCopy ca.crt, server.crt and server.key to the DoIP server device, and\n"
        "configure it to trust ca.crt for client certificates."
    )


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--out-dir", default=str(DEFAULT_CERT_DIR),
                        help="where to write the certificates (default: ./certs)")
    parser.add_argument("--key-type", choices=sorted(KEY_ARGUMENTS),
                        default=DEFAULT_KEY_TYPE,
                        help="'ec' for ecdsa_secp256r1_sha256, 'rsa' for RSA 2048 "
                             "(default: %(default)s)")
    parser.add_argument("--key-format", choices=KEY_FORMATS,
                        default=DEFAULT_KEY_FORMAT,
                        help="PEM private key encoding: 'pkcs8' for BEGIN PRIVATE KEY, "
                             "'sec1' for BEGIN EC PRIVATE KEY (default: %(default)s)")
    parser.add_argument("--ecu-ip", action="append", dest="ecu_ips", metavar="IP",
                        help="IP address to put in the ECU certificate SAN; repeatable "
                             "(default: 192.168.225.1, 127.0.0.1)")
    parser.add_argument("--ecu-dns", action="append", dest="ecu_dns", metavar="NAME",
                        help="DNS name to put in the ECU certificate SAN; repeatable "
                             "(default: localhost, doip-ecu)")
    parser.add_argument("--days", default=DEFAULT_DAYS, help="validity in days")
    parser.add_argument("--force", action="store_true",
                        help="overwrite existing certificates")
    args = parser.parse_args()

    generate(
        out_dir=args.out_dir,
        ecu_ips=args.ecu_ips or DEFAULT_ECU_IPS,
        ecu_dns=args.ecu_dns or DEFAULT_ECU_DNS,
        days=args.days,
        force=args.force,
        key_type=args.key_type,
        key_format=args.key_format,
    )


if __name__ == "__main__":
    main()
