# `ca` — Mini Certificate Authority

Equivalent to `openssl ca`. Signs CSRs into certificates, maintains a
flat-file certificate database (`index.txt`-equivalent) and serial counter,
issues CRLs, and revokes certificates. The largest and most stateful
utility in this tool — nearly everything it does is driven by `-config`
(see [config-files.md](config-files.md)).

## Synopsis

```
mbedtls-clu ca [options] [certreq]
```

`ca` has three mutually-exclusive modes, selected by which options are
given: **sign** (default — needs `-in`/`certreq` and `-out`), **`-gencrl`**,
and **`-revoke`**.

## Options

| Flag | Argument | Description |
|---|---|---|
| `-help` | | Display usage summary |
| `-batch`, `-utf8` | | Accepted, silently ignored (batch mode is always on; UTF-8-specific handling unsupported) |
| `-in` | `infile` | CSR to sign (alternative to trailing positional `certreq`) |
| `-inform` | `PEM\|DER` | CSR input format; default PEM |
| `-out` | `outfile` | Output file — signed cert (sign mode) or CRL (`-gencrl`) |
| `-config` | `infile` | Config file path — see [config-files.md](config-files.md) |
| `-startdate` | `val` | Cert `notBefore`, `YYMMDDHHMMSSZ`; mutually exclusive with `-days` |
| `-enddate` | `val` | Cert `notAfter`, `YYMMDDHHMMSSZ`; mutually exclusive with `-days` |
| `-days` | `+int` | Cert validity in days; mutually exclusive with `-startdate`/`-enddate` |
| `-md` | `val` | Digest to sign with, e.g. `sha256` (note: `-md <name>`, unlike `req`'s `-<name>` form) |
| `-keyfile` | `val` | CA private key |
| `-passin` | `val` | CA key password source |
| `-cert` | `infile` | CA certificate |
| `-gencrl` | | Generate a new CRL instead of signing; mutually exclusive with `-revoke` |
| `-crl_days` | `+int` | Days until the generated CRL's next update is due |
| `-revoke` | `infile` | Revoke a certificate (given as a file); mutually exclusive with `-gencrl` |
| `-extfile` | `infile` | Extra x509-extensions file, same tag format as an `x509_extensions` config section (see [config-files.md](config-files.md)) — **accepted but not listed in `ca -help`'s own usage text** |
| `-crl_reason` | `val` | **Listed in the usage text but not implemented** — see gap below |
| `-name`, `-section` | `val` | **Always a usage error** — CA section selection is config-file-only, not exposed on the CLI |
| `-policy` | `val` | **Always a usage error** — same as above, and see the policy-enforcement gap below |
| `certreq` | (positional) | Alternative to `-in` for the CSR to sign |

## Modes

- **Sign** (default): requires an input CSR (`-in` or positional
  `certreq`) and `-out`. Signs it into a certificate using `-keyfile`/
  `-cert`, applying the config's `x509_extensions` section (plus
  `-extfile`, if given), and appends an entry to the database.
- **`-gencrl`**: requires `-out`. Generates a CRL covering all revoked
  entries in the database, applying the config's `crl_extensions` section.
- **`-revoke <cert>`**: marks the given certificate's database entry as
  revoked (`R`). Does not take `-out`.

## Database / directory state

`ca` expects the database file (`[CA_default] database`), serial file
(`serial`), and `new_certs_dir` referenced by `-config` to already exist —
it does not create a CA working directory from scratch. The database
format is a flat-file, tab-separated `index.txt`-equivalent (status,
expiration date, revocation date, serial, filename, DN) — not guaranteed
line-for-line identical to real OpenSSL's own format, but semantically
equivalent (this test suite treats it that way).

## Examples

Sign a CSR:

```sh
mbedtls-clu ca -config ca.cnf -keyfile ca.key.pem -cert ca.cert.pem \
    -in req.csr.pem -out signed.pem -days 365 -md sha256
```

Generate a CRL:

```sh
mbedtls-clu ca -config ca.cnf -keyfile ca.key.pem -cert ca.cert.pem \
    -gencrl -out ca.crl.pem -crl_days 30
```

Revoke a previously-issued certificate:

```sh
mbedtls-clu ca -config ca.cnf -keyfile ca.key.pem -cert ca.cert.pem \
    -revoke signed.pem
```

Sign with an additional extensions file:

```sh
mbedtls-clu ca -config ca.cnf -keyfile ca.key.pem -cert ca.cert.pem \
    -in req.csr.pem -out signed.pem -days 365 -md sha256 -extfile server_ext.cnf
```

See [`tests/fixtures/conf/ca.cnf`](../tests/fixtures/conf/ca.cnf) for a
complete example config, and `tests/cases/ca/*.sh` for further worked
examples (database format, CRL generation, revocation, cross-tool interop
with real `openssl`).

## Known differences/gaps vs OpenSSL

Full details in
[`tests/KNOWN_DIFFERENCES.md`](../tests/KNOWN_DIFFERENCES.md). Summary:

- **`-crl_reason` is unimplemented.** It's listed in the usage text but
  never registered in the argument parser, so passing it is a usage error;
  revocation reason codes are never written to CRLs.
- **`policy` is parsed but never enforced.** The config's `policy` tag and
  its section's `countryName`/`stateOrProvinceName`/etc. rules
  (`match`/`supplied`/`optional`) are located and read, but only ever
  logged at debug level — nothing compares them against the CSR's subject.
  In practice, `ca` copies the CSR's subject through unchanged regardless
  of the configured policy. `-name`/`-section`/`-policy` as *command-line*
  flags are unconditionally rejected — config is the only way to select a
  `[ca]` section, and even then the policy section is inert. See
  [config-files.md](config-files.md).
- **No `$dir`-style variable expansion** in config values — every path in
  `[CA_default]` must be written out literally.
- **No private-key passphrase/encryption support.**
