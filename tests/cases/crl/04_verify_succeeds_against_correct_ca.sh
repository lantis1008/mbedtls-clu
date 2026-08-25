. "$TESTS_DIR/lib/framework.sh"
case_workdir_init
ca_db_init

run_clu ca -config ca.cnf -keyfile ca.key.pem -cert ca.cert.pem -gencrl -out crl.pem -crl_days 30
assert_exit_zero "$LAST_EXIT" "ca -gencrl should exit 0"

run_clu crl -in crl.pem -verify -CAfile ca.cert.pem
assert_exit_zero "$LAST_EXIT" "crl -verify against the correct CA should exit 0"
assert_contains "$LAST_STDOUT" "verify OK" "should report verify OK"
