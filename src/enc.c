/* enc -	Symmetric file encryption/decryption
 *				This utility attempts to be syntax and file-format compatible
 *				with the equivalent openssl utlility: openssl enc -aes-256-cbc
 *				-salt -in /path/to/file -out /path/to/file.enc -k password
 * 			Originally created for the Gargoyle Web Interface
 *
 * 			Created By Michael Gray
 * 			http://www.lantisproject.com
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

#include "enc.h"

#define SALT_MAGIC		"Salted__"
#define SALT_MAGIC_LEN	8
#define SALT_LEN		8
#define CHUNK_SIZE		8192
#define MAX_KEY_LEN		32
#define MAX_IV_LEN		16
#define MAX_BLOCK_LEN	16

#define USAGE \
    "\n usage: enc [options]\n"																\
    "\n\n General options:\n"																	\
    "    -help					Display this summary\n"											\
	"    -e						Encrypt (default if neither -e nor -d given)\n"				\
	"    -d						Decrypt\n"														\
	"    -p						Print the derived key/IV\n"									\
	"    -P						Print the derived key/IV and exit (no en/decryption)\n"		\
	"\n\n Input options:\n"																	\
	"    -in infile				Input file (default: stdin)\n"								\
	"    -k val					Passphrase\n"													\
	"    -kfile infile			Read passphrase from a file\n"								\
	"    -pass val				Passphrase source (pass:value or file:path)\n"				\
	"\n\n Output options:\n"																	\
	"    -out outfile			Output file (default: stdout)\n"								\
	"    -a						Base64 encode/decode, depending on -e/-d\n"					\
	"    -base64				Same as -a\n"													\
	"    -A						Used with -a to write base64 as a single line\n"				\
	"\n\n Encryption options:\n"																\
	"    -salt					Use a salt in the KDF (default)\n"							\
	"    -nosalt				Do not use a salt in the KDF\n"								\
	"    -nopad					Disable standard block padding\n"							\
	"    -K val					Raw key, in hex (bypasses the KDF)\n"							\
	"    -S val					Salt, in hex\n"												\
	"    -iv val					IV, in hex\n"													\
	"    -md val					Digest to use for the KDF (default: sha256)\n"				\
	"    -pbkdf2				Use PBKDF2 for the KDF\n"										\
	"    -iter +int				Iteration count (implies -pbkdf2); default: 10000\n"			\
	"    -aes-128-cbc / -aes-192-cbc / -aes-256-cbc\n"											\
	"    -aes-128-cfb / -aes-192-cfb / -aes-256-cfb\n"											\
	"    -aes-128-ofb / -aes-192-ofb / -aes-256-ofb\n"											\
	"    -aes-128-ctr / -aes-192-ctr / -aes-256-ctr\n"											\
	"								Cipher to use (one required)\n"

typedef struct {
	const char* flag;
	mbedtls_cipher_type_t type;
} enc_cipher_entry;

static const enc_cipher_entry enc_ciphers[] = {
	{ "-aes-128-cbc", MBEDTLS_CIPHER_AES_128_CBC },
	{ "-aes-192-cbc", MBEDTLS_CIPHER_AES_192_CBC },
	{ "-aes-256-cbc", MBEDTLS_CIPHER_AES_256_CBC },
	{ "-aes-128-cfb", MBEDTLS_CIPHER_AES_128_CFB128 },
	{ "-aes-192-cfb", MBEDTLS_CIPHER_AES_192_CFB128 },
	{ "-aes-256-cfb", MBEDTLS_CIPHER_AES_256_CFB128 },
	{ "-aes-128-ofb", MBEDTLS_CIPHER_AES_128_OFB },
	{ "-aes-192-ofb", MBEDTLS_CIPHER_AES_192_OFB },
	{ "-aes-256-ofb", MBEDTLS_CIPHER_AES_256_OFB },
	{ "-aes-128-ctr", MBEDTLS_CIPHER_AES_128_CTR },
	{ "-aes-192-ctr", MBEDTLS_CIPHER_AES_192_CTR },
	{ "-aes-256-ctr", MBEDTLS_CIPHER_AES_256_CTR },
	{ NULL, 0 }
};

static int hexpair_to_int_enc(char c)
{
	if(c >= '0' && c <= '9') return c - '0';
	if(c >= 'a' && c <= 'f') return c - 'a' + 10;
	if(c >= 'A' && c <= 'F') return c - 'A' + 10;
	return -1;
}

/* Parses a hex string (as used by -K/-S/-iv) into raw bytes. */
static int enc_hex_decode(const char* hex, unsigned char* out, size_t out_cap, size_t* out_len)
{
	size_t len = strlen(hex);
	size_t i;

	if(len % 2 != 0 || (len / 2) > out_cap)
	{
		return -1;
	}

	for(i = 0; i < len; i += 2)
	{
		int hi = hexpair_to_int_enc(hex[i]);
		int lo = hexpair_to_int_enc(hex[i+1]);
		if(hi < 0 || lo < 0)
		{
			return -1;
		}
		out[i/2] = (unsigned char) ((hi << 4) | lo);
	}

	*out_len = len / 2;
	return 0;
}

static void print_hex_upper(const unsigned char* buf, size_t len)
{
	size_t i;
	for(i = 0; i < len; i++)
	{
		mbedtls_printf("%02X", buf[i]);
	}
}

/*
 * OpenSSL's classic EVP_BytesToKey key derivation (still the default -
 * without -pbkdf2/-iter - for `openssl enc`, despite the library deprecating
 * it): single iteration, D_1 = Hash(pw||salt), D_n = Hash(D_{n-1}||pw||salt),
 * concatenated and truncated to key_len+iv_len bytes
 */
static int legacy_bytes_to_key(mbedtls_md_type_t md_type,
                               const unsigned char* salt, size_t salt_len,
                               const unsigned char* pw, size_t pw_len,
                               unsigned char* out, size_t out_len)
{
	const mbedtls_md_info_t* md_info = mbedtls_md_info_from_type(md_type);
	unsigned char digest[MBEDTLS_MD_MAX_SIZE];
	unsigned char* input;
	size_t digest_len, input_len, produced = 0, take;
	int ret;

	if(md_info == NULL)
	{
		return -1;
	}
	digest_len = mbedtls_md_get_size(md_info);

	input = malloc(digest_len + pw_len + salt_len);
	if(input == NULL)
	{
		return -1;
	}

	while(produced < out_len)
	{
		input_len = 0;
		if(produced > 0)
		{
			memcpy(input, digest, digest_len);
			input_len = digest_len;
		}
		memcpy(input + input_len, pw, pw_len);
		input_len += pw_len;
		memcpy(input + input_len, salt, salt_len);
		input_len += salt_len;

		if((ret = mbedtls_md(md_info, input, input_len, digest)) != 0)
		{
			free(input);
			return ret;
		}

		take = digest_len;
		if(produced + take > out_len)
		{
			take = out_len - produced;
		}
		memcpy(out + produced, digest, take);
		produced += take;
	}

	free(input);
	return 0;
}

/* Derives key||iv (key_len+iv_len bytes total) from a passphrase and salt,
 * using either PBKDF2 or the legacy KDF, then splits the result into
 * separate key/iv output buffers. */
static int derive_key_iv(mbedtls_md_type_t md_type, int use_pbkdf2, int iterations,
                         const unsigned char* salt, size_t salt_len,
                         const char* passphrase,
                         unsigned char* key, size_t key_len,
                         unsigned char* iv, size_t iv_len)
{
	unsigned char keyiv[MAX_KEY_LEN + MAX_IV_LEN];
	int ret;

	if(use_pbkdf2)
	{
		ret = mbedtls_pkcs5_pbkdf2_hmac_ext(md_type,
			(const unsigned char*) passphrase, strlen(passphrase),
			salt, salt_len, (unsigned int) iterations,
			(uint32_t) (key_len + iv_len), keyiv);
	}
	else
	{
		ret = legacy_bytes_to_key(md_type, salt, salt_len,
			(const unsigned char*) passphrase, strlen(passphrase),
			keyiv, key_len + iv_len);
	}
	if(ret != 0)
	{
		return ret;
	}

	memcpy(key, keyiv, key_len);
	memcpy(iv, keyiv + key_len, iv_len);
	return 0;
}

/* Reads all of stdin/a file into a growable heap buffer. */
static int enc_read_all(FILE* f, unsigned char** out, size_t* out_len)
{
	size_t cap = CHUNK_SIZE;
	size_t len = 0;
	unsigned char* buf = malloc(cap);
	unsigned char* tmp;
	size_t n;

	if(buf == NULL)
	{
		return -1;
	}

	while((n = fread(buf + len, 1, cap - len, f)) > 0)
	{
		len += n;
		if(len == cap)
		{
			cap *= 2;
			tmp = realloc(buf, cap);
			if(tmp == NULL)
			{
				free(buf);
				return -1;
			}
			buf = tmp;
		}
	}

	*out = buf;
	*out_len = len;
	return 0;
}

/* Base64-encodes src (as produced by openssl's -a: standard base64, wrapped
 * at 64 chars/line unless singleline is set) and writes it to fout. */
static void write_base64(FILE* fout, const unsigned char* src, size_t src_len, int singleline)
{
	size_t need = 0, b64len = 0, off;
	unsigned char* b64buf;

	mbedtls_base64_encode(NULL, 0, &need, src, src_len);
	b64buf = malloc(need > 0 ? need : 1);
	mbedtls_base64_encode(b64buf, need, &b64len, src, src_len);

	if(singleline)
	{
		fwrite(b64buf, 1, b64len, fout);
		fwrite("\n", 1, 1, fout);
	}
	else
	{
		for(off = 0; off < b64len; off += 64)
		{
			size_t linelen = (b64len - off) < 64 ? (b64len - off) : 64;
			fwrite(b64buf + off, 1, linelen, fout);
			fwrite("\n", 1, 1, fout);
		}
	}
	free(b64buf);
}

#if !defined(MBEDTLS_CIPHER_C) || !defined(MBEDTLS_MD_C) || \
	!defined(MBEDTLS_ENTROPY_C) || !defined(MBEDTLS_CTR_DRBG_C) || \
	!defined(MBEDTLS_FS_IO)
int enc_main(void)
{
    mbedtls_printf("MBEDTLS_CIPHER_C and/or MBEDTLS_MD_C and/or "
                   "MBEDTLS_ENTROPY_C and/or MBEDTLS_CTR_DRBG_C and/or "
                   "MBEDTLS_FS_IO not defined.\n");
    mbedtls_exit(0);
}
#else

int enc_main(int argc, char** argv, int argi)
{
	int ret = 1;
	int exit_code = MBEDTLS_EXIT_FAILURE;
	int i;
	char* p;
	mbedtls_entropy_context entropy;
	mbedtls_ctr_drbg_context ctr_drbg;
	mbedtls_cipher_context_t cipher_ctx;
	const char* pers = "enc";
	int cipher_ctx_setup = 0;

	int do_decrypt = 0;
	int do_print = 0;
	int print_only = 0;
	char* infile = NULL;
	char* outfile = NULL;
	char* passphrase = NULL;
	int have_passphrase = 0;
	int base64 = 0;
	int base64_singleline = 0;
	int nosalt = 0;
	int nopad = 0;
	char* K_hex = NULL;
	char* S_hex = NULL;
	char* iv_hex = NULL;
	char* md_name = NULL;
	int use_pbkdf2 = 0;
	int iterations = 10000;
	const enc_cipher_entry* chosen_cipher = NULL;

	FILE* fin = NULL;
	FILE* fout = NULL;
	int fin_is_stdin = 0;
	int fout_is_stdout = 0;

	const mbedtls_cipher_info_t* cipher_info = NULL;
	mbedtls_cipher_mode_t cipher_mode = MBEDTLS_MODE_NONE;
	size_t key_bitlen = 0, key_len = 0, iv_len = 0, block_len = 0;
	unsigned char key[MAX_KEY_LEN];
	unsigned char iv[MAX_IV_LEN];
	unsigned char salt[SALT_LEN];
	size_t salt_len = 0;
	mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;
	int raw_key_mode; /* -K given: no KDF, no salt header at all */
	int salt_from_stream; /* decrypting, salted, but -S not given: must read
	                        * the header before the key can be derived */

	mbedtls_ctr_drbg_init(&ctr_drbg);
	mbedtls_entropy_init(&entropy);
	mbedtls_cipher_init(&cipher_ctx);

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
			exit_code = MBEDTLS_EXIT_SUCCESS;
			goto usage;
		}
		else if(strcmp(p,"-e") == 0)
		{
			do_decrypt = 0;
		}
		else if(strcmp(p,"-d") == 0)
		{
			do_decrypt = 1;
		}
		else if(strcmp(p,"-p") == 0)
		{
			do_print = 1;
		}
		else if(strcmp(p,"-P") == 0)
		{
			do_print = 1;
			print_only = 1;
		}
		else if(strcmp(p,"-a") == 0 || strcmp(p,"-base64") == 0)
		{
			base64 = 1;
		}
		else if(strcmp(p,"-A") == 0)
		{
			base64_singleline = 1;
		}
		else if(strcmp(p,"-salt") == 0)
		{
			nosalt = 0;
		}
		else if(strcmp(p,"-nosalt") == 0)
		{
			nosalt = 1;
		}
		else if(strcmp(p,"-nopad") == 0)
		{
			nopad = 1;
		}
		else if(strcmp(p,"-pbkdf2") == 0)
		{
			use_pbkdf2 = 1;
		}
		else if(strcmp(p,"-in") == 0 && i + 1 < argc)
		{
			i += 1;
			infile = strdup(argv[i]);
		}
		else if(strcmp(p,"-out") == 0 && i + 1 < argc)
		{
			i += 1;
			outfile = strdup(argv[i]);
		}
		else if(strcmp(p,"-k") == 0 && i + 1 < argc)
		{
			i += 1;
			free(passphrase);
			passphrase = strdup(argv[i]);
			have_passphrase = 1;
		}
		else if(strcmp(p,"-kfile") == 0 && i + 1 < argc)
		{
			FILE* kf;
			char linebuf[1024];
			i += 1;
			if ((kf = fopen(argv[i], "r")) == NULL) {
				mbedtlsclu_prio_printf(MBEDTLSCLU_ERR,"Could not open -kfile %s\n", argv[i]);
				goto exit;
			}
			if (fgets(linebuf, sizeof(linebuf), kf) != NULL) {
				linebuf[strcspn(linebuf, "\r\n")] = '\0';
				free(passphrase);
				passphrase = strdup(linebuf);
				have_passphrase = 1;
			}
			fclose(kf);
		}
		else if(strcmp(p,"-pass") == 0 && i + 1 < argc)
		{
			i += 1;
			p = argv[i];
			if(strncmp(p,"pass:",5) == 0)
			{
				free(passphrase);
				passphrase = strdup(p+5);
				have_passphrase = 1;
			}
			else if(strncmp(p,"file:",5) == 0)
			{
				FILE* kf;
				char linebuf[1024];
				if ((kf = fopen(p+5, "r")) == NULL) {
					mbedtlsclu_prio_printf(MBEDTLSCLU_ERR,"Could not open -pass file %s\n", p+5);
					goto exit;
				}
				if (fgets(linebuf, sizeof(linebuf), kf) != NULL) {
					linebuf[strcspn(linebuf, "\r\n")] = '\0';
					free(passphrase);
					passphrase = strdup(linebuf);
					have_passphrase = 1;
				}
				fclose(kf);
			}
			else
			{
				goto usage;
			}
		}
		else if(strcmp(p,"-K") == 0 && i + 1 < argc)
		{
			i += 1;
			K_hex = strdup(argv[i]);
		}
		else if(strcmp(p,"-S") == 0 && i + 1 < argc)
		{
			i += 1;
			S_hex = strdup(argv[i]);
		}
		else if(strcmp(p,"-iv") == 0 && i + 1 < argc)
		{
			i += 1;
			iv_hex = strdup(argv[i]);
		}
		else if(strcmp(p,"-md") == 0 && i + 1 < argc)
		{
			i += 1;
			md_name = strdup(argv[i]);
		}
		else if(strcmp(p,"-iter") == 0 && i + 1 < argc)
		{
			i += 1;
			iterations = atoi(argv[i]);
			use_pbkdf2 = 1;
			if(iterations <= 0)
			{
				goto usage;
			}
		}
		else
		{
			const enc_cipher_entry* c;
			int matched = 0;
			for(c = enc_ciphers; c->flag != NULL; c++)
			{
				if(strcmp(p, c->flag) == 0)
				{
					chosen_cipher = c;
					matched = 1;
					break;
				}
			}
			if(!matched)
			{
				goto usage;
			}
		}
	}

	if(chosen_cipher == NULL || (K_hex == NULL && !have_passphrase))
	{
		goto usage;
	}

	cipher_info = mbedtls_cipher_info_from_type(chosen_cipher->type);
	if(cipher_info == NULL)
	{
		mbedtlsclu_prio_printf(MBEDTLSCLU_ERR,"Cipher not available in this build\n");
		goto exit;
	}
	cipher_mode = mbedtls_cipher_info_get_mode(cipher_info);
	key_bitlen = mbedtls_cipher_info_get_key_bitlen(cipher_info);
	key_len = key_bitlen / 8;
	iv_len = mbedtls_cipher_info_get_iv_size(cipher_info);
	block_len = mbedtls_cipher_info_get_block_size(cipher_info);

	if(md_name != NULL)
	{
		char* mdup = strdup(md_name);
		const mbedtls_md_info_t* md_info;
		to_uppercase(mdup);
		md_info = mbedtls_md_info_from_string(mdup);
		free(mdup);
		if(md_info == NULL)
		{
			mbedtlsclu_prio_printf(MBEDTLSCLU_ERR,"Invalid -md digest: %s\n", md_name);
			goto exit;
		}
		md_type = mbedtls_md_get_type(md_info);
	}

	raw_key_mode = (K_hex != NULL);
	salt_from_stream = (!raw_key_mode && do_decrypt && !nosalt && S_hex == NULL);

	if(raw_key_mode)
	{
		size_t klen = 0, ivlen = 0;
		if(enc_hex_decode(K_hex, key, sizeof(key), &klen) != 0 || klen != key_len)
		{
			mbedtlsclu_prio_printf(MBEDTLSCLU_ERR,"Invalid -K value\n");
			goto exit;
		}
		if(iv_len > 0)
		{
			if(iv_hex == NULL || enc_hex_decode(iv_hex, iv, sizeof(iv), &ivlen) != 0 || ivlen != iv_len)
			{
				mbedtlsclu_prio_printf(MBEDTLSCLU_ERR,"-K requires a matching -iv\n");
				goto exit;
			}
		}
	}
	else if(!nosalt)
	{
		if(S_hex != NULL)
		{
			if(enc_hex_decode(S_hex, salt, sizeof(salt), &salt_len) != 0 || salt_len != SALT_LEN)
			{
				mbedtlsclu_prio_printf(MBEDTLSCLU_ERR,"Invalid -S value\n");
				goto exit;
			}
		}
		else if(!do_decrypt)
		{
			if ((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
											 (const unsigned char*) pers, strlen(pers))) != 0) {
				mbedtlsclu_prio_printf(MBEDTLSCLU_ERR,"mbedtls_ctr_drbg_seed returned %d\n", ret);
				goto exit;
			}
			if ((ret = mbedtls_ctr_drbg_random(&ctr_drbg, salt, SALT_LEN)) != 0) {
				mbedtlsclu_prio_printf(MBEDTLSCLU_ERR,"mbedtls_ctr_drbg_random returned %d\n", ret);
				goto exit;
			}
			salt_len = SALT_LEN;
		}
		/* else: decrypting, no -S - salt_from_stream is set, resolved below */
	}

	if(!raw_key_mode && !salt_from_stream)
	{
		if ((ret = derive_key_iv(md_type, use_pbkdf2, iterations, salt, salt_len,
								 passphrase, key, key_len, iv, iv_len)) != 0) {
			mbedtlsclu_prio_printf(MBEDTLSCLU_ERR,"Key derivation failed: %d\n", ret);
			goto exit;
		}
	}

	if(infile != NULL)
	{
		if ((fin = fopen(infile, "rb")) == NULL) {
			mbedtlsclu_prio_printf(MBEDTLSCLU_ERR,"Could not open -in %s\n", infile);
			goto exit;
		}
	}
	else
	{
		fin = stdin;
		fin_is_stdin = 1;
	}

	if(!print_only)
	{
		if(outfile != NULL)
		{
			if ((fout = fopen(outfile, "wb")) == NULL) {
				mbedtlsclu_prio_printf(MBEDTLSCLU_ERR,"Could not open -out %s\n", outfile);
				goto exit;
			}
		}
		else
		{
			fout = stdout;
			fout_is_stdout = 1;
		}
	}

	if(base64 || salt_from_stream)
	{
		/* Buffered path: whole input read into memory up front. Required
		 * for -a (see KNOWN_DIFFERENCES.md - not truly streamed in this
		 * implementation) and for salt_from_stream (need to peek the
		 * header before the key can be derived, before we can even set up
		 * the cipher context) UNLESS this is the streaming non-base64 case,
		 * which instead reads just the 16-byte header directly below. */
		if(base64)
		{
			unsigned char* filebuf = NULL;
			size_t filebuf_len = 0;
			unsigned char* databuf;
			size_t databuf_len;
			unsigned char* plainbuf;
			size_t plainbuf_len;
			unsigned char* result;
			size_t result_cap, result_len, olen;

			if(enc_read_all(fin, &filebuf, &filebuf_len) != 0)
			{
				mbedtlsclu_prio_printf(MBEDTLSCLU_ERR,"Failed to read input\n");
				goto exit;
			}

			if(do_decrypt)
			{
				size_t need = 0;
				mbedtls_base64_decode(NULL, 0, &need, filebuf, filebuf_len);
				databuf = malloc(need > 0 ? need : 1);
				if (mbedtls_base64_decode(databuf, need, &databuf_len, filebuf, filebuf_len) != 0) {
					mbedtlsclu_prio_printf(MBEDTLSCLU_ERR,"Invalid base64 input\n");
					free(filebuf);
					goto exit;
				}
				free(filebuf);
			}
			else
			{
				databuf = filebuf;
				databuf_len = filebuf_len;
			}

			if(salt_from_stream)
			{
				if(databuf_len < SALT_MAGIC_LEN + SALT_LEN || memcmp(databuf, SALT_MAGIC, SALT_MAGIC_LEN) != 0)
				{
					mbedtlsclu_prio_printf(MBEDTLSCLU_ERR,"bad decrypt\n  !  bad magic number (not a salted enc file)\n\n");
					free(databuf);
					goto exit;
				}
				memcpy(salt, databuf + SALT_MAGIC_LEN, SALT_LEN);
				salt_len = SALT_LEN;
				if ((ret = derive_key_iv(md_type, use_pbkdf2, iterations, salt, salt_len,
										 passphrase, key, key_len, iv, iv_len)) != 0) {
					mbedtlsclu_prio_printf(MBEDTLSCLU_ERR,"Key derivation failed: %d\n", ret);
					free(databuf);
					goto exit;
				}
				plainbuf = databuf + SALT_MAGIC_LEN + SALT_LEN;
				plainbuf_len = databuf_len - SALT_MAGIC_LEN - SALT_LEN;
			}
			else
			{
				plainbuf = databuf;
				plainbuf_len = databuf_len;
			}

			if(do_print)
			{
				if(salt_len > 0) { mbedtls_printf("salt="); print_hex_upper(salt, salt_len); mbedtls_printf("\n"); }
				mbedtls_printf("key="); print_hex_upper(key, key_len); mbedtls_printf("\n");
				if(iv_len > 0) { mbedtls_printf("iv ="); print_hex_upper(iv, iv_len); mbedtls_printf("\n"); }
			}
			if(print_only)
			{
				free(databuf);
				exit_code = MBEDTLS_EXIT_SUCCESS;
				goto exit;
			}

			if ((ret = mbedtls_cipher_setup(&cipher_ctx, cipher_info)) != 0) {
				mbedtlsclu_prio_printf(MBEDTLSCLU_ERR,"mbedtls_cipher_setup returned %d\n", ret);
				free(databuf); goto exit;
			}
			cipher_ctx_setup = 1;
			if ((ret = mbedtls_cipher_setkey(&cipher_ctx, key, (int) key_bitlen,
											 do_decrypt ? MBEDTLS_DECRYPT : MBEDTLS_ENCRYPT)) != 0) {
				mbedtlsclu_prio_printf(MBEDTLSCLU_ERR,"mbedtls_cipher_setkey returned %d\n", ret);
				free(databuf); goto exit;
			}
#if defined(MBEDTLS_CIPHER_MODE_WITH_PADDING)
			if(cipher_mode == MBEDTLS_MODE_CBC)
			{
				if ((ret = mbedtls_cipher_set_padding_mode(&cipher_ctx,
						nopad ? MBEDTLS_PADDING_NONE : MBEDTLS_PADDING_PKCS7)) != 0) {
					mbedtlsclu_prio_printf(MBEDTLSCLU_ERR,"mbedtls_cipher_set_padding_mode returned %d\n", ret);
					free(databuf); goto exit;
				}
			}
#endif /* MBEDTLS_CIPHER_MODE_WITH_PADDING */
			if(iv_len > 0)
			{
				if ((ret = mbedtls_cipher_set_iv(&cipher_ctx, iv, iv_len)) != 0) {
					mbedtlsclu_prio_printf(MBEDTLSCLU_ERR,"mbedtls_cipher_set_iv returned %d\n", ret);
					free(databuf); goto exit;
				}
			}
			if ((ret = mbedtls_cipher_reset(&cipher_ctx)) != 0) {
				mbedtlsclu_prio_printf(MBEDTLSCLU_ERR,"mbedtls_cipher_reset returned %d\n", ret);
				free(databuf); goto exit;
			}

			result_cap = plainbuf_len + block_len + block_len;
			result = malloc(result_cap > 0 ? result_cap : 1);
			if ((ret = mbedtls_cipher_update(&cipher_ctx, plainbuf, plainbuf_len, result, &olen)) != 0) {
				mbedtlsclu_prio_printf(MBEDTLSCLU_ERR,"%s\n", do_decrypt ? "bad decrypt" : "bad encrypt");
				free(result); free(databuf); goto exit;
			}
			result_len = olen;
			if ((ret = mbedtls_cipher_finish(&cipher_ctx, result + result_len, &olen)) != 0) {
				mbedtlsclu_prio_printf(MBEDTLSCLU_ERR,"%s\n", do_decrypt ? "bad decrypt" : "bad encrypt");
				free(result); free(databuf); goto exit;
			}
			result_len += olen;

			if(!do_decrypt)
			{
				unsigned char* withheader = malloc(SALT_MAGIC_LEN + salt_len + result_len);
				size_t wh_len = 0;
				if(salt_len > 0)
				{
					memcpy(withheader, SALT_MAGIC, SALT_MAGIC_LEN);
					memcpy(withheader + SALT_MAGIC_LEN, salt, salt_len);
					wh_len = SALT_MAGIC_LEN + salt_len;
				}
				memcpy(withheader + wh_len, result, result_len);
				wh_len += result_len;
				write_base64(fout, withheader, wh_len, base64_singleline);
				free(withheader);
			}
			else
			{
				fwrite(result, 1, result_len, fout);
			}

			free(result);
			free(databuf);
		}
		else
		{
			/* salt_from_stream, non-base64: peek just the 16-byte header,
			 * derive the key, then fall through to true chunked streaming
			 * for the remainder of the file (fin's read position already
			 * continues right after the header - no seeking needed). */
			unsigned char header[SALT_MAGIC_LEN + SALT_LEN];

			if(fread(header, 1, sizeof(header), fin) != sizeof(header) ||
			   memcmp(header, SALT_MAGIC, SALT_MAGIC_LEN) != 0)
			{
				mbedtlsclu_prio_printf(MBEDTLSCLU_ERR,"bad decrypt\n  !  bad magic number (not a salted enc file)\n\n");
				goto exit;
			}
			memcpy(salt, header + SALT_MAGIC_LEN, SALT_LEN);
			salt_len = SALT_LEN;

			if ((ret = derive_key_iv(md_type, use_pbkdf2, iterations, salt, salt_len,
									 passphrase, key, key_len, iv, iv_len)) != 0) {
				mbedtlsclu_prio_printf(MBEDTLSCLU_ERR,"Key derivation failed: %d\n", ret);
				goto exit;
			}
			goto setup_and_stream;
		}
	}
	else
	{
setup_and_stream:
		if(do_print)
		{
			if(salt_len > 0) { mbedtls_printf("salt="); print_hex_upper(salt, salt_len); mbedtls_printf("\n"); }
			mbedtls_printf("key="); print_hex_upper(key, key_len); mbedtls_printf("\n");
			if(iv_len > 0) { mbedtls_printf("iv ="); print_hex_upper(iv, iv_len); mbedtls_printf("\n"); }
		}
		if(print_only)
		{
			exit_code = MBEDTLS_EXIT_SUCCESS;
			goto exit;
		}

		if ((ret = mbedtls_cipher_setup(&cipher_ctx, cipher_info)) != 0) {
			mbedtlsclu_prio_printf(MBEDTLSCLU_ERR,"mbedtls_cipher_setup returned %d\n", ret);
			goto exit;
		}
		cipher_ctx_setup = 1;
		if ((ret = mbedtls_cipher_setkey(&cipher_ctx, key, (int) key_bitlen,
										 do_decrypt ? MBEDTLS_DECRYPT : MBEDTLS_ENCRYPT)) != 0) {
			mbedtlsclu_prio_printf(MBEDTLSCLU_ERR,"mbedtls_cipher_setkey returned %d\n", ret);
			goto exit;
		}
#if defined(MBEDTLS_CIPHER_MODE_WITH_PADDING)
		if(cipher_mode == MBEDTLS_MODE_CBC)
		{
			if ((ret = mbedtls_cipher_set_padding_mode(&cipher_ctx,
					nopad ? MBEDTLS_PADDING_NONE : MBEDTLS_PADDING_PKCS7)) != 0) {
				mbedtlsclu_prio_printf(MBEDTLSCLU_ERR,"mbedtls_cipher_set_padding_mode returned %d\n", ret);
				goto exit;
			}
		}
#endif /* MBEDTLS_CIPHER_MODE_WITH_PADDING */
		if(iv_len > 0)
		{
			if ((ret = mbedtls_cipher_set_iv(&cipher_ctx, iv, iv_len)) != 0) {
				mbedtlsclu_prio_printf(MBEDTLSCLU_ERR,"mbedtls_cipher_set_iv returned %d\n", ret);
				goto exit;
			}
		}
		if ((ret = mbedtls_cipher_reset(&cipher_ctx)) != 0) {
			mbedtlsclu_prio_printf(MBEDTLSCLU_ERR,"mbedtls_cipher_reset returned %d\n", ret);
			goto exit;
		}

		{
			unsigned char inbuf[CHUNK_SIZE];
			unsigned char outbuf[CHUNK_SIZE + MAX_BLOCK_LEN + MAX_BLOCK_LEN];
			size_t n, olen;

			if(!do_decrypt && salt_len > 0)
			{
				fwrite(SALT_MAGIC, 1, SALT_MAGIC_LEN, fout);
				fwrite(salt, 1, salt_len, fout);
			}

			while((n = fread(inbuf, 1, sizeof(inbuf), fin)) > 0)
			{
				if ((ret = mbedtls_cipher_update(&cipher_ctx, inbuf, n, outbuf, &olen)) != 0) {
					mbedtlsclu_prio_printf(MBEDTLSCLU_ERR,"%s\n", do_decrypt ? "bad decrypt" : "bad encrypt");
					goto exit;
				}
				fwrite(outbuf, 1, olen, fout);
			}
			if ((ret = mbedtls_cipher_finish(&cipher_ctx, outbuf, &olen)) != 0) {
				mbedtlsclu_prio_printf(MBEDTLSCLU_ERR,"%s\n", do_decrypt ? "bad decrypt" : "bad encrypt");
				goto exit;
			}
			fwrite(outbuf, 1, olen, fout);
		}
	}

	exit_code = MBEDTLS_EXIT_SUCCESS;

exit:

	if(fin != NULL && !fin_is_stdin) fclose(fin);
	if(fout != NULL && !fout_is_stdout) fclose(fout);
	if(cipher_ctx_setup) mbedtls_cipher_free(&cipher_ctx);
	mbedtls_ctr_drbg_free(&ctr_drbg);
	mbedtls_entropy_free(&entropy);
#if defined(MBEDTLS_USE_PSA_CRYPTO)
	mbedtls_psa_crypto_free();
#endif /* MBEDTLS_USE_PSA_CRYPTO */

	return exit_code;
}
#endif /* MBEDTLS_CIPHER_C && MBEDTLS_MD_C && MBEDTLS_ENTROPY_C &&
		MBEDTLS_CTR_DRBG_C && MBEDTLS_FS_IO */
