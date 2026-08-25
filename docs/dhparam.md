# `dhparam` — Diffie-Hellman Parameter Generation

Equivalent to `openssl dhparam`. Generates or validates DH parameters.

## Synopsis

```
mbedtls-clu dhparam [options] [numbits]
```

## Options

| Flag | Argument | Description |
|---|---|---|
| `-help` | | Display usage summary |
| `-in` | `infile` | Input DH params file (skips generation; use with `-check`/`-out`/`-text`) |
| `-check` | | Check the parameters are valid/safe |
| `-out` | `outfile` | Output file |
| `-outform` | `PEM\|DER` | Output format |
| `-text` | | Print a text form of the DH parameters |
| `-noout` | | Don't output the DH parameters |
| `-2` | | Generate using generator 2 (default) |
| `-3` | | Generate using generator 3 |
| `-5` | | Generate using generator 5 |
| `numbits` | (positional) | Bits to generate, if generating; default 2048 |

## Behavior notes

- Passing `-in` skips generation; the tool reads the given file instead.
  This can be combined with `-check` and/or `-out`/`-outform` to
  validate/convert an existing params file. `-check` combined with `-out`
  still writes the (validated) params through — it doesn't short-circuit
  before the write step.
- Without `-in`, `dhparam` generates new parameters at `numbits` (default
  2048) using the selected generator.
- **`-out` has no stdout fallback.** If `-out` is omitted and `-noout` is
  not given, nothing is written anywhere — unlike real OpenSSL, which
  defaults `-out` to stdout.

## Examples

Generate 2048-bit parameters with generator 2 (default):

```sh
mbedtls-clu dhparam -out dh.pem 2048
```

Generate with generator 5:

```sh
mbedtls-clu dhparam -5 -out dh.pem 2048
```

Validate an existing params file:

```sh
mbedtls-clu dhparam -in dh.pem -check
```

Validate and re-write in another format:

```sh
mbedtls-clu dhparam -in dh.pem -check -outform DER -out dh.der
```

## Known differences/gaps vs OpenSSL

Full details in
[`tests/KNOWN_DIFFERENCES.md`](../tests/KNOWN_DIFFERENCES.md); one
additional gap found and verified while writing this page, not yet in that
file:

- **`-noout` and `-text`, if given as the very last command-line token,
  are silently misparsed as the `numbits` positional argument instead of
  taking effect.** The argument parser only recognizes `-noout`/`-text`
  when *something else follows them* on the command line (an off-by-one in
  the bounds check, `i + 1 < argc`, that both flags share). Concretely,
  `dhparam -in dh.pem -check -noout` is silently reinterpreted as
  "generate new 0-bit parameters" — `-in` is ignored (`Warning, input file
  dh.pem ignored`) and the run fails outright
  (`mbedtls_mpi_gen_prime returned -4`). **Workaround**: always put
  `-noout`/`-text` before another flag, e.g.
  `dhparam -in dh.pem -noout -check` (works correctly), never last.
- **`-check` without `-out` writes nothing** — real OpenSSL's
  `dhparam -in x.pem -check` (no `-out`, no `-noout`) both reports the
  check result *and* prints the PEM to stdout by defaulting `-out` to
  stdout; mbedtls-clu only reports the check result.
