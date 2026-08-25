# `crl` — CRL Display and Verification

Equivalent to a minimal, high-value subset of `openssl crl`. Displays
fields from a Certificate Revocation List and can verify its signature
against a trusted CA certificate. Read-side counterpart to
[`ca -gencrl`](ca.md), which generates CRLs.

## Synopsis

```
mbedtls-clu crl [options]
```

## Options

| Flag | Argument | Description |
|---|---|---|
| `-help` | | Display usage summary |
| `-in` | `infile` | CRL input file — PEM or DER, auto-detected (no `-inform` needed) |
| `-out` | `outfile` | Write the parsed CRL back out (PEM by default) |
| `-outform` | `PEM\|DER` | Output format for `-out`; default PEM |
| `-noout` | | Don't write `-out` — only print requested fields |
| `-text` | | Print the CRL in text form |
| `-issuer` | | Print the issuer DN |
| `-lastupdate` | | Print the `lastUpdate` field |
| `-nextupdate` | | Print the `nextUpdate` field |
| `-verify` | | Verify the CRL's signature (requires `-CAfile`) |
| `-CAfile` | `infile` | Trusted CA certificate (or a concatenated PEM bundle) to verify against |

## Examples

```sh
mbedtls-clu crl -in ca.crl.pem -noout -text
mbedtls-clu crl -in ca.crl.pem -noout -issuer -lastupdate -nextupdate
mbedtls-clu crl -in ca.crl.pem -noout -verify -CAfile ca.cert.pem
mbedtls-clu crl -in ca.crl.der -outform PEM -out ca.crl.pem   # PEM/DER convert (input auto-detected)
```

## Known differences/gaps vs OpenSSL

Full details in
[`tests/KNOWN_DIFFERENCES.md`](../tests/KNOWN_DIFFERENCES.md). Deliberately
out of scope for now:

- `-inform` (input is auto-detected instead — matches real OpenSSL's own
  documented "has no effect" for this flag)
- `-key`/`-keyform` (only meaningful for `-gendelta`, also out of scope)
- `-dateopt`/`-nameopt` (cosmetic text-output formatting)
- `-hash`/`-hash_old`/`-fingerprint`
- `-crlnumber` (would need raw-ASN.1 extension parsing not currently
  available in this codebase for reading; worth adding later)
- `-CApath`/`-CAstore`/`-no-CAfile`/`-no-CApath`/`-no-CAstore` (mbedtls has
  no hash-directory/store trust model — `-CAfile` alone, optionally a
  concatenated PEM bundle, covers the same use case since
  `mbedtls_x509_crt_parse_file` already chains multiple PEM certs)
- `-badsig`/`-gendelta` (testing/delta-CRL features)
