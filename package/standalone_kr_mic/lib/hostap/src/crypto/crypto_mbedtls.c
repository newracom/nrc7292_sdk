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
#include "mbedtls/sha256.h"
#include "mbedtls/pkcs5.h"

#include "mbedtls/aes.h"
#include "mbedtls/rsa.h"
#include "mbedtls/arc4.h"

#include "system.h"

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
	mbedtls_mpi mpi_base;
	mbedtls_mpi mpi_power;
	mbedtls_mpi mpi_modul;
	mbedtls_mpi mpi_result;

	mbedtls_mpi_init(&mpi_base);
	mbedtls_mpi_init(&mpi_power);
	mbedtls_mpi_init(&mpi_modul);
	mbedtls_mpi_init(&mpi_result);

	mbedtls_mpi_read_binary(&mpi_base, base, base_len);
	mbedtls_mpi_read_binary(&mpi_power, power, power_len);
	mbedtls_mpi_read_binary(&mpi_modul, modulus, modulus_len);

	if (mbedtls_mpi_exp_mod(&mpi_result, &mpi_base, &mpi_power,
				&mpi_modul, NULL) != 0)
		return -1;

	*result_len = mbedtls_mpi_size(&mpi_result);
	if (mbedtls_mpi_write_binary(&mpi_result, result, *result_len) != 0)
		return -1;

	mbedtls_mpi_free(&mpi_base);
	mbedtls_mpi_free(&mpi_power);
	mbedtls_mpi_free(&mpi_modul);
	mbedtls_mpi_free(&mpi_result);

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
