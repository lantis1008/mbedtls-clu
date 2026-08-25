#!/bin/sh
# run.sh - runs the mbedtls-clu vs openssl comparison test suite.
# POSIX sh. See tests/README.md for full documentation.
#
# Usage:
#   sh tests/run.sh                          # run every case
#   sh tests/run.sh req                      # run only tests/cases/req/*
#   sh tests/run.sh req/05_config_extensions_applied
#                                             # run a single case (path relative
#                                             # to tests/cases/, .sh optional)
#
# Options (may appear anywhere on the command line):
#   -k            keep each case's workdir under tests/workdir/ (same as KEEP_WORKDIR=1)
#   -v            stream each case's own stdout/stderr live instead of only on failure
#
# Env vars:
#   MBEDTLS_CLU_BIN   path to the binary under test (default: tests/build/host/mbedtls-clu)
#   OPENSSL_BIN       openssl binary to use as the oracle (default: "openssl" on PATH)
#   KEEP_WORKDIR=1    same effect as -k
#   TESTS_VERBOSE=1   same effect as -v

set -u

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
TESTS_DIR="$SCRIPT_DIR"
export TESTS_DIR

: "${MBEDTLS_CLU_BIN:=$TESTS_DIR/build/host/mbedtls-clu}"
: "${OPENSSL_BIN:=openssl}"
: "${KEEP_WORKDIR:=0}"
: "${TESTS_VERBOSE:=0}"
export MBEDTLS_CLU_BIN OPENSSL_BIN

PATTERN=""
for arg in "$@"; do
    case "$arg" in
        -k) KEEP_WORKDIR=1 ;;
        -v) TESTS_VERBOSE=1 ;;
        -*)
            echo "run.sh: unknown option: $arg" >&2
            exit 2
            ;;
        *)
            if [ -n "$PATTERN" ]; then
                echo "run.sh: only one case/utility pattern may be given (already have '$PATTERN')" >&2
                exit 2
            fi
            PATTERN="$arg"
            ;;
    esac
done
export KEEP_WORKDIR TESTS_VERBOSE

if ! command -v "$OPENSSL_BIN" >/dev/null 2>&1; then
    echo "run.sh: openssl not found on PATH (OPENSSL_BIN=$OPENSSL_BIN)" >&2
    exit 1
fi

if [ ! -x "$MBEDTLS_CLU_BIN" ]; then
    echo "run.sh: mbedtls-clu binary not found/executable at: $MBEDTLS_CLU_BIN" >&2
    echo "  Build it first with:  make build   (or: make -C tests build)" >&2
    echo "  which itself requires the one-time:  sh tests/setup-host-mbedtls.sh" >&2
    exit 1
fi

mkdir -p "$TESTS_DIR/workdir"

# Strip a trailing .sh and/or leading ./ from the user-supplied pattern so
# both "req" and "req/05_config_extensions_applied.sh" work.
PATTERN=${PATTERN%.sh}
PATTERN=${PATTERN#./}

CASE_LIST_FILE=$(mktemp)
trap 'rm -f "$CASE_LIST_FILE"' EXIT
find "$TESTS_DIR/cases" -type f -name '*.sh' | sort > "$CASE_LIST_FILE"

npass=0
nfail=0
nskip=0
failed_ids=""

echo "mbedtls-clu:  $MBEDTLS_CLU_BIN"
echo "openssl:      $($OPENSSL_BIN version 2>/dev/null || echo "$OPENSSL_BIN")"
echo ""

while IFS= read -r case_file; do
    case_id=${case_file#"$TESTS_DIR/cases/"}
    case_id=${case_id%.sh}

    if [ -n "$PATTERN" ]; then
        case "$case_id" in
            "$PATTERN"|"$PATTERN"/*) ;;
            *) continue ;;
        esac
    fi

    log_file=$(mktemp)
    CASE_ID="$case_id" sh "$case_file" >"$log_file" 2>&1
    case_exit=$?
    [ "$TESTS_VERBOSE" = "1" ] && cat "$log_file"

    if [ "$case_exit" = "0" ]; then
        echo "PASS $case_id"
        npass=$((npass + 1))
    elif [ "$case_exit" = "77" ]; then
        echo "SKIP $case_id"
        nskip=$((nskip + 1))
    else
        echo "FAIL $case_id"
        if [ "$TESTS_VERBOSE" != "1" ]; then
            sed 's/^/  | /' "$log_file"
        fi
        nfail=$((nfail + 1))
        failed_ids="$failed_ids $case_id"
    fi
    rm -f "$log_file"
done < "$CASE_LIST_FILE"

echo ""
echo "$npass passed, $nfail failed, $nskip skipped"

if [ "$nfail" != "0" ]; then
    echo "Failed cases:$failed_ids"
    exit 1
fi

if [ "$npass" = "0" ] && [ "$nskip" = "0" ]; then
    echo "run.sh: no cases matched pattern '$PATTERN'" >&2
    exit 2
fi

exit 0
