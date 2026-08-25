. "$TESTS_DIR/lib/framework.sh"
case_workdir_init
ca_db_init

# -crl_reason is listed in ca.c's usage string but never registered as a
# recognized flag in the argument-parsing loop, so it falls through to
# "goto usage" - confirmed by reading ca.c. See KNOWN_DIFFERENCES.md.
run_clu ca -config ca.cnf -keyfile ca.key.pem -cert ca.cert.pem \
    -revoke "$FIXTURES/certs/ca.cert.pem" -crl_reason keyCompromise
assert_exit_nonzero "$LAST_EXIT" "-crl_reason should be rejected as an unrecognized flag, not silently accepted"
