# Top-level convenience Makefile for developers.
#
# NOT used by the OpenWrt package build - that build invokes `make -C src`
# directly with its own toolchain-supplied CC/CFLAGS/LDFLAGS and knows
# nothing about this file. This Makefile exists purely to give a developer
# on this host a single entry point for building mbedtls-clu for the host
# and running its test suite (see tests/README.md).

.PHONY: build test clean

build:
	$(MAKE) -C tests build

test:
	$(MAKE) -C tests test

clean:
	$(MAKE) -C tests clean
