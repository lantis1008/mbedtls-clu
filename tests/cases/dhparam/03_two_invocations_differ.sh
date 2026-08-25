. "$TESTS_DIR/lib/framework.sh"
case_workdir_init

run_clu dhparam -2 -out dh1.pem 512
assert_exit_zero "$LAST_EXIT" "first dhparam generation should exit 0"

run_clu dhparam -2 -out dh2.pem 512
assert_exit_zero "$LAST_EXIT" "second dhparam generation should exit 0"

p1=$(openssl_out dhparam -in dh1.pem -text -noout)
p2=$(openssl_out dhparam -in dh2.pem -text -noout)
assert_ne "$p1" "$p2" "two independent dhparam generations must not produce the same P"
