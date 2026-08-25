. "$TESTS_DIR/lib/framework.sh"
case_workdir_init
ca_db_init

run_clu ca -config ca.cnf -keyfile ca.key.pem -cert ca.cert.pem -gencrl -out crl.pem -crl_days 30
assert_exit_zero "$LAST_EXIT" "ca -gencrl should exit 0"

run_clu crl -in crl.pem -issuer -lastupdate -nextupdate -noout
assert_exit_zero "$LAST_EXIT" "crl -issuer -lastupdate -nextupdate should exit 0"
assert_contains "$LAST_STDOUT" "^issuer=" "output should contain an issuer= line"
assert_contains "$LAST_STDOUT" "^lastUpdate=" "output should contain a lastUpdate= line"
assert_contains "$LAST_STDOUT" "^nextUpdate=" "output should contain a nextUpdate= line"
assert_contains "$LAST_STDOUT" "mbedtls-clu Test CA" "issuer line should contain the CA's CN"
