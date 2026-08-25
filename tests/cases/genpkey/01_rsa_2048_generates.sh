. "$TESTS_DIR/lib/framework.sh"
case_workdir_init

run_clu genpkey -algorithm rsa -pkeyopt rsa_keygen_bits:2048 -out rsa.key
assert_exit_zero "$LAST_EXIT" "genpkey rsa 2048 should exit 0"

run_openssl rsa -in rsa.key -check -noout
assert_exit_zero "$LAST_EXIT" "openssl should accept the generated RSA key as structurally valid"
assert_contains "$LAST_STDOUT" "RSA key ok" "openssl rsa -check should report the key is ok"
