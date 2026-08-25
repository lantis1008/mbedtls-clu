. "$TESTS_DIR/lib/framework.sh"
case_workdir_init

# NOTE: mbedtls-clu's ec_paramgen_curve only accepts mbedtls's own curve
# names (e.g. secp256r1), not OpenSSL's common aliases (e.g. prime256v1 for
# the same curve) - see tests/KNOWN_DIFFERENCES.md. Use the mbedtls name here.
run_clu genpkey -algorithm ec -pkeyopt ec_paramgen_curve:secp256r1 -out ec.key
assert_exit_zero "$LAST_EXIT" "genpkey ec secp256r1 should exit 0"

run_openssl ec -in ec.key -check -noout
assert_exit_zero "$LAST_EXIT" "openssl should accept the generated EC key as structurally valid"

curve=$(openssl_out ec -in ec.key -text -noout | grep -i "ASN1 OID")
case "$curve" in
    *prime256v1*) ;;
    *) fail "expected curve prime256v1 (secp256r1), openssl reports: $curve" ;;
esac
