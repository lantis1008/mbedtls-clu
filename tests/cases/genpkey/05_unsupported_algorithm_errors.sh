. "$TESTS_DIR/lib/framework.sh"
case_workdir_init

# mbedtls-clu only supports rsa/ec (see KNOWN_DIFFERENCES.md). This also
# regression-guards the genpkey_main crash fixed this session: an early
# argument-parsing error used to call mbedtls_entropy_free() on an
# uninitialized entropy context and segfault instead of erroring cleanly.
run_clu genpkey -algorithm dsa -out dsa.key
assert_exit_nonzero "$LAST_EXIT" "genpkey -algorithm dsa should be rejected, not crash"
[ ! -f dsa.key ] || fail "genpkey -algorithm dsa should not have produced an output file"
