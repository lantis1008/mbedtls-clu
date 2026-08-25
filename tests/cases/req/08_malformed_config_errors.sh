. "$TESTS_DIR/lib/framework.sh"
case_workdir_init

run_clu req -x509 -new -key "$FIXTURES/keys/rsa2048.key.pem" -config "$FIXTURES/conf/malformed.cnf" -out bad.cert.pem
assert_exit_nonzero "$LAST_EXIT" "a malformed config file should be rejected cleanly, not crash"
[ ! -f bad.cert.pem ] || fail "should not have produced an output file"
[ ! -f core ] || fail "should not have dumped core"
