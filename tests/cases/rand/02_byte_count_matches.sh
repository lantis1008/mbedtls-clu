. "$TESTS_DIR/lib/framework.sh"
case_workdir_init

# Without -hex, mbedtls-clu rand still goes through mbedtls_mpi_write_file in
# hex form (see rand.c) - there is no raw-binary-output mode, unlike openssl
# rand <n> which writes raw bytes. This case checks the hex-digit count for a
# given byte count is consistent (2*n hex digits), which is what's actually
# comparable between the two tools here (see KNOWN_DIFFERENCES.md).
run_clu rand 20
assert_exit_zero "$LAST_EXIT" "rand 20 should exit 0"

hex=$(cat "$LAST_STDOUT" | tr -d '[:space:]')
assert_eq "${#hex}" "40" "rand 20 should produce 40 hex characters (20 bytes)"

run_openssl rand 20
assert_exit_zero "$LAST_EXIT" "openssl rand 20 should exit 0"
openssl_byte_count=$(wc -c < "$LAST_STDOUT" | tr -d '[:space:]')
assert_eq "$openssl_byte_count" "20" "openssl rand 20 should write exactly 20 raw bytes"
