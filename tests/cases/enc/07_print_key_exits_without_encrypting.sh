. "$TESTS_DIR/lib/framework.sh"
case_workdir_init

printf 'Hello, World! This is a test plaintext message.' > plain.txt

run_clu enc -aes-256-cbc -S 0102030405060708 -k testpass123 -in plain.txt -out should_not_exist.enc -P
assert_exit_zero "$LAST_EXIT" "enc -P should exit 0"
assert_contains "$LAST_STDOUT" "^salt=0102030405060708$" "should print the salt"
assert_contains "$LAST_STDOUT" "^key=70898BCD0785CEA435F84F91F2BECEA57F01AB0AB95EB4495279D66A2BF6B1B7$" \
    "derived key should match the value independently verified against real openssl enc -P"
assert_contains "$LAST_STDOUT" "^iv =61C94DF9220C099B45169DA392DCE5D8$" \
    "derived iv should match the value independently verified against real openssl enc -P"

[ ! -f should_not_exist.enc ] || fail "-P should not have written -out"
