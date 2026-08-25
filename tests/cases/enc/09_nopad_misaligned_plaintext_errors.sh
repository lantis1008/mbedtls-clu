. "$TESTS_DIR/lib/framework.sh"
case_workdir_init

# 47 bytes - not a multiple of the AES block size (16) - -nopad should
# reject this cleanly rather than silently truncating/corrupting, mirroring
# real openssl's "wrong final block length" behavior (confirmed
# empirically before implementing this).
printf 'Hello, World! This is a test plaintext message.' > plain.txt
[ "$(wc -c < plain.txt)" -eq 47 ] || fail "test setup: expected a 47-byte (non-block-aligned) plaintext"

run_clu enc -aes-256-cbc -nopad -S 0102030405060708 -in plain.txt -out bad.enc -k testpass123
assert_exit_nonzero "$LAST_EXIT" "-nopad with non-block-aligned plaintext should be rejected cleanly"
[ "$LAST_EXIT" -lt 128 ] || fail "should not have been killed by a signal (crash)"
