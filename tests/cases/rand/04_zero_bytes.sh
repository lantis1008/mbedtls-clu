. "$TESTS_DIR/lib/framework.sh"
case_workdir_init

# Edge case: 0 bytes requested should not crash. Real openssl actually
# rejects "rand -hex 0" outright (requires a positive count) - mbedtls-clu is
# more lenient and accepts it, producing empty output. Both are acceptable
# ("doesn't crash"); this only pins down mbedtls-clu's own side.
run_clu rand -hex 0
assert_exit_zero "$LAST_EXIT" "rand -hex 0 should exit 0, not crash"
hex=$(cat "$LAST_STDOUT" | tr -d '[:space:]')
assert_eq "$hex" "" "rand -hex 0 should produce no hex digits (only the trailing CRLF)"
