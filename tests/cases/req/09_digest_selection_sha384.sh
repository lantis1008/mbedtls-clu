. "$TESTS_DIR/lib/framework.sh"
case_workdir_init

run_clu req -x509 -sha384 -new -key "$FIXTURES/keys/rsa2048.key.pem" \
    -subj "C=GB,O=mbedtls-clu Tests,CN=test.example.com" -days 365 -out sha384.cert.pem
assert_exit_zero "$LAST_EXIT" "req -x509 -sha384 should exit 0"

openssl_out x509 -in sha384.cert.pem -noout -text > cert.text
assert_contains cert.text "sha384WithRSAEncryption" "-sha384 should select the SHA-384 signature algorithm"
