. "$TESTS_DIR/lib/framework.sh"
case_workdir_init

# Regression guard: two independent invocations must not produce the same
# random bytes. This is the kind of bug (RNG not actually wired to entropy,
# or accidentally deterministic) that would otherwise pass every other check.
run_clu rand -hex 32
assert_exit_zero "$LAST_EXIT" "first rand -hex 32 should exit 0"
cp "$LAST_STDOUT" run1.hex

run_clu rand -hex 32
assert_exit_zero "$LAST_EXIT" "second rand -hex 32 should exit 0"
cp "$LAST_STDOUT" run2.hex

assert_files_differ run1.hex run2.hex "two independent rand -hex 32 runs must not be identical"
