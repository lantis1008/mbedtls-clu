. "$TESTS_DIR/lib/framework.sh"
case_workdir_init
ca_db_init

# Verifies the two tools' CA databases (index.txt/serial) are genuinely
# interchangeable, not just individually openssl-compatible in isolation:
# both tools sign into the SAME shared database (with the serial file
# correctly advancing across the tool boundary), then each tool revokes the
# OTHER tool's entry from that shared database.

serial_status() {
    # $1 = serial (as stored in index.txt, e.g. "1000")
    awk -F'\t' -v s="$1" '$4 == s { print $1 }' index.txt
}

# --- openssl signs cert A into the shared database ---
run_openssl ca -config ca.cnf -in "$FIXTURES/csr/rsa2048.csr.pem" -out certA.pem -batch
assert_exit_zero "$LAST_EXIT" "openssl ca should sign cert A into the shared database"

serialA=$(openssl_out x509 -in certA.pem -noout -serial)
serialA=${serialA#serial=}

# --- mbedtls-clu signs cert B (a fresh CSR) into the SAME shared database ---
run_openssl req -new -key "$FIXTURES/keys/ec-p256.key.pem" \
    -subj "/C=GB/O=mbedtls-clu Tests/CN=crossb.example.com" -out csrB.pem
assert_exit_zero "$LAST_EXIT" "generating cert B's CSR with openssl should exit 0"

run_clu ca -config ca.cnf -keyfile ca.key.pem -cert ca.cert.pem -in csrB.pem -out certB.pem -days 365 -md sha256
assert_exit_zero "$LAST_EXIT" "mbedtls-clu ca should sign cert B into the shared database"

serialB=$(openssl_out x509 -in certB.pem -noout -serial)
serialB=${serialB#serial=}

assert_ne "$serialA" "$serialB" "the shared serial file should have advanced between the two signings"
assert_eq "$(serial_status "$serialA")" "V" "cert A should be valid after openssl signs it"
assert_eq "$(serial_status "$serialB")" "V" "cert B should be valid after mbedtls-clu signs it"

# --- mbedtls-clu revokes the openssl-signed cert ---
run_clu ca -config ca.cnf -keyfile ca.key.pem -cert ca.cert.pem -revoke certA.pem
assert_exit_zero "$LAST_EXIT" "mbedtls-clu should revoke the openssl-signed cert A"
assert_eq "$(serial_status "$serialA")" "R" "cert A's status should become R"

# --- openssl revokes the mbedtls-clu-signed cert ---
run_openssl ca -config ca.cnf -revoke certB.pem
assert_exit_zero "$LAST_EXIT" "openssl should revoke the mbedtls-clu-signed cert B"
assert_eq "$(serial_status "$serialB")" "R" "cert B's status should become R"
