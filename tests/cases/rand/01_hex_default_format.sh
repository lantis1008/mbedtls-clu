. "$TESTS_DIR/lib/framework.sh"
case_workdir_init

# mbedtls-clu rand -hex N should match openssl's rand -hex N in length (2N
# hex chars) even though the VALUES necessarily differ (see
# 03_two_invocations_differ.sh) and the exact text formatting doesn't:
# mbedtls-clu emits uppercase hex with a trailing CRLF (via mbedtls's own
# mbedtls_mpi_write_file), openssl emits lowercase with a trailing LF - see
# tests/KNOWN_DIFFERENCES.md. [:space:] strips the CRLF along with any LF.
run_clu rand -hex 16
assert_exit_zero "$LAST_EXIT" "rand -hex 16 should exit 0"

hex=$(cat "$LAST_STDOUT" | tr -d '[:space:]')
len=${#hex}
assert_eq "$len" "32" "rand -hex 16 should produce 32 hex characters"

case "$hex" in
    *[!0-9a-fA-F]*) fail "rand -hex 16 output is not hex: $hex" ;;
esac
