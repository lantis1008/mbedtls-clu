# `genpkey` — Private Key Generation

Equivalent to a subset of `openssl genpkey`. Generates RSA or EC private
keys. Used internally by [`req -newkey`](req.md).

## Synopsis

```
mbedtls-clu genpkey [options]
```

## Options

| Flag | Argument | Description |
|---|---|---|
| `-help` | | Display usage summary |
| `-algorithm` | `rsa\|ec` | Public key algorithm — only these two are supported |
| `-pkeyopt` | `opt:value` | Set an algorithm option — see table below |
| `-usedevrandom` | | Use `/dev/random` instead of the default entropy source (only available when built with `MBEDTLS_FS_IO`) |
| `-out` | `outfile` | Output file |
| `-outform` | `PEM\|DER` | Output format |
| `-text` | | Print the private key in text form |
| `-pass` | `val` | **UNSUPPORTED** — listed for compatibility, has no effect |
| `-*` | | **UNSUPPORTED** — cipher-to-encrypt-the-key flags accepted for compatibility, have no effect |

### `-pkeyopt` values

| Option | Applies to | Description |
|---|---|---|
| `rsa_keygen_bits:<bits>` | `-algorithm rsa` | RSA key size; default 2048 |
| `ec_paramgen_curve:<curve>` | `-algorithm ec` | EC curve — **mbedtls's own curve names only** (e.g. `secp256r1`), not OpenSSL aliases (e.g. `prime256v1`) — see gap below |
| `ec_param_enc:named_curve\|explicit` | `-algorithm ec` | Parsed but has no effect — see gap below |

Run `mbedtls-clu genpkey -help` for the full list of accepted curve names.

## Examples

```sh
mbedtls-clu genpkey -algorithm rsa -pkeyopt rsa_keygen_bits:2048 -out rsa.key.pem
mbedtls-clu genpkey -algorithm ec -pkeyopt ec_paramgen_curve:secp256r1 -out ec.key.pem
```

## Known differences/gaps vs OpenSSL

Full details in
[`tests/KNOWN_DIFFERENCES.md`](../tests/KNOWN_DIFFERENCES.md). Summary:

- **Only `rsa` and `ec`** — no dsa/ed25519/dh key types.
- **`ec_paramgen_curve` only accepts mbedtls's own curve names**, not
  OpenSSL's common aliases for the same curves.
- **`ec_param_enc` is parsed but has no effect** — `named_curve` and
  `explicit` produce identical output (always named-curve form).
- **No private-key passphrase/encryption support** — `-pass` and cipher
  selection are explicitly unsupported.
