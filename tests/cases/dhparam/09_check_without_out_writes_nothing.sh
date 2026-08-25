. "$TESTS_DIR/lib/framework.sh"
case_workdir_init

# Regression guard: "-in x -check" with no -out must not attempt to write
# anything (real openssl falls back to writing to stdout when -out is
# omitted; mbedtls-clu has no such fallback - see KNOWN_DIFFERENCES.md - so
# it must skip the write step entirely here rather than erroring out trying
# to fopen() a NULL output path). This exact case briefly regressed while
# implementing the -in/-check restructuring this session.
run_clu dhparam -2 -out orig.pem 512
assert_exit_zero "$LAST_EXIT" "generating the input params should exit 0"

run_clu dhparam -in orig.pem -check
assert_exit_zero "$LAST_EXIT" "dhparam -in ... -check with no -out should still exit 0"
assert_contains "$LAST_STDOUT" "OK" "-check should still report the params are OK"
