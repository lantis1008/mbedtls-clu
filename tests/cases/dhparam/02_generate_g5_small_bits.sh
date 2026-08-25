. "$TESTS_DIR/lib/framework.sh"
case_workdir_init

run_clu dhparam -5 -out dh.pem 512
assert_exit_zero "$LAST_EXIT" "dhparam -5 512 should exit 0"

g=$(openssl_out dhparam -in dh.pem -text -noout | grep -Eo 'G: *[0-9]+ \(0x[0-9a-fA-F]+\)')
case "$g" in
    *"(0x5)") ;;
    *) fail "expected generator 5, openssl reports: $g" ;;
esac
