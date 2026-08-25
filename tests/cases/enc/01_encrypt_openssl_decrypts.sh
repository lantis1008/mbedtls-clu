. "$TESTS_DIR/lib/framework.sh"
case_workdir_init

printf 'Hello, World! This is a test plaintext message.' > plain.txt

run_clu enc -aes-256-cbc -salt -e -in plain.txt -out clu.enc -k testpass123
assert_exit_zero "$LAST_EXIT" "mbedtls-clu enc should exit 0"

run_openssl enc -aes-256-cbc -d -in clu.enc -out clu.dec -k testpass123
assert_exit_zero "$LAST_EXIT" "openssl should decrypt the mbedtls-clu-encrypted file"

assert_files_equal plain.txt clu.dec "round-tripped plaintext should match byte-for-byte"
