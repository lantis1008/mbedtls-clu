. "$TESTS_DIR/lib/framework.sh"
case_workdir_init

# -subj now accepts OpenSSL's "/type=value/type=value" syntax in addition
# to mbedtls's native "type=value,type=value" syntax (see
# convert_openssl_subj_to_mbedtls_subj() in req.c). Both should produce the
# same subject for the same logical DN.
run_clu req -new -sha256 -key "$FIXTURES/keys/rsa2048.key.pem" \
    -subj "/C=GB/O=mbedtls-clu Tests/CN=test.example.com" -out slash.csr.pem
assert_exit_zero "$LAST_EXIT" "req -new with slash-syntax -subj should exit 0"

run_clu req -new -sha256 -key "$FIXTURES/keys/rsa2048.key.pem" \
    -subj "C=GB,O=mbedtls-clu Tests,CN=test.example.com" -out comma.csr.pem
assert_exit_zero "$LAST_EXIT" "req -new with comma-syntax -subj should exit 0"

slash_subject=$(openssl_out req -in slash.csr.pem -noout -subject)
comma_subject=$(openssl_out req -in comma.csr.pem -noout -subject)
assert_eq "$slash_subject" "$comma_subject" "slash and comma -subj syntax for the same DN should produce the same subject"
