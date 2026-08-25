. "$TESTS_DIR/lib/framework.sh"
case_workdir_init

SUBJ="C=GB,O=mbedtls-clu Tests,CN=test.example.com"

run_clu req -x509v1 -sha256 -new -key "$FIXTURES/keys/rsa2048.key.pem" -subj "$SUBJ" -days 365 -out v1.cert.pem
assert_exit_zero "$LAST_EXIT" "req -x509v1 should exit 0"
openssl_out x509 -in v1.cert.pem -noout -text > v1.text
assert_contains v1.text "Version: 1" "-x509v1 should produce a Version 1 certificate"

run_clu req -x509 -sha256 -new -key "$FIXTURES/keys/rsa2048.key.pem" -subj "$SUBJ" -days 365 -out v3.cert.pem
assert_exit_zero "$LAST_EXIT" "req -x509 should exit 0"
openssl_out x509 -in v3.cert.pem -noout -text > v3.text
assert_contains v3.text "Version: 3" "default -x509 should produce a Version 3 certificate"
