. "$TESTS_DIR/lib/framework.sh"
case_workdir_init

# Build two config files identical to the req fixture except for a distinct
# commonName, so we can tell which one won. keyUsage is trimmed to avoid the
# known digitalSignature bug (see 05_config_extensions_applied.sh /
# KNOWN_DIFFERENCES.md) - not what this case is testing.
sed 's/conf-test.example.com/mbedtls-wins.example.com/;
     s/keyUsage = critical,digitalSignature,keyCertSign/keyUsage = critical,keyCertSign/' \
    "$FIXTURES/conf/req.cnf" > mbedtls.cnf
sed 's/conf-test.example.com/openssl-wins.example.com/;
     s/keyUsage = critical,digitalSignature,keyCertSign/keyUsage = critical,keyCertSign/' \
    "$FIXTURES/conf/req.cnf" > openssl.cnf

MBEDTLS_CONF="$PWD/mbedtls.cnf" OPENSSL_CONF="$PWD/openssl.cnf" \
    run_clu req -x509 -new -key "$FIXTURES/keys/rsa2048.key.pem" -out both.cert.pem
assert_exit_zero "$LAST_EXIT" "req with both env vars set should exit 0"
subject=$(openssl_out x509 -in both.cert.pem -noout -subject)
case "$subject" in
    *mbedtls-wins.example.com*) ;;
    *) fail "MBEDTLS_CONF should take precedence over OPENSSL_CONF when both are set: $subject" ;;
esac

env -u MBEDTLS_CONF OPENSSL_CONF="$PWD/openssl.cnf" \
    "$MBEDTLS_CLU_BIN" req -x509 -new -key "$FIXTURES/keys/rsa2048.key.pem" -out ossl.cert.pem \
    >"$WORKDIR/clu.stdout" 2>"$WORKDIR/clu.stderr"
LAST_EXIT=$?
LAST_STDOUT="$WORKDIR/clu.stdout"
assert_exit_zero "$LAST_EXIT" "req with only OPENSSL_CONF set should exit 0"
subject=$(openssl_out x509 -in ossl.cert.pem -noout -subject)
case "$subject" in
    *openssl-wins.example.com*) ;;
    *) fail "OPENSSL_CONF should be honored when MBEDTLS_CONF is unset: $subject" ;;
esac
