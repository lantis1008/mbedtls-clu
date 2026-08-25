. "$TESTS_DIR/lib/framework.sh"
case_workdir_init

# KNOWN BUG (see KNOWN_DIFFERENCES.md): req.c declares `char key_usage = 0`
# but mbedtls_x509write_crt_set_key_usage() takes `unsigned int`. Setting the
# digitalSignature bit (0x80) makes the char negative; sign-extension on the
# implicit conversion trips mbedtls's "disallowed bits" check, so ANY
# -config with keyUsage containing digitalSignature currently fails outright
# (ca.c has the equivalent variable correctly typed as unsigned int and is
# NOT affected - only req.c's -x509/CSR extension path). This case encodes
# the CORRECT/intended behavior and is expected to fail until that's fixed;
# it will start passing automatically once it is.
run_clu req -x509 -new -key "$FIXTURES/keys/rsa2048.key.pem" -config "$FIXTURES/conf/req.cnf" -out conf.cert.pem
assert_exit_zero "$LAST_EXIT" "req -x509 -config should exit 0 (see KNOWN_DIFFERENCES.md: req.c keyUsage sign-extension bug)"

openssl_out x509 -in conf.cert.pem -noout -subject > subject.txt
case "$(cat subject.txt)" in
    *"CN = conf-test.example.com"*) ;;
    *) fail "subject should come from the config file's commonName_default: $(cat subject.txt)" ;;
esac

openssl_out x509 -in conf.cert.pem -noout -text > cert.text
assert_contains cert.text "X509v3 Subject Key Identifier" "subjectKeyIdentifier = hash should add the extension"
assert_contains cert.text "CA:TRUE" "basicConstraints = critical,CA:TRUE should be applied"
assert_contains cert.text "Digital Signature" "keyUsage should include digitalSignature"
assert_contains cert.text "Certificate Sign" "keyUsage should include keyCertSign"
