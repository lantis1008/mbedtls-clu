# tests/Makefile - builds mbedtls-clu for THIS HOST (for comparison testing
# against openssl) and runs the test suite. Never invoked by the OpenWrt
# package build, which calls `make -C src` directly with its own toolchain.
#
# Targets:
#   build   - builds tests/build/host/mbedtls-clu against the host mbedtls
#             produced by setup-host-mbedtls.sh
#   test    - build, then run the suite (ARGS= is passed through to run.sh,
#             e.g. `make -C tests test ARGS=req`)
#   clean   - removes tests/build/ and tests/workdir/ (NOT host-mbedtls/,
#             which is the expensive one-time build - see realclean)
#   realclean - clean, plus removes tests/host-mbedtls/ too

THIS_DIR   := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
REPO_ROOT  := $(abspath $(THIS_DIR)/..)
SRC_DIR    := $(REPO_ROOT)/src
HOST_PREFIX:= $(THIS_DIR)host-mbedtls/prefix
BUILD_DIR  := $(THIS_DIR)build/host

HOST_CC ?= cc

.PHONY: build test clean realclean check-host-mbedtls

check-host-mbedtls:
	@if [ ! -f "$(HOST_PREFIX)/lib/libmbedtls.a" ]; then \
		echo "tests/Makefile: host mbedtls not found at $(HOST_PREFIX)" >&2; \
		echo "  Run this first:  sh tests/setup-host-mbedtls.sh" >&2; \
		exit 1; \
	fi

# Builds src/ in place against the host mbedtls prefix, then relocates the
# resulting binary out to tests/build/host/ and cleans src/ again so a
# concurrent/subsequent OpenWrt cross-build of the same checkout starts from
# a pristine src/. See tests/README.md for the "don't run both at once"
# caveat this implies.
build: check-host-mbedtls
	$(MAKE) -C "$(SRC_DIR)" clean
	$(MAKE) -C "$(SRC_DIR)" \
		CC="$(HOST_CC)" \
		CFLAGS="-I$(HOST_PREFIX)/include" \
		LDFLAGS="-L$(HOST_PREFIX)/lib -Wl,-rpath,$(HOST_PREFIX)/lib" \
		STATIC_LIBS=1
	mkdir -p "$(BUILD_DIR)"
	cp "$(SRC_DIR)/mbedtls-clu" "$(BUILD_DIR)/mbedtls-clu"
	$(MAKE) -C "$(SRC_DIR)" clean

test: build
	sh "$(THIS_DIR)run.sh" $(ARGS)

clean:
	rm -rf "$(THIS_DIR)build" "$(THIS_DIR)workdir"

realclean: clean
	rm -rf "$(THIS_DIR)host-mbedtls"
