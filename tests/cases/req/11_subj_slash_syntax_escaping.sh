. "$TESTS_DIR/lib/framework.sh"
case_workdir_init

# The core trap in supporting OpenSSL's slash syntax: OpenSSL uses '/' as
# the field separator, so a literal comma in a value needs NO escaping
# there (unlike mbedtls's own comma-separated syntax, where it does) - a
# naive '/'-to-','-substitution would misparse "Smith, Jones and Co" as two
# fields. Verified against real openssl's own -subj behavior before
# implementing (see git history / KNOWN_DIFFERENCES.md).
run_clu req -new -sha256 -key "$FIXTURES/keys/rsa2048.key.pem" \
    -subj "/O=Smith, Jones and Co/CN=test.example.com" -out comma_in_value.csr.pem
assert_exit_zero "$LAST_EXIT" "a literal unescaped comma in a slash-syntax value should be accepted"

openssl_out req -in comma_in_value.csr.pem -noout -subject > subj1.txt
assert_contains subj1.txt "Smith, Jones and Co" "the literal comma should survive intact in the value"

# OpenSSL's slash syntax requires an embedded literal '/' to be escaped as
# '\/' (otherwise it's misread as a field boundary - confirmed against real
# openssl, which produces a warning and a truncated subject in that case).
run_clu req -new -sha256 -key "$FIXTURES/keys/rsa2048.key.pem" \
    -subj "/O=A\\/B Corp/CN=test.example.com" -out escaped_slash.csr.pem
assert_exit_zero "$LAST_EXIT" "an escaped slash (\\/) in a slash-syntax value should be accepted"

openssl_out req -in escaped_slash.csr.pem -noout -subject > subj2.txt
assert_contains subj2.txt "A/B Corp" "the escaped slash should decode to a literal '/' in the value"
