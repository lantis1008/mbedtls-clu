. "$TESTS_DIR/lib/framework.sh"
case_workdir_init

# mbedtls-clu's -text formatting is not line-for-line identical to openssl's
# (see KNOWN_DIFFERENCES.md) - only loose containment of the subject CN is
# checked here, not a full-format match.
run_clu x509 -in "$FIXTURES/certs/ca.cert.pem" -text -noout
assert_exit_zero "$LAST_EXIT" "x509 -text on an openssl-generated cert should exit 0"
assert_contains "$LAST_STDOUT" "mbedtls-clu Test CA" "output should contain the cert's subject CN"
