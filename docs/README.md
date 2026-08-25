# MbedTLS-CLU Documentation

Per-command reference for the `mbedtls-clu` utilities. This documents current
behavior as implemented — options, inputs/outputs, syntax, and where each
command differs from (or is a subset of) the equivalent `openssl` command.

For build/test instructions and architecture, see the top-level
[`README.md`](../README.md) and [`CLAUDE.md`](../CLAUDE.md). For the full,
authoritative list of intentional OpenSSL-compatibility gaps and known bugs
(the source of truth this documentation summarizes per-command), see
[`tests/KNOWN_DIFFERENCES.md`](../tests/KNOWN_DIFFERENCES.md).

## Commands

`mbedtls-clu <utility> [utility options]` — a single multi-call binary, one
utility per page:

| Utility | Page | Equivalent to | Purpose |
|---|---|---|---|
| `ca` | [ca.md](ca.md) | `openssl ca` | Mini Certificate Authority: sign CSRs, maintain a cert database, issue CRLs, revoke certs |
| `req` | [req.md](req.md) | `openssl req` | Generate CSRs and self-signed certificates |
| `x509` | [x509.md](x509.md) | `openssl x509` | Certificate display (read-only) |
| `crl` | [crl.md](crl.md) | `openssl crl` | CRL display and signature verification (read-only) |
| `genpkey` | [genpkey.md](genpkey.md) | `openssl genpkey` | Private key generation (RSA, EC) |
| `dhparam` | [dhparam.md](dhparam.md) | `openssl dhparam` | Diffie-Hellman parameter generation/validation |
| `enc` | [enc.md](enc.md) | `openssl enc` | Symmetric file encryption/decryption |
| `rand` | [rand.md](rand.md) | `openssl rand` | Random byte generation |

Also see [config-files.md](config-files.md) for the `openssl.cnf`-style
config file format shared by `req` and `ca` (`-config`).

Run `mbedtls-clu help` or `mbedtls-clu <utility> -help` for the tool's own
built-in usage summary at any time — that summary is the first thing each
page below is built from, cross-checked against the argument parser and, for
gaps, the test suite in `tests/`.

## Conventions used on these pages

- **Synopsis** — the command's usage line.
- **Options** — every flag the argument parser actually accepts (not just
  those printed by `-help` — a couple of commands accept flags their own
  `-help` text omits, noted where that happens).
- **Known differences from OpenSSL** — a short, command-specific excerpt of
  [`tests/KNOWN_DIFFERENCES.md`](../tests/KNOWN_DIFFERENCES.md); consult that
  file for the complete, current list and rationale.

None of these tools default `-out` to stdout the way real OpenSSL does
(except `enc`, which reads stdin/writes stdout when `-in`/`-out` are
omitted, matching real `openssl enc`) — see
[`tests/KNOWN_DIFFERENCES.md`](../tests/KNOWN_DIFFERENCES.md) for the general
note on this.
