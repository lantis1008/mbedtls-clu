. "$TESTS_DIR/lib/framework.sh"
case_workdir_init

# mbedtls-clu's own "-serial" prints colon-separated hex (e.g. "0A:76:D3"),
# openssl's prints plain concatenated hex (e.g. "0A76D3") - see
# KNOWN_DIFFERENCES.md. Strip colons before comparing.
run_clu x509 -in "$FIXTURES/certs/ca.cert.pem" -serial -noout
assert_exit_zero "$LAST_EXIT" "x509 -serial on an openssl-generated cert should exit 0"
clu_serial=$(cat "$LAST_STDOUT" | tr -d ':\r\n')
clu_serial=${clu_serial#serial=}
clu_serial=$(printf '%s' "$clu_serial" | tr 'a-f' 'A-F')

openssl_serial=$(openssl_out x509 -in "$FIXTURES/certs/ca.cert.pem" -noout -serial)
openssl_serial=${openssl_serial#serial=}

assert_eq "$clu_serial" "$openssl_serial" "serial mismatch between mbedtls-clu and openssl"
