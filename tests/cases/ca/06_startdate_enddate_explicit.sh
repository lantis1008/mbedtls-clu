. "$TESTS_DIR/lib/framework.sh"
case_workdir_init
ca_db_init

run_clu ca -config ca.cnf -keyfile ca.key.pem -cert ca.cert.pem \
    -in "$FIXTURES/csr/rsa2048.csr.pem" -out signed.pem \
    -startdate 260101000000Z -enddate 270101000000Z -md sha256
assert_exit_zero "$LAST_EXIT" "ca with explicit -startdate/-enddate should exit 0"

dates=$(openssl_out x509 -in signed.pem -noout -dates)
printf '%s\n' "$dates" > dates.txt
assert_contains dates.txt "Jan  1 00:00:00 2026 GMT" "notBefore should match -startdate exactly"
assert_contains dates.txt "Jan  1 00:00:00 2027 GMT" "notAfter should match -enddate exactly"
