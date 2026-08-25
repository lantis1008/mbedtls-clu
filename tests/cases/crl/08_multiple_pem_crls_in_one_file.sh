. "$TESTS_DIR/lib/framework.sh"
case_workdir_init
ca_db_init

# mbedtls_x509_crl_parse_file chains multiple concatenated PEM CRLs via
# crl->next. Full multi-CRL enumeration isn't part of this utility's v1
# flag surface - this only guards against choking on legitimately-formatted
# multi-CRL input (crl.c only ever displays the first entry in the chain).
run_clu ca -config ca.cnf -keyfile ca.key.pem -cert ca.cert.pem -gencrl -out crl1.pem -crl_days 30
assert_exit_zero "$LAST_EXIT" "first ca -gencrl should exit 0"

run_clu ca -config ca.cnf -keyfile ca.key.pem -cert ca.cert.pem -gencrl -out crl2.pem -crl_days 60
assert_exit_zero "$LAST_EXIT" "second ca -gencrl should exit 0"

cat crl1.pem crl2.pem > combined.pem

run_clu crl -in combined.pem -text -noout
assert_exit_zero "$LAST_EXIT" "crl -text on a file with two concatenated PEM CRLs should not choke"
