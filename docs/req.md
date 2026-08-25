# `req` — Certificate Requests and Self-Signed Certificates

Equivalent to `openssl req`. Generates a PKCS#10 CSR, or (with `-x509`) a
self-signed certificate, from an existing or newly-generated private key.
Also has a display-only mode for an existing CSR (`-in`).

## Synopsis

```
mbedtls-clu req [options]
```

## Options

| Flag | Argument | Description |
|---|---|---|
| `-help` | | Display usage summary |
| `-config` | `infile` | Config file path (see [config-files.md](config-files.md)); command-line flags override config values |
| `-in` | `infile` | **Display only** — parse and print an existing CSR's summary; see gap below |
| `-new` | | New request (accepted; effectively always implied — see notes) |
| `-x509` | | Output a self-signed X.509 certificate (v3 by default) instead of a CSR |
| `-x509v1` | | Output a v1-format certificate; implies `-x509` |
| `-days` | `+int` | Certificate validity period (`-x509` only) |
| `-set_serial` | `val` | Serial number for the certificate (`-x509` only); **base-10 only**, no `0x` hex form |
| `-subj` | `val` | Set/override the subject — mbedtls native `type=value,type=value` form, or OpenSSL `/type=value/type=value` slash form (auto-detected) |
| `-key` | `val` | Existing private key file to sign with |
| `-newkey` | `val` | Generate a new key: **`rsa:<nbits>` only** — see gap below |
| `-keyout` | `outfile` | File to write the newly-generated private key to (required with `-newkey`) |
| `-passin` | `val` | Private key password source (parsed, plumbed through to key parsing) |
| `-out` | `outfile` | Output file |
| `-outform` | `PEM\|DER` | Output format |
| `-text` | | Print the request/certificate in text form |
| `-noout` | | Suppress writing `-out` |
| `-<digest>` | | Digest to sign with, e.g. `-sha256`, `-sha1`, `-sha384` (OpenSSL-style bare flag, not `-md <name>`) |
| `-batch`, `-utf8`, `-nodes`, `-noenc` | | Accepted, silently ignored — see notes |

## Behavior notes

- **`-batch`/`-utf8`/`-nodes`/`-noenc` are no-ops.** mbedtls-clu never runs
  interactively (batch is always on), doesn't support UTF-8-specific string
  types, and never encrypts private keys — these flags exist purely so
  OpenSSL-style invocations don't fail to parse.
- **`-key` and `-newkey` are mutually exclusive**; supplying both is a
  usage error.
- **Digest selection**: pass `-<digest-name>` directly (e.g. `-sha256`),
  not `-md sha256` (unlike `ca`, which does use `-md`). If omitted, falls
  back to `-config`'s `[req] default_md`; if neither is given, `req` exits
  to usage rather than defaulting to SHA-256, despite what the usage text
  implies (see gap below).
- **Subject**: `-subj` accepts both the OpenSSL slash syntax
  (`/C=GB/O=Org/CN=host`) and mbedtls's native comma syntax
  (`C=GB,O=Org,CN=host`); which one is used is auto-detected from the
  leading character.

## Config-driven fields

When `-config` is given, `req` reads `[req]`'s `distinguished_name` and
`x509_extensions` tags (the latter only applied when `-x509` is set) — full
tag reference in [config-files.md](config-files.md).

## Output

- **CSR mode** (default): writes a PKCS#10 CSR to `-out` in the requested
  `-outform`.
- **`-x509` mode**: writes a self-signed certificate to `-out` instead.
- **`-in` mode**: prints the parsed CSR's summary (subject, key size,
  signature algorithm) to stdout; nothing is written to `-out` (see gap).

## Examples

Generate a CSR with an existing key:

```sh
mbedtls-clu req -new -sha256 -key key.pem \
    -subj "C=GB,O=Example Org,CN=host.example.com" -out req.csr.pem
```

Same, using OpenSSL slash-syntax subject:

```sh
mbedtls-clu req -new -sha256 -key key.pem \
    -subj "/C=GB/O=Example Org/CN=host.example.com" -out req.csr.pem
```

Self-signed certificate from an existing key:

```sh
mbedtls-clu req -x509 -new -sha256 -key key.pem -days 365 \
    -subj "/C=GB/O=Example Org/CN=host.example.com" -out cert.pem
```

Self-signed certificate, generating a new RSA key in the same step
(**requires `-config` with `default_keyfile` set — see gap below**):

```sh
mbedtls-clu req -config req.cnf -x509 -new -newkey rsa:2048 \
    -keyout key.pem -out cert.pem -sha256 \
    -subj "/C=GB/O=Example Org/CN=host.example.com"
```

Apply config-driven DN defaults and extensions:

```sh
mbedtls-clu req -config req.cnf -x509 -new -key key.pem -out cert.pem
```

(see [`tests/fixtures/conf/req.cnf`](../tests/fixtures/conf/req.cnf) for a
complete example config).

## Known differences/gaps vs OpenSSL

Full details and rationale in
[`tests/KNOWN_DIFFERENCES.md`](../tests/KNOWN_DIFFERENCES.md). Summary:

- **`-newkey` without `-config` fails.** `req_main` requires either `-key`
  or `-config`'s `[req] default_keyfile` to be set *before* it ever reaches
  the `-newkey` key-generation step — so `-newkey rsa:2048 -keyout key.pem`
  with no `-key` and no `-config` exits to usage immediately, even though
  `-keyout` was correctly supplied. **Workaround**: pass any `-config` file
  with `default_keyfile` set to any placeholder value (its actual value is
  never used when `-newkey` is also given) — see the example above.
- **`-newkey` only supports `rsa:<nbits>`.** The usage text advertises
  `[<alg>:]<nbits>` or `<alg>[:<file>]` or `param:<file>`, but the
  implementation only recognizes the literal form `rsa:<nbits>`
  (1024–`MBEDTLS_MPI_MAX_BITS`); anything else, including `ec:...`, is a
  usage error. Use `genpkey` + `-key` as a two-step alternative for EC.
- **No digest default in practice.** Despite the usage text claiming
  SHA-256 is the default, there is no actual fallback if neither a
  `-<digest>` flag nor `[req] default_md` is set — `req` exits to usage.
- **`-in <csr>` is display-only.** It prints the CSR's summary but is never
  fed into `-x509` conversion or any output-writing path; `-out`/`-outform`
  are silently ignored when `-in` is used (still exits 0). There is no
  "convert an existing CSR to a self-signed cert" workflow in `req` — use
  `ca` to turn a CSR into a certificate instead.
- **`-x509`'s config-driven `extendedKeyUsage` is parsed but never
  applied** to the certificate. `ca` applies the equivalent tag correctly
  when signing — this gap is isolated to `req`'s self-signed-cert path.
- **`-subj` slash-syntax**: a malformed segment (e.g. empty `//`, or a
  segment with no `type=`) is rejected outright rather than silently
  dropped-with-a-warning the way real OpenSSL handles it; and multi-backslash
  escaping isn't guaranteed byte-identical to OpenSSL's own (single-backslash
  escaping is well-defined and consistent either way).
- **`-set_serial` is base-10 only** — no `0x`-prefixed hex form.
