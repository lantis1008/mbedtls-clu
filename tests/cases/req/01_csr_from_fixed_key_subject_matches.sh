. "$TESTS_DIR/lib/framework.sh"
case_workdir_init

# NOTE: mbedtls-clu's -subj requires mbedtls's native comma-separated DN
# syntax (C=..,O=..,CN=..), NOT OpenSSL's slash-prefixed syntax
# (/C=../O=../CN=..) - see KNOWN_DIFFERENCES.md. The fixture CSR was built
# with real openssl using the equivalent slash syntax; openssl is used here
# as the extractor on both sides so the DN encoding is compared the way
# openssl itself understands it, not as raw text.
SUBJ="C=GB,O=mbedtls-clu Tests,CN=test.example.com"

run_clu req -new -sha256 -key "$FIXTURES/keys/rsa2048.key.pem" -subj "$SUBJ" -out clu.csr.pem
assert_exit_zero "$LAST_EXIT" "req -new should exit 0"

clu_subject=$(openssl_out req -in clu.csr.pem -noout -subject)
fixture_subject=$(openssl_out req -in "$FIXTURES/csr/rsa2048.csr.pem" -noout -subject)
assert_eq "$clu_subject" "$fixture_subject" "CSR subject should match the openssl-generated fixture CSR (same key+DN)"
