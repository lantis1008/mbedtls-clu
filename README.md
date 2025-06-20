MbedTLS Command-Line Utility (MbedTLS-CLU)
==========================================

MbedTLS-CLU is a C application which makes use of the [MbedTLS SSL library][1] for the creation of Public Key Infrastructure.  
This tool has been initially created with the [Gargoyle Router Project][2] in mind, but may be useful to others.

What version of mbedtls does it compile against?
-----------
MbedTLS-CLU is tested and compiled against v2.28.9, with [one additional patch][5] required to add extended key usage extension into the codebase

What is it?
-----------

MbedTLS-CLU is a minimal set of command line tools for use with [OpenVPN EasyRSA 3][3] and the [Gargoyle Router Project][2] firmware. It can:
- Generate random bytes
- Generate Diffie-Hellman parameters
- Generate Certificate Requests
- Generate Certificates
- Sign Certificates
- Act as a *mini* Certificate Authority

It is designed to be *mostly* compatible with the equivalent [OpenSSL utilities (openssl)][4]

What isn't it?
--------------

MbedTLS-CLU is **not** a full replacement for the equivalent [OpenSSL utilities][4]. Not all features are supported.

[1]: <https://github.com/Mbed-TLS/mbedtls> "MbedTLS Git"
[2]: <https://github.com/ericpaulbishop/gargoyle> "Gargoyle Router Project Git"
[3]: <https://github.com/OpenVPN/easy-rsa> "OpenVPN EasyRSA"
[4]: <https://github.com/openssl/openssl> "OpenSSL Git"
[5]: <https://raw.githubusercontent.com/ericpaulbishop/gargoyle/a9e9ad4291ecd58a01b2f5377fc0aa942cd89235/patches-generic/130-mbedtls_extended_key_usage.patch> "mbedtls v2.28.9 patch"
