/* crl -	Display and verify x509 CRLs
 *				This utility attempts to be syntax compatible with the equivalent
 *				openssl utlility: openssl crl -in /path/to/file -text -noout etc...
 * 			Originally created for the Gargoyle Web Interface
 *
 * 			Created By Michael Gray
 * 			http://www.lantisproject.com
 *
 *			Based on example mbedtls/programs/x509/crl_app.c
 *			Copyright The Mbed TLS Contributors
 *			Licensed under the Apache License, Version 2.0
 *			http://www.apache.org/licenses/LICENSE-2.0
 *
 * Copyright © 2024 by Michael Gray <support@lantisproject.com>
 *
 * This file is free software: you may copy, redistribute and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 2 of the License, or (at your
 * option) any later version.
 *
 * This file is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "crl.h"

// mbedtls_pem_write_buffer is declared in mbedtls/pem.h (already included via
// crl.h) and defined by libmbedcrypto. These two labels are the read-side
// counterpart to the ones in x509write_crl.h - duplicated here rather than
// pulling in that write-path header from this read-only utility.
#define PEM_BEGIN_CRL	"-----BEGIN X509 CRL-----\n"
#define PEM_END_CRL		"-----END X509 CRL-----\n"

#define USAGE \
    "\n usage: crl [options]\n"															\
    "\n\n General options:\n"																\
    "    -help					Display this summary\n"										\
	"    -in infile				CRL input file (PEM or DER, auto-detected)\n"				\
	"    -out outfile			Write the parsed CRL back out (PEM by default)\n"			\
	"    -outform PEM|DER		Output format for -out (default PEM)\n"						\
	"    -noout					Don't write -out (only print requested fields)\n"			\
	"\n\n CRL printing options:\n"															\
	"    -text					Print the CRL in text form\n"								\
	"    -issuer				Print the issuer DN\n"										\
	"    -lastupdate			Print the lastUpdate field\n"								\
	"    -nextupdate			Print the nextUpdate field\n"								\
	"\n\n CRL verification options:\n"														\
	"    -verify				Verify the CRL's signature (requires -CAfile)\n"			\
	"    -CAfile infile			Trusted CA certificate (or PEM bundle) to verify against\n"

#if !defined(MBEDTLS_BIGNUM_C) || !defined(MBEDTLS_ENTROPY_C) ||  \
    !defined(MBEDTLS_X509_CRL_PARSE_C) || !defined(MBEDTLS_X509_CRT_PARSE_C) || \
	!defined(MBEDTLS_FS_IO) || !defined(MBEDTLS_CTR_DRBG_C) || \
	!defined(MBEDTLS_PK_C) || !defined(MBEDTLS_MD_C)
int crl_main(void)
{
    mbedtls_printf("MBEDTLS_BIGNUM_C and/or MBEDTLS_ENTROPY_C and/or "
                   "MBEDTLS_X509_CRL_PARSE_C and/or MBEDTLS_X509_CRT_PARSE_C and/or "
                   "MBEDTLS_FS_IO and/or MBEDTLS_CTR_DRBG_C and/or "
                   "MBEDTLS_PK_C and/or MBEDTLS_MD_C "
                   "not defined.\n");
    mbedtls_exit(0);
}
#else

int crl_main(int argc, char** argv, int argi)
{
    int ret = 1;
    int exit_code = MBEDTLS_EXIT_FAILURE;
    char buf[16000];
    int i;
    char *p;
    mbedtls_x509_crl crl;
    mbedtls_x509_crt cacert;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;

	int text = 0;
	int noout = 0;
	int print_issuer = 0;
	int print_lastupdate = 0;
	int print_nextupdate = 0;
	int do_verify = 0;
	int output_format = FORMAT_PEM;
	char* crlfile_in = NULL;
	char* crlfile_out = NULL;
	char* cafile_in = NULL;

    /*
     * Set to sane values
     */
    mbedtls_ctr_drbg_init(&ctr_drbg);
    mbedtls_entropy_init(&entropy);
	mbedtls_x509_crl_init(&crl);
	mbedtls_x509_crt_init(&cacert);

#if defined(MBEDTLS_USE_PSA_CRYPTO)
    psa_status_t status = psa_crypto_init();
    if (status != PSA_SUCCESS) {
        mbedtlsclu_prio_printf(MBEDTLSCLU_ERR, "Failed to initialize PSA Crypto implementation: %d\n",
                        (int) status);
        goto exit;
    }
#endif /* MBEDTLS_USE_PSA_CRYPTO */

	if(argc < 2)
	{
usage:
		mbedtls_printf(USAGE);
		goto exit;
	}

	for(i = argi; i < argc; i++)
	{
		p = argv[i];

		if(strcmp(p,"-help") == 0)
		{
			goto usage;
		}
		else if(strcmp(p,"-text") == 0)
		{
			text = 1;
		}
		else if(strcmp(p,"-noout") == 0)
		{
			noout = 1;
		}
		else if(strcmp(p,"-issuer") == 0)
		{
			print_issuer = 1;
		}
		else if(strcmp(p,"-lastupdate") == 0)
		{
			print_lastupdate = 1;
		}
		else if(strcmp(p,"-nextupdate") == 0)
		{
			print_nextupdate = 1;
		}
		else if(strcmp(p,"-verify") == 0)
		{
			do_verify = 1;
		}
		else if(strcmp(p,"-in") == 0 && i + 1 < argc)
		{
			// argv[i+1] should be the CRL file. Advance i
			i += 1;
			crlfile_in = strdup(argv[i]);
		}
		else if(strcmp(p,"-out") == 0 && i + 1 < argc)
		{
			// argv[i+1] should be the output file. Advance i
			i += 1;
			crlfile_out = strdup(argv[i]);
		}
		else if(strcmp(p,"-outform") == 0 && i + 1 < argc)
		{
			// argv[i+1] should be the output format. Advance i
			i += 1;
			p = argv[i];
			if(strcmp(p,"PEM") == 0)
			{
				output_format = FORMAT_PEM;
			}
			else if(strcmp(p,"DER") == 0)
			{
				output_format = FORMAT_DER;
			}
			else
			{
				goto usage;
			}
		}
		else if(strcmp(p,"-CAfile") == 0 && i + 1 < argc)
		{
			// argv[i+1] should be the CA cert file. Advance i
			i += 1;
			cafile_in = strdup(argv[i]);
		}
		else
		{
			// unknown param
			goto usage;
		}
	}

	if(crlfile_in == NULL)
	{
		goto usage;
	}
	if(do_verify && cafile_in == NULL)
	{
		goto usage;
	}

	mbedtlsclu_prio_printf(MBEDTLSCLU_DEBUG,"cl: crlfile_in: %s\n", crlfile_in);
	mbedtlsclu_prio_printf(MBEDTLSCLU_DEBUG,"cl: crlfile_out: %s\n", crlfile_out);
	mbedtlsclu_prio_printf(MBEDTLSCLU_DEBUG,"cl: outform: %s\n", (output_format == FORMAT_PEM ? "PEM" : "DER"));
	mbedtlsclu_prio_printf(MBEDTLSCLU_DEBUG,"cl: noout: %d\n", noout);
	mbedtlsclu_prio_printf(MBEDTLSCLU_DEBUG,"cl: text: %d\n", text);
	mbedtlsclu_prio_printf(MBEDTLSCLU_DEBUG,"cl: verify: %d\n", do_verify);
	mbedtlsclu_prio_printf(MBEDTLSCLU_DEBUG,"cl: cafile_in: %s\n", cafile_in);

	/*
	 * 1.0. Load the CRL
	 */
	mbedtlsclu_prio_printf(MBEDTLSCLU_INFO,"\n  . Loading the CRL ...");
	fflush(stdout);

	ret = mbedtls_x509_crl_parse_file(&crl, crlfile_in);
	if (ret != 0) {
		mbedtlsclu_prio_printf(MBEDTLSCLU_ERR," failed\n  !  mbedtls_x509_crl_parse_file returned -0x%04x\n\n", (unsigned int) -ret);
		goto exit;
	}

	mbedtlsclu_prio_printf(MBEDTLSCLU_INFO," ok\n");

	if(text)
	{
		ret = mbedtls_x509_crl_info(buf, sizeof(buf) - 1, "      ", &crl);
		if (ret == -1) {
			mbedtlsclu_prio_printf(MBEDTLSCLU_ERR," failed\n  !  mbedtls_x509_crl_info returned %d\n\n", ret);
			goto exit;
		}
		mbedtls_printf("%s\n", buf);
	}

	if(print_issuer)
	{
		char dnbuf[512];
		ret = mbedtls_x509_dn_gets(dnbuf, sizeof(dnbuf), &crl.issuer);
		if (ret < 0) {
			mbedtlsclu_prio_printf(MBEDTLSCLU_ERR," failed\n  !  mbedtls_x509_dn_gets returned %d\n\n", ret);
			goto exit;
		}
		mbedtls_printf("issuer=%s\n", dnbuf);
	}

	if(print_lastupdate)
	{
		mbedtls_printf("lastUpdate=%04d-%02d-%02d %02d:%02d:%02d UTC\n",
			crl.this_update.year, crl.this_update.mon, crl.this_update.day,
			crl.this_update.hour, crl.this_update.min, crl.this_update.sec);
	}

	if(print_nextupdate)
	{
		mbedtls_printf("nextUpdate=%04d-%02d-%02d %02d:%02d:%02d UTC\n",
			crl.next_update.year, crl.next_update.mon, crl.next_update.day,
			crl.next_update.hour, crl.next_update.min, crl.next_update.sec);
	}

	if(do_verify)
	{
		mbedtls_x509_crt* cur;
		int verify_ok = 0;

		ret = mbedtls_x509_crt_parse_file(&cacert, cafile_in);
		if (ret != 0) {
			mbedtlsclu_prio_printf(MBEDTLSCLU_ERR," failed\n  !  mbedtls_x509_crt_parse_file returned -0x%04x\n\n", (unsigned int) -ret);
			goto exit;
		}

		cur = &cacert;
		while(cur != NULL && x509_name_cmp(&crl.issuer, &cur->subject) != 0)
		{
			cur = cur->next;
		}

		if(cur == NULL)
		{
			mbedtlsclu_prio_printf(MBEDTLSCLU_ERR,"verify failure\n  !  CRL issuer does not match any subject in -CAfile\n\n");
		}
		else
		{
			unsigned char hash[MBEDTLS_MD_MAX_SIZE];
			size_t hash_length = 0;

#if defined(MBEDTLS_USE_PSA_CRYPTO)
			psa_algorithm_t psa_algorithm = mbedtls_md_psa_alg_from_type(crl.MBEDTLS_PRIVATE(sig_md));
			if (psa_hash_compute(psa_algorithm, crl.tbs.p, crl.tbs.len,
								 hash, sizeof(hash), &hash_length) != PSA_SUCCESS) {
				mbedtlsclu_prio_printf(MBEDTLSCLU_ERR,"verify failure\n  !  psa_hash_compute failed\n\n");
				goto exit;
			}
#else
			const mbedtls_md_info_t* md_info = mbedtls_md_info_from_type(crl.MBEDTLS_PRIVATE(sig_md));
			hash_length = mbedtls_md_get_size(md_info);
			if (mbedtls_md(md_info, crl.tbs.p, crl.tbs.len, hash) != 0) {
				mbedtlsclu_prio_printf(MBEDTLSCLU_ERR,"verify failure\n  !  mbedtls_md failed\n\n");
				goto exit;
			}
#endif /* MBEDTLS_USE_PSA_CRYPTO */

			ret = mbedtls_pk_verify_ext(crl.MBEDTLS_PRIVATE(sig_pk), crl.MBEDTLS_PRIVATE(sig_opts),
										&cur->pk, crl.MBEDTLS_PRIVATE(sig_md),
										hash, hash_length,
										crl.MBEDTLS_PRIVATE(sig).p, crl.MBEDTLS_PRIVATE(sig).len);
			verify_ok = (ret == 0);
		}

		if(verify_ok)
		{
			mbedtls_printf("verify OK\n");
		}
		else
		{
			mbedtlsclu_prio_printf(MBEDTLSCLU_ERR,"verify failure\n");
			goto exit;
		}
	}

	if(!noout && crlfile_out != NULL)
	{
		FILE* fout;

		if ((fout = fopen(crlfile_out, "wb")) == NULL) {
			mbedtlsclu_prio_printf(MBEDTLSCLU_ERR," failed\n  !  Could not create %s\n\n", crlfile_out);
			goto exit;
		}

		if(output_format == FORMAT_DER)
		{
			if (fwrite(crl.raw.p, 1, crl.raw.len, fout) != crl.raw.len) {
				fclose(fout);
				goto exit;
			}
		}
		else
		{
			unsigned char pembuf[16000];
			size_t olen = 0;

			if (mbedtls_pem_write_buffer(PEM_BEGIN_CRL, PEM_END_CRL,
										 crl.raw.p, crl.raw.len,
										 pembuf, sizeof(pembuf), &olen) != 0) {
				fclose(fout);
				goto exit;
			}
			if (fwrite(pembuf, 1, strlen((char*) pembuf), fout) != strlen((char*) pembuf)) {
				fclose(fout);
				goto exit;
			}
		}

		fclose(fout);
	}

    exit_code = MBEDTLS_EXIT_SUCCESS;

exit:

	mbedtls_x509_crl_free(&crl);
	mbedtls_x509_crt_free(&cacert);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);
#if defined(MBEDTLS_USE_PSA_CRYPTO)
    mbedtls_psa_crypto_free();
#endif /* MBEDTLS_USE_PSA_CRYPTO */

    return exit_code;
}
#endif /* MBEDTLS_BIGNUM_C && MBEDTLS_ENTROPY_C && MBEDTLS_X509_CRL_PARSE_C &&
		MBEDTLS_X509_CRT_PARSE_C && MBEDTLS_FS_IO && MBEDTLS_CTR_DRBG_C &&
		MBEDTLS_PK_C && MBEDTLS_MD_C */
