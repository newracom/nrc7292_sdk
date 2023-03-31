/*
 * Simultaneous authentication of equals
 * Copyright (c) 2012-2016, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#include "includes.h"

#include "common.h"
#include "utils/const_time.h"
#include "crypto/crypto.h"
#include "crypto/sha256.h"
#include "crypto/random.h"
#include "crypto/dh_groups.h"
#include "ieee802_11_defs.h"
#include "dragonfly.h"
#include "sae.h"

#ifdef MBEDTLS_SELF_TEST
#include "system.h"
#include "system_common.h"
#endif /* MBEDTLS_SELF_TEST */

int sae_set_group(struct sae_data *sae, int group)
{
	struct sae_temporary_data *tmp;

	sae_clear_data(sae);
	tmp = sae->tmp = os_zalloc(sizeof(*tmp));
	if (tmp == NULL)
		return -1;

	/* First, check if this is an ECC group */
	tmp->ec = crypto_ec_init(group);
	if (tmp->ec) {
		wpa_printf(MSG_DEBUG, "SAE: Selecting supported ECC group %d",
			   group);
		sae->group = group;
		tmp->prime_len = crypto_ec_prime_len(tmp->ec);
		tmp->prime = crypto_ec_get_prime(tmp->ec);
		tmp->order_len = crypto_ec_order_len(tmp->ec);
		tmp->order = crypto_ec_get_order(tmp->ec);
		return 0;
	}

	/* Not an ECC group, check FFC */
	tmp->dh = dh_groups_get(group);
	if (tmp->dh) {
		wpa_printf(MSG_DEBUG, "SAE: Selecting supported FFC group %d",
			   group);
		sae->group = group;
		tmp->prime_len = tmp->dh->prime_len;
		if (tmp->prime_len > SAE_MAX_PRIME_LEN) {
			sae_clear_data(sae);
			return -1;
		}

		tmp->prime_buf = crypto_bignum_init_set(tmp->dh->prime,
							tmp->prime_len);
		if (tmp->prime_buf == NULL) {
			sae_clear_data(sae);
			return -1;
		}
		tmp->prime = tmp->prime_buf;

		tmp->order_len = tmp->dh->order_len;
		tmp->order_buf = crypto_bignum_init_set(tmp->dh->order,
							tmp->dh->order_len);
		if (tmp->order_buf == NULL) {
			sae_clear_data(sae);
			return -1;
		}
		tmp->order = tmp->order_buf;

		return 0;
	}

	/* Unsupported group */
	wpa_printf(MSG_DEBUG,
		   "SAE: Group %d not supported by the crypto library", group);
	return -1;
}


void sae_clear_temp_data(struct sae_data *sae)
{
	struct sae_temporary_data *tmp;
	if (sae == NULL || sae->tmp == NULL)
		return;
	tmp = sae->tmp;
	crypto_ec_deinit(tmp->ec);
	crypto_bignum_deinit(tmp->prime_buf, 0);
	crypto_bignum_deinit(tmp->order_buf, 0);
	crypto_bignum_deinit(tmp->sae_rand, 1);
	crypto_bignum_deinit(tmp->pwe_ffc, 1);
	crypto_bignum_deinit(tmp->own_commit_scalar, 0);
	crypto_bignum_deinit(tmp->own_commit_element_ffc, 0);
	crypto_bignum_deinit(tmp->peer_commit_element_ffc, 0);
	crypto_ec_point_deinit(tmp->pwe_ecc, 1);
	crypto_ec_point_deinit(tmp->own_commit_element_ecc, 0);
	crypto_ec_point_deinit(tmp->peer_commit_element_ecc, 0);
	wpabuf_free(tmp->anti_clogging_token);
	os_free(tmp->pw_id);
	bin_clear_free(tmp, sizeof(*tmp));
	sae->tmp = NULL;
}


void sae_clear_data(struct sae_data *sae)
{
	if (sae == NULL)
		return;
	sae_clear_temp_data(sae);
	crypto_bignum_deinit(sae->peer_commit_scalar, 0);
	os_memset(sae, 0, sizeof(*sae));
}

static struct crypto_bignum * sae_get_rand(struct sae_data *sae)
{
	u8 val[SAE_MAX_PRIME_LEN];
	int iter = 0;
	struct crypto_bignum *bn = NULL;
	int order_len_bits = crypto_bignum_bits(sae->tmp->order);
	size_t order_len = (order_len_bits + 7) / 8;

	if (order_len > sizeof(val))
		return NULL;

	for (;;) {
		if (iter++ > 100 || random_get_bytes(val, order_len) < 0)
			return NULL;
		if (order_len_bits % 8)
			buf_shift_right(val, order_len, 8 - order_len_bits % 8);
		bn = crypto_bignum_init_set(val, order_len);
		if (bn == NULL)
			return NULL;
		if (crypto_bignum_is_zero(bn) ||
		    crypto_bignum_is_one(bn) ||
		    crypto_bignum_cmp(bn, sae->tmp->order) >= 0) {
			crypto_bignum_deinit(bn, 0);
			continue;
		}
		break;
	}

	os_memset(val, 0, order_len);
	return bn;
}

static struct crypto_bignum * sae_get_rand_and_mask(struct sae_data *sae)
{
	crypto_bignum_deinit(sae->tmp->sae_rand, 1);
	sae->tmp->sae_rand = sae_get_rand(sae);
	if (sae->tmp->sae_rand == NULL)
		return NULL;
	return sae_get_rand(sae);
}

static void sae_pwd_seed_key(const u8 *addr1, const u8 *addr2, u8 *key)
{
	wpa_printf(MSG_DEBUG, "SAE: PWE derivation - addr1=" MACSTR
		   " addr2=" MACSTR, MAC2STR(addr1), MAC2STR(addr2));
	if (os_memcmp(addr1, addr2, ETH_ALEN) > 0) {
		os_memcpy(key, addr1, ETH_ALEN);
		os_memcpy(key + ETH_ALEN, addr2, ETH_ALEN);
	} else {
		os_memcpy(key, addr2, ETH_ALEN);
		os_memcpy(key + ETH_ALEN, addr1, ETH_ALEN);
	}
}

static struct crypto_bignum *
get_rand_1_to_p_1(const u8 *prime, size_t prime_len, size_t prime_bits,
		  int *r_odd)
{
	for (;;) {
		struct crypto_bignum *r;
		u8 tmp[SAE_MAX_ECC_PRIME_LEN];

		if (random_get_bytes(tmp, prime_len) < 0)
			break;
		if (prime_bits % 8)
			buf_shift_right(tmp, prime_len, 8 - prime_bits % 8);
		if (os_memcmp(tmp, prime, prime_len) >= 0)
			continue;
		r = crypto_bignum_init_set(tmp, prime_len);
		if (!r)
			break;
		if (crypto_bignum_is_zero(r)) {
			crypto_bignum_deinit(r, 0);
			continue;
		}

		*r_odd = tmp[prime_len - 1] & 0x01;
		return r;
	}

	return NULL;
}

static int is_quadratic_residue_blind(struct sae_data *sae,
				      const u8 *prime, size_t bits,
				      const struct crypto_bignum *qr,
				      const struct crypto_bignum *qnr,
				      const struct crypto_bignum *y_sqr)
{
	struct crypto_bignum *r, *num;
	int r_odd, check, res = -1;

	/*
	 * Use the blinding technique to mask y_sqr while determining
	 * whether it is a quadratic residue modulo p to avoid leaking
	 * timing information while determining the Legendre symbol.
	 *
	 * v = y_sqr
	 * r = a random number between 1 and p-1, inclusive
	 * num = (v * r * r) modulo p
	 */
	r = get_rand_1_to_p_1(prime, sae->tmp->prime_len, bits, &r_odd);
	if (!r)
		return -1;

	num = crypto_bignum_init();
	if (!num ||
	    crypto_bignum_mulmod(y_sqr, r, sae->tmp->prime, num) < 0 ||
	    crypto_bignum_mulmod(num, r, sae->tmp->prime, num) < 0)
		goto fail;

	if (r_odd) {
		/*
		 * num = (num * qr) module p
		 * LGR(num, p) = 1 ==> quadratic residue
		 */
		if (crypto_bignum_mulmod(num, qr, sae->tmp->prime, num) < 0)
			goto fail;
		check = 1;
	} else {
		/*
		 * num = (num * qnr) module p
		 * LGR(num, p) = -1 ==> quadratic residue
		 */
		if (crypto_bignum_mulmod(num, qnr, sae->tmp->prime, num) < 0)
			goto fail;
		check = -1;
	}

	res = crypto_bignum_legendre(num, sae->tmp->prime);
	if (res == -2) {
		res = -1;
		goto fail;
	}
	res = res == check;
fail:
	crypto_bignum_deinit(num, 1);
	crypto_bignum_deinit(r, 1);
	return res;
}

static int sae_test_pwd_seed_ecc(struct sae_data *sae, const u8 *pwd_seed,
				 const u8 *prime,
				 const struct crypto_bignum *qr,
				 const struct crypto_bignum *qnr,
				 struct crypto_bignum **ret_x_cand)
{
	u8 pwd_value[SAE_MAX_ECC_PRIME_LEN];
	struct crypto_bignum *y_sqr, *x_cand;
	int res;
	size_t bits;

	*ret_x_cand = NULL;

	wpa_hexdump_key(MSG_DEBUG, "SAE: pwd-seed", pwd_seed, SHA256_MAC_LEN);

	/* pwd-value = KDF-z(pwd-seed, "SAE Hunting and Pecking", p) */
	bits = crypto_ec_prime_len_bits(sae->tmp->ec);
	if (sha256_prf_bits(pwd_seed, SHA256_MAC_LEN, "SAE Hunting and Pecking",
			    prime, sae->tmp->prime_len, pwd_value, bits) < 0)
		return -1;
	if (bits % 8)
		buf_shift_right(pwd_value, sizeof(pwd_value), 8 - bits % 8);
	wpa_hexdump_key(MSG_DEBUG, "SAE: pwd-value",
			pwd_value, sae->tmp->prime_len);

	if (os_memcmp(pwd_value, prime, sae->tmp->prime_len) >= 0)
		return 0;

	x_cand = crypto_bignum_init_set(pwd_value, sae->tmp->prime_len);
	if (!x_cand)
		return -1;
	y_sqr = crypto_ec_point_compute_y_sqr(sae->tmp->ec, x_cand);
	if (!y_sqr) {
		crypto_bignum_deinit(x_cand, 1);
		return -1;
	}

	res = is_quadratic_residue_blind(sae, prime, bits, qr, qnr, y_sqr);
	crypto_bignum_deinit(y_sqr, 1);
	if (res <= 0) {
		crypto_bignum_deinit(x_cand, 1);
		return res;
	}

	*ret_x_cand = x_cand;
	return 1;
}

static int sae_test_pwd_seed_ffc(struct sae_data *sae, const u8 *pwd_seed,
				 struct crypto_bignum *pwe)
{
	u8 pwd_value[SAE_MAX_PRIME_LEN];
	size_t bits = sae->tmp->prime_len * 8;
	u8 exp[1];
	struct crypto_bignum *a, *b;
	int res;

	wpa_hexdump_key(MSG_DEBUG, "SAE: pwd-seed", pwd_seed, SHA256_MAC_LEN);

	/* pwd-value = KDF-z(pwd-seed, "SAE Hunting and Pecking", p) */
	if (sha256_prf_bits(pwd_seed, SHA256_MAC_LEN, "SAE Hunting and Pecking",
			    sae->tmp->dh->prime, sae->tmp->prime_len, pwd_value,
			    bits) < 0)
		return -1;
	wpa_hexdump_key(MSG_DEBUG, "SAE: pwd-value", pwd_value,
			sae->tmp->prime_len);

	if (os_memcmp(pwd_value, sae->tmp->dh->prime, sae->tmp->prime_len) >= 0)
	{
		wpa_printf(MSG_DEBUG, "SAE: pwd-value >= p");
		return 0;
	}

	/* PWE = pwd-value^((p-1)/r) modulo p */

	a = crypto_bignum_init_set(pwd_value, sae->tmp->prime_len);

	if (sae->tmp->dh->safe_prime) {
		/*
		 * r = (p-1)/2 for the group used here, so this becomes:
		 * PWE = pwd-value^2 modulo p
		 */
		exp[0] = 2;
		b = crypto_bignum_init_set(exp, sizeof(exp));
	} else {
		/* Calculate exponent: (p-1)/r */
		exp[0] = 1;
		b = crypto_bignum_init_set(exp, sizeof(exp));
		if (b == NULL ||
		    crypto_bignum_sub(sae->tmp->prime, b, b) < 0 ||
		    crypto_bignum_div(b, sae->tmp->order, b) < 0) {
			crypto_bignum_deinit(b, 0);
			b = NULL;
		}
	}

	if (a == NULL || b == NULL)
		res = -1;
	else
		res = crypto_bignum_exptmod(a, b, sae->tmp->prime, pwe);

	crypto_bignum_deinit(a, 0);
	crypto_bignum_deinit(b, 0);

	if (res < 0) {
		wpa_printf(MSG_DEBUG, "SAE: Failed to calculate PWE");
		return -1;
	}

	/* if (PWE > 1) --> found */
	if (crypto_bignum_is_zero(pwe) || crypto_bignum_is_one(pwe)) {
		wpa_printf(MSG_DEBUG, "SAE: PWE <= 1");
		return 0;
	}

	wpa_printf(MSG_DEBUG, "SAE: PWE found");
	return 1;
}

static int get_random_qr_qnr(const u8 *prime, size_t prime_len,
			     const struct crypto_bignum *prime_bn,
			     size_t prime_bits, struct crypto_bignum **qr,
			     struct crypto_bignum **qnr)
{
	*qr = NULL;
	*qnr = NULL;

	while (!(*qr) || !(*qnr)) {
		u8 tmp[SAE_MAX_ECC_PRIME_LEN];
		struct crypto_bignum *q;
		int res;

		if (random_get_bytes(tmp, prime_len) < 0)
			break;
		if (prime_bits % 8)
			buf_shift_right(tmp, prime_len, 8 - prime_bits % 8);

		if (os_memcmp(tmp, prime, prime_len) >= 0)
			continue;
		q = crypto_bignum_init_set(tmp, prime_len);
		if (!q)
			break;
		res = crypto_bignum_legendre(q, prime_bn);

		if (res == 1 && !(*qr))
			*qr = q;
		else if (res == -1 && !(*qnr))
			*qnr = q;
		else
			crypto_bignum_deinit(q, 0);
	}

	return (*qr && *qnr) ? 0 : -1;
}

static int sae_derive_pwe_ecc(struct sae_data *sae, const u8 *addr1,
			      const u8 *addr2, const u8 *password,
			      size_t password_len, const char *identifier)
{
	u8 counter, k = 40;
	u8 addrs[2 * ETH_ALEN];
	const u8 *addr[3];
	size_t len[3];
	size_t num_elem;
	u8 dummy_password[32];
	size_t dummy_password_len;
	int pwd_seed_odd = 0;
	u8 prime[SAE_MAX_ECC_PRIME_LEN];
	size_t prime_len;
	struct crypto_bignum *x = NULL, *qr, *qnr;
	size_t bits;
	int res;

	dummy_password_len = password_len;
	if (dummy_password_len > sizeof(dummy_password))
		dummy_password_len = sizeof(dummy_password);
	
	if (random_get_bytes(dummy_password, dummy_password_len) < 0)
		return -1;

	prime_len = sae->tmp->prime_len;
	if (crypto_bignum_to_bin(sae->tmp->prime, prime, sizeof(prime),
				 prime_len) < 0)
		return -1;
	bits = crypto_ec_prime_len_bits(sae->tmp->ec);
	/*
	 * Create a random quadratic residue (qr) and quadratic non-residue
	 * (qnr) modulo p for blinding purposes during the loop.
	 */
	if (get_random_qr_qnr(prime, prime_len, sae->tmp->prime, bits,
			      &qr, &qnr) < 0)
		return -1;
	
	wpa_hexdump_ascii_key(MSG_DEBUG, "SAE: password",
			      password, password_len);
	if (identifier)
		wpa_printf(MSG_DEBUG, "SAE: password identifier: %s",
			   identifier);

	/*
	 * H(salt, ikm) = HMAC-SHA256(salt, ikm)
	 * base = password [|| identifier]
	 * pwd-seed = H(MAX(STA-A-MAC, STA-B-MAC) || MIN(STA-A-MAC, STA-B-MAC),
	 *              base || counter)
	 */
	sae_pwd_seed_key(addr1, addr2, addrs);

	addr[0] = password;
	len[0] = password_len;
	num_elem = 1;
	if (identifier) {
		addr[num_elem] = (const u8 *) identifier;
		len[num_elem] = os_strlen(identifier);
		num_elem++;
	}
	addr[num_elem] = &counter;
	len[num_elem] = sizeof(counter);
	num_elem++;

	/*
	 * Continue for at least k iterations to protect against side-channel
	 * attacks that attempt to determine the number of iterations required
	 * in the loop.
	 */
	for (counter = 1; counter <= k || !x; counter++) {
		u8 pwd_seed[SHA256_MAC_LEN];
		struct crypto_bignum *x_cand;

		if (counter > 200) {
			/* This should not happen in practice */
			wpa_printf(MSG_DEBUG, "SAE: Failed to derive PWE");
			break;
		}

		wpa_printf(MSG_DEBUG, "SAE: counter = %u", counter);
		if (hmac_sha256_vector(addrs, sizeof(addrs), num_elem,
				       addr, len, pwd_seed) < 0)
			break;

		res = sae_test_pwd_seed_ecc(sae, pwd_seed,
					    prime, qr, qnr, &x_cand);
		if (res < 0)
			goto fail;
		if (res > 0 && !x) {
			wpa_printf(MSG_DEBUG,
				   "SAE: Selected pwd-seed with counter %u",
				   counter);
			x = x_cand;
			pwd_seed_odd = pwd_seed[SHA256_MAC_LEN - 1] & 0x01;
			os_memset(pwd_seed, 0, sizeof(pwd_seed));

			/*
			 * Use a dummy password for the following rounds, if
			 * any.
			 */
			addr[0] = dummy_password;
			len[0] = dummy_password_len;
		} else if (res > 0) {
			crypto_bignum_deinit(x_cand, 1);
		}
	}

	if (!x) {
		wpa_printf(MSG_DEBUG, "SAE: Could not generate PWE");
		res = -1;
		goto fail;
	}

	if (!sae->tmp->pwe_ecc)
		sae->tmp->pwe_ecc = crypto_ec_point_init(sae->tmp->ec);
	if (!sae->tmp->pwe_ecc)
		res = -1;
	else
		res = crypto_ec_point_solve_y_coord(sae->tmp->ec,
						    sae->tmp->pwe_ecc, x,
						    pwd_seed_odd);
	crypto_bignum_deinit(x, 1);
	if (res < 0) {
		/*
		 * This should not happen since we already checked that there
		 * is a result.
		 */
		wpa_printf(MSG_DEBUG, "SAE: Could not solve y");
	}

fail:
	crypto_bignum_deinit(qr, 0);
	crypto_bignum_deinit(qnr, 0);

	return res;
}

static int sae_derive_pwe_ffc(struct sae_data *sae, const u8 *addr1,
			      const u8 *addr2, const u8 *password,
			      size_t password_len, const char *identifier)
{
	u8 counter;
	u8 addrs[2 * ETH_ALEN];
	const u8 *addr[3];
	size_t len[3];
	size_t num_elem;
	int found = 0;

	if (sae->tmp->pwe_ffc == NULL) {
		sae->tmp->pwe_ffc = crypto_bignum_init();
		if (sae->tmp->pwe_ffc == NULL)
			return -1;
	}

	wpa_hexdump_ascii_key(MSG_DEBUG, "SAE: password",
			      password, password_len);

	/*
	 * H(salt, ikm) = HMAC-SHA256(salt, ikm)
	 * pwd-seed = H(MAX(STA-A-MAC, STA-B-MAC) || MIN(STA-A-MAC, STA-B-MAC),
	 *              password [|| identifier] || counter)
	 */
	sae_pwd_seed_key(addr1, addr2, addrs);

	addr[0] = password;
	len[0] = password_len;
	num_elem = 1;
	if (identifier) {
		addr[num_elem] = (const u8 *) identifier;
		len[num_elem] = os_strlen(identifier);
		num_elem++;
	}
	addr[num_elem] = &counter;
	len[num_elem] = sizeof(counter);
	num_elem++;

	for (counter = 1; !found; counter++) {
		u8 pwd_seed[SHA256_MAC_LEN];
		int res;

		if (counter > 200) {
			/* This should not happen in practice */
			wpa_printf(MSG_DEBUG, "SAE: Failed to derive PWE");
			break;
		}

		wpa_printf(MSG_DEBUG, "SAE: counter = %u", counter);
		if (hmac_sha256_vector(addrs, sizeof(addrs), num_elem,
				       addr, len, pwd_seed) < 0)
			break;
		res = sae_test_pwd_seed_ffc(sae, pwd_seed, sae->tmp->pwe_ffc);
		if (res < 0)
			break;
		if (res > 0) {
			wpa_printf(MSG_DEBUG, "SAE: Use this PWE");
			found = 1;
		}
	}

	return found ? 0 : -1;
}

static int sae_derive_commit_element_ecc(struct sae_data *sae,
					 struct crypto_bignum *mask)
{
	/* COMMIT-ELEMENT = inverse(scalar-op(mask, PWE)) */
	if (!sae->tmp->own_commit_element_ecc) {
		sae->tmp->own_commit_element_ecc =
			crypto_ec_point_init(sae->tmp->ec);
		if (!sae->tmp->own_commit_element_ecc)
			return -1;
	}

	if (crypto_ec_point_mul(sae->tmp->ec, sae->tmp->pwe_ecc, mask,
				sae->tmp->own_commit_element_ecc) < 0 ||
	    crypto_ec_point_invert(sae->tmp->ec,
				   sae->tmp->own_commit_element_ecc) < 0) {
		wpa_printf(MSG_DEBUG, "SAE: Could not compute commit-element");
		return -1;
	}

	return 0;
}


static int sae_derive_commit_element_ffc(struct sae_data *sae,
					 struct crypto_bignum *mask)
{
	/* COMMIT-ELEMENT = inverse(scalar-op(mask, PWE)) */
	if (!sae->tmp->own_commit_element_ffc) {
		sae->tmp->own_commit_element_ffc = crypto_bignum_init();
		if (!sae->tmp->own_commit_element_ffc)
			return -1;
	}

	if (crypto_bignum_exptmod(sae->tmp->pwe_ffc, mask, sae->tmp->prime,
				  sae->tmp->own_commit_element_ffc) < 0 ||
	    crypto_bignum_inverse(sae->tmp->own_commit_element_ffc,
				  sae->tmp->prime,
				  sae->tmp->own_commit_element_ffc) < 0) {
		wpa_printf(MSG_DEBUG, "SAE: Could not compute commit-element");
		return -1;
	}

	return 0;
}

static int sae_derive_commit(struct sae_data *sae)
{
	struct crypto_bignum *mask = NULL;
	int ret = -1;
	unsigned int counter = 0;

	do {
		counter++;
		if (counter > 100) {
			/*
			 * This cannot really happen in practice if the random
			 * number generator is working. Anyway, to avoid even a
			 * theoretical infinite loop, break out after 100
			 * attemps.
			 */
			return -1;
		}

		if (mask) {
		    crypto_bignum_deinit(mask, 1);
		}
		mask = sae_get_rand_and_mask(sae);
		if (mask == NULL) {
			wpa_printf(MSG_DEBUG, "SAE: Could not get rand/mask");
			return -1;
		}

		/* commit-scalar = (rand + mask) modulo r */
		if (!sae->tmp->own_commit_scalar) {
			sae->tmp->own_commit_scalar = crypto_bignum_init();
			if (!sae->tmp->own_commit_scalar)
				goto fail;
		}
		crypto_bignum_add(sae->tmp->sae_rand, mask,
				  sae->tmp->own_commit_scalar);
		crypto_bignum_mod(sae->tmp->own_commit_scalar, sae->tmp->order,
				  sae->tmp->own_commit_scalar);
	} while (crypto_bignum_is_zero(sae->tmp->own_commit_scalar) ||
		 crypto_bignum_is_one(sae->tmp->own_commit_scalar));

	if ((sae->tmp->ec && sae_derive_commit_element_ecc(sae, mask) < 0) ||
	    (sae->tmp->dh && sae_derive_commit_element_ffc(sae, mask) < 0))
		goto fail;


	ret = 0;
fail:
	crypto_bignum_deinit(mask, 1);

	return ret;
}

int sae_prepare_commit(const u8 *addr1, const u8 *addr2,
		       const u8 *password, size_t password_len,
		       const char *identifier, struct sae_data *sae)
{
	if (sae->tmp == NULL ||
	    (sae->tmp->ec && sae_derive_pwe_ecc(sae, addr1, addr2, password,
						password_len,
						identifier) < 0) ||
	    (sae->tmp->dh && sae_derive_pwe_ffc(sae, addr1, addr2, password,
						password_len,
						identifier) < 0) ||
	    sae_derive_commit(sae) < 0)
		return -1;
	return 0;
}


static int sae_derive_k_ecc(struct sae_data *sae, u8 *k)
{
	struct crypto_ec_point *K;
	int ret = -1;

	K = crypto_ec_point_init(sae->tmp->ec);
	if (K == NULL)
		goto fail;

	/*
	 * K = scalar-op(rand, (elem-op(scalar-op(peer-commit-scalar, PWE),
	 *                                        PEER-COMMIT-ELEMENT)))
	 * If K is identity element (point-at-infinity), reject
	 * k = F(K) (= x coordinate)
	 */

	if (crypto_ec_point_mul(sae->tmp->ec, sae->tmp->pwe_ecc,
				sae->peer_commit_scalar, K) < 0 ||
	    crypto_ec_point_add(sae->tmp->ec, K,
				sae->tmp->peer_commit_element_ecc, K) < 0 ||
	    crypto_ec_point_mul(sae->tmp->ec, K, sae->tmp->sae_rand, K) < 0 ||
	    crypto_ec_point_is_at_infinity(sae->tmp->ec, K) ||
	    crypto_ec_point_to_bin(sae->tmp->ec, K, k, NULL) < 0) {
		wpa_printf(MSG_DEBUG, "SAE: Failed to calculate K and k");
		goto fail;
	}

	wpa_hexdump_key(MSG_DEBUG, "SAE: k", k, sae->tmp->prime_len);

	ret = 0;
fail:
	crypto_ec_point_deinit(K, 1);
	return ret;
}


static int sae_derive_k_ffc(struct sae_data *sae, u8 *k)
{
	struct crypto_bignum *K;
	int ret = -1;

	K = crypto_bignum_init();
	if (K == NULL)
		goto fail;

	/*
	 * K = scalar-op(rand, (elem-op(scalar-op(peer-commit-scalar, PWE),
	 *                                        PEER-COMMIT-ELEMENT)))
	 * If K is identity element (one), reject.
	 * k = F(K) (= x coordinate)
	 */

	if (crypto_bignum_exptmod(sae->tmp->pwe_ffc, sae->peer_commit_scalar,
				  sae->tmp->prime, K) < 0 ||
	    crypto_bignum_mulmod(K, sae->tmp->peer_commit_element_ffc,
				 sae->tmp->prime, K) < 0 ||
	    crypto_bignum_exptmod(K, sae->tmp->sae_rand, sae->tmp->prime, K) < 0
	    ||
	    crypto_bignum_is_one(K) ||
	    crypto_bignum_to_bin(K, k, SAE_MAX_PRIME_LEN, sae->tmp->prime_len) <
	    0) {
		wpa_printf(MSG_DEBUG, "SAE: Failed to calculate K and k");
		goto fail;
	}

	wpa_hexdump_key(MSG_DEBUG, "SAE: k", k, sae->tmp->prime_len);

	ret = 0;
fail:
	crypto_bignum_deinit(K, 1);
	return ret;
}

static int sae_derive_keys(struct sae_data *sae, const u8 *k)
{
	u8 null_key[SAE_KEYSEED_KEY_LEN], val[SAE_MAX_PRIME_LEN];
	u8 keyseed[SHA256_MAC_LEN];
	u8 keys[SAE_KCK_LEN + SAE_PMK_LEN];
	struct crypto_bignum *tmp;
	int ret = -1;

	tmp = crypto_bignum_init();
	if (tmp == NULL)
		goto fail;

	/* keyseed = H(<0>32, k)
	 * KCK || PMK = KDF-512(keyseed, "SAE KCK and PMK",
	 *                      (commit-scalar + peer-commit-scalar) modulo r)
	 * PMKID = L((commit-scalar + peer-commit-scalar) modulo r, 0, 128)
	 */

	os_memset(null_key, 0, sizeof(null_key));
	hmac_sha256(null_key, sizeof(null_key), k, sae->tmp->prime_len,
		    keyseed);
	wpa_hexdump_key(MSG_DEBUG, "SAE: keyseed", keyseed, sizeof(keyseed));

	crypto_bignum_add(sae->tmp->own_commit_scalar, sae->peer_commit_scalar,
			  tmp);
	crypto_bignum_mod(tmp, sae->tmp->order, tmp);
	crypto_bignum_to_bin(tmp, val, sizeof(val), sae->tmp->prime_len);
	wpa_hexdump(MSG_DEBUG, "SAE: PMKID", val, SAE_PMKID_LEN);
	if (sha256_prf(keyseed, sizeof(keyseed), "SAE KCK and PMK",
		       val, sae->tmp->prime_len, keys, sizeof(keys)) < 0)
		goto fail;
	os_memset(keyseed, 0, sizeof(keyseed));
	os_memcpy(sae->tmp->kck, keys, SAE_KCK_LEN);
	os_memcpy(sae->pmk, keys + SAE_KCK_LEN, SAE_PMK_LEN);
	os_memcpy(sae->pmkid, val, SAE_PMKID_LEN);
	os_memset(keys, 0, sizeof(keys));
	wpa_hexdump_key(MSG_DEBUG, "SAE: KCK", sae->tmp->kck, SAE_KCK_LEN);
	wpa_hexdump_key(MSG_DEBUG, "SAE: PMK", sae->pmk, SAE_PMK_LEN);

	ret = 0;
fail:
	crypto_bignum_deinit(tmp, 0);
	return ret;
}


int sae_process_commit(struct sae_data *sae)
{
	u8 k[SAE_MAX_PRIME_LEN];
	if (sae->tmp == NULL ||
	    (sae->tmp->ec && sae_derive_k_ecc(sae, k) < 0) ||
	    (sae->tmp->dh && sae_derive_k_ffc(sae, k) < 0) ||
	    sae_derive_keys(sae, k) < 0)
		return -1;
	return 0;
}


void sae_write_commit(struct sae_data *sae, struct wpabuf *buf,
		      const struct wpabuf *token, const char *identifier)
{
	u8 *pos;

	if (sae->tmp == NULL)
		return;

	wpabuf_put_le16(buf, sae->group); /* Finite Cyclic Group */
	if (token) {
		wpabuf_put_buf(buf, token);
		wpa_hexdump(MSG_DEBUG, "SAE: Anti-clogging token",
			    wpabuf_head(token), wpabuf_len(token));
	}
	pos = wpabuf_put(buf, sae->tmp->prime_len);
	if (crypto_bignum_to_bin(sae->tmp->own_commit_scalar, pos,
				 sae->tmp->prime_len, sae->tmp->prime_len) < 0) {
		wpa_printf(MSG_ERROR, "SAE: failed bignum operation on own commit scalar");
		return;
	}
	wpa_hexdump(MSG_DEBUG, "SAE: own commit-scalar",
		    pos, sae->tmp->prime_len);
	if (sae->tmp->ec) {
		pos = wpabuf_put(buf, 2 * sae->tmp->prime_len);
		if (crypto_ec_point_to_bin(sae->tmp->ec,
					   sae->tmp->own_commit_element_ecc,
					   pos, pos + sae->tmp->prime_len) < 0) {
			wpa_printf(MSG_ERROR, "SAE: failed bignum op while deriving ec point");
			return;
		}
		wpa_hexdump(MSG_DEBUG, "SAE: own commit-element(x)",
			    pos, sae->tmp->prime_len);
		wpa_hexdump(MSG_DEBUG, "SAE: own commit-element(y)",
			    pos + sae->tmp->prime_len, sae->tmp->prime_len);
	} else {
		pos = wpabuf_put(buf, sae->tmp->prime_len);
		if (crypto_bignum_to_bin(sae->tmp->own_commit_element_ffc, pos,
					 sae->tmp->prime_len, sae->tmp->prime_len) < 0) {
			wpa_printf(MSG_ERROR, "SAE: failed bignum operation on commit elem ffc");
			return;
		}
		wpa_hexdump(MSG_DEBUG, "SAE: own commit-element",
			    pos, sae->tmp->prime_len);
	}

	if (identifier) {
		/* Password Identifier element */
		wpabuf_put_u8(buf, WLAN_EID_EXTENSION);
		wpabuf_put_u8(buf, 1 + os_strlen(identifier));
		wpabuf_put_u8(buf, WLAN_EID_EXT_PASSWORD_IDENTIFIER);
		wpabuf_put_str(buf, identifier);
		wpa_printf(MSG_DEBUG, "SAE: own Password Identifier: %s",
			   identifier);
	}
}


u16 sae_group_allowed(struct sae_data *sae, int *allowed_groups, u16 group)
{
	if (allowed_groups) {
		int i;
		for (i = 0; allowed_groups[i] > 0; i++) {
			if (allowed_groups[i] == group)
				break;
		}
		if (allowed_groups[i] != group) {
			wpa_printf(MSG_DEBUG, "SAE: Proposed group %u not "
				   "enabled in the current configuration",
				   group);
			return WLAN_STATUS_FINITE_CYCLIC_GROUP_NOT_SUPPORTED;
		}
	}

	if (sae->state == SAE_COMMITTED && group != sae->group) {
		wpa_printf(MSG_DEBUG, "SAE: Do not allow group to be changed");
		return WLAN_STATUS_FINITE_CYCLIC_GROUP_NOT_SUPPORTED;
	}

	if (group != sae->group && sae_set_group(sae, group) < 0) {
		wpa_printf(MSG_DEBUG, "SAE: Unsupported Finite Cyclic Group %u",
			   group);
		return WLAN_STATUS_FINITE_CYCLIC_GROUP_NOT_SUPPORTED;
	}

	if (sae->tmp == NULL) {
		wpa_printf(MSG_DEBUG, "SAE: Group information not yet initialized");
		return WLAN_STATUS_UNSPECIFIED_FAILURE;
	}

	if (sae->tmp->dh && !allowed_groups) {
		wpa_printf(MSG_DEBUG, "SAE: Do not allow FFC group %u without "
			   "explicit configuration enabling it", group);
		return WLAN_STATUS_FINITE_CYCLIC_GROUP_NOT_SUPPORTED;
	}

	return WLAN_STATUS_SUCCESS;
}


static int sae_is_password_id_elem(const u8 *pos, const u8 *end)
{
	return end - pos >= 3 &&
		pos[0] == WLAN_EID_EXTENSION &&
		pos[1] >= 1 &&
		end - pos - 2 >= pos[1] &&
		pos[2] == WLAN_EID_EXT_PASSWORD_IDENTIFIER;
}


static void sae_parse_commit_token(struct sae_data *sae, const u8 **pos,
				   const u8 *end, const u8 **token,
				   size_t *token_len)
{
	size_t scalar_elem_len, tlen;
	const u8 *elem;

	if (token)
		*token = NULL;
	if (token_len)
		*token_len = 0;

	scalar_elem_len = (sae->tmp->ec ? 3 : 2) * sae->tmp->prime_len;
	if (scalar_elem_len >= (size_t) (end - *pos))
		return; /* No extra data beyond peer scalar and element */

	/* It is a bit difficult to parse this now that there is an
	 * optional variable length Anti-Clogging Token field and
	 * optional variable length Password Identifier element in the
	 * frame. We are sending out fixed length Anti-Clogging Token
	 * fields, so use that length as a requirement for the received
	 * token and check for the presence of possible Password
	 * Identifier element based on the element header information.
	 */
	tlen = end - (*pos + scalar_elem_len);

	if (tlen < SHA256_MAC_LEN) {
		wpa_printf(MSG_DEBUG,
			   "SAE: Too short optional data (%u octets) to include our Anti-Clogging Token",
			   (unsigned int) tlen);
		return;
	}

	elem = *pos + scalar_elem_len;
	if (sae_is_password_id_elem(elem, end)) {
		 /* Password Identifier element takes out all available
		  * extra octets, so there can be no Anti-Clogging token in
		  * this frame. */
		return;
	}

	elem += SHA256_MAC_LEN;
	if (sae_is_password_id_elem(elem, end)) {
		 /* Password Identifier element is included in the end, so
		  * remove its length from the Anti-Clogging token field. */
		tlen -= 2 + elem[1];
	}

	wpa_hexdump(MSG_DEBUG, "SAE: Anti-Clogging Token", *pos, tlen);
	if (token)
		*token = *pos;
	if (token_len)
		*token_len = tlen;
	*pos += tlen;
}


static u16 sae_parse_commit_scalar(struct sae_data *sae, const u8 **pos,
				   const u8 *end)
{
	struct crypto_bignum *peer_scalar;

	if (sae->tmp->prime_len > end - *pos) {
		wpa_printf(MSG_DEBUG, "SAE: Not enough data for scalar");
		return WLAN_STATUS_UNSPECIFIED_FAILURE;
	}

	peer_scalar = crypto_bignum_init_set(*pos, sae->tmp->prime_len);
	if (peer_scalar == NULL)
		return WLAN_STATUS_UNSPECIFIED_FAILURE;

	/*
	 * IEEE Std 802.11-2012, 11.3.8.6.1: If there is a protocol instance for
	 * the peer and it is in Authenticated state, the new Commit Message
	 * shall be dropped if the peer-scalar is identical to the one used in
	 * the existing protocol instance.
	 */
	if (sae->state == SAE_ACCEPTED && sae->peer_commit_scalar &&
	    crypto_bignum_cmp(sae->peer_commit_scalar, peer_scalar) == 0) {
		wpa_printf(MSG_DEBUG, "SAE: Do not accept re-use of previous "
			   "peer-commit-scalar");
		crypto_bignum_deinit(peer_scalar, 0);
		return WLAN_STATUS_UNSPECIFIED_FAILURE;
	}

	/* 1 < scalar < r */
	if (crypto_bignum_is_zero(peer_scalar) ||
	    crypto_bignum_is_one(peer_scalar) ||
	    crypto_bignum_cmp(peer_scalar, sae->tmp->order) >= 0) {
		wpa_printf(MSG_DEBUG, "SAE: Invalid peer scalar");
		crypto_bignum_deinit(peer_scalar, 0);
		return WLAN_STATUS_UNSPECIFIED_FAILURE;
	}


	crypto_bignum_deinit(sae->peer_commit_scalar, 0);
	sae->peer_commit_scalar = peer_scalar;
	wpa_hexdump(MSG_DEBUG, "SAE: Peer commit-scalar",
		    *pos, sae->tmp->prime_len);
	*pos += sae->tmp->prime_len;

	return WLAN_STATUS_SUCCESS;
}


static u16 sae_parse_commit_element_ecc(struct sae_data *sae, const u8 **pos,
					const u8 *end)
{
	u8 prime[SAE_MAX_ECC_PRIME_LEN];

	if (2 * sae->tmp->prime_len > end - *pos) {
		wpa_printf(MSG_DEBUG, "SAE: Not enough data for "
			   "commit-element");
		return WLAN_STATUS_UNSPECIFIED_FAILURE;
	}

	if (crypto_bignum_to_bin(sae->tmp->prime, prime, sizeof(prime),
				 sae->tmp->prime_len) < 0)
		return WLAN_STATUS_UNSPECIFIED_FAILURE;

	/* element x and y coordinates < p */
	if (os_memcmp(*pos, prime, sae->tmp->prime_len) >= 0 ||
	    os_memcmp(*pos + sae->tmp->prime_len, prime,
		      sae->tmp->prime_len) >= 0) {
		wpa_printf(MSG_DEBUG, "SAE: Invalid coordinates in peer "
			   "element");
		return WLAN_STATUS_UNSPECIFIED_FAILURE;
	}

	wpa_hexdump(MSG_DEBUG, "SAE: Peer commit-element(x)",
		    *pos, sae->tmp->prime_len);
	wpa_hexdump(MSG_DEBUG, "SAE: Peer commit-element(y)",
		    *pos + sae->tmp->prime_len, sae->tmp->prime_len);

	crypto_ec_point_deinit(sae->tmp->peer_commit_element_ecc, 0);
	sae->tmp->peer_commit_element_ecc =
		crypto_ec_point_from_bin(sae->tmp->ec, *pos);
	if (sae->tmp->peer_commit_element_ecc == NULL)
		return WLAN_STATUS_UNSPECIFIED_FAILURE;

	if (!crypto_ec_point_is_on_curve(sae->tmp->ec,
					 sae->tmp->peer_commit_element_ecc)) {
		wpa_printf(MSG_DEBUG, "SAE: Peer element is not on curve");
		return WLAN_STATUS_UNSPECIFIED_FAILURE;
	}

	*pos += 2 * sae->tmp->prime_len;

	return WLAN_STATUS_SUCCESS;
}


static u16 sae_parse_commit_element_ffc(struct sae_data *sae, const u8 **pos,
					const u8 *end)
{
	struct crypto_bignum *res, *one;
	const u8 one_bin[1] = { 0x01 };

	if (sae->tmp->prime_len > end - *pos) {
		wpa_printf(MSG_DEBUG, "SAE: Not enough data for "
			   "commit-element");
		return WLAN_STATUS_UNSPECIFIED_FAILURE;
	}
	wpa_hexdump(MSG_DEBUG, "SAE: Peer commit-element", *pos,
		    sae->tmp->prime_len);

	crypto_bignum_deinit(sae->tmp->peer_commit_element_ffc, 0);
	sae->tmp->peer_commit_element_ffc =
		crypto_bignum_init_set(*pos, sae->tmp->prime_len);
	if (sae->tmp->peer_commit_element_ffc == NULL)
		return WLAN_STATUS_UNSPECIFIED_FAILURE;
	/* 1 < element < p - 1 */
	res = crypto_bignum_init();
	one = crypto_bignum_init_set(one_bin, sizeof(one_bin));
	if (!res || !one ||
	    crypto_bignum_sub(sae->tmp->prime, one, res) ||
	    crypto_bignum_is_zero(sae->tmp->peer_commit_element_ffc) ||
	    crypto_bignum_is_one(sae->tmp->peer_commit_element_ffc) ||
	    crypto_bignum_cmp(sae->tmp->peer_commit_element_ffc, res) >= 0) {
		crypto_bignum_deinit(res, 0);
		crypto_bignum_deinit(one, 0);
		wpa_printf(MSG_DEBUG, "SAE: Invalid peer element");
		return WLAN_STATUS_UNSPECIFIED_FAILURE;
	}
	crypto_bignum_deinit(one, 0);

	/* scalar-op(r, ELEMENT) = 1 modulo p */
	if (crypto_bignum_exptmod(sae->tmp->peer_commit_element_ffc,
				  sae->tmp->order, sae->tmp->prime, res) < 0 ||
	    !crypto_bignum_is_one(res)) {
		wpa_printf(MSG_DEBUG, "SAE: Invalid peer element (scalar-op)");
		crypto_bignum_deinit(res, 0);
		return WLAN_STATUS_UNSPECIFIED_FAILURE;
	}
	crypto_bignum_deinit(res, 0);

	*pos += sae->tmp->prime_len;

	return WLAN_STATUS_SUCCESS;
}


static u16 sae_parse_commit_element(struct sae_data *sae, const u8 **pos,
				    const u8 *end)
{
	if (sae->tmp->dh)
		return sae_parse_commit_element_ffc(sae, pos, end);
	return sae_parse_commit_element_ecc(sae, pos, end);
}


static int sae_parse_password_identifier(struct sae_data *sae,
					 const u8 *pos, const u8 *end)
{
	wpa_hexdump(MSG_DEBUG, "SAE: Possible elements at the end of the frame",
		    pos, end - pos);
	if (!sae_is_password_id_elem(pos, end)) {
		if (sae->tmp->pw_id) {
			wpa_printf(MSG_DEBUG,
				   "SAE: No Password Identifier included, but expected one (%s)",
				   sae->tmp->pw_id);
			return WLAN_STATUS_UNKNOWN_PASSWORD_IDENTIFIER;
		}
		os_free(sae->tmp->pw_id);
		sae->tmp->pw_id = NULL;
		return WLAN_STATUS_SUCCESS; /* No Password Identifier */
	}

	if (sae->tmp->pw_id &&
	    (pos[1] - 1 != (int) os_strlen(sae->tmp->pw_id) ||
	     os_memcmp(sae->tmp->pw_id, pos + 3, pos[1] - 1) != 0)) {
		wpa_printf(MSG_DEBUG,
			   "SAE: The included Password Identifier does not match the expected one (%s)",
			   sae->tmp->pw_id);
		return WLAN_STATUS_UNKNOWN_PASSWORD_IDENTIFIER;
	}

	os_free(sae->tmp->pw_id);
	sae->tmp->pw_id = os_malloc(pos[1]);
	if (!sae->tmp->pw_id)
		return WLAN_STATUS_UNSPECIFIED_FAILURE;
	os_memcpy(sae->tmp->pw_id, pos + 3, pos[1] - 1);
	sae->tmp->pw_id[pos[1] - 1] = '\0';
	return WLAN_STATUS_SUCCESS;
}


u16 sae_parse_commit(struct sae_data *sae, const u8 *data, size_t len,
		     const u8 **token, size_t *token_len, int *allowed_groups)
{
	const u8 *pos = data, *end = data + len;
	u16 res;

	/* Check Finite Cyclic Group */
	if (end - pos < 2)
		return WLAN_STATUS_UNSPECIFIED_FAILURE;
	res = sae_group_allowed(sae, allowed_groups, WPA_GET_LE16(pos));
	if (res != WLAN_STATUS_SUCCESS)
		return res;
	pos += 2;

	/* Optional Anti-Clogging Token */
	sae_parse_commit_token(sae, &pos, end, token, token_len);

	/* commit-scalar */
	res = sae_parse_commit_scalar(sae, &pos, end);
	if (res != WLAN_STATUS_SUCCESS)
		return res;

	/* commit-element */
	res = sae_parse_commit_element(sae, &pos, end);
	if (res != WLAN_STATUS_SUCCESS)
		return res;

	/* Optional Password Identifier element */
	res = sae_parse_password_identifier(sae, pos, end);
	if (res != WLAN_STATUS_SUCCESS)
		return res;

	/*
	 * Check whether peer-commit-scalar and PEER-COMMIT-ELEMENT are same as
	 * the values we sent which would be evidence of a reflection attack.
	 */
	if (!sae->tmp->own_commit_scalar ||
	    crypto_bignum_cmp(sae->tmp->own_commit_scalar,
			      sae->peer_commit_scalar) != 0 ||
	    (sae->tmp->dh &&
	     (!sae->tmp->own_commit_element_ffc ||
	      crypto_bignum_cmp(sae->tmp->own_commit_element_ffc,
				sae->tmp->peer_commit_element_ffc) != 0)) ||
	    (sae->tmp->ec &&
	     (!sae->tmp->own_commit_element_ecc ||
	      crypto_ec_point_cmp(sae->tmp->ec,
				  sae->tmp->own_commit_element_ecc,
				  sae->tmp->peer_commit_element_ecc) != 0)))
		return WLAN_STATUS_SUCCESS; /* scalars/elements are different */

	/*
	 * This is a reflection attack - return special value to trigger caller
	 * to silently discard the frame instead of replying with a specific
	 * status code.
	 */
	return SAE_SILENTLY_DISCARD;
}


static void sae_cn_confirm(struct sae_data *sae, const u8 *sc,
			   const struct crypto_bignum *scalar1,
			   const u8 *element1, size_t element1_len,
			   const struct crypto_bignum *scalar2,
			   const u8 *element2, size_t element2_len,
			   u8 *confirm)
{
	const u8 *addr[5];
	size_t len[5];
	u8 scalar_b1[SAE_MAX_PRIME_LEN], scalar_b2[SAE_MAX_PRIME_LEN];

	/* Confirm
	 * CN(key, X, Y, Z, ...) =
	 *    HMAC-SHA256(key, D2OS(X) || D2OS(Y) || D2OS(Z) | ...)
	 * confirm = CN(KCK, send-confirm, commit-scalar, COMMIT-ELEMENT,
	 *              peer-commit-scalar, PEER-COMMIT-ELEMENT)
	 * verifier = CN(KCK, peer-send-confirm, peer-commit-scalar,
	 *               PEER-COMMIT-ELEMENT, commit-scalar, COMMIT-ELEMENT)
	 */
	addr[0] = sc;
	len[0] = 2;
	crypto_bignum_to_bin(scalar1, scalar_b1, sizeof(scalar_b1),
			     sae->tmp->prime_len);
	addr[1] = scalar_b1;
	len[1] = sae->tmp->prime_len;
	addr[2] = element1;
	len[2] = element1_len;
	crypto_bignum_to_bin(scalar2, scalar_b2, sizeof(scalar_b2),
			     sae->tmp->prime_len);
	addr[3] = scalar_b2;
	len[3] = sae->tmp->prime_len;
	addr[4] = element2;
	len[4] = element2_len;
	hmac_sha256_vector(sae->tmp->kck, sizeof(sae->tmp->kck), 5, addr, len,
			   confirm);
}

static int sae_cn_confirm_ecc(struct sae_data *sae, const u8 *sc,
			       const struct crypto_bignum *scalar1,
			       const struct crypto_ec_point *element1,
			       const struct crypto_bignum *scalar2,
			       const struct crypto_ec_point *element2,
			       u8 *confirm)
{
	u8 element_b1[2 * SAE_MAX_ECC_PRIME_LEN];
	u8 element_b2[2 * SAE_MAX_ECC_PRIME_LEN];

	if (crypto_ec_point_to_bin(sae->tmp->ec, element1, element_b1,
			       element_b1 + sae->tmp->prime_len) < 0) {
		wpa_printf(MSG_ERROR, "SAE: failed bignum op while deriving ec point");
		return -1;
	}
	if (crypto_ec_point_to_bin(sae->tmp->ec, element2, element_b2,
			       element_b2 + sae->tmp->prime_len) < 0) {
		wpa_printf(MSG_ERROR, "SAE: failed bignum op while deriving ec point");
		return -1;
	}

	sae_cn_confirm(sae, sc, scalar1, element_b1, 2 * sae->tmp->prime_len,
		       scalar2, element_b2, 2 * sae->tmp->prime_len, confirm);
	return 0;
}

static int sae_cn_confirm_ffc(struct sae_data *sae, const u8 *sc,
			      const struct crypto_bignum *scalar1,
			      const struct crypto_bignum *element1,
			      const struct crypto_bignum *scalar2,
			      const struct crypto_bignum *element2,
			      u8 *confirm)
{
	u8 element_b1[SAE_MAX_PRIME_LEN];
	u8 element_b2[SAE_MAX_PRIME_LEN];

	if (crypto_bignum_to_bin(element1, element_b1, sizeof(element_b1),
			         sae->tmp->prime_len) < 0) {
		wpa_printf(MSG_ERROR, "SAE: failed bignum op while generating SAE confirm - e1");
		return -1;
	}
	if (crypto_bignum_to_bin(element2, element_b2, sizeof(element_b2),
				 sae->tmp->prime_len) < 0) {
		wpa_printf(MSG_ERROR, "SAE: failed bignum op while generating SAE confirm - e2");
		return -1;
	}

	sae_cn_confirm(sae, sc, scalar1, element_b1, sae->tmp->prime_len,
		       scalar2, element_b2, sae->tmp->prime_len, confirm);
	return 0;
}


void sae_write_confirm(struct sae_data *sae, struct wpabuf *buf)
{
	const u8 *sc;

	if (sae->tmp == NULL)
		return;

	/* Send-Confirm */
	sc = wpabuf_put(buf, 0);
	wpabuf_put_le16(buf, sae->send_confirm);
	if (sae->send_confirm < 0xffff)
		sae->send_confirm++;

	if (sae->tmp->ec) {
		if (sae_cn_confirm_ecc(sae, sc, sae->tmp->own_commit_scalar,
				       sae->tmp->own_commit_element_ecc,
				       sae->peer_commit_scalar,
				       sae->tmp->peer_commit_element_ecc,
				       wpabuf_put(buf, SHA256_MAC_LEN))) {
			wpa_printf(MSG_ERROR, "SAE: failed generate SAE confirm (ecc)");
			return;
		}
	} else {
		if (sae_cn_confirm_ffc(sae, sc, sae->tmp->own_commit_scalar,
				       sae->tmp->own_commit_element_ffc,
				       sae->peer_commit_scalar,
				       sae->tmp->peer_commit_element_ffc,
				       wpabuf_put(buf, SHA256_MAC_LEN))) {
			wpa_printf(MSG_ERROR, "SAE: failed generate SAE confirm (ffc)");
			return;
		}
	}
	return;
}

int sae_check_confirm(struct sae_data *sae, const u8 *data, size_t len)
{
	u8 verifier[SHA256_MAC_LEN];

	if (len < 2 + SHA256_MAC_LEN) {
		wpa_printf(MSG_DEBUG, "SAE: Too short confirm message");
		return -1;
	}

	wpa_printf(MSG_DEBUG, "SAE: peer-send-confirm %u", WPA_GET_LE16(data));

	if (sae->tmp == NULL) {
		wpa_printf(MSG_DEBUG, "SAE: Temporary data not yet available");
		return -1;
	}

	if (sae->tmp->ec) {
		if (sae_cn_confirm_ecc(sae, data, sae->peer_commit_scalar,
				       sae->tmp->peer_commit_element_ecc,
				       sae->tmp->own_commit_scalar,
				       sae->tmp->own_commit_element_ecc,
				       verifier)) {
			wpa_printf(MSG_ERROR, "SAE: failed to check SAE confirm (ecc)");
			return -1;
		}
	} else {
		if (sae_cn_confirm_ffc(sae, data, sae->peer_commit_scalar,
				       sae->tmp->peer_commit_element_ffc,
				       sae->tmp->own_commit_scalar,
				       sae->tmp->own_commit_element_ffc,
				       verifier)) {
			wpa_printf(MSG_ERROR, "SAE: failed check SAE confirm (ffc)");
			return -1;
		}
	}

	if (os_memcmp(verifier, data + 2, SHA256_MAC_LEN) != 0) {
		wpa_printf(MSG_DEBUG, "SAE: Confirm mismatch");
		wpa_hexdump(MSG_DEBUG, "SAE: Received confirm",
			    data + 2, SHA256_MAC_LEN);
		wpa_hexdump(MSG_DEBUG, "SAE: Calculated verifier",
			    verifier, SHA256_MAC_LEN);
		return -1;
	}

	return 0;
}

const char * sae_state_txt(enum sae_state state)
{
	switch (state) {
	case SAE_NOTHING:
		return "Nothing";
	case SAE_COMMITTED:
		return "Committed";
	case SAE_CONFIRMED:
		return "Confirmed";
	case SAE_ACCEPTED:
		return "Accepted";
	}
	return "?";
}

#ifdef MBEDTLS_SELF_TEST

static int sae_tests(cmd_tbl_t *t, int argc, char *argv[])
{
#ifdef CONFIG_SAE
	struct sae_data sae;
	int ret = -1;
	/* IEEE P802.11-REVmd/D2.1, Annex J.10 */
	const u8 addr1[ETH_ALEN] = { 0x82, 0x7b, 0x91, 0x9d, 0xd4, 0xb9 };
	const u8 addr2[ETH_ALEN] = { 0x1e, 0xec, 0x49, 0xea, 0x64, 0x88 };
	const char *pw = "mekmitasdigoat";
	const char *pwid = "psk4internet";
	const u8 local_rand[] = {
		0xa9, 0x06, 0xf6, 0x1e, 0x4d, 0x3a, 0x5d, 0x4e,
		0xb2, 0x96, 0x5f, 0xf3, 0x4c, 0xf9, 0x17, 0xdd,
		0x04, 0x44, 0x45, 0xc8, 0x78, 0xc1, 0x7c, 0xa5,
		0xd5, 0xb9, 0x37, 0x86, 0xda, 0x9f, 0x83, 0xcf
	};
	const u8 local_mask[] = {
		0x42, 0x34, 0xb4, 0xfb, 0x17, 0xaa, 0x43, 0x5c,
		0x52, 0xfb, 0xfd, 0xeb, 0xe6, 0x40, 0x39, 0xb4,
		0x34, 0x78, 0x20, 0x0e, 0x54, 0xff, 0x7b, 0x6e,
		0x07, 0xb6, 0x9c, 0xad, 0x74, 0x15, 0x3c, 0x15
	};
	const u8 local_commit[] = {
		0x13, 0x00, 0xeb, 0x3b, 0xab, 0x19, 0x64, 0xe4,
		0xa0, 0xab, 0x05, 0x92, 0x5d, 0xdf, 0x33, 0x39,
		0x51, 0x91, 0x38, 0xbc, 0x65, 0xd6, 0xcd, 0xc0,
		0xf8, 0x13, 0xdd, 0x6f, 0xd4, 0x34, 0x4e, 0xb4,
		0xbf, 0xe4, 0x4b, 0x5c, 0x21, 0x59, 0x76, 0x58,
		0xf4, 0xe3, 0xed, 0xdf, 0xb4, 0xb9, 0x9f, 0x25,
		0xb4, 0xd6, 0x54, 0x0f, 0x32, 0xff, 0x1f, 0xd5,
		0xc5, 0x30, 0xc6, 0x0a, 0x79, 0x44, 0x48, 0x61,
		0x0b, 0xc6, 0xde, 0x3d, 0x92, 0xbd, 0xbb, 0xd4,
		0x7d, 0x93, 0x59, 0x80, 0xca, 0x6c, 0xf8, 0x98,
		0x8a, 0xb6, 0x63, 0x0b, 0xe6, 0x76, 0x4c, 0x88,
		0x5c, 0xeb, 0x97, 0x93, 0x97, 0x0f, 0x69, 0x52,
		0x17, 0xee, 0xff, 0x0d, 0x21, 0x70, 0x73, 0x6b,
		0x34, 0x69, 0x6e, 0x74, 0x65, 0x72, 0x6e, 0x65,
		0x74
	};
	const u8 peer_commit[] = {
		0x13, 0x00, 0x55, 0x64, 0xf0, 0x45, 0xb2, 0xea,
		0x1e, 0x56, 0x6c, 0xf1, 0xdd, 0x74, 0x1f, 0x70,
		0xd9, 0xbe, 0x35, 0xd2, 0xdf, 0x5b, 0x9a, 0x55,
		0x02, 0x94, 0x6e, 0xe0, 0x3c, 0xf8, 0xda, 0xe2,
		0x7e, 0x1e, 0x05, 0xb8, 0x43, 0x0e, 0xb7, 0xa9,
		0x9e, 0x24, 0x87, 0x7c, 0xe6, 0x9b, 0xaf, 0x3d,
		0xc5, 0x80, 0xe3, 0x09, 0x63, 0x3d, 0x6b, 0x38,
		0x5f, 0x83, 0xee, 0x1c, 0x3e, 0xc3, 0x59, 0x1f,
		0x1a, 0x53, 0x93, 0xc0, 0x6e, 0x80, 0x5d, 0xdc,
		0xeb, 0x2f, 0xde, 0x50, 0x93, 0x0d, 0xd7, 0xcf,
		0xeb, 0xb9, 0x87, 0xc6, 0xff, 0x96, 0x66, 0xaf,
		0x16, 0x4e, 0xb5, 0x18, 0x4d, 0x8e, 0x66, 0x62,
		0xed, 0x6a, 0xff, 0x0d, 0x21, 0x70, 0x73, 0x6b,
		0x34, 0x69, 0x6e, 0x74, 0x65, 0x72, 0x6e, 0x65,
		0x74
	};
	const u8 kck[] = {
		0x59, 0x9d, 0x6f, 0x1e, 0x27, 0x54, 0x8b, 0xe8,
		0x49, 0x9d, 0xce, 0xed, 0x2f, 0xec, 0xcf, 0x94,
		0x81, 0x8c, 0xe1, 0xc7, 0x9f, 0x1b, 0x4e, 0xb3,
		0xd6, 0xa5, 0x32, 0x28, 0xa0, 0x9b, 0xf3, 0xed
	};
	const u8 pmk[] = {
		0x7a, 0xea, 0xd8, 0x6f, 0xba, 0x4c, 0x32, 0x21,
		0xfc, 0x43, 0x7f, 0x5f, 0x14, 0xd7, 0x0d, 0x85,
		0x4e, 0xa5, 0xd5, 0xaa, 0xc1, 0x69, 0x01, 0x16,
		0x79, 0x30, 0x81, 0xed, 0xa4, 0xd5, 0x57, 0xc5
	};
	const u8 pmkid[] = {
		0x40, 0xa0, 0x9b, 0x60, 0x17, 0xce, 0xbf, 0x00,
		0x72, 0x84, 0x3b, 0x53, 0x52, 0xaa, 0x2b, 0x4f
	};
	const u8 local_confirm[] = {
		0x01, 0x00, 0x12, 0xd9, 0xd5, 0xc7, 0x8c, 0x50,
		0x05, 0x26, 0xd3, 0x6c, 0x41, 0xdb, 0xc5, 0x6a,
		0xed, 0xf2, 0x91, 0x4c, 0xed, 0xdd, 0xd7, 0xca,
		0xd4, 0xa5, 0x8c, 0x48, 0xf8, 0x3d, 0xbd, 0xe9,
		0xfc, 0x77
	};
	const u8 peer_confirm[] = {
		0x01, 0x00, 0x02, 0x87, 0x1c, 0xf9, 0x06, 0x89,
		0x8b, 0x80, 0x60, 0xec, 0x18, 0x41, 0x43, 0xbe,
		0x77, 0xb8, 0xc0, 0x8a, 0x80, 0x19, 0xb1, 0x3e,
		0xb6, 0xd0, 0xae, 0xf0, 0xd8, 0x38, 0x3d, 0xfa,
		0xc2, 0xfd
	};
	struct wpabuf *buf = NULL;
	struct crypto_bignum *mask = NULL;

	os_memset(&sae, 0, sizeof(sae));
	buf = wpabuf_alloc(1000);
	if (!buf ||
	    sae_set_group(&sae, 19) < 0 ||
	    sae_prepare_commit(addr1, addr2, (const u8 *) pw, os_strlen(pw),
			       pwid, &sae) < 0)
		goto fail;

	wpa_printf(MSG_DEBUG, "SAE TEST #1\n");
	

	/* Override local values based on SAE test vector */
	crypto_bignum_deinit(sae.tmp->sae_rand, 1);
	wpa_printf(MSG_DEBUG, "SAE TEST #2\n");
	sae.tmp->sae_rand = crypto_bignum_init_set(local_rand,
						   sizeof(local_rand));
	wpa_printf(MSG_DEBUG, "SAE TEST #3\n");
	mask = crypto_bignum_init_set(local_mask, sizeof(local_mask));
	wpa_printf(MSG_DEBUG, "SAE TEST #4\n");
	if (!sae.tmp->sae_rand || !mask)
		goto fail;

	if (crypto_bignum_add(sae.tmp->sae_rand, mask,
			      sae.tmp->own_commit_scalar) < 0 ||
	    crypto_bignum_mod(sae.tmp->own_commit_scalar, sae.tmp->order,
			      sae.tmp->own_commit_scalar) < 0 ||
	    crypto_ec_point_mul(sae.tmp->ec, sae.tmp->pwe_ecc, mask,
				sae.tmp->own_commit_element_ecc) < 0 ||
	    crypto_ec_point_invert(sae.tmp->ec,
				   sae.tmp->own_commit_element_ecc) < 0)
		goto fail;
	
	wpa_printf(MSG_DEBUG, "SAE TEST #5\n");

	/* Check that output matches the test vector */
	sae_write_commit(&sae, buf, NULL, pwid);
	wpa_printf(MSG_DEBUG, "SAE TEST #6\n");
	wpa_hexdump_buf(MSG_DEBUG, "SAE: Commit message", buf);

	if (wpabuf_len(buf) != sizeof(local_commit) ||
	    os_memcmp(wpabuf_head(buf), local_commit,
		      sizeof(local_commit)) != 0) {
		wpa_printf(MSG_ERROR, "SAE: Mismatch in local commit");
		goto fail;
	}

	if (sae_parse_commit(&sae, peer_commit, sizeof(peer_commit), NULL, NULL,
		    NULL) != 0 ||
	    sae_process_commit(&sae) < 0)
		goto fail;

	if (os_memcmp(kck, sae.tmp->kck, SAE_KCK_LEN) != 0) {
		wpa_printf(MSG_ERROR, "SAE: Mismatch in KCK");
		goto fail;
	}

	if (os_memcmp(pmk, sae.pmk, SAE_PMK_LEN) != 0) {
		wpa_printf(MSG_ERROR, "SAE: Mismatch in PMK");
		goto fail;
	}

	if (os_memcmp(pmkid, sae.pmkid, SAE_PMKID_LEN) != 0) {
		wpa_printf(MSG_ERROR, "SAE: Mismatch in PMKID");
		goto fail;
	}

	buf->used = 0;
	sae.send_confirm = 1;
	sae_write_confirm(&sae, buf);
	wpa_hexdump_buf(MSG_DEBUG, "SAE: Confirm message", buf);

	if (wpabuf_len(buf) != sizeof(local_confirm) ||
	    os_memcmp(wpabuf_head(buf), local_confirm,
		      sizeof(local_confirm)) != 0) {
		wpa_printf(MSG_ERROR, "SAE: Mismatch in local confirm");
		goto fail;
	}

	if (sae_check_confirm(&sae, peer_confirm, sizeof(peer_confirm)) < 0)
		goto fail;

	ret = 0;
fail:
	sae_clear_data(&sae);
	wpabuf_free(buf);
	crypto_bignum_deinit(mask, 1);
	A("FAIL!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
	return ret;
#else /* CONFIG_SAE */
	return 0;
#endif /* CONFIG_SAE */
}

SUBCMD_MAND(test,
	  sae,
	  sae_tests,
	  "test sae",
	  "test sae");

#endif /* MBEDTLS_SELF_TEST */