. "$TESTS_DIR/lib/framework.sh"
case_workdir_init
ca_db_init

# KNOWN BUG (see KNOWN_DIFFERENCES.md): ca.c's -revoke serial lookup compares
# the certificate's serial as formatted by mbedtls_x509_serial_gets()
# (colon-separated hex, e.g. "10:00") against the database's stored serial
# (colon-less, e.g. "1000") with a plain strcmp() - they can never match for
# any multi-byte serial, so revoke fails to find any cert this tool itself
# just issued. This case encodes the CORRECT/intended behavior and is
# expected to fail until that's fixed; it will start passing automatically
# once it is.
run_clu ca -config ca.cnf -keyfile ca.key.pem -cert ca.cert.pem \
    -in "$FIXTURES/csr/rsa2048.csr.pem" -out signed.pem -days 365 -md sha256
assert_exit_zero "$LAST_EXIT" "ca signing should exit 0"

run_clu ca -config ca.cnf -keyfile ca.key.pem -cert ca.cert.pem -revoke signed.pem
assert_exit_zero "$LAST_EXIT" "ca -revoke should exit 0 (see KNOWN_DIFFERENCES.md: ca.c revoke serial-format mismatch)"

line=$(head -1 index.txt)
status=$(printf '%s' "$line" | cut -f1)
assert_eq "$status" "R" "revoked cert's database status should become 'R': $line"
