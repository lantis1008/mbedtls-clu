. "$TESTS_DIR/lib/framework.sh"
case_workdir_init

# KNOWN GAP (see KNOWN_DIFFERENCES.md): "-in <csr>" only prints the CSR's
# summary (subject/key/signature algorithm) via mbedtls_x509_csr_info; it is
# never fed into an -x509 self-signed conversion or any other output-writing
# path in req.c. "-out"/"-outform" are silently ignored whenever "-in" is
# used, and the command still reports success (exit 0) even though nothing
# is written. This pins down that actual (surprising) behavior so it's not
# mistaken for a "convert CSR to cert" feature and so a future accidental
# behavior change is caught either way.
run_clu req -sha256 -in "$FIXTURES/csr/rsa2048.csr.pem" -key "$FIXTURES/keys/rsa2048.key.pem" \
    -subj "C=GB,O=mbedtls-clu Tests,CN=test.example.com" -text -out display.out
assert_exit_zero "$LAST_EXIT" "req -in should exit 0"
assert_contains "$LAST_STDOUT" "subject name" "req -in should print the CSR's subject to stdout"

[ ! -f display.out ] || fail "req -in should not have written an -out file (documents current display-only behavior)"
