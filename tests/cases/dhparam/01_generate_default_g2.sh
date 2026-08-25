. "$TESTS_DIR/lib/framework.sh"
case_workdir_init

# Sanity check the default generator (-2) produces something openssl
# considers structurally valid. Deliberately small (512 bits, well below the
# documented default of 2048): a real 2048-bit safe-prime search takes
# ~30s with this tool, which is too slow for a routine test run - see
# tests/README.md. 512 bits is still large enough that openssl's -check
# (which rejects anything under ~512 bits outright) exercises real
# validation, not just a trivial small-modulus short-circuit.
run_clu dhparam -2 -out dh.pem 512
assert_exit_zero "$LAST_EXIT" "dhparam -2 512 should exit 0"

run_openssl dhparam -in dh.pem -check -noout
assert_exit_zero "$LAST_EXIT" "openssl should accept mbedtls-clu-generated DH params"
