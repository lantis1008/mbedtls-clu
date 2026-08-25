. "$TESTS_DIR/lib/framework.sh"
case_workdir_init

printf 'Hello, World! This is a test plaintext message.' > plain.txt

# -pbkdf2 is a genuinely different code path (mbedtls_pkcs5_pbkdf2_hmac_ext
# vs the legacy EVP_BytesToKey reimplementation) - test it explicitly in
# both directions, not just the default legacy KDF.
run_clu enc -aes-256-cbc -pbkdf2 -in plain.txt -out clu.enc -k testpass123
assert_exit_zero "$LAST_EXIT" "mbedtls-clu enc -pbkdf2 should exit 0"

run_openssl enc -aes-256-cbc -d -pbkdf2 -in clu.enc -out clu.dec -k testpass123
assert_exit_zero "$LAST_EXIT" "openssl -pbkdf2 should decrypt the mbedtls-clu-encrypted file"
assert_files_equal plain.txt clu.dec "clu-encrypted/openssl-decrypted plaintext should match"

run_openssl enc -aes-256-cbc -pbkdf2 -in plain.txt -out ossl.enc -k testpass123
assert_exit_zero "$LAST_EXIT" "openssl enc -pbkdf2 should exit 0"

run_clu enc -aes-256-cbc -d -pbkdf2 -in ossl.enc -out ossl.dec -k testpass123
assert_exit_zero "$LAST_EXIT" "mbedtls-clu -pbkdf2 should decrypt the openssl-encrypted file"
assert_files_equal plain.txt ossl.dec "openssl-encrypted/clu-decrypted plaintext should match"
