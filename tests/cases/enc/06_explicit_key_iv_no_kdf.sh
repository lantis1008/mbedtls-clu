. "$TESTS_DIR/lib/framework.sh"
case_workdir_init

printf 'Hello, World! This is a test plaintext message.' > plain.txt

K=000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f
IV=000102030405060708090a0b0c0d0e0f

run_clu enc -aes-256-cbc -K "$K" -iv "$IV" -in plain.txt -out clu.enc
assert_exit_zero "$LAST_EXIT" "mbedtls-clu enc -K/-iv should exit 0"

# Raw -K/-iv mode bypasses the KDF entirely - no "Salted__" header at all.
case "$(head -c 8 clu.enc)" in
    Salted__) fail "raw -K/-iv mode should not write a Salted__ header" ;;
esac

run_openssl enc -aes-256-cbc -K "$K" -iv "$IV" -d -in clu.enc -out clu.dec
assert_exit_zero "$LAST_EXIT" "openssl should decrypt with the same raw key/iv"
assert_files_equal plain.txt clu.dec "round-tripped plaintext should match"

run_openssl enc -aes-256-cbc -K "$K" -iv "$IV" -in plain.txt -out ossl.enc
assert_exit_zero "$LAST_EXIT" "openssl enc -K/-iv should exit 0"

run_clu enc -aes-256-cbc -K "$K" -iv "$IV" -d -in ossl.enc -out ossl.dec
assert_exit_zero "$LAST_EXIT" "mbedtls-clu should decrypt openssl's raw-key output"
assert_files_equal plain.txt ossl.dec "round-tripped plaintext should match (reverse direction)"
