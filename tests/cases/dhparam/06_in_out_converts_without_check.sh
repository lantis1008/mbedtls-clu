. "$TESTS_DIR/lib/framework.sh"
case_workdir_init

# Regression test for the core bug fixed this session: "-in x.pem -out
# y.pem" (no -check) used to silently ignore -in and generate brand new,
# unrelated parameters instead of converting the input. Confirmed against
# real openssl (which performs a byte-identical passthrough here) before
# fixing - see KNOWN_DIFFERENCES.md history / git log.
run_clu dhparam -2 -out orig.pem 512
assert_exit_zero "$LAST_EXIT" "generating the input params should exit 0"

run_clu dhparam -in orig.pem -out converted.pem
assert_exit_zero "$LAST_EXIT" "dhparam -in ... -out ... (no -check) should exit 0"

assert_files_equal orig.pem converted.pem "-in -out without -check should convert (pass through) the input, not generate fresh params"
