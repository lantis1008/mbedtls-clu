. "$TESTS_DIR/lib/framework.sh"
case_workdir_init
ca_db_init

run_clu ca -config ca.cnf -keyfile ca.key.pem -cert ca.cert.pem -gencrl -out crl.pem -crl_days 30
assert_exit_zero "$LAST_EXIT" "ca -gencrl should exit 0"

run_clu crl -in crl.pem -out crl.der -outform DER
assert_exit_zero "$LAST_EXIT" "crl -out ... -outform DER should exit 0"

run_openssl crl -in crl.der -inform DER -noout -text
assert_exit_zero "$LAST_EXIT" "openssl should accept mbedtls-clu's DER output as a valid CRL"
