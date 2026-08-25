# `enc` — Symmetric File Encryption/Decryption

Equivalent to `openssl enc`. File-format compatible with real OpenSSL —
files produced by either tool can be decrypted by the other (`Salted__`
header, both the legacy and `-pbkdf2` key-derivation schemes). Unlike every
other utility here, `enc` reads stdin / writes stdout when `-in`/`-out` are
omitted, matching real `openssl enc` (piping is central to how it's used).

## Synopsis

```
mbedtls-clu enc [options]
```

## Options

| Flag | Argument | Description |
|---|---|---|
| `-help` | | Display usage summary |
| `-e` | | Encrypt (default if neither `-e` nor `-d` given) |
| `-d` | | Decrypt |
| `-p` | | Print the derived key/IV |
| `-P` | | Print the derived key/IV and exit (no en/decryption) |
| `-in` | `infile` | Input file (default: stdin) |
| `-k` | `val` | Passphrase |
| `-kfile` | `infile` | Read passphrase from a file |
| `-pass` | `val` | Passphrase source — `pass:value` or `file:path` only (see gap below) |
| `-out` | `outfile` | Output file (default: stdout) |
| `-a`, `-base64` | | Base64 encode/decode, depending on `-e`/`-d` |
| `-A` | | With `-a`, write base64 as a single line |
| `-salt` | | Use a salt in the KDF (default) |
| `-nosalt` | | Do not use a salt in the KDF |
| `-nopad` | | Disable standard block padding |
| `-K` | `val` | Raw key, in hex — bypasses the KDF entirely |
| `-S` | `val` | Salt, in hex |
| `-iv` | `val` | IV, in hex |
| `-md` | `val` | Digest for the KDF (default: sha256) |
| `-pbkdf2` | | Use PBKDF2 for the KDF instead of the legacy scheme |
| `-iter` | `+int` | Iteration count; implies `-pbkdf2`; default 10000 |
| `-aes-{128,192,256}-{cbc,cfb,ofb,ctr}` | | Cipher to use (one required) — 12 total combinations |

## Key derivation

- **Default (legacy)**: OpenSSL's classic `EVP_BytesToKey`-style KDF from
  `-k`/`-kfile`/`-pass`, salted (`-salt`, default) or not (`-nosalt`),
  digest selectable via `-md` (default sha256).
- **`-pbkdf2`** (or any `-iter`): `mbedtls_pkcs5_pbkdf2_hmac_ext`, a
  genuinely different code path from the legacy KDF — interoperable with
  `openssl enc -pbkdf2` in both directions.
- **`-K`/`-iv`** bypasses the KDF entirely: raw hex key/IV supplied
  directly, no `Salted__` header written at all.

## Output

Ciphertext (or plaintext, when decrypting) to `-out`, or stdout if `-out`
is omitted. With `-a`/`-base64`, output is base64-encoded/decoded around
the cipher stream.

## Examples

Passphrase-based, default (legacy) KDF:

```sh
mbedtls-clu enc -aes-256-cbc -in plain.txt -out cipher.enc -k mypassword
mbedtls-clu enc -d -aes-256-cbc -in cipher.enc -out plain.txt -k mypassword
```

PBKDF2, round-trippable with real OpenSSL:

```sh
mbedtls-clu enc -aes-256-cbc -pbkdf2 -in plain.txt -out cipher.enc -k mypassword
openssl enc -aes-256-cbc -d -pbkdf2 -in cipher.enc -out plain.txt -k mypassword
```

Raw key/IV, no KDF:

```sh
mbedtls-clu enc -aes-256-cbc \
    -K 000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f \
    -iv 000102030405060708090a0b0c0d0e0f \
    -in plain.txt -out cipher.enc
```

Base64, single-line, over a pipe:

```sh
echo "secret" | mbedtls-clu enc -aes-256-cbc -a -A -k mypassword \
    | mbedtls-clu enc -d -aes-256-cbc -a -k mypassword
```

## Known differences/gaps vs OpenSSL

Full details in
[`tests/KNOWN_DIFFERENCES.md`](../tests/KNOWN_DIFFERENCES.md). Summary:

- **Only AES (128/192/256) in CBC/CFB(128)/OFB/CTR** — no
  3DES/DES/Camellia/ARIA/Blowfish/CAST/ChaCha20/key-wrap ciphers. **ECB is
  deliberately excluded** (mbedtls's generic cipher layer has no automatic
  padding for it). **CFB1/CFB8 sub-variants aren't available** (mbedtls
  only exposes 128-bit-segment CFB, matching real OpenSSL's bare
  `-aes-256-cfb`). **GCM/ChaCha20-Poly1305/AEAD are excluded by OpenSSL's
  own `enc` too**, not just here.
- **`-a`/`-base64` is not truly streamed** — the whole input (encrypt) or
  file (decrypt, before base64-decoding) is read into memory rather than
  incrementally encoded/decoded. Non-base64 mode streams properly for
  arbitrarily large files.
- **`-pass` only supports `pass:literal` and `file:path`** — not OpenSSL's
  `env:`/`fd:`/interactive-`stdin:` sources.
