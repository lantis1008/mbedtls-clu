/* req -	Generate Certificate Request
 *				This utility attempts to be syntax compatible with the equivalent
 *				openssl utlility: openssl req -new -out /path/to/file etc...
 * 			Originally created for the Gargoyle Web Interface
 *
 * 			Created By Michael Gray
 * 			http://www.lantisproject.com
 *
 *			Based on example mbedtls/programs/x509/cert_req.c
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

#include "req.h"
#include "genpkey.h"

#define DFL_FILENAME            "keyfile.key"
#define DFL_PASSWORD            NULL
#define DFL_DEBUG_LEVEL         0
#define DFL_OUTPUT_FILENAME     "cert.req"
#define DFL_SUBJECT_NAME        "CN=Cert,O=mbed TLS,C=UK"
#define DFL_KEY_USAGE           0
#define DFL_FORCE_KEY_USAGE     0
#define DFL_NS_CERT_TYPE        0
#define DFL_FORCE_NS_CERT_TYPE  0
#define DFL_MD_ALG              MBEDTLS_MD_SHA256

#define DFL_SERIAL				"1"
#define DFL_DAYS				365
#define DFL_VERSION				MBEDTLS_X509_CRT_VERSION_3

#define USAGE \
    "\n usage: req [options]\n"																				\
    "\n\n General options:\n"																				\
    "    -help					Display this summary\n"														\
    "    -config infile			Filepath to config file\n"													\
    "							NOTE: Command line parameters will override any config file equivalents\n"	\
    "							Config file can also be set through environment variables\n"				\
    "							NOTE: Environment variable config file will override command line\n"		\
	"    -in infile				X.509 request input file\n"													\
	"\n\n Certificate options:\n"																			\
    "    -new					New request\n"																\
    "    -x509					Output an X.509 certificate structure instead of a cert request\n"			\
    "    -days +int				Number of days the cert is valid for\n"										\
    "    -set_serial val		Serial number to use\n"														\
    "\n"																									\
    "    -subj val				Set or modify subject of request or cert\n"									\
	"\n\n Key and signing options:\n"																		\
    "    -key val				Key for signing\n"															\
    "    -passin val			Private key and certificate password source\n"								\
    "    -newkey val			Generate new key with [<alg>:]<nbits> or <alg>[:<file>] or param:<file>\n"	\
    "    -keyout outfile		File to write private key to\n"												\
	"\n\n Output options:\n"																				\
	"    -out outfile			Output file\n"																\
	"    -outform PEM|DER		Output format (DER or PEM)\n"												\
	"    -text					Print the certificate request in text\n"									\
	"    -*						Message Digest (MD) (default SHA256)\n"										\
	"								possible values:\n"														\
	"								MD2, MD4, MD5, RIPEMD160, SHA1\n"										\
	"								SHA224, SHA256, SHA384, SHA512\n"

#if !defined(MBEDTLS_X509_CSR_WRITE_C) || !defined(MBEDTLS_FS_IO) ||  \
    !defined(MBEDTLS_PK_PARSE_C) || !defined(MBEDTLS_SHA256_C) || \
    !defined(MBEDTLS_ENTROPY_C) || !defined(MBEDTLS_CTR_DRBG_C) || \
    !defined(MBEDTLS_PEM_WRITE_C) || \
	!defined(MBEDTLS_X509_CRT_WRITE_C) || \
    !defined(MBEDTLS_X509_CRT_PARSE_C) || \
    !defined(MBEDTLS_ERROR_C)
int req_main(void)
{
    mbedtls_printf("MBEDTLS_X509_CSR_WRITE_C and/or MBEDTLS_FS_IO and/or "
                   "MBEDTLS_PK_PARSE_C and/or MBEDTLS_SHA256_C and/or "
                   "MBEDTLS_ENTROPY_C and/or MBEDTLS_CTR_DRBG_C "
				   "MBEDTLS_X509_CRT_WRITE_C and/or MBEDTLS_X509_CRT_PARSE_C and/or "
                   "MBEDTLS_ERROR_C "
                   "not defined.\n");
    mbedtls_exit(0);
}
#else

int write_certificate_request_buffer(mbedtls_x509write_csr *req, int format, char* output_buf,
                              size_t output_buf_size, size_t* len,
                              int (*f_rng)(void *, unsigned char *, size_t),
                              void *p_rng)
{
	int ret;
	
	memset(output_buf, 0, output_buf_size);
	if(format == FORMAT_PEM)
	{
		if ((ret = mbedtls_x509write_csr_pem(req, output_buf, output_buf_size, f_rng, p_rng)) < 0) {
			return ret;
		}

		*len = strlen((char *) output_buf);
	}
    else
	{
		if ((ret = mbedtls_x509write_csr_der(req, output_buf, output_buf_size, f_rng, p_rng)) < 0) {
            return ret;
        }

        *len = ret;
        //c = output_buf + sizeof(output_buf) - len;
	}
}

int write_certificate_request(mbedtls_x509write_csr *req, int format, const char *output_file,
                              int (*f_rng)(void *, unsigned char *, size_t),
                              void *p_rng)
{
    int ret;
    FILE *f;
    unsigned char output_buf[4096];
	unsigned char* c = output_buf;
    size_t len = 0;

    memset(output_buf, 0, 4096);
	
	if((ret = write_certificate_request_buffer(req, format, output_buf, sizeof(output_buf), &len, f_rng, p_rng)) < 0)
	{
		return ret;
	}
	
	if(format == FORMAT_DER)
	{
		c = output_buf + sizeof(output_buf) - len;
	}

    if ((f = fopen(output_file, "w")) == NULL) {
        return -1;
    }

    if (fwrite(c, 1, len, f) != len) {
        fclose(f);
        return -1;
    }

    fclose(f);

    return 0;
}

int req_main(int argc, char** argv, int argi)
{
    int ret = 1;
    int exit_code = MBEDTLS_EXIT_FAILURE;
    mbedtls_pk_context key;
    char buf[1024];
    int i;
    char *p, *q;
    mbedtls_x509write_csr req;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    const char* pers = "req";

	int reqtype = REQ_TYPE_CSR;
	int text = 0;
	char* outfile = NULL;
	int output_format = FORMAT_PEM;
	
	char* conffile = NULL;
	char* conffilein = NULL;
	char* mbedtls_env_conf = NULL;
#ifdef OPENSSL_ENV_CONF_COMPAT
	char* openssl_env_conf = NULL;
#endif
	
	char* subject_name = NULL;
	char* key_filein = NULL;
	char* key_passin = NULL;
	mbedtls_md_type_t md_alg;
	char* md_alg_name = NULL;
	char* md_alg_in = NULL;
	char* daysin = NULL;
	char* serialin = NULL;
	
	int newkey = 0;
	char* newkey_optsin = NULL;
	char* newkey_outfile = NULL;
	char* rsa_keysize = NULL;
	
	int noenc = 1;
	int newreq = 0;
	char* csr_infile = NULL;
	int noout = 0;
	
	conf_req_csr_parameters req_params;
	initialise_conf_req_csr_parameters(&req_params);
	
    /*
     * Set to sane values
     */
    mbedtls_x509write_csr_init(&req);
    mbedtls_pk_init(&key);
    mbedtls_ctr_drbg_init(&ctr_drbg);
    memset(buf, 0, sizeof(buf));
    mbedtls_entropy_init(&entropy);
	
#if defined(MBEDTLS_USE_PSA_CRYPTO)
    psa_status_t status = psa_crypto_init();
    if (status != PSA_SUCCESS) {
        mbedtlsclu_prio_printf(MBEDTLSCLU_ERR,"Failed to initialize PSA Crypto implementation: %d\n",
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
		else if(strcmp(p,"-batch") == 0 || strcmp(p,"-utf8") == 0 || strcmp(p,"-nodes") == 0 || strcmp(p,"-noenc") == 0)
		{
			// These parameters are silently ignored. They are included for compatibility with the openssl equivalent utility
			// -new is implied as we don't support taking an existing request and signing it
			// We don't support interactive mode so -batch is the default
			// We don't support -utf8
			// We don't support password encrypting so nodes/noenc are implied
		}
		else if(strcmp(p,"-new") == 0)
		{
			// Can't request a new file and input a file at the same time
			if(csr_infile != NULL)
			{
				goto usage;
			}
			newreq = 1;
		}
		else if(strcmp(p,"-in") == 0 && i + 1 < argc)
		{
			// Can't request a new file and input a file at the same time
			if(newreq)
			{
				goto usage;
			}
			// argv[i+1] should be the CSR file. Advance i
			i += 1;
			p = argv[i];
			csr_infile = strdup(p);
		}
		else if(strcmp(p,"-x509") == 0)
		{
			// We are doing a Cert not a Cert Req
			reqtype = REQ_TYPE_CRT;
		}
		else if(strcmp(p,"-days") == 0 && i + 1 < argc)
		{
			// argv[i+1] should be the number of days. Advance i
			i += 1;
			p = argv[i];
			daysin = strdup(p);
		}
		else if(strcmp(p,"-set_serial") == 0 && i + 1 < argc)
		{
			// argv[i+1] should be the serial number. Advance i
			i += 1;
			p = argv[i];
			serialin = strdup(p);
		}
		else if(strcmp(p,"-config") == 0 && i + 1 < argc)
		{
			// argv[i+1] should be the config path. Advance i
			i += 1;
			p = argv[i];
			conffilein = strdup(p);
		}
		else if(strcmp(p,"-subj") == 0 && i + 1 < argc)
		{
			// argv[i+1] should be the subject value. Advance i
			i += 1;
			p = argv[i];
			subject_name = strdup(p);
		}
		else if(strcmp(p,"-key") == 0 && i + 1 < argc)
		{
			// argv[i+1] should be the key file. Advance i
			i += 1;
			p = argv[i];
			key_filein = strdup(p);
			if(newkey)
			{
				// Can't supply a key and ask for a new one
				goto usage;
			}
		}
		else if(strcmp(p,"-newkey") == 0 && i + 1 < argc)
		{
			// argv[i+1] should be the keyopts (similar options to pkeyopt). Advance i
			i += 1;
			p = argv[i];
			newkey_optsin = strdup(p);
			newkey = 1;
			if(key_filein != NULL)
			{
				// Can't supply a key and ask for a new one
				goto usage;
			}
		}
		else if(strcmp(p,"-keyout") == 0 && i + 1 < argc)
		{
			// argv[i+1] should be the outfile. Advance i
			i += 1;
			p = argv[i];
			newkey_outfile = strdup(p);
		}
		else if(strcmp(p,"-passin") == 0 && i + 1 < argc)
		{
			// argv[i+1] should be the key password. Advance i
			i += 1;
			p = argv[i];
			key_passin = strdup(p);
		}
		else if(strcmp(p,"-text") == 0 )
		{
			text = 1;
		}
		else if(strcmp(p,"-out") == 0 && i + 1 < argc)
		{
			// argv[i+1] should be the filepath. Advance i
			i += 1;
			outfile = strdup(argv[i]);
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
		else if(strcmp(p,"-noout") == 0)
		{
			noout = 1;
		}
		else
		{
			// Check if this is a md algorithm
			// Advance past the "-" and give it a go
			if(md_alg_in == NULL)
			{
				char* md = strdup(p+1);
				const mbedtls_md_info_t* md_info;
				// Compatibility with openssl-util lowercase digests
				to_uppercase(md);
				
				md_info = mbedtls_md_info_from_string(md);
				if (md_info == NULL) {
					mbedtls_printf("Invalid digest provided: %s\n", p);
					goto usage;
				}
				md_alg_in = strdup(p+1);
			}
			else
			{
				// unknown param
				goto usage;
			}
		}
	}
	
	if(outfile == NULL && csr_infile == NULL)
	{
		goto usage;
	}
	
	mbedtlsclu_prio_printf(MBEDTLSCLU_DEBUG,"cl: out: %s\n", outfile);
	mbedtlsclu_prio_printf(MBEDTLSCLU_DEBUG,"cl: outform: %s\n", (output_format == FORMAT_PEM ? "PEM" : "DER"));
	mbedtlsclu_prio_printf(MBEDTLSCLU_DEBUG,"cl: text: %d\n", text);
	mbedtlsclu_prio_printf(MBEDTLSCLU_DEBUG,"cl: digest: %s\n", md_alg_in);
	mbedtlsclu_prio_printf(MBEDTLSCLU_DEBUG,"cl: key: %s\n", key_filein);
	mbedtlsclu_prio_printf(MBEDTLSCLU_DEBUG,"cl: key_passin: %s\n", key_passin);
	mbedtlsclu_prio_printf(MBEDTLSCLU_DEBUG,"cl: subj: %s\n", subject_name);
	mbedtlsclu_prio_printf(MBEDTLSCLU_DEBUG,"cl: newkey: %d\n", newkey);
	mbedtlsclu_prio_printf(MBEDTLSCLU_DEBUG,"cl: newkey_opts: %s\n", newkey_optsin);
	mbedtlsclu_prio_printf(MBEDTLSCLU_DEBUG,"cl: newkey_outfile: %s\n", newkey_outfile);
	mbedtlsclu_prio_printf(MBEDTLSCLU_DEBUG,"cl: conffile: %s\n", conffilein);
	
	// Check if the ENV has a config file defined
	mbedtls_env_conf = getenv(MBEDTLS_ENV_CONF);
	mbedtlsclu_prio_printf(MBEDTLSCLU_DEBUG,"env: mbedtls_conf: %s\n", mbedtls_env_conf);
#ifdef OPENSSL_ENV_CONF_COMPAT
	openssl_env_conf = getenv(OPENSSL_ENV_CONF);
	mbedtlsclu_prio_printf(MBEDTLSCLU_DEBUG,"env: openssl_conf: %s\n", openssl_env_conf);
#endif
	if(mbedtls_env_conf != NULL)
	{
		conffile = mbedtls_env_conf;
	}
#ifdef OPENSSL_ENV_CONF_COMPAT
	else if(openssl_env_conf != NULL)
	{
		conffile = openssl_env_conf;
	}
#endif
	else
	{
		conffile = conffilein;
	}
	
	mbedtlsclu_prio_printf(MBEDTLSCLU_DEBUG,"Final conffile: %s\n", conffile);
	
	mbedtlsclu_prio_printf(MBEDTLSCLU_INFO,"  . Parsing the config file...");
	if((ret = parse_config_file(conffile, REQ_TYPE_CSR, (void*)(&req_params))) != 0)
	{
		mbedtlsclu_prio_printf(MBEDTLSCLU_ERR," failed\n  !  parse_config_file returned %d", ret);
        goto exit;
	}
	mbedtlsclu_prio_printf(MBEDTLSCLU_DEBUG,"conf: default_bits %s\n", req_params.default_bits);
	mbedtlsclu_prio_printf(MBEDTLSCLU_DEBUG,"conf: default_keyfile %s\n", req_params.default_keyfile);
	mbedtlsclu_prio_printf(MBEDTLSCLU_DEBUG,"conf: default_md %s\n", req_params.default_md);
	mbedtlsclu_prio_printf(MBEDTLSCLU_DEBUG,"conf: distinguished_name %s\n", req_params.distinguished_name_tag);
	mbedtlsclu_prio_printf(MBEDTLSCLU_DEBUG,"conf: x509_extensions %s\n", req_params.x509_extensions_tag);
	mbedtlsclu_prio_printf(MBEDTLSCLU_DEBUG,"conf: default_country %s\n", req_params.default_country);
	mbedtlsclu_prio_printf(MBEDTLSCLU_DEBUG,"conf: default_state %s\n", req_params.default_state);
	mbedtlsclu_prio_printf(MBEDTLSCLU_DEBUG,"conf: default_locality %s\n", req_params.default_locality);
	mbedtlsclu_prio_printf(MBEDTLSCLU_DEBUG,"conf: default_org %s\n", req_params.default_org);
	mbedtlsclu_prio_printf(MBEDTLSCLU_DEBUG,"conf: default_orgunit %s\n", req_params.default_orgunit);
	mbedtlsclu_prio_printf(MBEDTLSCLU_DEBUG,"conf: default_commonname %s\n", req_params.default_commonname);
	mbedtlsclu_prio_printf(MBEDTLSCLU_DEBUG,"conf: default_email %s\n", req_params.default_email);
	mbedtlsclu_prio_printf(MBEDTLSCLU_DEBUG,"conf: default_serial %s\n", req_params.default_serial);
	mbedtlsclu_prio_printf(MBEDTLSCLU_DEBUG,"conf: subject_key_identifier %s\n", req_params.subject_key_identifier);
	mbedtlsclu_prio_printf(MBEDTLSCLU_DEBUG,"conf: authority_key_identifier %s\n", req_params.authority_key_identifier);
	mbedtlsclu_prio_printf(MBEDTLSCLU_DEBUG,"conf: basic_contraints %s\n", req_params.basic_contraints);
	mbedtlsclu_prio_printf(MBEDTLSCLU_DEBUG,"conf: key_usage %s\n", req_params.key_usage);
	mbedtlsclu_prio_printf(MBEDTLSCLU_DEBUG,"conf: ns_cert_type %s\n", req_params.ns_cert_type);
	
	// Set the keyfile
	if(key_filein == NULL && req_params.default_keyfile == NULL)
	{
		goto usage;
	}
	else if(key_filein == NULL && req_params.default_keyfile != NULL)
	{
		// Config file had a value, use it
		key_filein = req_params.default_keyfile;
	}
	
	// Set the md
	if(md_alg_in == NULL && req_params.default_md == NULL)
	{
		goto usage;
	}
	else if(md_alg_in == NULL && req_params.default_md != NULL)
	{
		// Config file had a value, use it
		md_alg_in = req_params.default_md;
	}
	// Now check md is valid
	if(md_alg_in != NULL)
	{
		char* md = strdup(md_alg_in);
		const mbedtls_md_info_t* md_info;
		// Compatibility with openssl-util lowercase digests
		to_uppercase(md);
		
		md_info = mbedtls_md_info_from_string(md);
		if (md_info == NULL) {
			mbedtls_printf("Invalid digest provided: %s\n", p);
			goto usage;
		}
		md_alg = mbedtls_md_get_type(md_info);
		md_alg_name = mbedtls_md_get_name(md_info);
	}
	else
	{
		goto usage;
	}
	
	// Set subject_name
	if(subject_name == NULL && req_params.default_commonname == NULL)
	{
		goto usage;
	}
	else if(subject_name == NULL && req_params.default_commonname != NULL)
	{
		// Config file had some values, let's string them together
		char* tmpstr = NULL;
		subject_name = dynamic_strcat(2,"CN=",req_params.default_commonname);
		if(req_params.default_email != NULL)
		{
			tmpstr = dynamic_strcat(2,",emailAddress=",req_params.default_email);
			subject_name = dcat_and_free(&subject_name,&tmpstr,1,1);
		}
		if(req_params.default_org != NULL)
		{
			tmpstr = dynamic_strcat(2,",O=",req_params.default_org);
			subject_name = dcat_and_free(&subject_name,&tmpstr,1,1);
		}
		if(req_params.default_orgunit != NULL)
		{
			tmpstr = dynamic_strcat(2,",OU=",req_params.default_orgunit);
			subject_name = dcat_and_free(&subject_name,&tmpstr,1,1);
		}
		if(req_params.default_locality != NULL)
		{
			tmpstr = dynamic_strcat(2,",L=",req_params.default_locality);
			subject_name = dcat_and_free(&subject_name,&tmpstr,1,1);
		}
		if(req_params.default_state != NULL)
		{
			tmpstr = dynamic_strcat(2,",ST=",req_params.default_state);
			subject_name = dcat_and_free(&subject_name,&tmpstr,1,1);
		}
		if(req_params.default_country != NULL)
		{
			tmpstr = dynamic_strcat(2,",C=",req_params.default_country);
			subject_name = dcat_and_free(&subject_name,&tmpstr,1,1);
		}
	}
	
	if(csr_infile != NULL)
	{
		// Just verify the CSR
		char csrbuf[10000];
		mbedtls_x509_csr csr;
		mbedtls_x509_csr_init(&csr);
		
		/*
		 * 1.1. Load the CSR
		 */
		mbedtlsclu_prio_printf(MBEDTLSCLU_INFO,"\n  . Loading the CSR ...");
		fflush(stdout);

		ret = mbedtls_x509_csr_parse_file(&csr, csr_infile);

		if (ret != 0) {
			mbedtlsclu_prio_printf(MBEDTLSCLU_ERR," failed\n  !  mbedtls_x509_csr_parse_file returned %d\n\n", ret);
			mbedtls_x509_csr_free(&csr);
			goto exit;
		}

		mbedtlsclu_prio_printf(MBEDTLSCLU_INFO," ok\n");
		
		if(!noout)
		{
			/*
			 * 1.2 Print the CSR
			 */
			mbedtlsclu_prio_printf(MBEDTLSCLU_INFO,"  . CSR information    ...\n");
			ret = mbedtls_x509_csr_info((char *) csrbuf, sizeof(csrbuf) - 1, "      ", &csr);
			if (ret == -1) {
				mbedtlsclu_prio_printf(MBEDTLSCLU_ERR," failed\n  !  mbedtls_x509_csr_info returned %d\n\n", ret);
				mbedtls_x509_csr_free(&csr);
				goto exit;
			}

			mbedtls_printf("%s\n", csrbuf);
		}
		mbedtls_x509_csr_free(&csr);
		
		exit_code = MBEDTLS_EXIT_SUCCESS;
		goto exit;
	}

	// Generate a new key if requested
	if(newkey)
	{
		if(noout)
		{
			mbedtlsclu_prio_printf(MBEDTLSCLU_ERR,"Can't ask for a new key AND noout\n");
			goto usage;
		}
		char* algorithm = NULL;
		
		mbedtlsclu_prio_printf(MBEDTLSCLU_INFO,"New Key Generation requested\n");
		
		if(newkey_optsin == NULL || newkey_outfile == NULL)
		{
			goto usage;
		}
		
		p = newkey_optsin;
		if((q = strchr(p, ':')) == NULL)
		{
			goto usage;
		}
		*q++ = '\0';
		
		if(strcmp(p,"rsa") == 0)
		{
			int tmpkeysize;
			//rsa:bits
			algorithm = strdup("rsa");
			tmpkeysize = atoi(q);
			if(tmpkeysize < 1024 || tmpkeysize > MBEDTLS_MPI_MAX_BITS)
			{
				goto usage;
			}
			rsa_keysize = strdup(q);
		}
		else
		{
			//ec:paramfile we will support later (probably)
			goto usage;
		}
		
		char** genpkey_args = (char**)malloc(7*sizeof(char*));
		genpkey_args[0] = strdup("genpkey");
		genpkey_args[1] = strdup("-algorithm");
		genpkey_args[2] = strdup(algorithm);
		genpkey_args[3] = strdup("-pkeyopt");
		genpkey_args[4] = dynamic_strcat(2,"rsa_keygen_bits:",rsa_keysize);
		genpkey_args[5] = strdup("-out");
		genpkey_args[6] = strdup(newkey_outfile);
		mbedtlsclu_prio_printf(MBEDTLSCLU_DEBUG,"Calling genpkey internally\n");
		genpkey_main(7,genpkey_args,1);
		
		free(algorithm);
		free(rsa_keysize);
		
		key_filein = newkey_outfile;
	}
	
	mbedtlsclu_prio_printf(MBEDTLSCLU_DEBUG,"Final subject name: %s\n", subject_name);
	mbedtlsclu_prio_printf(MBEDTLSCLU_DEBUG,"Final digest: %s\n", md_alg_name);
	mbedtlsclu_prio_printf(MBEDTLSCLU_DEBUG,"Final keyfile: %s\n", key_filein);
	
	mbedtls_x509write_csr_set_md_alg(&req, md_alg);

    /*if (opt.key_usage || opt.force_key_usage == 1) {
        mbedtls_x509write_csr_set_key_usage(&req, opt.key_usage);
    }

    if (opt.ns_cert_type || opt.force_ns_cert_type == 1) {
        mbedtls_x509write_csr_set_ns_cert_type(&req, opt.ns_cert_type);
    }*/

    /*
     * 0. Seed the PRNG
     */
    mbedtlsclu_prio_printf(MBEDTLSCLU_INFO,"  . Seeding the random number generator...");
    fflush(stdout);

    if ((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
                                     (const unsigned char *) pers,
                                     strlen(pers))) != 0) {
        mbedtlsclu_prio_printf(MBEDTLSCLU_ERR," failed\n  !  mbedtls_ctr_drbg_seed returned %d", ret);
        goto exit;
    }

    mbedtlsclu_prio_printf(MBEDTLSCLU_INFO," ok\n");

	/*
     * 1.0. Check the subject name for validity
     */
    mbedtlsclu_prio_printf(MBEDTLSCLU_INFO,"  . Checking subject name...");
    fflush(stdout);

    if ((ret = mbedtls_x509write_csr_set_subject_name(&req, subject_name)) != 0) {
        mbedtlsclu_prio_printf(MBEDTLSCLU_ERR," failed\n  !  mbedtls_x509write_csr_set_subject_name returned %d", ret);
        goto exit;
    }

    mbedtlsclu_prio_printf(MBEDTLSCLU_INFO," ok\n");

    /*
     * 1.1. Load the key
     */
    mbedtlsclu_prio_printf(MBEDTLSCLU_INFO,"  . Loading the private key ...");
    fflush(stdout);

    ret = mbedtls_pk_parse_keyfile(&key, key_filein, key_passin);

    if (ret != 0) {
        mbedtlsclu_prio_printf(MBEDTLSCLU_ERR," failed\n  !  mbedtls_pk_parse_keyfile returned %d", ret);
        goto exit;
    }

    mbedtls_x509write_csr_set_key(&req, &key);

    mbedtlsclu_prio_printf(MBEDTLSCLU_INFO," ok\n");

	if(!noout)
	{
		/*
		 * 1.2. Writing the request
		 */
		mbedtlsclu_prio_printf(MBEDTLSCLU_INFO,"  . Writing the certificate request ...");
		fflush(stdout);

		if ((ret = write_certificate_request(&req, output_format, outfile,
											 mbedtls_ctr_drbg_random, &ctr_drbg)) != 0) {
			mbedtlsclu_prio_printf(MBEDTLSCLU_ERR," failed\n  !  write_certificate_request %d", ret);
			goto exit;
		}

		mbedtlsclu_prio_printf(MBEDTLSCLU_INFO," ok\n");
	}
	
	/*
     * 2.0. Generate a certificate
     */
    if(reqtype == REQ_TYPE_CRT)
	{
		ret = 1;
		exit_code = MBEDTLS_EXIT_FAILURE;
		mbedtls_x509_crt issuer_crt;
		mbedtls_pk_context loaded_issuer_key, loaded_subject_key;
		mbedtls_pk_context *issuer_key = &loaded_issuer_key,
						   *subject_key = &loaded_subject_key;
		char issuer_name[256];
#if defined(MBEDTLS_X509_CSR_PARSE_C)
		mbedtls_x509_csr csr;
#endif
		mbedtls_x509write_cert crt;
		mbedtls_mpi serial;
		char* serialval = NULL;
		int version = DFL_VERSION;
		int days = 0;
		char time_notbefore[256];
		char time_notafter[256];
		
		/*
		 * Set to sane values
		 */
		mbedtls_x509write_crt_init(&crt);
		mbedtls_pk_init(&loaded_issuer_key);
		mbedtls_pk_init(&loaded_subject_key);
#if defined(MBEDTLS_X509_CSR_PARSE_C)
		mbedtls_x509_csr_init(&csr);
#endif
		mbedtls_x509_crt_init(&issuer_crt);
		mbedtls_mpi_init(&serial);
		
		mbedtlsclu_prio_printf(MBEDTLSCLU_INFO,"\n  . Starting certificate writing process ...");
		
		// Parse serial to MPI
		// Can come from either command line input or conf input
		mbedtlsclu_prio_printf(MBEDTLSCLU_INFO,"  . Reading serial number...");
		fflush(stdout);
		
		if(serialin != NULL)
		{
			// CLI input
			serialval = serialin;
		}
		/*else if(req_params.default_serial != NULL)
		{
			// Config input -- maybe we don't want this in x509 mode?
			serialval = req_params.default_serial;
		}*/
		else
		{
			//serialval = strdup(DFL_SERIAL);
			mbedtls_mpi_fill_random(&serial, 20, mbedtls_ctr_drbg_random, &ctr_drbg);
			//mbedtls_mpi_write_file("serial: ", &serial, 16, NULL);
		}
		
		if(serialval != NULL)
		{
			if ((ret = mbedtls_mpi_read_string(&serial, 10, serialval)) != 0) {
				mbedtls_strerror(ret, buf, 1024);
				mbedtlsclu_prio_printf(MBEDTLSCLU_ERR," failed\n  !  mbedtls_mpi_read_string "
							   "returned -0x%04x - %s\n\n", (unsigned int) -ret, buf);
				goto exit;
			}
		}
		
		mbedtlsclu_prio_printf(MBEDTLSCLU_INFO," ok\n");
		
#if defined(MBEDTLS_X509_CSR_PARSE_C)
		// Use the certificate request we just generated
		mbedtlsclu_prio_printf(MBEDTLSCLU_INFO,"  . Parsing certificate request...");
		
		unsigned char output_buf[4096];
		unsigned char* c = output_buf;
		size_t len = 0;

		memset(output_buf, 0, 4096);
		
		if((ret = write_certificate_request_buffer(&req, output_format, output_buf, sizeof(output_buf), &len, mbedtls_ctr_drbg_random, &ctr_drbg)) < 0)
		{
			mbedtlsclu_prio_printf(MBEDTLSCLU_ERR," failed\n  !  write_certificate_request_buffer returned %d\n", ret);
			goto exit;
		}
		if(output_format == FORMAT_DER)
		{
			c = output_buf + sizeof(output_buf) - len;
		}
		
		mbedtlsclu_prio_printf(MBEDTLSCLU_DEBUG,"output_buf: %s\n", c);
		if((ret = mbedtls_x509_csr_parse(&csr, c, len+1)) != 0)
		{
			mbedtlsclu_prio_printf(MBEDTLSCLU_ERR," failed \n  !  mbedtls_x509_csr_parser returned %d\n", ret);
			goto exit;
		}

		free(subject_name);
		subject_name = (char*)malloc(MBEDTLS_X509_MAX_DN_NAME_SIZE*sizeof(char));
		ret = mbedtls_x509_dn_gets(subject_name, MBEDTLS_X509_MAX_DN_NAME_SIZE,
								   &csr.subject);
		if (ret < 0) {
			mbedtls_strerror(ret, buf, sizeof(buf));
			mbedtlsclu_prio_printf(MBEDTLSCLU_ERR," failed\n  !  mbedtls_x509_dn_gets "
						   "returned -0x%04x - %s\n\n", (unsigned int) -ret, buf);
			goto exit;
		}
		mbedtlsclu_prio_printf(MBEDTLSCLU_INFO," ok\n");
		mbedtlsclu_prio_printf(MBEDTLSCLU_DEBUG,"subject_name: %s\n", subject_name);
		subject_key = &csr.pk;
		
		issuer_key = &key;
		memset(issuer_name, 0, sizeof(issuer_name));
		memcpy(issuer_name, subject_name, strlen(subject_name));
		
		mbedtls_x509write_crt_set_subject_key(&crt, subject_key);
		mbedtls_x509write_crt_set_issuer_key(&crt, issuer_key);
		
		/*
		 * 1.0. Check the names for validity
		 */
		if ((ret = mbedtls_x509write_crt_set_subject_name(&crt, subject_name)) != 0) {
			mbedtls_strerror(ret, buf, sizeof(buf));
			mbedtlsclu_prio_printf(MBEDTLSCLU_ERR," failed\n  !  mbedtls_x509write_crt_set_subject_name "
						   "returned -0x%04x - %s\n\n", (unsigned int) -ret, buf);
			goto exit;
		}

		if ((ret = mbedtls_x509write_crt_set_issuer_name(&crt, issuer_name)) != 0) {
			mbedtls_strerror(ret, buf, sizeof(buf));
			mbedtlsclu_prio_printf(MBEDTLSCLU_ERR," failed\n  !  mbedtls_x509write_crt_set_issuer_name "
						   "returned -0x%04x - %s\n\n", (unsigned int) -ret, buf);
			goto exit;
		}

		mbedtlsclu_prio_printf(MBEDTLSCLU_INFO,"  . Setting certificate values ...");
		fflush(stdout);

		mbedtls_x509write_crt_set_version(&crt, version);
		mbedtls_x509write_crt_set_md_alg(&crt, md_alg);
		
		ret = mbedtls_x509write_crt_set_serial(&crt, &serial);
		if (ret != 0) {
			mbedtls_strerror(ret, buf, sizeof(buf));
			mbedtlsclu_prio_printf(MBEDTLSCLU_ERR," failed\n  !  mbedtls_x509write_crt_set_serial "
						   "returned -0x%04x - %s\n\n", (unsigned int) -ret, buf);
			goto exit;
		}

		// Create validity
		if(daysin != NULL)
		{
			// CLI input
			days = atoi(daysin);
		}
		else
		{
			// Set a default value
			days = DFL_DAYS;
		}
		mbedtlsclu_prio_printf(MBEDTLSCLU_DEBUG,"days: %d\n",days);
		if(days <= 0)
		{
			mbedtlsclu_prio_printf(MBEDTLSCLU_ERR,"Days cannot be <= 0\n");
			goto usage;
		}
		else
		{
			// Calculate not_before and not_after
			struct tm today, future;
			const time_t ONEDAY = 24 * 60 * 60;
			time_t timenow = time(NULL);
			today = *gmtime(&timenow);
			timenow += (days * ONEDAY);
			future = *gmtime(&timenow);
			sprintf(time_notbefore, "%04d%02d%02d%02d%02d%02d", today.tm_year + 1900, today.tm_mon + 1, today.tm_mday,
					today.tm_hour, today.tm_min, today.tm_sec);
			
			sprintf(time_notafter, "%04d%02d%02d%02d%02d%02d", future.tm_year + 1900, future.tm_mon + 1, future.tm_mday,
					future.tm_hour, future.tm_min, future.tm_sec);
		}
		mbedtlsclu_prio_printf(MBEDTLSCLU_DEBUG,"notbefore: %s, notafter: %s\n",time_notbefore, time_notafter);
		ret = mbedtls_x509write_crt_set_validity(&crt, time_notbefore, time_notafter);
		if (ret != 0) {
			mbedtls_strerror(ret, buf, sizeof(buf));
			mbedtlsclu_prio_printf(MBEDTLSCLU_ERR," failed\n  !  mbedtls_x509write_crt_set_validity "
						   "returned -0x%04x - %s\n\n", (unsigned int) -ret, buf);
			goto exit;
		}

		mbedtlsclu_prio_printf(MBEDTLSCLU_INFO," ok\n");
		
		if(version == MBEDTLS_X509_CRT_VERSION_3)
		{
			char key_usage = 0;
			char ns_cert_type = 0;
			if(req_params.basic_contraints != NULL)
			{
				// Config input
				mbedtlsclu_prio_printf(MBEDTLSCLU_INFO,"  . Adding the Basic Constraints extension ...");
				fflush(stdout);
				
				int is_ca = 1;
				int max_pathlen = -1;
				char* line = req_params.basic_contraints;
				unsigned long num_line_pieces;
				char* separators = ",";
				char** line_pieces = split_on_separators(line, separators, 1, -1, 0, &num_line_pieces);
				
				for(int i = 0; i < num_line_pieces; i++)
				{
					char* line_piece = trim_flanking_whitespace(line_pieces[i]);
					if(strstr(line_piece, "CA:") != NULL)
					{
						// Found CA constraint
						char* tmp = strdup(line_piece+3);
						to_lowercase(tmp);
						if(strcmp(tmp,"true") == 0)
						{
							is_ca = 1;
						}
						else
						{
							is_ca = 0;
						}
						free(tmp);
					}
					else if(strstr(line_piece, "pathlen:") != NULL)
					{
						// Found path length constraint
						char* tmp = strdup(line_piece+8);
						max_pathlen = atoi(tmp);
						free(tmp);
					}
				}
				
				free_null_terminated_string_array(line_pieces);
				
				ret = mbedtls_x509write_crt_set_basic_constraints(&crt, is_ca, max_pathlen);
				if (ret != 0) {
					mbedtls_strerror(ret, buf, 1024);
					mbedtlsclu_prio_printf(MBEDTLSCLU_ERR," failed\n  !  x509write_crt_set_basic_constraints "
								   "returned -0x%04x - %s\n\n", (unsigned int) -ret, buf);
					goto exit;
				}

				mbedtlsclu_prio_printf(MBEDTLSCLU_INFO," ok\n");
			}
			
#if defined(MBEDTLS_SHA1_C)
			if(req_params.subject_key_identifier != NULL)
			{
				int setExt = 1; // for req, ca, x509 this should be the default anyway
				char* line = req_params.subject_key_identifier;
				unsigned long num_line_pieces;
				char* separators = ",";
				char** line_pieces = split_on_separators(line, separators, 1, -1, 0, &num_line_pieces);
				
				for(int i = 0; i < num_line_pieces; i++)
				{
					char* line_piece = trim_flanking_whitespace(line_pieces[i]);
					to_lowercase(line_piece);
					if(strcmp(line_piece, "hash") == 0)
					{
						setExt = 1;
					}
					else if(strcmp(line_piece, "none") == 0)
					{
						setExt = 0;
					}
				}
				
				free_null_terminated_string_array(line_pieces);
				
				if(setExt)
				{
					mbedtlsclu_prio_printf(MBEDTLSCLU_INFO,"  . Adding the Subject Key Identifier ...");
					fflush(stdout);

					ret = mbedtls_x509write_crt_set_subject_key_identifier(&crt);
					if (ret != 0) {
						mbedtls_strerror(ret, buf, 1024);
						mbedtlsclu_prio_printf(MBEDTLSCLU_ERR," failed\n  !  mbedtls_x509write_crt_set_subject"
									   "_key_identifier returned -0x%04x - %s\n\n",
									   (unsigned int) -ret, buf);
						goto exit;
					}

					mbedtlsclu_prio_printf(MBEDTLSCLU_INFO," ok\n");
				}
			}
			
			if(req_params.authority_key_identifier != NULL)
			{
				int setExt = 0; // for req, ca, x509 this should be the default for self-signed
				char* line = req_params.authority_key_identifier;
				unsigned long num_line_pieces;
				char* separators = ",";
				char** line_pieces = split_on_separators(line, separators, 1, -1, 0, &num_line_pieces);
				
				for(int i = 0; i < num_line_pieces; i++)
				{
					char* line_piece = trim_flanking_whitespace(line_pieces[i]);
					to_lowercase(line_piece);
					if(strstr(line_piece, "keyid") != NULL)
					{
						setExt = 1;
					}
					else if(strstr(line_piece, "issuer") != NULL)
					{
						// Note we don't write out the issuer name/serial currently, so this is silently ignored
						setExt = 1;
					}
					else if(strstr(line_piece, "none") != NULL)
					{
						setExt = 0;
					}
				}
				
				free_null_terminated_string_array(line_pieces);
				
				if(setExt)
				{
					mbedtlsclu_prio_printf(MBEDTLSCLU_INFO,"  . Adding the Authority Key Identifier ...");
					fflush(stdout);

					ret = mbedtls_x509write_crt_set_authority_key_identifier(&crt);
					if (ret != 0) {
						mbedtls_strerror(ret, buf, 1024);
						mbedtlsclu_prio_printf(MBEDTLSCLU_ERR," failed\n  !  mbedtls_x509write_crt_set_authority_"
									   "key_identifier returned -0x%04x - %s\n\n",
									   (unsigned int) -ret, buf);
						goto exit;
					}

					mbedtlsclu_prio_printf(MBEDTLSCLU_INFO," ok\n");
				}
			}
			
#endif
			if(req_params.key_usage != NULL)
			{
				char* line = req_params.key_usage;
				unsigned long num_line_pieces;
				char* separators = ",";
				char** line_pieces = split_on_separators(line, separators, 1, -1, 0, &num_line_pieces);
				
				for(int i = 0; i < num_line_pieces; i++)
				{
					char* line_piece = trim_flanking_whitespace(line_pieces[i]);
					to_lowercase(line_piece);
					if(strcmp(line_piece, "digitalsignature") == 0)
					{
						key_usage |= MBEDTLS_X509_KU_DIGITAL_SIGNATURE;
					}
					else if(strcmp(line_piece, "nonrepudiation") == 0)
					{
						key_usage |= MBEDTLS_X509_KU_NON_REPUDIATION;
					}
					else if(strcmp(line_piece, "keyencipherment") == 0)
					{
						key_usage |= MBEDTLS_X509_KU_KEY_ENCIPHERMENT;
					}
					else if(strcmp(line_piece, "dataencipherment") == 0)
					{
						key_usage |= MBEDTLS_X509_KU_DATA_ENCIPHERMENT;
					}
					else if(strcmp(line_piece, "keyagreement") == 0)
					{
						key_usage |= MBEDTLS_X509_KU_KEY_AGREEMENT;
					}
					else if(strcmp(line_piece, "keycertsign") == 0)
					{
						key_usage |= MBEDTLS_X509_KU_KEY_CERT_SIGN;
					}
					else if(strcmp(line_piece, "crlsign") == 0)
					{
						key_usage |= MBEDTLS_X509_KU_CRL_SIGN;
					}
				}
				
				free_null_terminated_string_array(line_pieces);
				
				mbedtlsclu_prio_printf(MBEDTLSCLU_INFO,"  . Adding the Key Usage extension ...");
				fflush(stdout);

				ret = mbedtls_x509write_crt_set_key_usage(&crt, key_usage);
				if (ret != 0) {
					mbedtls_strerror(ret, buf, 1024);
					mbedtlsclu_prio_printf(MBEDTLSCLU_ERR," failed\n  !  mbedtls_x509write_crt_set_key_usage "
								   "returned -0x%04x - %s\n\n", (unsigned int) -ret, buf);
					goto exit;
				}

				mbedtlsclu_prio_printf(MBEDTLSCLU_INFO," ok\n");
			}
			
			if(req_params.ns_cert_type != NULL)
			{
				char* line = req_params.ns_cert_type;
				unsigned long num_line_pieces;
				char* separators = ",";
				char** line_pieces = split_on_separators(line, separators, 1, -1, 0, &num_line_pieces);
				
				for(int i = 0; i < num_line_pieces; i++)
				{
					char* line_piece = trim_flanking_whitespace(line_pieces[i]);
					to_lowercase(line_piece);
					if(strcmp(line_piece, "client") == 0)
					{
						ns_cert_type |= MBEDTLS_X509_NS_CERT_TYPE_SSL_CLIENT;
					}
					else if(strcmp(line_piece, "server") == 0)
					{
						ns_cert_type |= MBEDTLS_X509_NS_CERT_TYPE_SSL_SERVER;
					}
					else if(strcmp(line_piece, "email") == 0)
					{
						ns_cert_type |= MBEDTLS_X509_NS_CERT_TYPE_EMAIL;
					}
					else if(strcmp(line_piece, "objsign") == 0)
					{
						ns_cert_type |= MBEDTLS_X509_NS_CERT_TYPE_OBJECT_SIGNING;
					}
					else if(strcmp(line_piece, "sslca") == 0)
					{
						ns_cert_type |= MBEDTLS_X509_NS_CERT_TYPE_SSL_CA;
					}
					else if(strcmp(line_piece, "emailca") == 0)
					{
						ns_cert_type |= MBEDTLS_X509_NS_CERT_TYPE_EMAIL_CA;
					}
					else if(strcmp(line_piece, "objca") == 0)
					{
						ns_cert_type |= MBEDTLS_X509_NS_CERT_TYPE_OBJECT_SIGNING_CA;
					}
				}
				
				free_null_terminated_string_array(line_pieces);
				
				mbedtlsclu_prio_printf(MBEDTLSCLU_INFO,"  . Adding the NS Cert Type extension ...");
				fflush(stdout);

				ret = mbedtls_x509write_crt_set_ns_cert_type(&crt, ns_cert_type);
				if (ret != 0) {
					mbedtls_strerror(ret, buf, 1024);
					mbedtlsclu_prio_printf(MBEDTLSCLU_ERR," failed\n  !  mbedtls_x509write_crt_set_ns_cert_type "
								   "returned -0x%04x - %s\n\n", (unsigned int) -ret, buf);
					goto exit;
				}

				mbedtlsclu_prio_printf(MBEDTLSCLU_INFO," ok\n");
			}
		}
		
		if(!noout)
		{
			/*
			 * 1.2. Writing the certificate
			 */
			mbedtlsclu_prio_printf(MBEDTLSCLU_INFO,"  . Writing the certificate...");
			fflush(stdout);

			if ((ret = write_certificate(&crt, outfile,
										 mbedtls_ctr_drbg_random, &ctr_drbg)) != 0) {
				mbedtls_strerror(ret, buf, 1024);
				mbedtlsclu_prio_printf(MBEDTLSCLU_ERR," failed\n  !  write_certificate -0x%04x - %s\n\n",
							   (unsigned int) -ret, buf);
				goto exit;
			}

			mbedtlsclu_prio_printf(MBEDTLSCLU_INFO," ok\n");
		}


#endif /* MBEDTLS_X509_CSR_PARSE_C */
	}
	

    exit_code = MBEDTLS_EXIT_SUCCESS;

exit:

    mbedtls_x509write_csr_free(&req);
    mbedtls_pk_free(&key);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);
#if defined(MBEDTLS_USE_PSA_CRYPTO)
    mbedtls_psa_crypto_free();
#endif /* MBEDTLS_USE_PSA_CRYPTO */

    return exit_code;
}
#endif /* MBEDTLS_X509_CSR_WRITE_C && MBEDTLS_PK_PARSE_C && MBEDTLS_FS_IO &&
          MBEDTLS_ENTROPY_C && MBEDTLS_CTR_DRBG_C && MBEDTLS_PEM_WRITE_C */
