. "$TESTS_DIR/lib/framework.sh"
case_workdir_init
ca_db_init

run_clu ca -config ca.cnf -keyfile ca.key.pem -cert ca.cert.pem -gencrl -out crl.pem -crl_days 30
assert_exit_zero "$LAST_EXIT" "ca -gencrl should exit 0"

# A genuinely unrelated self-signed cert, built fresh with real openssl.
run_openssl req -x509 -new -key "$FIXTURES/keys/ec-p256.key.pem" \
    -subj "/C=GB/O=mbedtls-clu Tests/CN=Unrelated CA" -days 365 -out unrelated.cert.pem
assert_exit_zero "$LAST_EXIT" "generating the unrelated cert should exit 0"

run_clu crl -in crl.pem -verify -CAfile unrelated.cert.pem
assert_exit_nonzero "$LAST_EXIT" "crl -verify against an unrelated CA should be rejected cleanly, not crash"
