. "$TESTS_DIR/lib/framework.sh"
case_workdir_init

run_clu genpkey -algorithm rsa -pkeyopt rsa_keygen_bits:2048 -outform DER -out rsa.der
assert_exit_zero "$LAST_EXIT" "genpkey -outform DER should exit 0"

run_openssl rsa -inform DER -in rsa.der -check -noout
assert_exit_zero "$LAST_EXIT" "openssl should accept mbedtls-clu's DER-format key directly"
assert_contains "$LAST_STDOUT" "RSA key ok" "openssl rsa -check should report the key is ok"
