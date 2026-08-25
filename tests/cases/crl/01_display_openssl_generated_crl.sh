. "$TESTS_DIR/lib/framework.sh"
case_workdir_init
ca_db_init

# Real openssl has no -crl_days flag (unlike mbedtls-clu's ca) - CRL
# duration is config-only, via default_crl_days in [CA_default].
sed 's/^default_days.*/&\ndefault_crl_days = 30/' ca.cnf > ca_with_crl_days.cnf

run_openssl ca -config ca_with_crl_days.cnf -gencrl -out crl.pem
assert_exit_zero "$LAST_EXIT" "openssl ca -gencrl should exit 0"

run_clu crl -in crl.pem -text -noout
assert_exit_zero "$LAST_EXIT" "crl -text on an openssl-generated CRL should exit 0"
assert_contains "$LAST_STDOUT" "mbedtls-clu Test CA" "output should contain the CRL issuer's CN"
