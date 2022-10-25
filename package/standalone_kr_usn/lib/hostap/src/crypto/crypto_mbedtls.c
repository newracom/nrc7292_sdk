/*
 * WPA Supplicant / wrapper functions for mbedtls
 * Copyright (c) 2004-2009, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#include "includes.h"

#include "common.h"
#include "crypto.h"

#include "mbedtls/md5.h"
#include "mbedtls/des.h"

#include "mbedtls/sha1.h"
#include "mbedtls/mbed_sha256.h"
#if defined(MBEDTLS_SHA512_C)
#include "mbedtls/mbed_sha512.h"
#endif
#include "mbedtls/pkcs5.h"

#include "mbedtls/aes.h"
#include "mbedtls/rsa.h"
#include "mbedtls/arc4.h"

#include "system.h"
#include "platform.h"

#include "system_common.h"

#if (defined (CONFIG_SAE) || defined (CONFIG_OWE))
#include "random.h"

#include "mbedtls/ecp.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"

#include "mbedtls/pk.h"
#include "mbedtls/ecdh.h"
#include "mbedtls/asn1write.h"
#include "mbedtls/error.h"
#include "mbedtls/oid.h"

#include "mbedtls/bignum.h"

//Print Elapsed time for ECC Proceeing for SAE/OWE for debug
#define DEBUG_ECC_PROCESS_TIME 1

#if DEBUG_ECC_PROCESS_TIME //For Debug
#include "hal_lmac_register.h"
#define TSF     (*(volatile uint32_t*)(MAC_REG_TSF_0_LOWER_READONLY))
static void _wpa_msg_crypto(void *ctx, int level, const char *fmt, ...)
{
	va_list ap;
	if (level >= wpa_debug_level) {
		va_start(ap, fmt);
		system_vprintf(fmt, ap);
		system_printf("\n");
		va_end(ap);
	}
}
#endif //DEBUG_ECC_PROCESS_TIME

#endif //#if (defined (CONFIG_SAE) || defined (CONFIG_OWE))

#ifdef TAG
#undef TAG
#endif
#define TAG "tls: "

#ifndef PUT_UINT32_BE
#define PUT_UINT32_BE(n,b,i)                            \
{                                                       \
    (b)[(i)    ] = (unsigned char) ( (n) >> 24 );       \
    (b)[(i) + 1] = (unsigned char) ( (n) >> 16 );       \
    (b)[(i) + 2] = (unsigned char) ( (n) >>  8 );       \
    (b)[(i) + 3] = (unsigned char) ( (n)       );       \
}
#endif


static int mbedtls_hmac_vector(mbedtls_md_type_t type, const u8 *key,
			size_t key_len, size_t num_elem, const u8 *addr[],
			const size_t *len, u8 *mac)
{
	mbedtls_md_context_t ctx;
	size_t i;
	int res = 0;

	wpa_printf(MSG_ERROR, TAG "%s\n", __func__);
	mbedtls_md_init(&ctx);
	if (mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(type), 1) != 0)
		res = -1;

	if (res == 0 && mbedtls_md_hmac_starts(&ctx, key, key_len) != 0)
		res = -1;

	for (i = 0; res == 0 && i < num_elem; i++)
		res = mbedtls_md_hmac_update(&ctx, addr[i], len[i]);

	if (res == 0)
		res = mbedtls_md_hmac_finish(&ctx, mac);
	mbedtls_md_free(&ctx);

	wpa_printf(MSG_ERROR, TAG "ret - %d\n", res);
	return res == 0 ? 0 : -1;
}

int hmac_sha1_vector(const u8 *key, size_t key_len, size_t num_elem,
		     const u8 *addr[], const size_t *len, u8 *mac)
{
	return mbedtls_hmac_vector(MBEDTLS_MD_SHA1, key, key_len, num_elem, addr,
				len, mac);
}


int hmac_sha1(const u8 *key, size_t key_len, const u8 *data, size_t data_len,
	      u8 *mac)
{
	wpa_printf(MSG_ERROR, TAG "%s\n", __func__);
	return hmac_sha1_vector(key, key_len, 1, &data, &data_len, mac);
}
int pbkdf2_sha1(const char *passphrase, const u8 *ssid, size_t ssid_len,
		int iterations, u8 *buf, size_t buflen)
{
	mbedtls_md_context_t md;
	const mbedtls_md_info_t *info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA1);
	int res = 0;

	wpa_printf(MSG_ERROR, TAG "%s\n", __func__);
	mbedtls_md_init(&md);
	if (mbedtls_md_setup(&md, info, 1) == 0)
		res = mbedtls_pkcs5_pbkdf2_hmac(&md,
				(const unsigned char *) passphrase, os_strlen(passphrase),
				ssid, ssid_len, iterations, buflen, buf);
	mbedtls_md_free(&md);

	return res == 0 ? 0 : -1;
}


int des_encrypt(const u8 *clear, const u8 *key, u8 *cypher)
{
	mbedtls_des_context ctx;

	mbedtls_des_init(&ctx);
	mbedtls_des_setkey_enc(&ctx, key);
	mbedtls_des_crypt_ecb(&ctx, clear, cypher);
	mbedtls_des_free(&ctx);
	return 0;
}

int md5_vector(size_t num_elem, const u8 *addr[], const size_t *len, u8 *mac)
{
	mbedtls_md5_context ctx;
	size_t i;

	mbedtls_md5_init(&ctx);
	mbedtls_md5_starts(&ctx);

	for (i = 0; i < num_elem; ++i)
		mbedtls_md5_update(&ctx, addr[i], len[i]);

	mbedtls_md5_finish(&ctx, mac);
	mbedtls_md5_free(&ctx);

	return 0;
}

int hmac_md5_vector(const u8 *key, size_t key_len, size_t num_elem,
		    const u8 *addr[], const size_t *len, u8 *mac)
{
	return mbedtls_hmac_vector(MBEDTLS_MD_MD5, key, key_len, num_elem, addr,
				len, mac);
}


int hmac_md5(const u8 *key, size_t key_len, const u8 *data, size_t data_len,
	     u8 *mac)
{
	return hmac_md5_vector(key, key_len, 1, &data, &data_len, mac);
}

int sha1_vector(size_t num_elem, const u8 *addr[], const size_t *len, u8 *mac)
{
	mbedtls_sha1_context ctx;
	size_t i;

	mbedtls_sha1_init(&ctx);
	mbedtls_sha1_starts(&ctx);

	for (i = 0; i < num_elem; ++i)
		mbedtls_sha1_update(&ctx, addr[i], len[i]);

	mbedtls_sha1_finish(&ctx, mac);
	mbedtls_sha1_free(&ctx);

	return 0;
}

int sha256_vector(size_t num_elem, const u8 *addr[], const size_t *len, u8 *mac)
{
	mbedtls_sha256_context ctx;
	size_t i;

	mbedtls_sha256_init(&ctx);
	mbedtls_sha256_starts(&ctx, 0 /*use SHA256*/);

	for (i = 0; i < num_elem; ++i)
		mbedtls_sha256_update(&ctx, addr[i], len[i]);

	mbedtls_sha256_finish(&ctx, mac);
	mbedtls_sha256_free(&ctx);

	return 0;
}

int sha512_vector(size_t num_elem, const u8 *addr[], const size_t *len, u8 *mac)
{
	mbedtls_sha512_context ctx;
	size_t i;

	mbedtls_sha512_init(&ctx);
	mbedtls_sha512_starts(&ctx, 0 /*use SHA512*/);

	for (i = 0; i < num_elem; ++i)
		mbedtls_sha512_update(&ctx, addr[i], len[i]);

	mbedtls_sha512_finish(&ctx, mac);
	mbedtls_sha512_free(&ctx);

	return 0;
}

int sha384_vector(size_t num_elem, const u8 *addr[], const size_t *len, u8 *mac)
{
	mbedtls_sha512_context ctx;
	size_t i;

	mbedtls_sha512_init(&ctx);
	mbedtls_sha512_starts(&ctx, 1 /*use SHA384*/);

	for (i = 0; i < num_elem; ++i)
		mbedtls_sha512_update(&ctx, addr[i], len[i]);

	mbedtls_sha512_finish(&ctx, mac);
	mbedtls_sha512_free(&ctx);

	return 0;
}


int hmac_sha384_vector(const u8 *key, size_t key_len, size_t num_elem,
		const u8 *addr[], const size_t *len, u8 *mac)
{
	return mbedtls_hmac_vector(MBEDTLS_MD_SHA384, key, key_len, num_elem, addr,
				   len, mac);
}


int hmac_sha384(const u8 *key, size_t key_len, const u8 *data,
		size_t data_len, u8 *mac)
{
	return hmac_sha384_vector(key, key_len, 1, &data, &data_len, mac);
}

int hmac_sha512_vector(const u8 *key, size_t key_len, size_t num_elem,
		const u8 *addr[], const size_t *len, u8 *mac)
{
	return mbedtls_hmac_vector(MBEDTLS_MD_SHA512, key, key_len, num_elem, addr,
				   len, mac);
}


int hmac_sha512(const u8 *key, size_t key_len, const u8 *data,
		size_t data_len, u8 *mac)
{
	return hmac_sha512_vector(key, key_len, 1, &data, &data_len, mac);
}

void * aes_encrypt_init(const u8 *key, size_t len)
{
	mbedtls_aes_context *ctx = os_zalloc(sizeof(*ctx));

	mbedtls_aes_init(ctx);

	if (mbedtls_aes_setkey_enc(ctx, key, len * 8) == 0)
		return ctx;

	os_free(ctx);

	return NULL;
}


int aes_encrypt(void *ctx, const u8 *plain, u8 *crypt)
{
	if (ctx)
		mbedtls_aes_crypt_ecb(ctx, MBEDTLS_AES_ENCRYPT, plain, crypt);
	else
		return -1;

	return 0;
}


void aes_encrypt_deinit(void *ctx)
{
	if (ctx) {
		mbedtls_aes_free(ctx);
		os_free(ctx);
		ctx = NULL;
	}
}

void * aes_decrypt_init(const u8 *key, size_t len)
{
	mbedtls_aes_context *ctx = os_zalloc(sizeof(*ctx));

	mbedtls_aes_init(ctx);

	if (mbedtls_aes_setkey_dec(ctx, key, len * 8) == 0)
		return ctx;

	os_free(ctx);

	return NULL;
}

int aes_decrypt(void *ctx, const u8 *crypt, u8 *plain)
{
	if (!ctx)
		return -1;

	mbedtls_aes_crypt_ecb(ctx, MBEDTLS_AES_DECRYPT, crypt, plain);
	return 0;
}

void aes_decrypt_deinit(void *ctx)
{
	if (ctx) {
		mbedtls_aes_free(ctx);
		os_free(ctx);
		ctx = NULL;
	}
}

int crypto_mod_exp(const u8 *base, size_t base_len,
		   const u8 *power, size_t power_len,
		   const u8 *modulus, size_t modulus_len,
		   u8 *result, size_t *result_len)
{
	mbedtls_mpi mpi_prime, mpi_gen, key_private, mpi_res;
	// unsigned long flags = system_irq_save();

	mbedtls_mpi_init(&key_private);
	mbedtls_mpi_init(&mpi_prime);      
	mbedtls_mpi_init(&mpi_gen);
	mbedtls_mpi_init(&mpi_res);

	mbedtls_mpi_read_binary(&mpi_gen, base, base_len);				//Generator
	mbedtls_mpi_read_binary(&key_private, power, power_len);		//Private Key
	mbedtls_mpi_read_binary(&mpi_prime, modulus, modulus_len);		//Prime Number

	if(mbedtls_mpi_exp_mod(&mpi_res, &mpi_gen, &key_private, &mpi_prime, NULL) != 0)
		return -1;

	*result_len = mbedtls_mpi_size(&mpi_res);
	if (mbedtls_mpi_write_binary(&mpi_res, result, *result_len) != 0)
		return -1;

	mbedtls_mpi_free(&key_private);
	mbedtls_mpi_free(&mpi_prime);
	mbedtls_mpi_free(&mpi_gen);
	mbedtls_mpi_free(&mpi_res);

	// system_irq_restore(flags);
	return 0;
}

struct crypto_cipher {
	enum crypto_cipher_alg alg;
	union {
		mbedtls_arc4_context *arc4_ctx;
		mbedtls_aes_context *aes_ctx;
		struct _des{
			mbedtls_des_context *dec_ctx;
			mbedtls_des_context *enc_ctx;
		} des;
		struct _des3 {
			mbedtls_des3_context *dec_ctx;
			mbedtls_des3_context *enc_ctx;
		} des3;
		mbedtls_rsa_context *rsa_ctx;
	}u;
};

struct crypto_cipher * crypto_cipher_init(enum crypto_cipher_alg alg,
					  const u8 *iv, const u8 *key,
					  size_t key_len)
{
	struct crypto_cipher *ctx = os_zalloc(sizeof(*ctx));

	if (!ctx)
		return NULL;

	switch (alg) {
	case CRYPTO_CIPHER_ALG_RC4:
	ctx->alg = CRYPTO_CIPHER_ALG_RC4;
	ctx->u.arc4_ctx = os_zalloc(sizeof(*ctx->u.arc4_ctx));
	mbedtls_arc4_setup(ctx->u.arc4_ctx, key, key_len);
	break;
	case CRYPTO_CIPHER_ALG_AES:
	ctx->alg = CRYPTO_CIPHER_ALG_AES;
	ctx->u.aes_ctx = os_zalloc(sizeof(*ctx->u.aes_ctx));
	mbedtls_aes_init(ctx->u.aes_ctx);
	mbedtls_aes_setkey_enc(ctx->u.aes_ctx, key, key_len * 8);
	break;
	case CRYPTO_CIPHER_ALG_3DES:
	ctx->alg = CRYPTO_CIPHER_ALG_3DES;
	ctx->u.des3.enc_ctx = os_zalloc(sizeof(*ctx->u.des3.enc_ctx));
	ctx->u.des3.dec_ctx = os_zalloc(sizeof(*ctx->u.des3.dec_ctx));
	mbedtls_des3_set2key_enc(ctx->u.des3.enc_ctx, key);
	mbedtls_des3_set2key_dec(ctx->u.des3.dec_ctx, key);
	break;
	case CRYPTO_CIPHER_ALG_DES:
	ctx->alg = CRYPTO_CIPHER_ALG_DES;
	ctx->u.des.dec_ctx = os_zalloc(sizeof(*ctx->u.des.dec_ctx));
	ctx->u.des.enc_ctx = os_zalloc(sizeof(*ctx->u.des.enc_ctx));
	mbedtls_des_setkey_enc(ctx->u.des.enc_ctx, key);
	mbedtls_des_setkey_dec(ctx->u.des.dec_ctx, key);
	break;
	case CRYPTO_CIPHER_ALG_RC2:
	ctx->alg = CRYPTO_CIPHER_ALG_RC2;
	return NULL;
	case CRYPTO_CIPHER_NULL:
	ctx->alg = CRYPTO_CIPHER_NULL;
	break;
	default:
		os_free(ctx);
		return NULL;
	}

	return ctx;
}

int crypto_cipher_encrypt(struct crypto_cipher *ctx, const u8 *plain,
			  u8 *crypt, size_t len)
{
	if (!ctx)
		return -1;

	switch (ctx->alg) {
		case CRYPTO_CIPHER_ALG_RC4:
		mbedtls_arc4_crypt(ctx->u.arc4_ctx, len, plain, crypt);
		break;
		case CRYPTO_CIPHER_ALG_AES:
		mbedtls_aes_crypt_ecb(ctx->u.aes_ctx, MBEDTLS_AES_ENCRYPT, plain, crypt);
		break;
		case CRYPTO_CIPHER_ALG_3DES:
		mbedtls_des3_crypt_ecb(ctx->u.des3.enc_ctx, plain, crypt);
		break;
		case CRYPTO_CIPHER_ALG_DES:
		mbedtls_des_crypt_ecb(ctx->u.des.enc_ctx, plain, crypt);
		break;
		case CRYPTO_CIPHER_ALG_RC2:
		break;
		case CRYPTO_CIPHER_NULL:
		break;
	}

	return 0;
}

int crypto_cipher_decrypt(struct crypto_cipher *ctx, const u8 *crypt,
			  u8 *plain, size_t len)
{
	if (!ctx)
		return -1;

	switch (ctx->alg) {
		case CRYPTO_CIPHER_ALG_RC4:
		mbedtls_arc4_crypt(ctx->u.arc4_ctx, len, crypt, plain);
		break;
		case CRYPTO_CIPHER_ALG_AES:
		mbedtls_aes_crypt_ecb(ctx->u.aes_ctx, MBEDTLS_AES_DECRYPT, crypt, plain);
		break;
		case CRYPTO_CIPHER_ALG_3DES:
		mbedtls_des3_crypt_ecb(ctx->u.des3.dec_ctx, crypt, plain);
		break;
		case CRYPTO_CIPHER_ALG_DES:
		mbedtls_des_crypt_ecb(ctx->u.des.dec_ctx, crypt, plain);
		break;
		case CRYPTO_CIPHER_ALG_RC2:
		break;
		case CRYPTO_CIPHER_NULL:
		break;
	}

	return 0;
}

void crypto_cipher_deinit(struct crypto_cipher *ctx)
{
	if (!ctx)
		return;

	switch (ctx->alg) {
		case CRYPTO_CIPHER_ALG_RC4:
		mbedtls_arc4_free(ctx->u.arc4_ctx);
		os_free(ctx->u.arc4_ctx);
		break;
		case CRYPTO_CIPHER_ALG_AES:
		mbedtls_aes_free(ctx->u.aes_ctx);
		os_free(ctx->u.aes_ctx);
		break;
		case CRYPTO_CIPHER_ALG_3DES:
		mbedtls_des3_free(ctx->u.des3.enc_ctx);
		mbedtls_des3_free(ctx->u.des3.dec_ctx);
		os_free(ctx->u.des3.dec_ctx);
		os_free(ctx->u.des3.enc_ctx);
		break;
		case CRYPTO_CIPHER_ALG_DES:
		mbedtls_des_free(ctx->u.des.enc_ctx);
		mbedtls_des_free(ctx->u.des.dec_ctx);
		os_free(ctx->u.des.enc_ctx);
		os_free(ctx->u.des.dec_ctx);
		break;
		case CRYPTO_CIPHER_ALG_RC2:
		break;
		case CRYPTO_CIPHER_NULL:
		break;
	}
	os_free(ctx);
	ctx = NULL;
}

/**
 * aes_128_cbc_encrypt - AES-128 CBC encryption
 * @key: Encryption key
 * @iv: Encryption IV for CBC mode (16 bytes)
 * @data: Data to encrypt in-place
 * @data_len: Length of data in bytes (must be divisible by 16)
 * Returns: 0 on success, -1 on failure
 */
int aes_128_cbc_encrypt(const u8 *key, const u8 *iv, u8 *data, size_t data_len)
{
	mbedtls_aes_context ctx;
	mbedtls_aes_init(&ctx);
	u8 iv2[16] = {0,};
	os_memcpy(iv2, iv, 16);

	if (mbedtls_aes_setkey_enc(&ctx, key, 128) != 0) {
		wpa_printf(MSG_ERROR, "mbedtls_aes_setkey_enc error! ");
		return -1;
	}
	mbedtls_aes_crypt_cbc(&ctx, MBEDTLS_AES_ENCRYPT, data_len, iv2, data, data);
	mbedtls_aes_free(&ctx);

	return 0;
}

int aes_128_cbc_decrypt(const u8 *key, const u8 *iv, u8 *data, size_t data_len)
{
	mbedtls_aes_context ctx;
	mbedtls_aes_init(&ctx);
	u8 iv2[16] = {0,};
	os_memcpy(iv2, iv, 16);

	if (mbedtls_aes_setkey_dec(&ctx, key, 128) != 0) {
		wpa_printf(MSG_ERROR, "mbedtls_aes_setkey_dec error! ");
		return -1;
	}
	mbedtls_aes_crypt_cbc(&ctx, MBEDTLS_AES_DECRYPT, data_len, iv2, data, data);
	mbedtls_aes_free(&ctx);

	return 0;
}

#define S_SWAP(a,b) do { u8 t = S[a]; S[a] = S[b]; S[b] = t; } while(0)

/**
 * TODO: Could not find any code on mbedtls
 */
int rc4_skip(const u8 *key, size_t keylen, size_t skip,
	     u8 *data, size_t data_len)
{
	u32 i, j, k;
	u8 S[256], *pos;
	size_t kpos;
	wpa_printf(MSG_ERROR, "%s", __func__);

	/* Setup RC4 state */
	for (i = 0; i < 256; i++)
		S[i] = i;
	j = 0;
	kpos = 0;
	for (i = 0; i < 256; i++) {
		j = (j + S[i] + key[kpos]) & 0xff;
		kpos++;
		if (kpos >= keylen)
			kpos = 0;
		S_SWAP(i, j);
	}

	/* Skip the start of the stream */
	i = j = 0;
	for (k = 0; k < skip; k++) {
		i = (i + 1) & 0xff;
		j = (j + S[i]) & 0xff;
		S_SWAP(i, j);
	}

	/* Apply RC4 to data */
	pos = data;
	for (k = 0; k < data_len; k++) {
		i = (i + 1) & 0xff;
		j = (j + S[i]) & 0xff;
		S_SWAP(i, j);
		*pos++ ^= S[(S[i] + S[j]) & 0xff];
	}

	return 0;
}

#if (defined (CONFIG_SAE) || defined (CONFIG_OWE))
struct crypto_ec {
	mbedtls_ecp_group group;
};

struct crypto_ecdh {
	mbedtls_ecdh_context ctx;
	size_t length;
};

static mbedtls_ecp_group_id get_ecp_group_id(int id)
{
	mbedtls_ecp_group_id grp_id = MBEDTLS_ECP_DP_NONE;

	switch (id) {
	case 19:
		grp_id = MBEDTLS_ECP_DP_SECP256R1;
		break;
	case 20:
		grp_id = MBEDTLS_ECP_DP_SECP384R1;
		break;
	case 21:
		grp_id = MBEDTLS_ECP_DP_SECP521R1;
		break;
	case 25:
		grp_id = MBEDTLS_ECP_DP_SECP192R1;
		break;
	case 26:
		grp_id = MBEDTLS_ECP_DP_SECP224R1;
		break;
	case 27:
        ///////////////???
		break;
	case 28:
		grp_id = MBEDTLS_ECP_DP_BP256R1;
		break;
	case 29:
		grp_id = MBEDTLS_ECP_DP_BP384R1;
		break;
	case 30:
		grp_id = MBEDTLS_ECP_DP_BP512R1;
		break;
	default:	
		wpa_msg(0, MSG_ERROR, "ECC : [%s, %d] Selected group is not available.\n", __func__, __LINE__);
	}

	return grp_id;
}

struct crypto_ec *crypto_ec_init(int group)
{
	struct crypto_ec *e;

	mbedtls_ecp_group_id  grp_id;

	/* IANA registry to mbedtls internal mapping*/
	grp_id = get_ecp_group_id(group);

	e = os_zalloc(sizeof(*e));
	if (e == NULL) {
		return NULL;
	}

	mbedtls_ecp_group_init(&e->group);

	if (mbedtls_ecp_group_load(&e->group, grp_id)) {
		crypto_ec_deinit(e);
		e = NULL;
	}

	return e;
}

void crypto_ec_deinit(struct crypto_ec *e)
{
	if (e == NULL) {
		return;
	}

	mbedtls_ecp_group_free(&e->group);
	os_free(e);
}

const struct crypto_bignum *crypto_ec_get_prime(struct crypto_ec *e)
{
	return (const struct crypto_bignum *) &e->group.P;
}

size_t crypto_ec_prime_len(struct crypto_ec *e)
{
	return mbedtls_mpi_size(&e->group.P);
}

size_t crypto_ec_prime_len_bits(struct crypto_ec *e)
{
	return mbedtls_mpi_bitlen(&e->group.P);
}

const struct crypto_bignum *crypto_ec_get_order(struct crypto_ec *e)
{
	return (const struct crypto_bignum *) &e->group.N;
}

size_t crypto_ec_order_len(struct crypto_ec *e)
{
	return mbedtls_mpi_size(&e->group.N);
}

struct crypto_ec_point *crypto_ec_point_init(struct crypto_ec *e)
{
	mbedtls_ecp_point *pt;
	if (e == NULL) {
		return NULL;
	}

	pt = os_zalloc(sizeof(mbedtls_ecp_point));

	if( pt == NULL) {
		return NULL;
	}

	mbedtls_ecp_point_init(pt);

	return (struct crypto_ec_point *) pt;
}

void crypto_ec_point_deinit(struct crypto_ec_point *p, int clear)
{
	mbedtls_ecp_point_free((mbedtls_ecp_point *) p);
	os_free(p);
}

int crypto_ec_point_to_bin(struct crypto_ec *e,
		const struct crypto_ec_point *point, u8 *x, u8 *y)
{
	int len = mbedtls_mpi_size(&e->group.P);

	if (x) {
		if(crypto_bignum_to_bin((struct crypto_bignum *) & ((mbedtls_ecp_point *) point)->X,
					x, len, len) < 0) {
			return -1;
		}

	}

	if (y) {
		if(crypto_bignum_to_bin((struct crypto_bignum *) & ((mbedtls_ecp_point *) point)->Y,
					y, len, len) < 0) {
			return -1;
		}
	}

	return 0;
}

struct crypto_ec_point *crypto_ec_point_from_bin(struct crypto_ec *e,
		const u8 *val)
{
	mbedtls_ecp_point *pt;
	int len, ret;

	if (e == NULL) {
		return NULL;
	}

	len = mbedtls_mpi_size(&e->group.P);

	pt = os_zalloc(sizeof(mbedtls_ecp_point));
	mbedtls_ecp_point_init(pt);

	MBEDTLS_MPI_CHK(mbedtls_mpi_read_binary(&pt->X, val, len));
	MBEDTLS_MPI_CHK(mbedtls_mpi_read_binary(&pt->Y, val + len, len));
	MBEDTLS_MPI_CHK(mbedtls_mpi_lset((&pt->Z), 1));

	return (struct crypto_ec_point *) pt;

cleanup:
	mbedtls_ecp_point_free(pt);
	os_free(pt);
	return NULL;
}

int crypto_ec_point_add(struct crypto_ec *e, const struct crypto_ec_point *a,
		const struct crypto_ec_point *b,
		struct crypto_ec_point *c)
{
	int ret;
	mbedtls_mpi one;

	mbedtls_mpi_init(&one);

	MBEDTLS_MPI_CHK(mbedtls_mpi_lset( &one, 1 ));
	MBEDTLS_MPI_CHK(mbedtls_ecp_muladd(&e->group, (mbedtls_ecp_point *) c, &one, (const mbedtls_ecp_point *)a , &one, (const mbedtls_ecp_point *)b));

cleanup:
	mbedtls_mpi_free(&one);
	return ret ? -1 : 0;
}

int crypto_ec_point_mul(struct crypto_ec *e, const struct crypto_ec_point *p,
		const struct crypto_bignum *b,
		struct crypto_ec_point *res)
{
	int ret;
	mbedtls_entropy_context entropy;
	mbedtls_ctr_drbg_context ctr_drbg;

#if DEBUG_ECC_PROCESS_TIME //For Debug
	unsigned int ptime, ctime;
	ptime = TSF;
	_wpa_msg_crypto(NULL, MSG_DEBUG, TAG "[Enter] crypto_ec_point_mul");
#endif

	mbedtls_entropy_init(&entropy);
	mbedtls_ctr_drbg_init(&ctr_drbg);

	MBEDTLS_MPI_CHK(mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
				NULL, 0));

	MBEDTLS_MPI_CHK(mbedtls_ecp_mul(&e->group,
				(mbedtls_ecp_point *) res,
				(const mbedtls_mpi *)b,
				(const mbedtls_ecp_point *)p,
				mbedtls_ctr_drbg_random,
				&ctr_drbg));

cleanup:
	mbedtls_ctr_drbg_free(&ctr_drbg);
	mbedtls_entropy_free(&entropy);

#if DEBUG_ECC_PROCESS_TIME //For Debug
	ctime = TSF;
	_wpa_msg_crypto(NULL, MSG_DEBUG, TAG "[Exit ] crypto_ec_point_mul (elapsed time: %u us)",
			(ctime -ptime));
#endif

	return ret ? -1 : 0;
}

/*  Currently mbedtls does not have any function for inverse
 *  This function calculates inverse of a point.
 *  Set R = -P
 */
static int ecp_opp(const mbedtls_ecp_group *grp, mbedtls_ecp_point *R, const mbedtls_ecp_point *P)
{
	int ret = 0;

	/* Copy */
	if (R != P) {
		MBEDTLS_MPI_CHK(mbedtls_ecp_copy(R, P));
	}

	/* In-place opposite */
	if (mbedtls_mpi_cmp_int(&R->Y, 0) != 0) {
		MBEDTLS_MPI_CHK(mbedtls_mpi_sub_mpi(&R->Y, &grp->P, &R->Y));
	}

cleanup:
	return (ret );
}

int crypto_ec_point_invert(struct crypto_ec *e, struct crypto_ec_point *p)
{
	return ecp_opp(&e->group, (mbedtls_ecp_point *) p, (mbedtls_ecp_point *) p) ? -1 : 0;
}

int crypto_ec_point_cmp(const struct crypto_ec *e,
		const struct crypto_ec_point *a,
		const struct crypto_ec_point *b)
{
	return mbedtls_ecp_point_cmp((const mbedtls_ecp_point *) a,
			(const mbedtls_ecp_point *) b);
}

struct crypto_bignum *crypto_ec_point_compute_y_sqr(struct crypto_ec *e,
		const struct crypto_bignum *x)
{
	mbedtls_mpi temp, temp2, num;
	int ret = 0;

	mbedtls_mpi *y_sqr = os_zalloc(sizeof(mbedtls_mpi));
	if (y_sqr == NULL) {
		return NULL;
	}

	mbedtls_mpi_init(&temp);
	mbedtls_mpi_init(&temp2);
	mbedtls_mpi_init(&num);
	mbedtls_mpi_init(y_sqr);

	/* y^2 = x^3 + ax + b  mod  P*/
	/* mbedtls does not have mod-add or mod-mul apis.
	 *
	 */

	/* Calculate x^3  mod P*/
	MBEDTLS_MPI_CHK(mbedtls_mpi_lset(&num, 3));
	MBEDTLS_MPI_CHK(mbedtls_mpi_exp_mod(&temp, (const mbedtls_mpi *) x, &num, &e->group.P, NULL));

	/* Calculate ax  mod P*/
	MBEDTLS_MPI_CHK(mbedtls_mpi_lset(&num, -3));
	MBEDTLS_MPI_CHK(mbedtls_mpi_mul_mpi(&temp2, (const mbedtls_mpi *) x, &num));
	MBEDTLS_MPI_CHK(mbedtls_mpi_mod_mpi(&temp2, &temp2, &e->group.P));

	/* Calculate ax + b  mod P. Note that b is already < P*/
	MBEDTLS_MPI_CHK(mbedtls_mpi_add_mpi(&temp2, &temp2, &e->group.B));
	MBEDTLS_MPI_CHK(mbedtls_mpi_mod_mpi(&temp2, &temp2, &e->group.P));

	/* Calculate x^3 + ax + b  mod P*/
	MBEDTLS_MPI_CHK(mbedtls_mpi_add_mpi(&temp2, &temp2, &temp));
	MBEDTLS_MPI_CHK(mbedtls_mpi_mod_mpi(y_sqr, &temp2, &e->group.P));

cleanup:
	mbedtls_mpi_free(&temp);
	mbedtls_mpi_free(&temp2);
	mbedtls_mpi_free(&num);
	if (ret) {
		mbedtls_mpi_free(y_sqr);
		os_free(y_sqr);
		return NULL;
	} else {
		return (struct crypto_bignum *) y_sqr;
	}
}

int crypto_ec_point_solve_y_coord(struct crypto_ec *e,
		struct crypto_ec_point *p,
		const struct crypto_bignum *x, int y_bit)
{
	mbedtls_mpi temp;
	mbedtls_mpi *y_sqr, *y;
	mbedtls_mpi_init(&temp);
	int ret = 0;

	y = &((mbedtls_ecp_point *)p)->Y;

	/* Faster way to find sqrt
	 * Works only with curves having prime p
	 * such that p ��E��cO 3 (mod 4)
	 *  y_ = (y2 ^ ((p+1)/4)) mod p
	 *
	 *  if LSB of both x and y are same: y = y_
	 *   else y = p - y_
	 * y_bit is LSB of x
	 */
	y_bit = (y_bit != 0);

	y_sqr = (mbedtls_mpi *) crypto_ec_point_compute_y_sqr(e, x);

	if (y_sqr) {

		MBEDTLS_MPI_CHK(mbedtls_mpi_add_int(&temp, &e->group.P, 1));
		MBEDTLS_MPI_CHK(mbedtls_mpi_div_int(&temp, NULL, &temp, 4));
		MBEDTLS_MPI_CHK(mbedtls_mpi_exp_mod(y, y_sqr, &temp, &e->group.P, NULL));

		if (y_bit != mbedtls_mpi_get_bit(y, 0))
			MBEDTLS_MPI_CHK(mbedtls_mpi_sub_mpi(y, &e->group.P, y));

		MBEDTLS_MPI_CHK(mbedtls_mpi_copy(&((mbedtls_ecp_point* )p)->X, (const mbedtls_mpi*) x));
		MBEDTLS_MPI_CHK(mbedtls_mpi_lset(&((mbedtls_ecp_point *)p)->Z, 1));
	} else {
		ret = 1;
	}
cleanup:
	mbedtls_mpi_free(&temp);
	mbedtls_mpi_free(y_sqr);
	os_free(y_sqr);
	return ret ? -1 : 0;
}

int crypto_ec_point_is_at_infinity(struct crypto_ec *e,
		const struct crypto_ec_point *p)
{
	return mbedtls_ecp_is_zero((mbedtls_ecp_point *) p);
}
        
int crypto_ec_point_is_on_curve(struct crypto_ec *e,
		const struct crypto_ec_point *p)
{
	mbedtls_mpi y_sqr_lhs, *y_sqr_rhs = NULL, two;
	int ret = 0, on_curve = 0;

	mbedtls_mpi_init(&y_sqr_lhs);
	mbedtls_mpi_init(&two);

	/* Calculate y^2  mod P*/
	MBEDTLS_MPI_CHK(mbedtls_mpi_lset(&two, 2));
	MBEDTLS_MPI_CHK(mbedtls_mpi_exp_mod(&y_sqr_lhs, &((const mbedtls_ecp_point *)p)->Y , &two, &e->group.P, NULL));

	y_sqr_rhs = (mbedtls_mpi *) crypto_ec_point_compute_y_sqr(e, (const struct crypto_bignum *) & ((const mbedtls_ecp_point *)p)->X);

	if (y_sqr_rhs && (mbedtls_mpi_cmp_mpi(y_sqr_rhs, &y_sqr_lhs) == 0)) {
		on_curve = 1;
	}

cleanup:
	mbedtls_mpi_free(&y_sqr_lhs);
	mbedtls_mpi_free(&two);
	mbedtls_mpi_free(y_sqr_rhs);
	os_free(y_sqr_rhs);
	return (ret == 0) && (on_curve == 1);
}

struct crypto_bignum *crypto_bignum_init(void)
{
    mbedtls_mpi *bn = os_zalloc(sizeof(mbedtls_mpi));
    if (bn == NULL) {
        return NULL;
    }

    mbedtls_mpi_init(bn);

    return (struct crypto_bignum *)bn;
}

struct crypto_bignum *crypto_bignum_init_set(const u8 *buf, size_t len)
{
	int ret = 0;
	mbedtls_mpi *bn = os_zalloc(sizeof(mbedtls_mpi));
	if (bn == NULL) {
		return NULL;
	}

	MBEDTLS_MPI_CHK(mbedtls_mpi_read_binary(bn, buf, len));
	return (struct crypto_bignum *) bn;

cleanup:
	os_free(bn);
	return NULL;
}

void crypto_bignum_deinit(struct crypto_bignum *n, int clear)
{
    mbedtls_mpi_free((mbedtls_mpi *)n);
    os_free((mbedtls_mpi *)n);
}

int crypto_bignum_to_bin(const struct crypto_bignum *a,
                         u8 *buf, size_t buflen, size_t padlen)
{
    int num_bytes, offset;

    if (padlen > buflen) {
        return -1;
    }

    num_bytes = mbedtls_mpi_size((mbedtls_mpi *) a);

    if ((size_t) num_bytes > buflen) {
        return -1;
    }
    if (padlen > (size_t) num_bytes) {
        offset = padlen - num_bytes;
    } else {
        offset = 0;
    }

    os_memset(buf, 0, offset);
    mbedtls_mpi_write_binary((mbedtls_mpi *) a, buf + offset, mbedtls_mpi_size((mbedtls_mpi *)a) );

    return num_bytes + offset;
}

int crypto_bignum_rand(struct crypto_bignum *r, const struct crypto_bignum *m)
{
    return 0;
}

int crypto_bignum_add(const struct crypto_bignum *a,
                      const struct crypto_bignum *b,
                      struct crypto_bignum *c)
{
    return mbedtls_mpi_add_mpi((mbedtls_mpi *) c, (const mbedtls_mpi *) a, (const mbedtls_mpi *) b) ?
           -1 : 0;
}


int crypto_bignum_mod(const struct crypto_bignum *a,
                      const struct crypto_bignum *b,
                      struct crypto_bignum *c)
{
    return mbedtls_mpi_mod_mpi((mbedtls_mpi *) c, (const mbedtls_mpi *) a, (const mbedtls_mpi *) b) ? -1 : 0;
}

int crypto_bignum_sub(const struct crypto_bignum *a,
                      const struct crypto_bignum *b,
                      struct crypto_bignum *c)
{
    return mbedtls_mpi_sub_mpi((mbedtls_mpi *) c, (const mbedtls_mpi *) a, (const mbedtls_mpi *) b) ?
           -1 : 0;
}

int crypto_bignum_div(const struct crypto_bignum *a,
                      const struct crypto_bignum *b,
                      struct crypto_bignum *c)
{
    return mbedtls_mpi_div_mpi((mbedtls_mpi *) c, NULL, (const mbedtls_mpi *) a, (const mbedtls_mpi *) b) ?
           -1 : 0;
}

int crypto_bignum_mulmod(const struct crypto_bignum *a,
                         const struct crypto_bignum *b,
                         const struct crypto_bignum *c,
                         struct crypto_bignum *d)
{
    int res;
#if ALLOW_EVEN_MOD || !CONFIG_MBEDTLS_HARDWARE_MPI // Must enable ALLOW_EVEN_MOD if c is even
    mbedtls_mpi temp;
    mbedtls_mpi_init(&temp);

    res = mbedtls_mpi_mul_mpi(&temp, (const mbedtls_mpi *) a, (const mbedtls_mpi *) b);
    if (res) {
        return -1;
    }

    res = mbedtls_mpi_mod_mpi((mbedtls_mpi *) d, &temp, (mbedtls_mpi *) c);

    mbedtls_mpi_free(&temp);
#else
    // Works with odd modulus only, but it is faster with HW acceleration
    res = esp_mpi_mul_mpi_mod((mbedtls_mpi *) d, (mbedtls_mpi *) a, (mbedtls_mpi *) b, (mbedtls_mpi *) c);
#endif
    return res ? -1 : 0;
}

int crypto_bignum_cmp(const struct crypto_bignum *a,
                      const struct crypto_bignum *b)
{
    return mbedtls_mpi_cmp_mpi((const mbedtls_mpi *) a, (const mbedtls_mpi *) b);
}

int crypto_bignum_exptmod(const struct crypto_bignum *a,
                          const struct crypto_bignum *b,
                          const struct crypto_bignum *c,
                          struct crypto_bignum *d)
{
    return  mbedtls_mpi_exp_mod((mbedtls_mpi *) d, (const mbedtls_mpi *) a, (const mbedtls_mpi *) b, (const mbedtls_mpi *) c, NULL) ? -1 : 0;

}

int crypto_bignum_legendre(const struct crypto_bignum *a,
                           const struct crypto_bignum *p)
{
    mbedtls_mpi exp, tmp;
    int res = -2, ret;

    mbedtls_mpi_init(&exp);
    mbedtls_mpi_init(&tmp);

    /* exp = (p-1) / 2 */
    MBEDTLS_MPI_CHK(mbedtls_mpi_sub_int(&exp, (const mbedtls_mpi *) p, 1));
    MBEDTLS_MPI_CHK(mbedtls_mpi_shift_r(&exp, 1));
    MBEDTLS_MPI_CHK(mbedtls_mpi_exp_mod(&tmp, (const mbedtls_mpi *) a, &exp, (const mbedtls_mpi *) p, NULL));

    if (mbedtls_mpi_cmp_int(&tmp, 1) == 0) {
        res = 1;
    } else if (mbedtls_mpi_cmp_int(&tmp, 0) == 0
            /* The below check is workaround for the case where HW
             * does not behave properly for X ^ A mod M when X is
             * power of M. Instead of returning value 0, value M is
             * returned.*/
            || mbedtls_mpi_cmp_mpi(&tmp, (const mbedtls_mpi *)p) == 0) {
        res = 0;
    } else {
        res = -1;
    }

cleanup:
    mbedtls_mpi_free(&tmp);
    mbedtls_mpi_free(&exp);
    return res;
}

int crypto_bignum_inverse(const struct crypto_bignum *a,
                          const struct crypto_bignum *b,
                          struct crypto_bignum *c)
{
    return mbedtls_mpi_inv_mod((mbedtls_mpi *) c, (const mbedtls_mpi *) a,
                               (const mbedtls_mpi *) b) ? -1 : 0;
}

int crypto_bignum_is_odd(const struct crypto_bignum *a)
{
    return 1;
}

int crypto_bignum_is_one(const struct crypto_bignum *a)
{
    return (mbedtls_mpi_cmp_int((const mbedtls_mpi *) a, 1) == 0);
}
           
int crypto_bignum_is_zero(const struct crypto_bignum *a)
{
    return (mbedtls_mpi_cmp_int((const mbedtls_mpi *) a, 0) == 0);
}

int crypto_bignum_bits(const struct crypto_bignum *a)
{
    return mbedtls_mpi_bitlen((const mbedtls_mpi *) a);
}

struct crypto_ecdh * crypto_ecdh_init(int group)
{
	struct crypto_ecdh *ecdh;
	unsigned char buf[1000];
	size_t olen;

	mbedtls_ecp_group_id grp_id = MBEDTLS_ECP_DP_NONE;

	mbedtls_entropy_context entropy;
	mbedtls_ctr_drbg_context ctr_drbg;

#if DEBUG_ECC_PROCESS_TIME //For Debug
	unsigned int ptime, ctime;
	ptime = TSF;
	_wpa_msg_crypto(0, MSG_DEBUG, TAG "[Enter] crypto_ecdh_init");
#endif

	//INIT RND
	mbedtls_entropy_init(&entropy);
	mbedtls_ctr_drbg_init(&ctr_drbg);

	mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, NULL, 0);

	//Set Group ID
	grp_id = get_ecp_group_id(group);
	
	//INIT ECDH 
	ecdh = os_zalloc(sizeof(*ecdh));
	mbedtls_ecdh_init(&ecdh->ctx);
	if(mbedtls_ecdh_setup(&ecdh->ctx, grp_id) != 0)
	{
		wpa_msg(0, MSG_DEBUG, "OWE : Fail to setup ECDH\n");
		crypto_ecdh_deinit(ecdh);
		goto clean;
	}

	if(mbedtls_ecdh_make_public( &ecdh->ctx, &olen, buf, 1000,
                                  mbedtls_ctr_drbg_random, &ctr_drbg ) != 0)
	{
		wpa_msg(0, MSG_DEBUG, "OWE : Fail to make public key\n");
		crypto_ecdh_deinit(ecdh);
		goto clean;
	}

	// wpa_hexdump(MSG_DEBUG, "PUBLIC BUF", buf, olen);
	// bignum_print("PUBLIC RES-X : ", &ecdh->ctx.Q.X);
	// bignum_print("PUBLIC RES-Y : ", &ecdh->ctx.Q.Y);
	// bignum_print("PUBLIC RES-Z : ", &ecdh->ctx.Q.Z);

clean:

#if DEBUG_ECC_PROCESS_TIME //For Debug
	ctime = TSF;
	_wpa_msg_crypto(0, MSG_DEBUG, TAG "[Exit ] crypto_ecdh_init (elapsed time: %u us)",(ctime -ptime));
#endif

	mbedtls_ctr_drbg_free(&ctr_drbg);
	mbedtls_entropy_free(&entropy);

	return ecdh;

}

void crypto_ecdh_deinit(struct crypto_ecdh *ecdh)
{
	if(ecdh)
	{
		mbedtls_ecdh_free(&ecdh->ctx);
		os_free(ecdh);
	}
}

struct wpabuf * crypto_ecdh_get_pubkey(struct crypto_ecdh *ecdh, int inc_y)
{
    struct wpabuf *buf = NULL;
	int len = ecdh->ctx.Q.X.n * 2 + 2;
	size_t olen = 0;

	buf = wpabuf_alloc(len);
	
	if(mbedtls_mpi_write_binary(&ecdh->ctx.Q.X, wpabuf_put(buf, len), len) != 0)
	{
		wpa_msg(0, MSG_DEBUG, "OWE : Fail to get public-key\n");
		if(buf)
			wpabuf_free(buf);
		return NULL;
	}

	wpa_msg(0, MSG_DEBUG, "OWE : ECDH key length : %d\n", len);
	// bignum_print("Public Bignum", &ecdh->ctx.Q.X);
	wpa_hexdump(MSG_DEBUG, "OWE : Public Key", wpabuf_head(buf), olen);

    return buf;
}

struct wpabuf * crypto_ecdh_set_peerkey(struct crypto_ecdh *ecdh, int inc_y,
					const u8 *key, size_t len)
{
	struct wpabuf *secret = NULL;
	unsigned char res_buf[1000];
	size_t res_len;
	int res = 0;

	mbedtls_entropy_context entropy;
	mbedtls_ctr_drbg_context ctr_drbg;

#if DEBUG_ECC_PROCESS_TIME //For Debug
	unsigned int ptime, ctime;
	ptime = TSF;
	_wpa_msg_crypto(0, MSG_DEBUG, TAG "[Enter] crypto_ecdh_set_peerkey");
#endif

	//INIT RND
	mbedtls_entropy_init(&entropy);
	mbedtls_ctr_drbg_init(&ctr_drbg);

	mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, NULL, 0);

	mbedtls_ecp_point *peerKey;
	peerKey = &ecdh->ctx.Qp;

	if(mbedtls_mpi_read_binary(&peerKey->X, key, len) != 0)
	{
		wpa_msg(0, MSG_DEBUG, "OWE : [%s, %d] Fail to read peer public-key(point X)", __func__, __LINE__);
		goto clean;
	}

	if(mbedtls_mpi_lset(&peerKey->Z, 1) != 0)
	{
		wpa_msg(0, MSG_DEBUG, "OWE : [%s, %d] Fail to Set bit in public-key(point Z)", __func__, __LINE__);
		goto clean;
	}

	if(crypto_ec_point_solve_y_coord((void *)ecdh, (void *) peerKey, (void *) &peerKey->X, 0))
	{
		wpa_msg(0, MSG_DEBUG, "OWE : [%s, %d] Fail to calc Y using X(y^2 = x^3 + ax + b  mod  P)", __func__, __LINE__);
		goto clean;
	}

	// bignum_print("Public  Key : ", &ecdh->ctx.Q.X);
	// bignum_print("Private Key : ", &ecdh->ctx.d);
	// bignum_print("pr Public X Key : ", &ecdh->ctx.Qp.X);
	// bignum_print("pr Public Y Key : ", &ecdh->ctx.Qp.Y);
	// bignum_print("pr Public Z Key : ", &ecdh->ctx.Qp.Z);

	if(mbedtls_ecdh_calc_secret( &ecdh->ctx, &res_len, res_buf, 1000,
                                           mbedtls_ctr_drbg_random, &ctr_drbg ) != 0)
	{
		wpa_msg(0, MSG_DEBUG, "OWE : [%s, %d] Fail to calc secret", __func__, __LINE__);
		goto clean;
	}

	wpa_hexdump(MSG_DEBUG, "OWE : SHARED KEY", res_buf, res_len);

	ecdh->length = res_len;

	secret = wpabuf_alloc(ecdh->length);
	if (secret == NULL) {
		wpa_msg(0, MSG_DEBUG, "OWE : [%s, %d] Fail to alloc buffer", __func__, __LINE__);
		return NULL;
	}

	// mbedtls_mpi_write_binary(&ecdh->ctx.z, secret->buf, ecdh->length);
	os_memcpy(wpabuf_put(secret, ecdh->length), res_buf, ecdh->length);

clean:

#if DEBUG_ECC_PROCESS_TIME //For Debug
	ctime = TSF;
	_wpa_msg_crypto(0, MSG_DEBUG, TAG "[Exit ] crypto_ecdh_set_peerkey (elapsed time: %u us)",(ctime -ptime));
#endif

	mbedtls_ctr_drbg_free(&ctr_drbg);
	mbedtls_entropy_free(&entropy);

	return secret;
}

#if 0
int omac1_aes_128(const u8 *key, const u8 *data, size_t data_len, u8 *mac)
{
	return 0;
}
#endif

#endif //#if (defined (CONFIG_SAE) || defined (CONFIG_OWE))
