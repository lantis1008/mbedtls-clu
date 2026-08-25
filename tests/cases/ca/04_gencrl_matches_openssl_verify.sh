. "$TESTS_DIR/lib/framework.sh"
case_workdir_init
ca_db_init

run_clu ca -config ca.cnf -keyfile ca.key.pem -cert ca.cert.pem -gencrl -out crl.pem -crl_days 30
assert_exit_zero "$LAST_EXIT" "ca -gencrl should exit 0"

run_openssl crl -in crl.pem -noout -text
assert_exit_zero "$LAST_EXIT" "openssl should parse the generated CRL"
assert_contains "$LAST_STDOUT" "mbedtls-clu Test CA" "CRL issuer should be the CA"
