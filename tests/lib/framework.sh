# framework.sh - shared harness for mbedtls-clu vs openssl comparison tests.
# POSIX sh. Sourced by every case script as:
#   . "$TESTS_DIR/lib/framework.sh"
# Expects TESTS_DIR (absolute path to tests/) and CASE_ID (e.g. "req/05_config_extensions_applied")
# to already be exported by run.sh before the case script is invoked.

: "${TESTS_DIR:?framework.sh: TESTS_DIR must be set by run.sh before sourcing}"
: "${CASE_ID:?framework.sh: CASE_ID must be set by run.sh before sourcing}"

REPO_ROOT=$(cd "$TESTS_DIR/.." && pwd)
FIXTURES="$TESTS_DIR/fixtures"

: "${MBEDTLS_CLU_BIN:=$TESTS_DIR/build/host/mbedtls-clu}"
: "${OPENSSL_BIN:=openssl}"
: "${KEEP_WORKDIR:=0}"
: "${TESTS_VERBOSE:=0}"

SKIP_EXIT=77

if [ ! -x "$MBEDTLS_CLU_BIN" ]; then
    echo "FAIL [$CASE_ID] mbedtls-clu binary not found/executable at $MBEDTLS_CLU_BIN" >&2
    echo "  Run 'make build' (or 'make -C tests build') first. See tests/README.md." >&2
    exit 1
fi
if ! command -v "$OPENSSL_BIN" >/dev/null 2>&1; then
    echo "FAIL [$CASE_ID] openssl binary not found on PATH (OPENSSL_BIN=$OPENSSL_BIN)" >&2
    exit 1
fi

note() {
    [ "$TESTS_VERBOSE" = "1" ] && echo "  note [$CASE_ID]: $*" >&2
    return 0
}

fail() {
    echo "FAIL [$CASE_ID] $*" >&2
    if [ -n "${LAST_STDERR:-}" ] && [ -s "$LAST_STDERR" ]; then
        echo "  --- stderr of last command ($LAST_CMD_DESC) ---" >&2
        sed 's/^/  | /' "$LAST_STDERR" >&2
    fi
    exit 1
}

skip() {
    echo "SKIP [$CASE_ID] $*" >&2
    exit "$SKIP_EXIT"
}

# --- workdir -----------------------------------------------------------

_workdir_cleanup() {
    if [ "$KEEP_WORKDIR" != "1" ]; then
        cd "$REPO_ROOT" 2>/dev/null || cd /
        rm -rf "$WORKDIR"
    else
        echo "  note [$CASE_ID]: workdir kept at $WORKDIR" >&2
    fi
}

case_workdir_init() {
    safe_id=$(printf '%s' "$CASE_ID" | tr '/' '_')
    WORKDIR="$TESTS_DIR/workdir/${safe_id}.$$"
    mkdir -p "$WORKDIR"
    : > "$WORKDIR/commands.log"
    trap _workdir_cleanup EXIT
    cd "$WORKDIR" || fail "could not cd into workdir $WORKDIR"
}

# --- command execution ---------------------------------------------------
# Each call overwrites the "last" stdout/stderr files. If a case needs to
# keep output from more than one invocation, cp "$LAST_STDOUT"/"$LAST_STDERR"
# to a named file right after the call, before the next run_* call.

run_clu() {
    LAST_CMD_DESC="mbedtls-clu $*"
    LAST_STDOUT="$WORKDIR/clu.stdout"
    LAST_STDERR="$WORKDIR/clu.stderr"
    echo "+ $MBEDTLS_CLU_BIN $*" >> "$WORKDIR/commands.log"
    "$MBEDTLS_CLU_BIN" "$@" >"$LAST_STDOUT" 2>"$LAST_STDERR"
    LAST_EXIT=$?
    return "$LAST_EXIT"
}

run_openssl() {
    LAST_CMD_DESC="openssl $*"
    LAST_STDOUT="$WORKDIR/openssl.stdout"
    LAST_STDERR="$WORKDIR/openssl.stderr"
    echo "+ $OPENSSL_BIN $*" >> "$WORKDIR/commands.log"
    "$OPENSSL_BIN" "$@" >"$LAST_STDOUT" 2>"$LAST_STDERR"
    LAST_EXIT=$?
    return "$LAST_EXIT"
}

# openssl_out ARGS... - run openssl and echo its trimmed stdout. Used for
# one-off field extraction, e.g.: subj=$(openssl_out x509 -in f.pem -noout -subject)
openssl_out() {
    "$OPENSSL_BIN" "$@" 2>"$WORKDIR/openssl.stderr"
}

# --- assertions ------------------------------------------------------------
# All assertions call fail() (and thus exit 1) on failure, so case scripts
# read as a flat sequence of checks with no manual branching needed.

assert_exit_zero() {
    code="$1"; msg="$2"
    [ "$code" = "0" ] || fail "$msg (expected exit 0, got $code)"
}

assert_exit_nonzero() {
    code="$1"; msg="$2"
    [ "$code" != "0" ] || fail "$msg (expected nonzero exit, got 0)"
}

assert_eq() {
    a="$1"; b="$2"; msg="$3"
    [ "$a" = "$b" ] || fail "$msg (expected [$a] == [$b])"
}

assert_ne() {
    a="$1"; b="$2"; msg="$3"
    [ "$a" != "$b" ] || fail "$msg (expected values to differ, both were [$a])"
}

assert_files_equal() {
    f1="$1"; f2="$2"; msg="$3"
    cmp -s "$f1" "$f2" || fail "$msg (files differ: $f1 vs $f2)"
}

assert_files_differ() {
    f1="$1"; f2="$2"; msg="$3"
    cmp -s "$f1" "$f2" && fail "$msg (files are unexpectedly identical: $f1 vs $f2)"
    return 0
}

assert_contains() {
    file="$1"; pattern="$2"; msg="$3"
    grep -Eq "$pattern" "$file" || fail "$msg (pattern [$pattern] not found in $file)"
}

assert_empty_file() {
    file="$1"; msg="$2"
    [ ! -s "$file" ] || fail "$msg ($file was not empty)"
}

assert_verify_ok() {
    cert="$1"; cafile="$2"; msg="$3"
    run_openssl verify -CAfile "$cafile" "$cert"
    assert_exit_zero "$LAST_EXIT" "$msg"
    assert_contains "$LAST_STDOUT" ': OK$' "$msg (openssl verify did not report OK)"
}

assert_csr_verify_ok() {
    csr="$1"; msg="$2"
    run_openssl req -in "$csr" -verify -noout
    assert_exit_zero "$LAST_EXIT" "$msg"
}

# ca_db_init - sets up a fresh OpenSSL-style CA directory (index.txt,
# index.txt.attr, serial, certs/, newcerts/) in the current directory and
# copies in fixtures/conf/ca.cnf + the fixture CA key/cert, ready for
# `mbedtls-clu ca -config ca.cnf ...`. Used by every tests/cases/ca/*.sh case.
ca_db_init() {
    mkdir -p certs newcerts
    : > index.txt
    echo "unique_subject = no" > index.txt.attr
    echo 1000 > serial
    cp "$FIXTURES/conf/ca.cnf" .
    cp "$FIXTURES/certs/ca.cert.pem" .
    cp "$FIXTURES/certs/ca.key.pem" .
}
