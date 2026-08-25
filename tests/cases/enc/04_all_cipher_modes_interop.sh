. "$TESTS_DIR/lib/framework.sh"
case_workdir_init

printf 'Hello, World! This is a test plaintext message.' > plain.txt

for mode in cbc cfb ofb ctr
do
    run_clu enc -aes-256-$mode -in plain.txt -out clu.$mode -k testpass123
    assert_exit_zero "$LAST_EXIT" "mbedtls-clu enc -aes-256-$mode should exit 0"

    run_openssl enc -aes-256-$mode -d -in clu.$mode -out clu.$mode.dec -k testpass123
    assert_exit_zero "$LAST_EXIT" "openssl should decrypt aes-256-$mode from mbedtls-clu"
    assert_files_equal plain.txt clu.$mode.dec "aes-256-$mode: clu-encrypted/openssl-decrypted should match"

    run_openssl enc -aes-256-$mode -in plain.txt -out ossl.$mode -k testpass123
    assert_exit_zero "$LAST_EXIT" "openssl enc -aes-256-$mode should exit 0"

    run_clu enc -aes-256-$mode -d -in ossl.$mode -out ossl.$mode.dec -k testpass123
    assert_exit_zero "$LAST_EXIT" "mbedtls-clu should decrypt aes-256-$mode from openssl"
    assert_files_equal plain.txt ossl.$mode.dec "aes-256-$mode: openssl-encrypted/clu-decrypted should match"
done
