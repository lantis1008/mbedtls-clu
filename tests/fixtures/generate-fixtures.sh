#!/bin/sh
# generate-fixtures.sh - regenerates the checked-in files under tests/fixtures/.
# NOT invoked by run.sh or any Makefile target - fixtures are committed to git
# so test cases have a stable, reviewable input that doesn't depend on the
# host's RNG. Re-run this by hand (with real `openssl`) only if a fixture
# needs to be rotated/regenerated; it overwrites the fixture files in place.
#
# Deliberately small/weak key sizes (2048-bit RSA, P-256 EC) - these fixtures
# exist to exercise encoding/interop correctness, not to be production key
# material, and small keys keep the test suite fast.

set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
cd "$SCRIPT_DIR"

mkdir -p keys csr certs conf

echo "generate-fixtures.sh: RSA 2048 key..."
openssl genpkey -algorithm RSA -pkeyopt rsa_keygen_bits:2048 -out keys/rsa2048.key.pem 2>/dev/null

echo "generate-fixtures.sh: EC P-256 key..."
openssl genpkey -algorithm EC -pkeyopt ec_paramgen_curve:prime256v1 \
    -pkeyopt ec_param_enc:named_curve -out keys/ec-p256.key.pem 2>/dev/null

echo "generate-fixtures.sh: CSR from the RSA key..."
openssl req -new -key keys/rsa2048.key.pem \
    -subj "/C=GB/O=mbedtls-clu Tests/CN=test.example.com" \
    -out csr/rsa2048.csr.pem

echo "generate-fixtures.sh: CA key + self-signed CA cert..."
openssl genpkey -algorithm RSA -pkeyopt rsa_keygen_bits:2048 -out certs/ca.key.pem 2>/dev/null
openssl req -x509 -new -key certs/ca.key.pem \
    -subj "/C=GB/O=mbedtls-clu Tests/CN=mbedtls-clu Test CA" \
    -days 7300 -sha256 \
    -addext "basicConstraints=critical,CA:TRUE" \
    -addext "keyUsage=critical,keyCertSign,cRLSign" \
    -out certs/ca.cert.pem

cat > conf/req.cnf <<'EOF'
# Fixture config for tests/cases/req/05_config_extensions_applied.sh -
# exercises every [req]/[<dn>]/[<ext>] tag mbedtls-clu's config parser reads
# (see src/mbedtlsclu_common.c: parse_config_file / parse_x509_extensions).
[req]
distinguished_name = req_distinguished_name
x509_extensions = v3_ext
default_md = sha256

[req_distinguished_name]
countryName_default = GB
stateOrProvinceName_default = Test State
localityName_default = Test City
0.organizationName_default = mbedtls-clu Tests
organizationalUnitName_default = QA
commonName_default = conf-test.example.com
emailAddress_default = qa@example.com

[v3_ext]
subjectKeyIdentifier = hash
basicConstraints = critical,CA:TRUE
keyUsage = critical,digitalSignature,keyCertSign
nsCertType = server
EOF

cat > conf/ca.cnf <<'EOF'
# Fixture config for tests/cases/ca/*.sh. Paths are relative - ca test cases
# run from their own per-case workdir (see tests/lib/framework.sh
# case_workdir_init) and must create ./index.txt, ./serial, ./newcerts/ etc
# there before invoking `mbedtls-clu ca -config this-file`.
#
# mbedtls-clu's config parser (src/mbedtlsclu_common.c: locate_value) takes
# values verbatim - it does NOT expand OpenSSL-cnf-style "$dir" references -
# so every path below is written out literally rather than via $dir.
[ca]
default_ca = CA_default

[CA_default]
dir             = .
certs           = ./certs
new_certs_dir   = ./newcerts
database        = ./index.txt
serial          = ./serial
certificate     = ./ca.cert.pem
private_key     = ./ca.key.pem
default_days    = 365
default_md      = sha256
policy          = policy_any
x509_extensions = usr_cert

[policy_any]
countryName             = optional
stateOrProvinceName     = optional
organizationName        = optional
organizationalUnitName  = optional
commonName              = supplied
emailAddress            = optional

[usr_cert]
basicConstraints = CA:FALSE
keyUsage = critical,digitalSignature,keyEncipherment
EOF

# Intentionally malformed: distinguished_name points at a section that does
# not exist, and the [req] section itself is never closed. Used by
# req/08_malformed_config_errors.sh to confirm mbedtls-clu exits cleanly
# (nonzero, no crash) instead of misbehaving on a broken config.
cat > conf/malformed.cnf <<'EOF'
[req
distinguished_name = this_section_does_not_exist
x509_extensions = also_missing
EOF

echo "generate-fixtures.sh: done."
