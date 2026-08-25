. "$TESTS_DIR/lib/framework.sh"
case_workdir_init

run_clu x509 -in "$FIXTURES/certs/ca.cert.pem" -noout
assert_exit_zero "$LAST_EXIT" "x509 -noout (no -text/-serial) should exit 0"
[ ! -s "$LAST_STDOUT" ] || fail "-noout with no other display flag should produce no stdout"
