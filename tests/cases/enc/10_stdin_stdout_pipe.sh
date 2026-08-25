. "$TESTS_DIR/lib/framework.sh"
case_workdir_init

# enc is a deliberate, documented exception to the tool-wide "no utility
# falls back to stdin/stdout" convention (see KNOWN_DIFFERENCES.md) -
# piping is central to how `openssl enc` is actually used. run_clu/
# run_openssl don't support stdin redirection, so invoke the binaries
# directly here.
printf 'Hello, World! This is a test plaintext message.' > plain.txt

"$MBEDTLS_CLU_BIN" enc -aes-256-cbc -k testpass123 < plain.txt > piped.enc 2>"$WORKDIR/clu.stderr"
LAST_EXIT=$?
assert_exit_zero "$LAST_EXIT" "enc with no -in/-out (stdin/stdout) should exit 0"

"$OPENSSL_BIN" enc -aes-256-cbc -d -in piped.enc -out piped.dec -k testpass123 >"$WORKDIR/openssl.stdout" 2>"$WORKDIR/openssl.stderr"
LAST_EXIT=$?
LAST_STDOUT="$WORKDIR/openssl.stdout"
assert_exit_zero "$LAST_EXIT" "openssl should decrypt what came out of the pipe"

assert_files_equal plain.txt piped.dec "plaintext piped through stdin/stdout should round-trip correctly via openssl"
