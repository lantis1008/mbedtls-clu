. "$TESTS_DIR/lib/framework.sh"
case_workdir_init

# NOTE: -set_serial is base-10 only in mbedtls-clu (openssl also accepts a
# 0x-prefixed hex value) - see KNOWN_DIFFERENCES.md. 4096 decimal = 0x1000.
SUBJ="C=GB,O=mbedtls-clu Tests,CN=test.example.com"

run_clu req -x509 -sha256 -new -key "$FIXTURES/keys/rsa2048.key.pem" -subj "$SUBJ" \
    -days 3650 -set_serial 4096 -out clu.cert.pem
assert_exit_zero "$LAST_EXIT" "req -x509 should exit 0"

subject=$(openssl_out x509 -in clu.cert.pem -noout -subject)
issuer=$(openssl_out x509 -in clu.cert.pem -noout -issuer)
serial=$(openssl_out x509 -in clu.cert.pem -noout -serial)

case "$subject" in
    *"C = GB"*"O = mbedtls-clu Tests"*"CN = test.example.com"*) ;;
    *) fail "unexpected subject: $subject" ;;
esac
subject_dn=${subject#subject=}
issuer_dn=${issuer#issuer=}
assert_eq "$issuer_dn" "$subject_dn" "self-signed cert issuer should equal its subject"
assert_eq "$serial" "serial=1000" "serial 4096 decimal should be 0x1000 hex"
