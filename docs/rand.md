# `rand` — Random Byte Generation

Equivalent to a minimal subset of `openssl rand`. Generates random bytes
and prints them as hex.

## Synopsis

```
mbedtls-clu rand [options] [numbytes]
```

## Options

| Flag | Argument | Description |
|---|---|---|
| `-help` | | Display usage summary |
| `-hex` | | Print random bytes as hex (default, and currently the only mode) |
| `numbytes` | (positional) | How many bytes of random data to generate |

## Output

Hex-encoded random bytes, written via mbedtls's own
`mbedtls_mpi_write_file()` — see gaps below for the resulting cosmetic
differences from OpenSSL's own hex output.

## Examples

```sh
mbedtls-clu rand 32
mbedtls-clu rand -hex 16
```

## Known differences/gaps vs OpenSSL

Full details in
[`tests/KNOWN_DIFFERENCES.md`](../tests/KNOWN_DIFFERENCES.md). Summary:

- **Output is uppercase hex with a trailing CRLF**, vs OpenSSL's lowercase
  hex and a trailing LF. The CRLF is inherited unmodified from mbedtls's
  own `mbedtls_mpi_write_file()` library function, not something
  mbedtls-clu introduces itself.
- **No raw-binary output mode.** Every invocation goes through the hex
  writer regardless of `-hex`; there is no equivalent of OpenSSL's
  `rand <n>` writing `<n>` raw bytes to stdout.
