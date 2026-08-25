. "$TESTS_DIR/lib/framework.sh"
case_workdir_init

# When an explicit numbits argument is given alongside -in, it takes
# precedence and fresh parameters are generated (matching real openssl,
# which does the same and prints "Warning, input file ... ignored").
run_clu dhparam -2 -out orig.pem 512
assert_exit_zero "$LAST_EXIT" "generating the input params should exit 0"

run_clu dhparam -in orig.pem -out overridden.pem 256
assert_exit_zero "$LAST_EXIT" "dhparam -in ... <numbits> should exit 0"
assert_contains "$LAST_STDOUT" "ignored" "an explicit numbits argument should warn that -in is being ignored"

assert_files_differ orig.pem overridden.pem "an explicit numbits argument should override -in and generate fresh params"

p_overridden=$(openssl_out dhparam -in overridden.pem -text -noout | grep -A1 "DH Parameters")
case "$p_overridden" in
    *"256 bit"*) ;;
    *) fail "expected the overridden output to be 256 bits: $p_overridden" ;;
esac
