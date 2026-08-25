. "$TESTS_DIR/lib/framework.sh"
case_workdir_init

# Direct regression test for the genpkey_main crash fixed this session:
# "ec_paramgen_curve:prime256v1" (an OpenSSL curve alias mbedtls doesn't
# recognize by that name - see KNOWN_DIFFERENCES.md) used to segfault instead
# of erroring, because entropy_init() ran after the argument-parsing loop's
# error paths instead of before them.
run_clu genpkey -algorithm ec -pkeyopt ec_paramgen_curve:prime256v1 -out ec.key
assert_exit_nonzero "$LAST_EXIT" "an unrecognized curve name should be rejected cleanly, not crash"
[ ! -f ec.key ] || fail "should not have produced an output file"
