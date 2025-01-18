/*
 * a demo of calculating message digest / hash in C language and openssl library
 * references:
 * - wiki.openssl.org/index.php/EVP_message_Digests
 * - "MD5", "EVP_DigestInit" manual
 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/mman.h>
#include <openssl/sha.h>
#include <openssl/md5.h>
#include <openssl/evp.h>

#define BUFFER_LENGTH 4096UL
#define STR_TO_HASH "a string"

static void print_digest(const char *head, const unsigned char *digest, unsigned long length)
{
	printf("%s: ", head);
	for (unsigned long i = 0; i < length; i++)
		printf("%02x", digest[i]);
	printf("\n");
}

/*
 * call md5sum in shell, used here for cross-validation
 * @str: the string on which to calculate hash
 * @type: either "md5" or "sha256"
 */
static void str_to_digest(const char *str, const char *type, unsigned char *md, unsigned int md_len)
{
	char cmd[64];
	sprintf(cmd, "echo -n \"%s\" | %ssum", str, type);
	FILE *fp = popen(cmd, "r");
	if (fp == NULL) {
		printf("%s: failed to run '%s', errmsg='%s'.\n", __func__, cmd, strerror(errno));
		return;
	}
	for (unsigned int i = 0; i < md_len; i++) {
		unsigned int value;
		if (fscanf(fp, "%2x", &value) != 1) {
			fprintf(stderr, "failed to parse shell output");
			exit(-1);
		}
		md[i] = value;
	}
}

static void utils_demo(void)
{
	printf("message digest size: md5=%u, sha256=%u, max=%u\n", EVP_MD_size(EVP_md5()),
		EVP_MD_size(EVP_sha256()), EVP_MAX_MD_SIZE);
	if (EVP_MD_size(EVP_sha256()) != SHA256_DIGEST_LENGTH ||
		EVP_MD_size(EVP_md5()) != MD5_DIGEST_LENGTH) {
		fprintf(stderr, "EVP digest length != predefined macros.\n");
		exit(-1);
	}
	printf("EVP digest length = predefined macros.\n");
}

/*
 * a demo of EVP (envelop) for message digest.
 * EVP is a consistent, unified interface which works for multiple algorithms and engine.
 * the typical procedure to perform a hash
 *   1. allocate context
 *   2. initialize context: set algorithms and select engine
 *   3. provide message and calculate hash; the digest is stored in the context
 *   4. extract the message digest from the context
 *   5. clean up context
 * NOTE: some EVP function has a legacy version (no suffix) and a extended version
 *   - e.g. EVP_DigestInit vs. EVP_DigestInit_ex
 *   - the latter is recommended in most scenario, for its flexibility
 * For this example, we will stick with the default engine (and pass in the NULL pointer as @engine)
 */
static void calc_md5(EVP_MD_CTX *evp_ctx, unsigned char *buf, unsigned long buf_len,
		     unsigned char *md)
{
	unsigned int md_len;
	if (EVP_DigestInit_ex(evp_ctx, EVP_md5(), NULL) != 1) {
		fprintf(stderr, "failed to initialize digest context with md5.\n");
		exit(-1);
	}
	if (EVP_DigestUpdate(evp_ctx, buf, buf_len) != 1) {
		fprintf(stderr, "EVP digest update failed, buf_len=%#lx.\n", buf_len);
		exit(-1);
	}
	if (EVP_DigestFinal_ex(evp_ctx, md, &md_len) != 1) {
		fprintf(stderr, "EVP digest finalize failed.\n");
		exit(-1);
	}
	if (EVP_MD_CTX_reset(evp_ctx) != 1) {
		fprintf(stderr, "EVP digest context reset failed.\n");
		exit(-1);
	}
	if (md_len != MD5_DIGEST_LENGTH) {
		fprintf(stderr, "expect MD5 EVP digest length %#x, got %#x.\n",MD5_DIGEST_LENGTH,
			md_len);
		exit(-1);
	}
}

static void md5sum_demo(void)
{
	unsigned char md[MD5_DIGEST_LENGTH], ref_md[MD5_DIGEST_LENGTH], *buf;
	EVP_MD_CTX *evp_ctx;

	buf = (unsigned char*)mmap(NULL, BUFFER_LENGTH,
			    PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (buf == MAP_FAILED) {
		fprintf(stderr, "%s: failed to mmap buffer of size %#lx.\n", strerror(errno),
			BUFFER_LENGTH);
		exit(-1);
	}

	evp_ctx = EVP_MD_CTX_new();
	if (evp_ctx == NULL) {
		fprintf(stderr, "failed to allocate digest context.\n");
		exit(-1);
	}
	memset(buf, 0, BUFFER_LENGTH);
	calc_md5(evp_ctx, buf, BUFFER_LENGTH, md);
	print_digest("all-zero", md, MD5_DIGEST_LENGTH);

	buf[0] = 1;
	calc_md5(evp_ctx, buf, BUFFER_LENGTH, md);
	print_digest("lsbyte-1", md, MD5_DIGEST_LENGTH);

	buf[0] = 0;
	buf[BUFFER_LENGTH - 1] = 1;
	calc_md5(evp_ctx, buf, BUFFER_LENGTH, md);
	print_digest("msbyte-1", md, MD5_DIGEST_LENGTH);

	buf[BUFFER_LENGTH - 1] = 0;
	calc_md5(evp_ctx, buf, BUFFER_LENGTH, md);
	print_digest("all-zero", md, MD5_DIGEST_LENGTH);

	calc_md5(evp_ctx, buf, BUFFER_LENGTH / 2, md);
	print_digest("half-len", md, MD5_DIGEST_LENGTH);

	calc_md5(evp_ctx, (unsigned char*)STR_TO_HASH, strlen(STR_TO_HASH), md);
	print_digest("'" STR_TO_HASH "'   lib", md, MD5_DIGEST_LENGTH);

	str_to_digest(STR_TO_HASH, "md5", ref_md, MD5_DIGEST_LENGTH);
	print_digest("'" STR_TO_HASH "' shell", ref_md, MD5_DIGEST_LENGTH);

	if (memcmp(md, ref_md, MD5_DIGEST_LENGTH) != 0) {
		fprintf(stderr, "lib and shell md5 does not match");
		exit(-1);
	}

	EVP_MD_CTX_free(evp_ctx);

	munmap(buf, BUFFER_LENGTH);
}

static void sha256_demo(void)
{
	unsigned char md[SHA256_DIGEST_LENGTH], ref_md[SHA256_DIGEST_LENGTH], *buf;

	buf = (unsigned char*)mmap(NULL, BUFFER_LENGTH,
			    PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (buf == MAP_FAILED) {
		fprintf(stderr, "%s: failed to mmap buffer of size %#lx.\n", strerror(errno),
			BUFFER_LENGTH);
		exit(-1);
	}

	memset(buf, 0, BUFFER_LENGTH);
	SHA256(buf, BUFFER_LENGTH, md);
	print_digest("all-zero", md, SHA256_DIGEST_LENGTH);
	buf[0] = 1;
	SHA256(buf, BUFFER_LENGTH, md);
	print_digest("lsbyte-1", md, SHA256_DIGEST_LENGTH);
	buf[0] = 0;
	buf[BUFFER_LENGTH - 1] = 1;
	SHA256(buf, BUFFER_LENGTH, md);
	print_digest("msbyte-1", md, SHA256_DIGEST_LENGTH);
	buf[BUFFER_LENGTH - 1] = 0;
	SHA256(buf, BUFFER_LENGTH, md);
	print_digest("all-zero", md, SHA256_DIGEST_LENGTH);
	SHA256(buf, BUFFER_LENGTH / 2, md);
	print_digest("half-len", md, SHA256_DIGEST_LENGTH);
	SHA256((unsigned char*)STR_TO_HASH, strlen(STR_TO_HASH), md);
	print_digest("'" STR_TO_HASH "'   lib", md, SHA256_DIGEST_LENGTH);
	str_to_digest(STR_TO_HASH, "sha256", ref_md, SHA256_DIGEST_LENGTH);
	print_digest("'" STR_TO_HASH "' shell", ref_md, SHA256_DIGEST_LENGTH);

	if (memcmp(md, ref_md, SHA256_DIGEST_LENGTH) != 0) {
		fprintf(stderr, "lib and shell sha256 does not match");
		exit(-1);
	}

	munmap(buf, BUFFER_LENGTH);
}

int main(void)
{
	printf("* utils ***************************************************\n");
	utils_demo();
	printf("* sha256 **************************************************\n");
	sha256_demo();
	printf("* md5 *****************************************************\n");
	md5sum_demo();
	printf("* end *****************************************************\n");
	return 0;
}
