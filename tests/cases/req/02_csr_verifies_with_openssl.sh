. "$TESTS_DIR/lib/framework.sh"
case_workdir_init

run_clu req -new -sha256 -key "$FIXTURES/keys/rsa2048.key.pem" -subj "C=GB,O=mbedtls-clu Tests,CN=test.example.com" -out clu.csr.pem
assert_exit_zero "$LAST_EXIT" "req -new should exit 0"

assert_csr_verify_ok clu.csr.pem "mbedtls-clu-generated CSR should self-verify (correct signature) under openssl"
