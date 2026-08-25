. "$TESTS_DIR/lib/framework.sh"
case_workdir_init

# A malformed slash-syntax segment (e.g. an empty "//" field, or any
# segment with no "type=" before the next '/' or end of string) is
# rejected cleanly. This is a deliberate improvement over real openssl,
# which instead prints a warning and silently drops the affected field,
# continuing with a truncated subject - confirmed empirically before
# implementing (see KNOWN_DIFFERENCES.md).
run_clu req -new -sha256 -key "$FIXTURES/keys/rsa2048.key.pem" \
    -subj "/C=GB//CN=test.example.com" -out bad.csr.pem
assert_exit_nonzero "$LAST_EXIT" "an empty '//' segment should be rejected, not silently dropped"
[ ! -f bad.csr.pem ] || fail "should not have produced an output file"
