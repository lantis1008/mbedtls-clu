. "$TESTS_DIR/lib/framework.sh"
case_workdir_init

# Reverse-interop direction: openssl generates, mbedtls-clu reads + checks.
run_openssl dhparam -out dh.pem 512
assert_exit_zero "$LAST_EXIT" "openssl dhparam 512 should exit 0"

run_clu dhparam -in dh.pem -check
assert_exit_zero "$LAST_EXIT" "mbedtls-clu should accept and validate openssl-generated DH params"
assert_contains "$LAST_STDOUT" "OK" "mbedtls-clu -check should report the params are OK"
