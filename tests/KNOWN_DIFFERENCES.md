# Known differences from OpenSSL, and known bugs

This file distinguishes two categories, so the test suite's intent is clear:

- **Intentional gaps / design differences** — mbedtls-clu doesn't try to be a
  full OpenSSL replacement (see `README.md`); these are not bugs.
- **Known bugs** — real defects found while building this test suite. Each
  has a corresponding test case that encodes the *correct* behavior and is
  currently expected to **fail**. Fixing the bug should make that case start
  passing with no change to the test itself — that's how you'll know the fix
  is complete and correctly matched to what the test checks.

## Known bugs (tests currently red)

None currently — every case in this suite passes. This section stays as a
place to record the next one found.

## Intentional gaps / design differences (not bugs — tests are written around these)

- **No private-key passphrase/encryption support anywhere.** `genpkey -pass`
  and cipher selection are explicitly marked `UNSUPPORTED` in the usage
  text; `req`/`ca` have no equivalent either.
- **`ca -crl_reason` is unimplemented.** It's listed in `ca.c`'s usage
  string but never registered in the argument-parsing loop, so passing it
  hits the trailing `goto usage` and the process exits nonzero. Revocation
  reason codes are never written to CRLs.
- **`genpkey` only supports `rsa` and `ec`** (no dsa/ed25519/dh key types).
- **`req -subj` rejects a malformed OpenSSL-style slash segment** (e.g. an
  empty `//` field, or any segment with no `type=` before the next `/` or
  end of string) **instead of silently dropping it and continuing**, which
  is what real OpenSSL does (with a warning, producing a truncated
  subject). Deliberate: failing clearly beats silently issuing a
  certificate with a wrong/incomplete subject. See
  `convert_openssl_subj_to_mbedtls_subj()` in `req.c`.
- **`req -subj`'s slash-syntax escaping is a well-defined single-escape
  convention** (`\/` for a literal `/`, `\\` for a literal `\`), **not
  necessarily byte-for-byte identical to real OpenSSL's own escaping for
  inputs with two or more consecutive backslashes.** Empirically, real
  OpenSSL's handling there is oddly inconsistent (verified: one backslash
  before a character strips it, two leaves both untouched, three collapses
  to two — not a simple pattern), and wasn't considered worth reverse
  engineering byte-for-byte; a single backslash always escapes exactly the
  next character, consistently. This only matters for values containing
  multiple consecutive literal backslashes, which is vanishingly rare in
  practice (an org name is not going to contain `\\`).
- **`req -set_serial` is base-10 only.** OpenSSL also accepts a
  `0x`-prefixed hex value; mbedtls-clu's `mbedtls_mpi_read_string(&serial,
  10, serialval)` call is hardcoded to base 10.
- **`req`'s usage text claims digest defaults to SHA256, but there is no
  actual fallback.** If neither a digest flag (e.g. `-sha256`) nor a
  config `default_md` is supplied, `req_main` exits to usage instead of
  defaulting. Every `req` test case in this suite passes an explicit digest
  flag for this reason.
- **`genpkey -pkeyopt ec_paramgen_curve:` only accepts mbedtls's own curve
  names** (e.g. `secp256r1`), not OpenSSL's common aliases for the same
  curves (e.g. `prime256v1`). Run `mbedtls-clu genpkey -help` for the full
  list of accepted names.
- **`genpkey -pkeyopt ec_param_enc:` is parsed but has no effect.**
  `ec_curve_paramenc` is read from the CLI and even printed at
  `MBEDTLSCLU_DEBUG` level, but is never actually passed into the
  key-writing path — `named_curve` and `explicit` produce identical output
  (always named-curve form).
- **`crl` only supports a minimal, high-value subset of real `openssl crl`'s
  surface** — `-in`/`-out`/`-outform`/`-noout`, `-text`, `-issuer`,
  `-lastupdate`/`-nextupdate`, `-verify`/`-CAfile`. Deliberately out of
  scope for now: `-inform` (mbedtls's parser auto-detects PEM/DER itself,
  same as real OpenSSL's own documented "has no effect"); `-key`/`-keyform`
  (only meaningful for `-gendelta`, also out of scope); `-dateopt`/
  `-nameopt` (cosmetic text-output formatting, same as `x509 -text`);
  `-hash`/`-hash_old`/`-fingerprint` (no matching helper/algorithm exposed
  for a CRL-focused tool); `-crlnumber` (would need to walk `crl.crl_ext`'s
  raw ASN.1 to extract the CRL Number extension by OID — no existing
  extension-*parsing* helper in this codebase to reuse, unlike writing;
  worth adding later, not a permanent gap); `-CApath`/`-CAstore`/
  `-no-CAfile`/`-no-CApath`/`-no-CAstore` (mbedtls has no hash-directory or
  store-lookup trust model — `-CAfile` alone, optionally a concatenated PEM
  bundle since `mbedtls_x509_crt_parse_file` already chains multiple PEM
  certs, covers the same use case); `-badsig`/`-gendelta` (testing/delta-CRL
  features, irrelevant to parse/display/verify).
- **`enc` only supports AES (128/192/256-bit) in CBC/CFB(128)/OFB/CTR**,
  not real `openssl enc`'s much larger cipher list (3DES/DES, Camellia,
  ARIA, Blowfish, CAST, ChaCha20, key-wrap ciphers — all out of scope).
  **ECB is deliberately excluded** for a concrete technical reason:
  `mbedtls_cipher_set_padding_mode()` only accepts `MBEDTLS_MODE_CBC`
  (confirmed in mbedtls's `library/cipher.c`) — mbedtls's generic cipher
  layer has no automatic padding support for ECB at all, so supporting it
  would mean hand-rolling PKCS7 padding/unpadding ourselves; combined with
  ECB being the weakest mode `openssl enc` offers, not worth it for v1.
  **CFB1/CFB8 sub-variants aren't available either** — mbedtls only
  exposes the 128-bit-segment CFB (which does match real OpenSSL's bare
  `-aes-256-cfb`, no suffix). **GCM/ChaCha20-Poly1305/AEAD ciphers are
  excluded by OpenSSL itself**, not just here (`openssl enc -aes-256-gcm`
  errors with `AEAD ciphers not supported` — confirmed empirically before
  scoping this out).
- **`enc -a`/`-base64` is not truly streamed** — the whole input (encrypt)
  or whole input file (decrypt, before base64-decoding) is read into
  memory rather than incrementally base64-encoded/decoded in chunks.
  Non-base64 mode streams properly and handles arbitrarily large files;
  `-a` is realistically used for smaller, text-embeddable payloads anyway.
  A deliberate v1 scope/complexity tradeoff, not a silent gap.
- **`enc -pass` only supports the `pass:literal` and `file:path` schemes**,
  not OpenSSL's `env:`/`fd:`/interactive-`stdin:` sources — lower value
  for a file-based CLI tool, and `-k`/`-kfile` already cover the common
  cases directly.
- **No utility falls back to writing to stdout when `-out` is omitted.**
  Real OpenSSL defaults `-out` to stdout across most subcommands — e.g.
  `openssl dhparam -in x.pem -check` (no `-out`, no `-noout`) both reports
  the check result *and* prints the PEM to stdout. mbedtls-clu's output
  path always goes through `fopen(outfile, ...)`-style writers with no
  such fallback, so the equivalent mbedtls-clu invocation only reports the
  check result — nothing is written anywhere, silently, rather than
  appearing on stdout. `dhparam_main` accounts for this deliberately (skips
  the write step entirely rather than trying to `fopen(NULL, ...)`), but
  it's a real, tool-wide gap from OpenSSL's convention, not just a
  `dhparam` quirk. **`enc` is the one deliberate exception**: it reads from
  stdin / writes to stdout when `-in`/`-out` are omitted, matching real
  `openssl enc`, because piping is central to how `enc` is actually used
  (backup/transport pipelines) in a way it isn't for the cert/key/CRL
  tools above.
- **`req -in <csr>` is display-only.** It parses the CSR and prints its
  summary (subject, key size, signature algorithm) via
  `mbedtls_x509_csr_info`, but is never fed into `-x509` conversion or any
  other output-writing path — `-out`/`-outform` are silently ignored
  whenever `-in` is used, and the command still reports exit 0 even though
  nothing is written. There is no "convert an existing CSR into a
  self-signed certificate" workflow in `req` — use `ca` to turn a CSR into
  a certificate instead (see `tests/cases/ca/`).
- **`req -x509`'s config-driven `extendedKeyUsage` is parsed but never
  applied.** `parse_config_file` reads the `extendedKeyUsage` tag into
  `req_params.extended_key_usage` and `req.c` checks it against `NULL` (to
  decide whether to enter the extension-writing block at all), but never
  actually calls anything to write it into the certificate. `ca.c` **does**
  apply `extendedKeyUsage` correctly when signing — this gap is isolated to
  `req.c`'s self-signed-cert path.
- **`mbedtls-clu rand -hex` output differs cosmetically from OpenSSL's:**
  uppercase hex (OpenSSL: lowercase) and a trailing CRLF instead of LF. The
  CRLF is inherited unmodified from mbedtls's own
  `mbedtls_mpi_write_file()` library function (confirmed in
  `library/bignum.c`) — not something mbedtls-clu introduces itself.
- **`rand` has no raw-binary output mode.** Every invocation goes through
  `mbedtls_mpi_write_file()` in hex form regardless of whether `-hex` is
  passed; there is no equivalent of OpenSSL's `rand <n>` writing `<n>` raw
  bytes to stdout.
- **`x509 -text`/`ca`'s database format are not line-for-line identical to
  OpenSSL's own `-text`/`index.txt` formats**, by design — this test suite
  only ever asserts semantic/field content (via openssl-as-extractor, or
  loose `grep`-style containment on mbedtls-clu's own text output), never a
  full-text diff against OpenSSL's output.
