. "$TESTS_DIR/lib/framework.sh"
case_workdir_init

# -check does not preclude also writing -out (confirmed against real
# openssl: "dhparam -in x.pem -check -out y.pem" both validates AND writes,
# it doesn't short-circuit before the write step).
run_clu dhparam -2 -out orig.pem 512
assert_exit_zero "$LAST_EXIT" "generating the input params should exit 0"

run_clu dhparam -in orig.pem -check -out checked.pem
assert_exit_zero "$LAST_EXIT" "dhparam -in ... -check -out ... should exit 0"
assert_contains "$LAST_STDOUT" "OK" "-check should still report the params are OK"

assert_files_equal orig.pem checked.pem "-check combined with -out should still write the (validated) params through"
