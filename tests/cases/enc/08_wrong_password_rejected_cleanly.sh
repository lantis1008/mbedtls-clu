. "$TESTS_DIR/lib/framework.sh"
case_workdir_init

printf 'Hello, World! This is a test plaintext message.' > plain.txt

run_clu enc -aes-256-cbc -in plain.txt -out good.enc -k rightpassword
assert_exit_zero "$LAST_EXIT" "encrypting with the right password should exit 0"

run_clu enc -aes-256-cbc -d -in good.enc -out bad.dec -k wrongpassword
assert_exit_nonzero "$LAST_EXIT" "decrypting with the wrong password should be rejected cleanly, not crash"
[ "$LAST_EXIT" -lt 128 ] || fail "should not have been killed by a signal (crash)"
