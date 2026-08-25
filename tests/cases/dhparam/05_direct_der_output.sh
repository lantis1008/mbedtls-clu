. "$TESTS_DIR/lib/framework.sh"
case_workdir_init

# NOTE: mbedtls-clu's -in is only honored together with -check (see
# dhparam.c) - "dhparam -in x.pem -outform DER -out x.der" without -check
# does NOT convert the input file; it silently ignores -in and generates a
# brand new set of parameters instead. That's a real gap (see
# tests/KNOWN_DIFFERENCES.md), so this case does not attempt a PEM->DER
# *conversion* of existing params - it only checks that DER can be requested
# directly as the output format for a freshly generated set.
run_clu dhparam -2 -outform DER -out dh.der 512
assert_exit_zero "$LAST_EXIT" "dhparam -outform DER should exit 0"

run_openssl dhparam -inform DER -in dh.der -check -noout
assert_exit_zero "$LAST_EXIT" "openssl should accept mbedtls-clu's DER output directly"
