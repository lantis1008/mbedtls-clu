. "$TESTS_DIR/lib/framework.sh"
case_workdir_init

printf 'Hello, World! This is a test plaintext message.' > plain.txt

run_clu enc -aes-256-cbc -a -in plain.txt -out clu.b64 -k testpass123
assert_exit_zero "$LAST_EXIT" "mbedtls-clu enc -a should exit 0"

run_openssl enc -aes-256-cbc -d -a -in clu.b64 -out clu.b64.dec -k testpass123
assert_exit_zero "$LAST_EXIT" "openssl should decrypt the base64 output"
assert_files_equal plain.txt clu.b64.dec "clu-encrypted/openssl-decrypted (base64) should match"

run_openssl enc -aes-256-cbc -a -in plain.txt -out ossl.b64 -k testpass123
assert_exit_zero "$LAST_EXIT" "openssl enc -a should exit 0"

run_clu enc -aes-256-cbc -d -a -in ossl.b64 -out ossl.b64.dec -k testpass123
assert_exit_zero "$LAST_EXIT" "mbedtls-clu should decrypt openssl's base64 output"
assert_files_equal plain.txt ossl.b64.dec "openssl-encrypted/clu-decrypted (base64) should match"
