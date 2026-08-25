. "$TESTS_DIR/lib/framework.sh"
case_workdir_init
ca_db_init

run_clu ca -config ca.cnf -keyfile ca.key.pem -cert ca.cert.pem \
    -in "$FIXTURES/csr/rsa2048.csr.pem" -out signed.pem -days 365 -md sha256
assert_exit_zero "$LAST_EXIT" "ca signing a fixture CSR should exit 0"

assert_verify_ok signed.pem ca.cert.pem "mbedtls-clu-signed cert should verify against the CA cert with openssl"
