# Config file format (`-config`)

`req` and `ca` both accept `-config <file>`, an OpenSSL-`openssl.cnf`-style
config file. It is optional for `req` (only used for defaults/extensions);
it is what supplies `ca`'s CA state (database, serial, directories) since
`ca` has no equivalent of the individual `-database`/`-new_certs_dir`-style
flags OpenSSL offers as alternatives.

## Format

INI-style `[section]` / `tag = value` text, parsed by
`read_config_file`/`locate_tag`/`locate_value` in `mbedtlsclu_common.c`.
Two things differ from real OpenSSL's config parser worth knowing up front:

- **No `$dir`-style variable expansion.** OpenSSL's config format lets you
  write `new_certs_dir = $dir/newcerts` and have `$dir` substituted from
  the `dir` tag. mbedtls-clu's parser takes values verbatim — every path
  must be written out in full (see `tests/fixtures/conf/ca.cnf` for a
  config written this way).
- Values may be wrapped in double quotes (as EasyRSA's `expand_ssl_config()`
  sometimes emits); surrounding quotes are stripped.

## `MBEDTLS_CONF` / `OPENSSL_CONF` environment variables

In addition to `-config`, the config file path can be set via the
`MBEDTLS_CONF` environment variable. When built with
`OPENSSL_ENV_CONF_COMPAT` (the Makefile's default), `OPENSSL_CONF` is also
honored as a fallback — `MBEDTLS_CONF` wins if both are set. Command-line
`-config` overrides both.

## `[req]` section — read by `req`

```ini
[req]
distinguished_name = req_distinguished_name   # tag naming the DN defaults section
x509_extensions     = v3_ext                  # tag naming the extensions section (only under -x509)
default_md           = sha256                 # digest, if no -<digest> flag given on the CLI
default_bits          = 2048                  # (parsed; see req.md for current effect)
default_keyfile        = key.pem              # (parsed; see req.md for current effect)
```

### DN defaults section (named by `distinguished_name`)

```ini
[req_distinguished_name]
countryName_default            = GB
stateOrProvinceName_default    = Test State
localityName_default           = Test City
0.organizationName_default     = mbedtls-clu Tests
organizationalUnitName_default = QA
commonName_default             = conf-test.example.com
emailAddress_default           = qa@example.com
serialNumber_default           = 01
```

### Extensions section (named by `x509_extensions`, `req -x509` only)

```ini
[v3_ext]
subjectKeyIdentifier   = hash
authorityKeyIdentifier = keyid,issuer
basicConstraints        = critical,CA:TRUE
keyUsage                 = critical,digitalSignature,keyCertSign
extendedKeyUsage          = serverAuth
nsCertType                 = server
```

Values are passed straight to the same extension-writing logic used by
`ca`. Note the `req`-specific gap: `extendedKeyUsage` here is parsed but
never actually applied to the certificate (see req.md) — `ca` applies the
equivalent tag correctly when signing.

## `[ca]` / `[<default_ca>]` sections — read by `ca`

```ini
[ca]
default_ca = CA_default

[CA_default]
dir             = .                 # parsed but not otherwise used (no $dir expansion)
certs           = ./certs           # parsed; not read back by any current code path
crl_dir         = ./crl             # parsed; not read back by any current code path
new_certs_dir   = ./newcerts        # parsed; not read back by any current code path
database        = ./index.txt       # the flat-file cert database (index.txt-equivalent)
serial          = ./serial          # next-serial-number file
certificate     = ./ca.cert.pem     # (informational; CA cert is actually supplied via -cert)
private_key     = ./ca.key.pem      # (informational; CA key is actually supplied via -keyfile)
crl             = ./ca.crl.pem      # (informational; CRL output is via -out)
default_days    = 365               # cert validity, if -days/-startdate/-enddate not given
default_crl_days = 30               # CRL validity, if -crl_days not given
default_md      = sha256            # digest, if -md not given
preserve        = no                # parsed; no current effect
unique_subject  = yes               # parsed; no current effect
policy          = policy_any        # parsed and located, but never enforced (see note below)
x509_extensions = usr_cert          # tag naming the extensions section applied to signed certs
crl_extensions  = crl_ext           # tag naming the extensions section applied to generated CRLs
```

> **`policy` is not enforced.** mbedtls-clu locates the named policy section
> and reads its `countryName`/`stateOrProvinceName`/.../`emailAddress`
> values, but only ever logs them at debug level — there is no code path
> that actually compares the CSR's subject fields against the policy's
> `match`/`supplied`/`optional` rules. In practice, `ca` copies the CSR's
> subject through unchanged regardless of what `policy` says. `-name`,
> `-section`, and `-policy` as *command-line* flags are unconditionally
> rejected (`goto usage`) — config is the only way to point at a `[ca]`
> section, and even then the policy section's content is inert.

### Extensions sections (`x509_extensions` / `crl_extensions`)

Same tag set as `req`'s `[v3_ext]` above
(`subjectKeyIdentifier`/`authorityKeyIdentifier`/`basicConstraints`/
`keyUsage`/`extendedKeyUsage`/`nsCertType`), plus `-extfile <file>` on the
command line can supply an additional standalone extensions file parsed the
same way (`ca -extfile ext.cnf` — a file containing just an extensions-style
`tag = value` block, no `[section]` header needed since the whole file is
read as one block). Note `-extfile` is accepted by the argument parser but
is **not listed in `ca -help`'s own usage text** — a real gap between the
tool's self-documentation and its behavior.

## Full worked examples

See [`tests/fixtures/conf/req.cnf`](../tests/fixtures/conf/req.cnf) and
[`tests/fixtures/conf/ca.cnf`](../tests/fixtures/conf/ca.cnf) for complete,
tested config files, and `tests/cases/req/05_config_extensions_applied.sh` /
`tests/cases/ca/*.sh` for how they're driven end-to-end.
