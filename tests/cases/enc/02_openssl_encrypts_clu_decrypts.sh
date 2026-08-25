. "$TESTS_DIR/lib/framework.sh"
case_workdir_init

printf 'Hello, World! This is a test plaintext message.' > plain.txt

run_openssl enc -aes-256-cbc -salt -in plain.txt -out ossl.enc -k testpass123
assert_exit_zero "$LAST_EXIT" "openssl enc should exit 0"

run_clu enc -aes-256-cbc -d -in ossl.enc -out ossl.dec -k testpass123
assert_exit_zero "$LAST_EXIT" "mbedtls-clu should decrypt the openssl-encrypted file"

assert_files_equal plain.txt ossl.dec "round-tripped plaintext should match byte-for-byte"
