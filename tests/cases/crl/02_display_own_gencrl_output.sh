. "$TESTS_DIR/lib/framework.sh"
case_workdir_init
ca_db_init

run_clu ca -config ca.cnf -keyfile ca.key.pem -cert ca.cert.pem -gencrl -out crl.pem -crl_days 30
assert_exit_zero "$LAST_EXIT" "ca -gencrl should exit 0"

run_clu crl -in crl.pem -text -noout
assert_exit_zero "$LAST_EXIT" "crl -text on a mbedtls-clu-generated CRL should exit 0"
assert_contains "$LAST_STDOUT" "mbedtls-clu Test CA" "output should contain the CRL issuer's CN"
