# mbedtls-clu test suite

Compares `mbedtls-clu`'s behavior against the real `openssl` CLI: identical
(or field-equal) output where the two tools should agree, genuine
functional interop in both directions (mbedtls-clu reads openssl's output
and vice versa), and *different* output where randomness is involved (a
regression guard against broken RNG wiring). See `KNOWN_DIFFERENCES.md` for
where the two tools are expected to disagree, and why.

This suite runs **only** on a developer host. It is never part of the
OpenWrt package build, which invokes `make -C src` directly with its own
toolchain and knows nothing about anything under `tests/`.

## Prerequisites

- POSIX `sh` (tested against `dash`; plain `bash` also works)
- `openssl` (developed against 3.0.13; any 3.x should work)
- A host C compiler (`cc`/`gcc`) and `make`
- `curl` or `wget`, and `sha256sum` or `shasum` — only needed for the
  one-time mbedtls setup step below

## One-time setup: host mbedtls

mbedtls-clu is written against MbedTLS 3.6.x. Most distro package managers
(e.g. Ubuntu/Debian `apt`) only ship the API-incompatible 2.x line, so
running mbedtls-clu on this host next to `openssl` needs a real 3.6.x build.
`tests/setup-host-mbedtls.sh` does this: it downloads a **pinned** MbedTLS
3.6.3 release tarball, verifies it against a checksum hardcoded in the
script (nothing fetched at runtime is trusted), and builds it via mbedtls's
own plain `make` (not CMake — kept dependency-free of anything not already
needed to build mbedtls-clu itself) into `tests/host-mbedtls/prefix/`.

```sh
sh tests/setup-host-mbedtls.sh        # first run: downloads + builds (a minute or two)
sh tests/setup-host-mbedtls.sh -f     # force a clean rebuild
```

This step is **never** invoked automatically by anything else in `tests/`
or by the Makefiles — it touches the network, so it's a deliberate, manual,
one-time action. Re-running it after a successful build is a fast no-op.

## Building

```sh
make build              # from the repo root
# or: make -C tests build
```

This builds `src/` *in place* against the host mbedtls prefix (statically
linking the vendored `ericstools`, since it isn't installed system-wide
either), copies the resulting binary to `tests/build/host/mbedtls-clu`, and
runs `make -C src clean` afterward to leave `src/` pristine again.

**Concurrency caveat:** because this builds `src/` in place, don't run a
host test build and an OpenWrt cross-build in the same checkout at the same
time — use a second `git worktree` (or clone) if you need both at once.

## Running the tests

```sh
make test                       # build (if needed) + run everything
sh tests/run.sh                 # run everything, assuming already built
sh tests/run.sh req             # only tests/cases/req/*
sh tests/run.sh req/05_config_extensions_applied
                                 # a single case (".sh" optional)
```

Options (`run.sh [-k] [-v] [pattern]`, or the matching env vars):

- `-k` / `KEEP_WORKDIR=1` — keep each case's per-run workdir under
  `tests/workdir/` instead of deleting it on exit (useful for debugging a
  failure by hand)
- `-v` / `TESTS_VERBOSE=1` — stream each case's own output live instead of
  only showing it on failure

Output is one `PASS`/`FAIL`/`SKIP` line per case (failures show their
captured output indented underneath, unless `-v` already streamed it),
followed by a summary line. `run.sh` exits nonzero if anything failed.

Every case currently passes. See `KNOWN_DIFFERENCES.md` → "Known bugs" for
how bug-demonstrating cases are expected to work when one exists: a case
written to assert *correct* behavior, red until the underlying bug is
fixed, green afterward with no change to the test itself.

## Adding a new case

Copy an existing case in the relevant `tests/cases/<utility>/` directory as
a starting point; number new cases after the existing ones in that
directory. Every case:

1. Starts with `. "$TESTS_DIR/lib/framework.sh"` then `case_workdir_init`.
2. Uses `run_clu <args...>` / `run_openssl <args...>` to invoke the tool
   under test, which sets `$LAST_EXIT`, `$LAST_STDOUT`, `$LAST_STDERR` (file
   paths — copy them out with `cp` before the next `run_*` call if you need
   to keep more than one invocation's output around).
3. Checks results with the assertion helpers in `tests/lib/framework.sh`:
   `assert_exit_zero`/`assert_exit_nonzero`, `assert_eq`/`assert_ne`,
   `assert_files_equal`/`assert_files_differ`, `assert_contains` (grep -E
   against a file), `assert_verify_ok` (wraps `openssl verify -CAfile`),
   `assert_csr_verify_ok` (wraps `openssl req -verify -noout`). All of them
   report failure via `fail "<message>"` and exit 1, so a case reads as a
   flat sequence of checks with no manual branching.
4. Use `openssl_out <args...>` for one-off value extraction into a shell
   variable, e.g. `subj=$(openssl_out x509 -in f.pem -noout -subject)`.
   **openssl is always the extractor/oracle on both sides** of a
   "should be identical" comparison — don't diff mbedtls-clu's own `-text`
   output against openssl's `-text` output line-for-line, their formatting
   genuinely differs (see `KNOWN_DIFFERENCES.md`). Instead, run openssl's
   own parser against both artifacts and compare *its* output.
5. `skip "<reason>"` exits with status 77, reported as `SKIP` rather than
   `PASS`/`FAIL`.
6. Must be POSIX `sh` (no bashisms — no `[[ ]]`, no arrays, no `<(...)`
   process substitution) and must not depend on network access or on
   `setup-host-mbedtls.sh` having just been (re-)run.

If a case is demonstrating a known bug rather than checking a working
feature, write it to assert the **correct** behavior (so it goes green
automatically once the bug is fixed) and add an entry to
`KNOWN_DIFFERENCES.md` under "Known bugs" explaining why it currently fails
— see that file's "Discovered and fixed" note for past examples of the
pattern (`git log` on `tests/cases/ca/05_revoke_updates_database.sh` and
`tests/cases/req/05_config_extensions_applied.sh` shows both while they were
still red).

## Interpreting a failure / reproducing by hand

Each case runs in its own directory under `tests/workdir/`, normally deleted
on exit. Re-run with `-k`/`KEEP_WORKDIR=1` to keep it:

```sh
KEEP_WORKDIR=1 sh tests/run.sh ca/01_sign_csr_cert_validates_with_openssl -v
```

Inside the kept workdir: `commands.log` has every command the case ran
(with full paths), and `clu.stdout`/`clu.stderr`/`openssl.stdout`/
`openssl.stderr` hold the most recent invocation's captured output — copy
those commands and re-run them by hand to dig further.

## Fixtures

`tests/fixtures/` holds small, checked-in RSA/EC keys, a CSR, a CA
key+self-signed cert, and a few config files — deliberately weak/small key
sizes where strength doesn't matter to what's being tested, so "should be
identical" cases have a stable input that doesn't depend on the host's RNG.
`tests/fixtures/generate-fixtures.sh` documents (and can regenerate) all of
them using real `openssl` — it is **not** invoked by `run.sh`; fixtures are
committed to git and regenerated by hand only if one needs rotating.
