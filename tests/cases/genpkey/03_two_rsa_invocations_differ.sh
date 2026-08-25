. "$TESTS_DIR/lib/framework.sh"
case_workdir_init

run_clu genpkey -algorithm rsa -pkeyopt rsa_keygen_bits:2048 -out rsa1.key
assert_exit_zero "$LAST_EXIT" "first genpkey rsa should exit 0"

run_clu genpkey -algorithm rsa -pkeyopt rsa_keygen_bits:2048 -out rsa2.key
assert_exit_zero "$LAST_EXIT" "second genpkey rsa should exit 0"

mod1=$(openssl_out rsa -in rsa1.key -noout -modulus)
mod2=$(openssl_out rsa -in rsa2.key -noout -modulus)
assert_ne "$mod1" "$mod2" "two independent RSA key generations must not produce the same modulus"
