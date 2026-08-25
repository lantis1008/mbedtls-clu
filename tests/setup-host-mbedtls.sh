#!/bin/sh
# setup-host-mbedtls.sh - one-time, MANUAL setup step that builds a copy of
# MbedTLS from source for use ONLY by the host test suite in tests/.
#
# Why this exists: mbedtls-clu is written against MbedTLS 3.6.x (see
# ../README.md / ../CLAUDE.md), but distro package managers (e.g. Ubuntu/Debian
# apt) typically only ship the API-incompatible MbedTLS 2.x line. To run
# mbedtls-clu on this host next to `openssl` for comparison testing, we need a
# real 3.6.x build. This script is never invoked automatically by run.sh or
# any Makefile target - you must run it yourself, once, deliberately, because
# it downloads a file from the network.
#
# It does NOT touch anything outside tests/host-mbedtls/, and has no effect
# whatsoever on the OpenWrt package build path (which supplies its own
# mbedtls via the OpenWrt buildroot and never looks inside tests/).
#
# Usage:
#   sh tests/setup-host-mbedtls.sh          # build if not already built
#   sh tests/setup-host-mbedtls.sh -f       # force a clean rebuild
#
# Requires: curl or wget, sha256sum (or shasum), tar, make, a C compiler.

set -eu

MBEDTLS_VERSION=3.6.3
# Official release asset + published checksum, cross-verified by hand against
# https://github.com/Mbed-TLS/mbedtls/releases/tag/mbedtls-3.6.3 at the time
# this script was written. This is the ONLY checksum trusted - nothing fetched
# at runtime is trusted, by design.
MBEDTLS_TARBALL_SHA256=64cd73842cdc05e101172f7b437c65e7312e476206e1dbfd644433d11bc56327
MBEDTLS_TARBALL_URL="https://github.com/Mbed-TLS/mbedtls/releases/download/mbedtls-${MBEDTLS_VERSION}/mbedtls-${MBEDTLS_VERSION}.tar.bz2"

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
HOST_MBEDTLS_DIR="$SCRIPT_DIR/host-mbedtls"
SRC_DIR="$HOST_MBEDTLS_DIR/src"
PREFIX="$HOST_MBEDTLS_DIR/prefix"
TARBALL="$HOST_MBEDTLS_DIR/mbedtls-${MBEDTLS_VERSION}.tar.bz2"

FORCE=0
for arg in "$@"; do
    case "$arg" in
        -f|--force) FORCE=1 ;;
        *) echo "usage: $0 [-f|--force]" >&2; exit 2 ;;
    esac
done

if [ "$FORCE" != "1" ] && [ -f "$PREFIX/lib/libmbedtls.a" ]; then
    echo "setup-host-mbedtls.sh: already built at $PREFIX (use -f to force a rebuild)"
    exit 0
fi

mkdir -p "$HOST_MBEDTLS_DIR"

fetch() {
    url="$1"; out="$2"
    if command -v curl >/dev/null 2>&1; then
        curl -fsSL -o "$out" "$url"
    elif command -v wget >/dev/null 2>&1; then
        wget -q -O "$out" "$url"
    else
        echo "setup-host-mbedtls.sh: need curl or wget to download mbedtls" >&2
        exit 1
    fi
}

sha256_of() {
    if command -v sha256sum >/dev/null 2>&1; then
        sha256sum "$1" | awk '{print $1}'
    elif command -v shasum >/dev/null 2>&1; then
        shasum -a 256 "$1" | awk '{print $1}'
    else
        echo "setup-host-mbedtls.sh: need sha256sum or shasum to verify the download" >&2
        exit 1
    fi
}

if [ ! -f "$TARBALL" ]; then
    echo "setup-host-mbedtls.sh: downloading mbedtls ${MBEDTLS_VERSION}..."
    fetch "$MBEDTLS_TARBALL_URL" "$TARBALL.partial"
    mv "$TARBALL.partial" "$TARBALL"
fi

echo "setup-host-mbedtls.sh: verifying checksum..."
actual_sha256=$(sha256_of "$TARBALL")
if [ "$actual_sha256" != "$MBEDTLS_TARBALL_SHA256" ]; then
    echo "setup-host-mbedtls.sh: CHECKSUM MISMATCH for $TARBALL" >&2
    echo "  expected: $MBEDTLS_TARBALL_SHA256" >&2
    echo "  actual:   $actual_sha256" >&2
    echo "  refusing to build from an unverified download. Removing it." >&2
    rm -f "$TARBALL"
    exit 1
fi
echo "setup-host-mbedtls.sh: checksum OK"

rm -rf "$SRC_DIR"
mkdir -p "$SRC_DIR"
echo "setup-host-mbedtls.sh: extracting..."
tar -xjf "$TARBALL" -C "$SRC_DIR" --strip-components=1

echo "setup-host-mbedtls.sh: building (make, this can take a few minutes)..."
rm -rf "$PREFIX"
NPROC=$(command -v nproc >/dev/null 2>&1 && nproc || echo 2)
# Use mbedtls's own top-level GNU Makefile (no cmake dependency). The
# release tarball ships pre-generated sources, so this needs only a C
# compiler - no perl/python codegen step. "make lib" builds only
# library/{libmbedtls,libmbedx509,libmbedcrypto} - deliberately NOT "make
# install", whose "no_test" prerequisite also builds every example program
# and the entire test/fuzz suite. We copy headers+libs ourselves instead,
# mirroring what "make install" would put in place for just those two.
make -C "$SRC_DIR" -j"$NPROC" lib >"$HOST_MBEDTLS_DIR/build.log" 2>&1

mkdir -p "$PREFIX/include" "$PREFIX/lib"
cp -rp "$SRC_DIR/include/mbedtls" "$PREFIX/include/"
cp -rp "$SRC_DIR/include/psa" "$PREFIX/include/"
cp -P "$SRC_DIR"/library/libmbedtls.* "$PREFIX/lib/"
cp -P "$SRC_DIR"/library/libmbedx509.* "$PREFIX/lib/"
cp -P "$SRC_DIR"/library/libmbedcrypto.* "$PREFIX/lib/"

echo "setup-host-mbedtls.sh: done. Installed to $PREFIX"
echo "Next: make build   (from the repo root, or: make -C tests build)"
