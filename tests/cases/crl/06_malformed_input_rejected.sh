. "$TESTS_DIR/lib/framework.sh"
case_workdir_init

printf 'not a valid crl\n' > bogus.txt

run_clu crl -in bogus.txt -text
assert_exit_nonzero "$LAST_EXIT" "malformed CRL input should be rejected cleanly, not crash"
[ ! -f core ] || fail "should not have dumped core"
