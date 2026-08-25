. "$TESTS_DIR/lib/framework.sh"
case_workdir_init

run_clu genpkey -algorithm ec -pkeyopt ec_paramgen_curve:secp256r1 -out ec1.key
assert_exit_zero "$LAST_EXIT" "first genpkey ec should exit 0"

run_clu genpkey -algorithm ec -pkeyopt ec_paramgen_curve:secp256r1 -out ec2.key
assert_exit_zero "$LAST_EXIT" "second genpkey ec should exit 0"

priv1=$(openssl_out ec -in ec1.key -noout -text | grep -A3 "priv:")
priv2=$(openssl_out ec -in ec2.key -noout -text | grep -A3 "priv:")
assert_ne "$priv1" "$priv2" "two independent EC key generations must not produce the same private scalar"
