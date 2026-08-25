. "$TESTS_DIR/lib/framework.sh"
case_workdir_init
ca_db_init

run_clu ca -config ca.cnf -keyfile ca.key.pem -cert ca.cert.pem \
    -in "$FIXTURES/csr/rsa2048.csr.pem" -out signed.pem -days 365 -md sha256
assert_exit_zero "$LAST_EXIT" "ca signing should exit 0"

[ -s index.txt ] || fail "index.txt should have a line for the issued cert"
line=$(head -1 index.txt)
status=$(printf '%s' "$line" | cut -f1)
assert_eq "$status" "V" "expected the database line's status column to be 'V' (valid): $line"
case "$line" in
    *1000*) ;;
    *) fail "expected the database line to contain the issued serial 1000: $line" ;;
esac
