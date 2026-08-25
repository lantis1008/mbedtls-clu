. "$TESTS_DIR/lib/framework.sh"
case_workdir_init

run_clu req -x509 -sha256 -new -key "$FIXTURES/keys/rsa2048.key.pem" \
    -subj "C=GB,O=mbedtls-clu Tests,CN=test.example.com" -days 365 -out cert.pem
assert_exit_zero "$LAST_EXIT" "generating the input cert should exit 0"

run_clu x509 -in cert.pem -text -noout
assert_exit_zero "$LAST_EXIT" "x509 -text on a mbedtls-clu-generated cert should exit 0"
assert_contains "$LAST_STDOUT" "test.example.com" "output should contain the cert's subject CN"
