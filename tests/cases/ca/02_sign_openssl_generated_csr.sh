. "$TESTS_DIR/lib/framework.sh"
case_workdir_init
ca_db_init

# Genuinely external input: a CSR built fresh with real openssl (not the
# checked-in fixture CSR), signed by mbedtls-clu.
run_openssl req -new -key "$FIXTURES/keys/ec-p256.key.pem" -subj "/C=GB/O=mbedtls-clu Tests/CN=fresh.example.com" -out fresh.csr.pem
assert_exit_zero "$LAST_EXIT" "generating the CSR with real openssl should exit 0"

run_clu ca -config ca.cnf -keyfile ca.key.pem -cert ca.cert.pem \
    -in fresh.csr.pem -out signed.pem -days 365 -md sha256
assert_exit_zero "$LAST_EXIT" "ca signing an openssl-generated CSR should exit 0"

assert_verify_ok signed.pem ca.cert.pem "cert signed from an openssl-generated CSR should verify"
