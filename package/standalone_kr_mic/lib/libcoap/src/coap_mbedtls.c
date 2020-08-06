/*
* coap_mbedtls.c -- Datagram Transport Layer Support for libcoap with mbedtls
*
* Copyright (C) 2019 Younseog Chang <ys325.chang@newracom.com>
*
* This file is part of the CoAP library libcoap.
*
*/

#include "coap_config.h"

#ifdef HAVE_MBEDTLS

#include "net.h"
#ifdef NRC_LIBCOAP
#include "coap_mem.h"
#else
#include "mem.h"
#endif
#include "coap_debug.h"
#include "prng.h"
#include <string.h>
#include "mbedtls/version.h"
#include "mbedtls/ssl_cookie.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/debug.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/certs.h"
#include "mbedtls/timing.h"
#ifdef NRC_LIBCOAP
#include "coap2/coap.h"
#endif

#ifndef UNUSED
#ifdef __GNUC__
#define UNUSED __attribute__((unused))
#else
#define UNUSED
#endif /* __GNUC__ */
#endif /* UNUSED */

#define DEBUG_LEVEL 0

typedef struct coap_ssl_t {
	const uint8_t *pdu;
	unsigned pdu_len;
	unsigned peekmode;
	coap_tick_t timeout;
} coap_ssl_t;

/*
 * This structure encapsulates the MbedTLS session object.
 * It handles both TLS and DTLS.
 * c_session->tls points to this.
 */
typedef struct coap_mbedtls_env_t {
	mbedtls_ssl_context ctx;        /* mbedtls ssl context */
	mbedtls_ssl_config conf;        /* SSL configuration */
	mbedtls_net_context net;        /* Fill in socket id */
	mbedtls_entropy_context entropy;
	mbedtls_ctr_drbg_context ctr_drbg;
	mbedtls_x509_crt ca;
	mbedtls_x509_crt cert;
	mbedtls_pk_context pkey;
	mbedtls_timing_delay_context timer;
	mbedtls_x509_crt_profile profile;
	mbedtls_ssl_cookie_ctx cookie_ctx;
#if defined(MBEDTLS_SSL_CACHE_C)
	mbedtls_ssl_cache_context cache;
#endif
	coap_ssl_t coap_ssl_data;
	/* If not set, need to do mbedtls_handshake */
	int established;
	int seen_client_hello;
} coap_mbedtls_env_t;

#define IS_PSK (1 << 0)
#define IS_PKI (1 << 1)
#define IS_CLIENT (1 << 6)
#define IS_SERVER (1 << 7)

typedef struct sni_entry {
	char *sni;
	coap_dtls_key_t pki_key;
} sni_entry;

typedef struct coap_mbedtls_context_t {
	coap_dtls_pki_t setup_data;
	int psk_pki_enabled;
	size_t sni_count;
	sni_entry *sni_entry_list;
	char *root_ca_file;
	char *root_ca_path;
} coap_mbedtls_context_t;

typedef enum coap_free_bye_t {
	COAP_FREE_BYE_AS_TCP,
	COAP_FREE_BYE_AS_UDP,
	COAP_FREE_BYE_NONE
} coap_free_bye_t;

static int dtls_log_level = 0;
static int socket_set_nonblocking =0;

#define COAP_MBEDTLS_MALLOC(type) (type*)coap_malloc(sizeof(type))
#define COAP_MBEDTLS_FREE(object) coap_free(object)

#define DTLS_CT_HANDSHAKE          22
#define DTLS_HT_CLIENT_HELLO        1
#define OFF_CONTENT_TYPE 0     /* offset of content_type */
#define OFF_HANDSHAKE_TYPE 13  /* offset of handshake */

#define READ_TIMEOUT 1000

#define RSA_MIN_BIT_LENGTH 1024

#define CLIENT_IP_LEN 16
unsigned char client_ip[CLIENT_IP_LEN] = { 0 };
size_t cliip_len = CLIENT_IP_LEN;

int auth_mode = MBEDTLS_SSL_VERIFY_REQUIRED;	/* verify mode for connection */
int allow_sha1 = false;

static void coap_dtls_free_mbedtls_env(coap_mbedtls_context_t *g_context,
                          coap_mbedtls_env_t *g_env, coap_free_bye_t free_bye);


#if defined(MBEDTLS_X509_CRT_PARSE_C)

static int cert_verify( void *data, mbedtls_x509_crt *crt,
                      int depth, uint32_t *flags )
{
	int ret = 0;
	char buf[1024];
	((void) data);

	coap_log(LOG_DEBUG, "\nVerify requested for (Depth %d):\n", depth );
	ret = mbedtls_x509_crt_info( buf, sizeof( buf ) - 1, "", crt );

	if ( ( *flags ) == 0 ){
		coap_log(LOG_DEBUG, "  This certificate has no flags\n" );
	}else{
		mbedtls_x509_crt_verify_info( buf, sizeof( buf ), "  ! ", *flags );
	}
	return( 0 );
}

static int supported_hashes[] = {
#if defined(MBEDTLS_SHA512_C)
	MBEDTLS_MD_SHA512,
	MBEDTLS_MD_SHA384,
#endif
#if defined(MBEDTLS_SHA256_C)
	MBEDTLS_MD_SHA256,
	MBEDTLS_MD_SHA224,
#endif
#if defined(MBEDTLS_SHA1_C)
	/* Allow SHA-1 as we use it extensively in tests. */
	MBEDTLS_MD_SHA1,
#endif
	MBEDTLS_MD_NONE
};
#endif /* MBEDTLS_X509_CRT_PARSE_C */


int
coap_dtls_is_supported(void)
{
	return 1;
}

int
coap_tls_is_supported(void)
{
	return 0;
}

coap_tls_version_t *
coap_get_tls_library_version(void)
{
	static coap_tls_version_t version;

	version.version = mbedtls_version_get_number();
	version.built_version = mbedtls_version_get_number();
	version.type = COAP_TLS_LIBRARY_MBEDTLS;
	return &version;
}

static void
coap_dtls_debug( void *ctx, int level,
					const char *file, int line, const char *str )
{
	A( "%s", str );
}


int
coap_dtls_context_set_pki(coap_context_t *c_context,
                coap_dtls_pki_t* setup_data, coap_dtls_role_t role)
{
	coap_mbedtls_context_t *g_context =
				((coap_mbedtls_context_t *)c_context->dtls_context);

	if (!g_context || !setup_data)
		return 0;

	g_context->setup_data = *setup_data;
	g_context->psk_pki_enabled |= IS_PKI;
	return 1;
}


int
coap_dtls_context_set_pki_root_cas(struct coap_context_t *c_context,
                                   const char *ca_file, const char *ca_path)
{
	coap_mbedtls_context_t *g_context =
				((coap_mbedtls_context_t *)c_context->dtls_context);

	if (!g_context) {
		coap_log(LOG_WARNING,
		"coap_context_set_pki_root_cas: (D)TLS environment not set up\n");
		return 0;
	}

	if (ca_file == NULL && ca_path == NULL) {
		coap_log(LOG_WARNING,
		"coap_context_set_pki_root_cas: ca_file and/or ca_path not defined\n");
		return 0;
	}
	if (g_context->root_ca_file) {
		g_context->root_ca_file = NULL;
	}
	if (ca_file) {
		g_context->root_ca_file = (char*)ca_file;
	}
	if (g_context->root_ca_path) {
		g_context->root_ca_path = NULL;
	}
	if (ca_path) {
		g_context->root_ca_path = (char*)ca_path;
	}
	return 1;
}


int
coap_dtls_context_set_psk(coap_context_t *c_context,
                          const char *identity_hint, coap_dtls_role_t role)
{
	coap_mbedtls_context_t *g_context =
			((coap_mbedtls_context_t *)c_context->dtls_context);

	g_context->psk_pki_enabled |= IS_PSK;
	return 1;
}


int
coap_dtls_context_check_keys_enabled(coap_context_t *c_context)
{
	coap_mbedtls_context_t *g_context =
	                     ((coap_mbedtls_context_t *)c_context->dtls_context);

	return g_context->psk_pki_enabled ? 1 : 0;
}


void
coap_dtls_startup(void)
{
#if defined(MBEDTLS_DEBUG_C)
	mbedtls_debug_set_threshold( DEBUG_LEVEL );
#endif
}


void coap_dtls_set_log_level(int level)
{
	dtls_log_level = level;
}


int coap_dtls_get_log_level(void)
{
	return dtls_log_level;
}


void *
coap_dtls_new_context(struct coap_context_t *c_context)
{
	struct coap_mbedtls_context_t *g_context = (struct coap_mbedtls_context_t *)
							COAP_MBEDTLS_MALLOC(coap_mbedtls_context_t);

	if (g_context) {
		memset(g_context, 0, sizeof(struct coap_mbedtls_context_t));
	}
	return g_context;
}


void
coap_dtls_free_context(void *handle)
{
	size_t i;
	coap_mbedtls_context_t *g_context = (coap_mbedtls_context_t *)handle;

	for (i = 0; i < g_context->sni_count; i++) {
		COAP_MBEDTLS_FREE(g_context->sni_entry_list[i].sni);
	}

	if (g_context->sni_count)
		COAP_MBEDTLS_FREE(g_context->sni_entry_list);

	if (g_context)
		COAP_MBEDTLS_FREE(g_context);
}


static int
setup_psk_ssl(coap_session_t *c_session, coap_mbedtls_env_t *g_env)
{
	coap_mbedtls_context_t *g_context =
	         	(coap_mbedtls_context_t *)c_session->context->dtls_context;

	int ret = 0;

	ret = mbedtls_ssl_conf_psk(&g_env->conf, c_session->psk_key,  c_session->psk_key_len,
				c_session->psk_identity,  c_session->psk_identity_len);
	if( ret != 0 ){
	    coap_log(LOG_WARNING,  "failed\n  mbedtls_ssl_conf_psk returned -0x%04X\n\n", - ret );
	    goto exit;
	}
	return ret;

exit:
	coap_dtls_free_mbedtls_env(c_session->context->dtls_context,
	                     g_env,
	                     COAP_PROTO_NOT_RELIABLE(c_session->proto) ?
	                     COAP_FREE_BYE_AS_UDP : COAP_FREE_BYE_AS_TCP);
	return ret;
}


static int
setup_pki_ssl_common(coap_session_t *c_session, coap_mbedtls_env_t *g_env)
{
	coap_mbedtls_context_t *g_context =
	         	(coap_mbedtls_context_t *)c_session->context->dtls_context;

	int ret = 0;

	/* Initialize the CA certificate if provided */
	if(g_context->setup_data.pki_key.key.pem.ca_file){
		mbedtls_x509_crt_init(&g_env->ca);
		ret = mbedtls_x509_crt_parse( &g_env->ca,
					(const unsigned char *)g_context->setup_data.pki_key.key.pem.ca_file,
					strlen(g_context->setup_data.pki_key.key.pem.ca_file)+1);

		mbedtls_ssl_conf_ca_chain( &g_env->conf, &g_env->ca, NULL );
	}

	/* Load the certificates */
	mbedtls_x509_crt_init(&g_env->cert);
	ret = mbedtls_x509_crt_parse(&g_env->cert,
					(const unsigned char *) g_context->setup_data.pki_key.key.pem.public_cert,
					strlen(g_context->setup_data.pki_key.key.pem.public_cert)+1);
	if (ret != 0) {
		coap_log(LOG_WARNING,  "mbedtls_x509_crt_parse failed: %d\n", ret);
		goto fail;
	}

	/* Load the private key */
	mbedtls_pk_init(&g_env->pkey);
	ret = mbedtls_pk_parse_key(&g_env->pkey,
					(const unsigned char *) g_context->setup_data.pki_key.key.pem.private_key,
					strlen(g_context->setup_data.pki_key.key.pem.private_key)+1, NULL, 0 );
	if (ret != 0) {
		coap_log(LOG_WARNING,  "mbedtls_pk_parse_public_key failed: %d\n", ret);
 		goto fail;
	}
  	return 0;

fail:
	coap_dtls_free_mbedtls_env(c_session->context->dtls_context,
	                     g_env,
	                     COAP_PROTO_NOT_RELIABLE(c_session->proto) ?
	                     COAP_FREE_BYE_AS_UDP : COAP_FREE_BYE_AS_TCP);
	return ret;
}

/*
 * return 0   Success
 *        neg error code
 */
static int
setup_client_ssl_session(coap_session_t *c_session, coap_mbedtls_env_t *g_env)
{
	coap_mbedtls_context_t *g_context =
	         (coap_mbedtls_context_t *)c_session->context->dtls_context;

	int ret = 0;

	g_context->psk_pki_enabled |= IS_CLIENT;

	if (dtls_log_level >= LOG_DEBUG)
		coap_log(LOG_DEBUG,  "=> Setting up the DTLS client data..."  );

	if( ( ret = mbedtls_ssl_config_defaults(&g_env->conf,
	               MBEDTLS_SSL_IS_CLIENT,
	               MBEDTLS_SSL_TRANSPORT_DATAGRAM,
	               MBEDTLS_SSL_PRESET_DEFAULT ) ) != 0 ){
		if (dtls_log_level >= LOG_WARNING)
			coap_log(LOG_WARNING,  " failed\n  ! mbedtls_ssl_config_defaults returned %d\n\n", ret);
		goto fail;
	}

#if defined(MBEDTLS_X509_CRT_PARSE_C)
	g_env->profile = mbedtls_x509_crt_profile_default;
	g_env->profile.rsa_min_bitlen = RSA_MIN_BIT_LENGTH;
	if(allow_sha1){
		g_env->profile.allowed_mds |= MBEDTLS_X509_ID_FLAG( MBEDTLS_MD_SHA1 );
		mbedtls_ssl_conf_cert_profile(&g_env->conf, &g_env->profile);
		mbedtls_ssl_conf_sig_hashes(&g_env->conf, supported_hashes );
	}
	mbedtls_ssl_conf_cert_profile(&g_env->conf, &g_env->profile);
	mbedtls_ssl_conf_verify(&g_env->conf, cert_verify, NULL );
#endif /* MBEDTLS_X509_CRT_PARSE_C */

	mbedtls_ssl_conf_rng(&g_env->conf, mbedtls_ctr_drbg_random, &g_env->ctr_drbg);
	mbedtls_ssl_conf_dbg(&g_env->conf, coap_dtls_debug, NULL);

	mbedtls_ssl_conf_authmode(&g_env->conf, auth_mode);

	ret = mbedtls_ssl_conf_own_cert(&g_env->conf, &g_env->cert, &g_env->pkey);
	if (ret != 0) {
		coap_log(LOG_WARNING,  "mbedtls_ssl_conf_own_cert failed: %d\n", ret);
		goto fail;
	}

	if( ( ret = mbedtls_ssl_setup(&g_env->ctx, &g_env->conf ) ) != 0 ){
		if (dtls_log_level >= LOG_WARNING)
			coap_log(LOG_WARNING,  " failed\n  ! mbedtls_ssl_setup returned %x\n\n", ret);
		goto fail;
	}

	mbedtls_ssl_conf_read_timeout(&g_env->conf, READ_TIMEOUT );

	mbedtls_ssl_set_timer_cb(&g_env->ctx, &g_env->timer, mbedtls_timing_set_delay, mbedtls_timing_get_delay );
	mbedtls_ssl_session_reset(&g_env->ctx );
	g_env->net.fd = (c_session->sock.fd);

	if(socket_set_nonblocking)
		ret = mbedtls_net_set_nonblock(&g_env->net);
	else
		ret = mbedtls_net_set_block(&g_env->net);

	if( ret != 0 )	{
		coap_log(LOG_WARNING,  " failed\n  ! net_set_(non)block() returned -0x%x\n\n", -ret);
		goto fail;
	}

	mbedtls_ssl_set_bio(&g_env->ctx, &g_env->net,
	                     mbedtls_net_send, mbedtls_net_recv, mbedtls_net_recv_timeout);

	if (dtls_log_level >= LOG_DEBUG)
		coap_log(LOG_DEBUG,  " ok\n");

	return 0;

fail:
	return ret;
}


/*
 * return 0   Success
 *        neg error code
 */
static int
setup_server_ssl_session(coap_session_t *c_session, coap_mbedtls_env_t *g_env)
{
	coap_mbedtls_context_t *g_context =
	         (coap_mbedtls_context_t *)c_session->context->dtls_context;

	int ret = 0;
	struct sockaddr_in server_addr;

	g_context->psk_pki_enabled |= IS_SERVER;

	if (dtls_log_level >= LOG_DEBUG)
		coap_log(LOG_DEBUG,  "=> Setting up the DTLS server data..."  );

	if( ( ret = mbedtls_ssl_config_defaults(&g_env->conf,
				MBEDTLS_SSL_IS_SERVER,
				MBEDTLS_SSL_TRANSPORT_DATAGRAM,
				MBEDTLS_SSL_PRESET_DEFAULT ) ) != 0 ){
		if (dtls_log_level >= LOG_WARNING)
			coap_log(LOG_WARNING,  " failed\n  ! mbedtls_ssl_config_defaults returned %d\n\n", ret);
		goto fail;
	}

#if defined(MBEDTLS_X509_CRT_PARSE_C)
	g_env->profile = mbedtls_x509_crt_profile_default;
	g_env->profile.rsa_min_bitlen = RSA_MIN_BIT_LENGTH;
	if(allow_sha1){
		g_env->profile.allowed_mds |= MBEDTLS_X509_ID_FLAG( MBEDTLS_MD_SHA1 );
		mbedtls_ssl_conf_cert_profile(&g_env->conf, &g_env->profile);
		mbedtls_ssl_conf_sig_hashes(&g_env->conf, supported_hashes );
	}
	mbedtls_ssl_conf_cert_profile(&g_env->conf, &g_env->profile);
	mbedtls_ssl_conf_verify(&g_env->conf, cert_verify, NULL );
#endif /* MBEDTLS_X509_CRT_PARSE_C */

	mbedtls_ssl_conf_rng(&g_env->conf, mbedtls_ctr_drbg_random, &g_env->ctr_drbg);
	mbedtls_ssl_conf_dbg(&g_env->conf, coap_dtls_debug, NULL);

#if defined(MBEDTLS_SSL_CACHE_C)
	mbedtls_ssl_conf_session_cache( &g_env->conf, &g_env->cache,
	                     mbedtls_ssl_cache_get,mbedtls_ssl_cache_set );
#endif

	if( ( ret = mbedtls_ssl_cookie_setup( &g_env->cookie_ctx,
	                     mbedtls_ctr_drbg_random, &g_env->ctr_drbg ) ) != 0 ){
		coap_log(LOG_WARNING, " failed\n  ! mbedtls_ssl_cookie_setup returned %d\n\n", ret );
		goto fail;
	}

	mbedtls_ssl_conf_dtls_cookies( &g_env->conf, mbedtls_ssl_cookie_write, mbedtls_ssl_cookie_check,
	                           &g_env->cookie_ctx );
	mbedtls_ssl_conf_ca_chain(&g_env->conf, &g_env->cert, NULL);

	ret = mbedtls_ssl_conf_own_cert(&g_env->conf, &g_env->cert, &g_env->pkey);
	if (ret != 0) {
		coap_log(LOG_WARNING,  "mbedtls_ssl_conf_own_cert failed: %d\n", ret);
		goto fail;
	}

	if( ( ret = mbedtls_ssl_setup(&g_env->ctx, &g_env->conf ) ) != 0 ){
		if (dtls_log_level >= LOG_WARNING)
			coap_log(LOG_WARNING,  " failed\n  ! mbedtls_ssl_setup returned %x\n\n", ret);
		goto fail;
	}

	mbedtls_ssl_conf_read_timeout(&g_env->conf, READ_TIMEOUT );

	mbedtls_ssl_set_timer_cb(&g_env->ctx, &g_env->timer, mbedtls_timing_set_delay, mbedtls_timing_get_delay );
	mbedtls_ssl_session_reset(&g_env->ctx );
	g_env->net.fd = (c_session->sock.fd);

	mbedtls_ssl_set_bio(&g_env->ctx, &g_env->net,
	                     mbedtls_net_send, mbedtls_net_recv, mbedtls_net_recv_timeout);

	if (dtls_log_level >= LOG_DEBUG)
		coap_log(LOG_DEBUG,  " ok\n");

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = c_session->remote_addr.addr.sin.sin_port;
	server_addr.sin_len = sizeof(server_addr);
	memcpy(&server_addr.sin_addr.s_addr , &c_session->remote_addr.addr.sin.sin_addr, sizeof(struct in_addr));
	inet_ntop(AF_INET, &(server_addr.sin_addr), ( char *)client_ip, INET_ADDRSTRLEN);

	if( connect( g_env->net.fd , (struct sockaddr *)&server_addr, sizeof(server_addr)) != 0 ){
		return( MBEDTLS_ERR_NET_ACCEPT_FAILED );
	}

	return 0;

fail:
	return ret;
}

/*
 * return +ve data amount
 *        0   no more
 *        -1  error (error in errno)
 */
static ssize_t
coap_dgram_read(coap_session_t * context, void *out, size_t outl)
{
	ssize_t ret = 0;
	coap_session_t *c_session = (struct coap_session_t *)context;
	coap_ssl_t *data = &((coap_mbedtls_env_t *)c_session->tls)->coap_ssl_data;

	if (!c_session->tls) {
		return -1;
	}

	if (out != NULL) {
		if (data != NULL && data->pdu_len > 0) {
			if (outl < data->pdu_len) {
				memcpy(out, data->pdu, outl);
				ret = outl;
				data->pdu += outl;
				data->pdu_len -= outl;
			} else {
				memcpy(out, data->pdu, data->pdu_len);
				ret = data->pdu_len;
				if (!data->peekmode) {
					data->pdu_len = 0;
					data->pdu = NULL;
				}
			}
		}else {
			ret = -1;
		}
	}
	return ret;
}


static ssize_t
coap_dgram_write(coap_session_t *  context, const void *send_buffer,
                  size_t send_buffer_length)
{
	ssize_t result = -1;
	coap_session_t *c_session = (struct coap_session_t *)context;

	if (c_session) {
		result = coap_session_send(c_session, send_buffer, send_buffer_length);
		if (result != (int)send_buffer_length) {
			coap_log(LOG_WARNING, "coap_network_send failed\n");
			result = 0;
		}
	}else {
		result = 0;
	}
	return result;
}


int coap_dtls_is_handshake_established(coap_session_t *c_session)
{
	coap_mbedtls_env_t *g_env = NULL;

	if(c_session == NULL)
		return 0;

	g_env = (coap_mbedtls_env_t *)c_session->tls;

	if(g_env == NULL)
		return 0;

	if(g_env->established)
		return 1;
	else
		return 0;
}


static coap_mbedtls_env_t *
coap_dtls_new_mbedtls_env(coap_session_t *c_session, int type)
{
	coap_mbedtls_context_t *g_context =
			((coap_mbedtls_context_t *)c_session->context->dtls_context);
	coap_mbedtls_env_t *g_env = (coap_mbedtls_env_t *)c_session->tls;
	const char *pers = "dtls_client";
	int ret;

	if (g_env){
		return g_env;
	}

	g_env = COAP_MBEDTLS_MALLOC(coap_mbedtls_env_t);

	if (!g_env){
		return NULL;
	}

	memset(g_env, 0, sizeof(coap_mbedtls_context_t));
	mbedtls_net_init(&g_env->net);
	mbedtls_ssl_init(&g_env->ctx);
	mbedtls_ssl_config_init(&g_env->conf);
	mbedtls_ctr_drbg_init(&g_env->ctr_drbg);
	mbedtls_entropy_init(&g_env->entropy);
	mbedtls_ssl_cookie_init(&g_env->cookie_ctx);
#if defined(MBEDTLS_SSL_CACHE_C)
	mbedtls_ssl_cache_init(&g_env->cache);
#endif

	coap_log(LOG_DEBUG, "=> Seeding the random number generator..." );
	if( ( ret = mbedtls_ctr_drbg_seed(&g_env->ctr_drbg, mbedtls_entropy_func,\
			&g_env->entropy, (const unsigned char *) pers, strlen( pers ) ) )!= 0 ){
		if (dtls_log_level >= LOG_WARNING)
			coap_log(LOG_WARNING, "failed\n ! mbedtls_ctr_drbg_seed returned %d\n", ret);
		goto fail;
	}

	if (g_context->psk_pki_enabled & IS_PSK) {
		ret = setup_psk_ssl(c_session, g_env);
	}

	if ((g_context->psk_pki_enabled & IS_PKI) ||(g_context->psk_pki_enabled & (IS_PSK | IS_PKI)) == 0){
		ret = setup_pki_ssl_common(c_session, g_env);
	}

	if(type == IS_SERVER){
		ret = setup_server_ssl_session(c_session, g_env);
	}else{
		ret = setup_client_ssl_session(c_session, g_env);
	}

	return g_env;

fail:
	if (g_env)
		COAP_MBEDTLS_FREE(g_env);
	return NULL;
}

static void
coap_dtls_free_mbedtls_env(coap_mbedtls_context_t *g_context,
                          coap_mbedtls_env_t *g_env,
                          coap_free_bye_t free_bye)
{
	if (g_env) {
		g_env->net.fd = -1;
		if(&g_env->net)
			mbedtls_net_free(&g_env->net);
		if(&g_env->ctx)
			mbedtls_ssl_free(&g_env->ctx);
		if(&g_env->conf)
			mbedtls_ssl_config_free(&g_env->conf);
		if(&g_env->ca)
			mbedtls_x509_crt_free(&g_env->ca);
		if(&g_env->cert)
			mbedtls_x509_crt_free(&g_env->cert);
		if(&g_env->pkey)
			mbedtls_pk_free(&g_env->pkey);
		if(&g_env->ctr_drbg)
			mbedtls_ctr_drbg_free(&g_env->ctr_drbg);
		if(&g_env->entropy)
			mbedtls_entropy_free(&g_env->entropy);
		if(&g_env->cookie_ctx)
			mbedtls_ssl_cookie_free(&g_env->cookie_ctx);
#if defined(MBEDTLS_SSL_CACHE_C)
		if(&g_env->cache)
			mbedtls_ssl_cache_free(&g_env->cache);
#endif
		COAP_MBEDTLS_FREE(g_env);
	}
}

void *
coap_dtls_new_server_session(coap_session_t *c_session)
{
	coap_mbedtls_env_t *g_env =
	       (coap_mbedtls_env_t *)c_session->endpoint->hello.tls;

	/* For the next one */
	c_session->endpoint->hello.tls = NULL;

	return g_env;
}


/*
 * return -1  failure
 *         0  not completed
 *         1  established
 */
static int
do_mbedtls_handshake(coap_session_t *c_session, coap_mbedtls_env_t *g_env)
{
	int ret = -1;

	do ret = mbedtls_ssl_handshake(&g_env->ctx);
	while( ret == MBEDTLS_ERR_SSL_WANT_READ ||
	       ret == MBEDTLS_ERR_SSL_WANT_WRITE );

	if( ret == MBEDTLS_ERR_SSL_HELLO_VERIFY_REQUIRED )	{
		coap_log(LOG_WARNING, "=> Hello verification requested\n" );
		g_env->seen_client_hello = 1;
		ret = 0;
	}else if( ret != 0 ){
		coap_log(LOG_WARNING,  " failed\n  ! mbedtls_ssl_handshake returned -0x%x\n\n", -ret );
		ret = -1;
	}else{
		coap_log(LOG_DEBUG,  "=> DTLS handshake success\n" );
		ret = 1;
	}
	return ret;
}


void *
coap_dtls_new_client_session(coap_session_t *c_session)
{
	coap_mbedtls_context_t *g_context =
			((coap_mbedtls_context_t *)c_session->context->dtls_context);
	coap_mbedtls_env_t *g_env = coap_dtls_new_mbedtls_env(c_session, IS_CLIENT);

	int ret;
	int flags;

	if (g_env) {
		// Handshake
		if (dtls_log_level >= LOG_DEBUG)
			coap_log(LOG_DEBUG,  "  . Performing the DTLS handshake...");

		ret = do_mbedtls_handshake(c_session, g_env);
		if (ret < 0) {
			goto fail;
		}

		g_env->established = 1;

		coap_session_connected(c_session);

		if (dtls_log_level >= LOG_DEBUG)
			coap_log(LOG_DEBUG,  " ok\n" );

		if (g_context->psk_pki_enabled & IS_PKI) {
			// Verify the server certificate
			if (dtls_log_level >= LOG_DEBUG)
				coap_log(LOG_DEBUG,  "  . Verifying peer X.509 certificate..."  );

			if( ( flags = mbedtls_ssl_get_verify_result(&g_env->ctx) ) != 0 ){
				char vrfy_buf[512];
				if (dtls_log_level >= LOG_WARNING)
					coap_log(LOG_WARNING,  " failed\n"   );

				mbedtls_x509_crt_verify_info( vrfy_buf, sizeof( vrfy_buf ), "  ! ", flags );
				if (dtls_log_level >= LOG_WARNING)
					coap_log(LOG_WARNING, "%s\n", vrfy_buf );
			}else{
				if (dtls_log_level >= LOG_DEBUG)
					coap_log(LOG_DEBUG,  " ok\n" );
			}
		}
	}
	return g_env;

fail:
	coap_dtls_free_mbedtls_env(c_session->context->dtls_context,
	                     g_env,
	                     COAP_PROTO_NOT_RELIABLE(c_session->proto) ?
	                     COAP_FREE_BYE_AS_UDP : COAP_FREE_BYE_AS_TCP);
	return NULL;
}

void
coap_dtls_free_session(coap_session_t *c_session)
{
	coap_mbedtls_env_t *g_env = (coap_mbedtls_env_t *)c_session->tls;
	int ret;
 	int fd = g_env->net.fd;

	if (c_session && c_session->context) {
		if (dtls_log_level >= LOG_DEBUG)
			coap_log(LOG_DEBUG,  "=> Closing the session...\n" );

		/* No error checking, the connection might be closed already */
		do ret = mbedtls_ssl_close_notify(&g_env->ctx );
		while( ret == MBEDTLS_ERR_SSL_WANT_WRITE );

		coap_dtls_free_mbedtls_env(c_session->context->dtls_context,
	                     c_session->tls,
	                     COAP_PROTO_NOT_RELIABLE(c_session->proto) ?
	                     COAP_FREE_BYE_AS_UDP : COAP_FREE_BYE_AS_TCP);
		c_session->tls = NULL;

		if(fd){
			lwip_disconnect(fd);
		}
	}
}

void
coap_dtls_session_update_mtu(coap_session_t *c_session)
{
	coap_mbedtls_env_t *g_env = (coap_mbedtls_env_t *)c_session->tls;

	mbedtls_ssl_set_mtu(&g_env->ctx, c_session->mtu);
}


/*
 * return +ve data amount
 *        0   no more
 *        -1  error
 */
int
coap_dtls_send(coap_session_t *c_session, const uint8_t *data, size_t data_len)
{
	int ret;
	coap_mbedtls_env_t *g_env = (coap_mbedtls_env_t *)c_session->tls;

	assert(g_env != NULL);

	c_session->dtls_event = -1;
	if (g_env->established) {
		do ret = mbedtls_ssl_write(&g_env->ctx, (unsigned char *) data, data_len );
		while( ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE );

		if( ret < 0 ){
			if (dtls_log_level >= LOG_WARNING)
				coap_log(LOG_WARNING,  " failed\n  ! mbedtls_ssl_write returned %d\n\n", ret );
			return ret;
		}
		if (dtls_log_level >= LOG_DEBUG)
			coap_log(LOG_DEBUG,  "%s bytes_written:%d\n", __func__, ret);
		return ret;
	}else{
		ret = do_mbedtls_handshake(c_session, g_env);
		if (ret < 0) {
			return ret;
		}
		g_env->established = 1;
		coap_session_connected(c_session);
		return coap_dtls_send(c_session, data, data_len);
	}
}

int
coap_dtls_is_context_timeout(void)
{
	return 1;
}

coap_tick_t
coap_dtls_get_context_timeout(void *dtls_context)
{
	return 0;
}

coap_tick_t
coap_dtls_get_timeout(coap_session_t *c_session)
{
	return 0;
}

void coap_dtls_handle_timeout(coap_session_t *c_session)
{
	;
}

/*
 * return +ve data amount
 *        0   no more
 *        -1  error
 */
int
coap_dtls_receive(coap_session_t *c_session, const uint8_t *data, size_t data_len)
{
	coap_mbedtls_env_t *g_env = (coap_mbedtls_env_t *)c_session->tls;
	int ret = 0;

	ret = coap_handle_dgram(c_session->context, c_session, (uint8_t *)data, (size_t)data_len);

	return ret;
}


int
coap_dtls_hello(coap_session_t *c_session, const uint8_t *data, size_t data_len)
{
	coap_mbedtls_env_t *g_env = (coap_mbedtls_env_t *)c_session->tls;
	coap_ssl_t *ssl_data = g_env ? &g_env->coap_ssl_data : NULL;
	int ret;

	if (!g_env) {
		if (data_len < (OFF_HANDSHAKE_TYPE + 1)) {
			coap_log(LOG_DEBUG,
			"coap_dtls_hello: ContentType %d Short Packet (%d < %d) dropped\n",
			data[OFF_CONTENT_TYPE], data_len, OFF_HANDSHAKE_TYPE + 1);
			return 0;
		}
		if (data[OFF_CONTENT_TYPE] != DTLS_CT_HANDSHAKE ||
			data[OFF_HANDSHAKE_TYPE] != DTLS_HT_CLIENT_HELLO) {
			coap_log(LOG_DEBUG,
			"coap_dtls_hello: ContentType %d Handshake %d dropped\n",
			data[OFF_CONTENT_TYPE], data[OFF_HANDSHAKE_TYPE]);
			return 0;
		}

		g_env = coap_dtls_new_mbedtls_env(c_session, IS_SERVER);
		if (g_env) {
			c_session->tls = g_env;
			ssl_data = &g_env->coap_ssl_data;
			ssl_data->pdu = data;
			ssl_data->pdu_len = (unsigned)data_len;

			/* Copy data to in_buf */
			memcpy(( unsigned char *)g_env->ctx.in_buf, data, data_len );
			g_env->ctx.in_left = data_len;
			g_env->ctx.state = MBEDTLS_SSL_HELLO_REQUEST;
			g_env->ctx.in_msgtype = MBEDTLS_SSL_MSG_HANDSHAKE;

			if( ( ret = mbedtls_ssl_set_client_transport_id( &g_env->ctx, client_ip, cliip_len ) ) != 0 ){
				coap_log(LOG_WARNING, " failed\n  ! mbedtls_ssl_set_client_transport_id() returned -0x%x\n\n", -ret  );
			}

			ret = do_mbedtls_handshake(c_session, g_env);
			if (ret == 1 && g_env->seen_client_hello) {
				g_env->seen_client_hello = 0;
				g_env->established = 1;
				coap_session_connected(c_session);
				return 1;
			}

			if (ret == -1) {
				coap_dtls_free_mbedtls_env(c_session->context->dtls_context,
	                     		g_env,
	                     		COAP_PROTO_NOT_RELIABLE(c_session->proto) ?
	                     		COAP_FREE_BYE_AS_UDP : COAP_FREE_BYE_AS_TCP);
				return 0;
			}
		}
		return 0;
	}
	mbedtls_ssl_session_reset(&g_env->ctx);

	if( ( ret = mbedtls_ssl_set_client_transport_id( &g_env->ctx, client_ip, cliip_len ) ) != 0 ){
		coap_log(LOG_WARNING, " failed\n  ! mbedtls_ssl_set_client_transport_id() returned -0x%x\n\n", -ret  );
	}

	ssl_data->pdu = data;
	ssl_data->pdu_len = (unsigned)data_len;

	/* Copy data to in_buf */
	memcpy(( unsigned char *)g_env->ctx.in_buf, data, data_len );
	g_env->ctx.in_left = data_len;
	g_env->ctx.state = MBEDTLS_SSL_HELLO_REQUEST;
	g_env->ctx.in_msgtype = MBEDTLS_SSL_MSG_HANDSHAKE;

	ret = do_mbedtls_handshake(c_session, g_env);
	if (ret == 1 && g_env->seen_client_hello) {
		g_env->seen_client_hello = 0;
		g_env->established = 1;
		return 1;
	}

	if (ret == -1) {
		coap_dtls_free_mbedtls_env(c_session->context->dtls_context,
			g_env,
			COAP_PROTO_NOT_RELIABLE(c_session->proto) ?
			COAP_FREE_BYE_AS_UDP : COAP_FREE_BYE_AS_TCP);
		return 0;
	}
	return 0;
}


unsigned int
coap_dtls_get_overhead(coap_session_t *c_session)
{
	return 37;
}


/*
 * return +ve data amount
 *        0   no more
 *        -1  error (error in errno)
 */
static ssize_t
coap_sock_read(coap_session_t * context, void *out, size_t outl)
{
	int ret = 0;
	coap_session_t *c_session = (struct coap_session_t *)context;

	if (out != NULL) {
#ifdef _WIN32
		ret = recv(c_session->sock.fd, (char *)out, (int)outl, 0);
#else
		ret = recv(c_session->sock.fd, out, outl, 0);
#endif
		if (ret == 0) {
			/* graceful shutdown */
			c_session->sock.flags &= ~COAP_SOCKET_CAN_READ;
			return 0;
		} else if (ret == COAP_SOCKET_ERROR)
			c_session->sock.flags &= ~COAP_SOCKET_CAN_READ;
		else if (ret < (ssize_t)outl)
			c_session->sock.flags &= ~COAP_SOCKET_CAN_READ;
		return ret;
	}
	return ret;
}

/*
 * return +ve data amount
 *        0   no more
 *        -1  error (error in errno)
 */
static ssize_t
coap_sock_write(coap_session_t * context, const void *in, size_t inl)
{
	int ret = 0;
	coap_session_t *c_session = (struct coap_session_t *)context;

	ret = (int)coap_socket_write(&c_session->sock, in, inl);
	if (ret == 0) {
		ret = -1;
	}
	return ret;
}

void *
coap_tls_new_client_session(coap_session_t *c_session, int *connected)
{
	return NULL;
}


void *
coap_tls_new_server_session(coap_session_t *c_session, int *connected)
{
	return NULL;
}


void
coap_tls_free_session(coap_session_t *c_session)
{
	return;
}


ssize_t
coap_tls_write(coap_session_t *c_session, const uint8_t *data, size_t data_len)
{
	return 0;
}


ssize_t
coap_dtls_read(coap_session_t *c_session, struct coap_packet_t *packet)
{
	coap_mbedtls_env_t *g_env = (coap_mbedtls_env_t *)c_session->tls;
	ssize_t bytes_read;
	int ret;

	if (!g_env)
	return -1;

	coap_address_copy(&packet->dst, &c_session->local_addr);
	coap_address_copy(&packet->src, &c_session->remote_addr);
	packet->ifindex = c_session->ifindex;

	bytes_read = mbedtls_ssl_read(&g_env->ctx, ( unsigned char *)packet->payload, COAP_RXBUFFER_SIZE );
	packet->length = bytes_read;

	if(bytes_read == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY ){
		c_session->sock.flags = COAP_SOCKET_EMPTY; // without socket close
		return -2;
	}

	return (bytes_read);
}


/*
 * return +ve data amount
 *        0   no more
 *        -1  error (error in errno)
 */
ssize_t
coap_tls_read(coap_session_t *c_session, uint8_t *data, size_t data_len)
{
	return 0;
}

#else /* !HAVE_MBEDTLS */

#ifdef __clang__
/* Make compilers happy that do not like empty modules. As this function is
 * never used, we ignore -Wunused-function at the end of compiling this file
 */
#pragma GCC diagnostic ignored "-Wunused-function"
#endif
static inline void dummy(void) {
}

#endif /* HAVE_MBEDTLS */
