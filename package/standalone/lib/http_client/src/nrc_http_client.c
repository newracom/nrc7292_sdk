/*
 * MIT License
 *
 * Copyright (c) 2022 Newracom, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */


#include "nrc_sdk.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "lwip/err.h"
#include "mbedtls/net.h"
#include "nrc_http_client.h"
#include "nrc_http_client_debug.h"

#define HTTPC_MAX_HOSTNAME 128
#define HTTPC_MAX_URI 1024

typedef enum {
	METHOD_GET,
	METHOD_POST,
	METHOD_PUT,
	METHOD_DELETE,
} http_method_e;

typedef enum {
	HTTP,
#if defined(SUPPORT_HTTPS_CLIENT)
	HTTPS,
#endif
} protocol_scheme_e;

typedef struct {
	const char *header;
	char host[HTTPC_MAX_HOSTNAME];
	char uri[HTTPC_MAX_URI];
	httpc_data_t *data;
	protocol_scheme_e scheme;
	char port[6];
} http_info_t;

/** This macro defines the deault HTTP port.  */
#define HTTP_PORT   80

#if defined(SUPPORT_HTTPS_CLIENT)
#include "mbedtls/ssl.h"
#include "mbedtls/certs.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/debug.h"

typedef struct {
	mbedtls_ssl_context ssl_ctx;        /* mbedtls ssl context */
	mbedtls_net_context net_ctx;        /* Fill in socket id */
	mbedtls_ssl_config ssl_conf;        /* SSL configuration */
	mbedtls_entropy_context entropy;
	mbedtls_ctr_drbg_context ctr_drbg;
	mbedtls_x509_crt_profile profile;
	mbedtls_x509_crt cacert;
	mbedtls_x509_crt clicert;
	mbedtls_pk_context pkey;
} http_ssl_t;

/** This macro defines the deault HTTPS port.  */
#define HTTPS_PORT 443


/** read timeout to 10 sec */
#define HTTP_SSL_READ_TIMEOUT 10000
#endif

static const char *module_name()
{
	return "sdk_httpc: ";
}

static int httpc_parse_url(const char *url, http_info_t *info)
{
	int url_len = strlen(url);
	char *uri;
	char *port;
	char *url_ptr = (char*)url;

	/* Protocol Scheme */
	if (strncmp(url, "http://", 7) == 0) {
		info->scheme = HTTP;
		url_ptr += 7;
	}
#if defined(SUPPORT_HTTPS_CLIENT)
	else if (strncmp(url, "https://", 8) == 0) {
		info->scheme = HTTPS;
		url_ptr += 8;
	}
#endif
	else {
		HTTPC_LOGE("[%s] Error!! HTTPC_RET_ERROR_SCHEME_NOT_FOUND returned.", __func__);
		return HTTPC_RET_ERROR_SCHEME_NOT_FOUND;
	}

	/* Host, Port and URI */
	port = strchr(url_ptr, ':');
	uri = strchr(url_ptr, '/');
	int host_len = 0;
	int uri_len = 0;

	if (port != NULL) {
		host_len = port - url_ptr;

		if (host_len + 1 > HTTPC_MAX_HOSTNAME) {
			HTTPC_LOGE("[%s] Error: Hostname too long (max: %d - 1).", __func__, HTTPC_MAX_HOSTNAME);
			goto alloc_fail;
		}
		memset(info->host, 0x0, sizeof(info->host));

		if (info->host != NULL) {
			memcpy(info->host, url_ptr, host_len);
			*(info->host + host_len) = 0;
		} else {
			goto alloc_fail;
		}

		if (uri != NULL) {
			memset(info->port, 0, sizeof(info->port));
			memcpy(info->port, (port + 1), (uri - port - 1));
			uri_len = url_len - (uri - url);
			if (uri_len + 1 > HTTPC_MAX_URI) {
				HTTPC_LOGE("[%s] Error: URI too long (max: %d - 1).", __func__, HTTPC_MAX_URI);
				goto alloc_fail;
			}
			memset(info->uri, 0x0, sizeof(info->uri));

			if (info->uri != NULL) {
				memcpy(info->uri, uri, uri_len);
				*(info->uri + uri_len + 1) = 0;
			} else {
				goto alloc_fail;
			}
		} else {
			memset(info->port, 0, sizeof(info->port));
			strncpy(info->port, (port + 1), (url_len - (port - url) - 1));
			memset(info->uri, 0x0, sizeof(info->uri));

			if (info->uri != NULL) {
				*(info->uri) = '/';
				*(info->uri + 1) = 0;
			} else {
				goto alloc_fail;
			}
		}
	} else {
		if (info->scheme == HTTP) {
			sprintf(info->port, "%d", HTTP_PORT);
		}
#if defined(SUPPORT_HTTPS_CLIENT)
		else {
			sprintf(info->port, "%d", HTTPS_PORT);
		}
#endif
		if (uri != NULL) {
			host_len = uri - url_ptr;
			if (host_len + 1 > HTTPC_MAX_HOSTNAME) {
				HTTPC_LOGE("[%s] Error: Hostname too long (max: %d - 1).", __func__, HTTPC_MAX_HOSTNAME);
				goto alloc_fail;
			}

			memset(info->host, 0x0, sizeof(info->host));

			if (info->host != NULL) {
				memcpy(info->host, url_ptr, host_len);
				*(info->host + host_len) = 0;
			} else {
				goto alloc_fail;
			}
			uri_len = url_len - (uri - url);
			if (uri_len + 1 > HTTPC_MAX_URI) {
				HTTPC_LOGE("[%s] Error: URI too long (max: %d - 1).", __func__, HTTPC_MAX_URI);
				goto alloc_fail;
			}

			memset(info->uri, 0x0, sizeof(info->uri));

			if (info->uri != NULL) {
				strncpy(info->uri, uri, uri_len);
				*(info->uri + uri_len + 1) = 0;
			} else {
				goto alloc_fail;
			}
		} else {
			host_len = url_len - (info->scheme == HTTP ? 7 : 8);
			if (host_len + 1 > HTTPC_MAX_HOSTNAME) {
				HTTPC_LOGE("[%s] Error: Hostname too long (max: %d - 1).", __func__, HTTPC_MAX_HOSTNAME);
				goto alloc_fail;
			}

			memset(info->host, 0x0, sizeof(info->host));

			if (info->host != NULL) {
				strncpy(info->host, url_ptr, host_len);
				*(info->host + host_len) = 0;
			} else {
				goto alloc_fail;
			}
			memset(info->uri, 0x0, sizeof(info->uri));

			if (info->uri != NULL) {
				*(info->uri) = '/';
				*(info->uri + 1) = 0;
			} else {
				goto alloc_fail;
			}
		}
	}

	/* TODO : Is it necessary to handle the URL fragment by '#'? */
	HTTPC_LOGI("[%s] scheme:%s, host:%s, port:%s, uri:%s", __func__,
	info->scheme == HTTP ? "http" : "https",
	info->host, info->port, info->uri);

	return HTTPC_RET_OK;

alloc_fail:
	HTTPC_LOGE("[%s] Error!! HTTPC_RET_ERROR_ALLOC_FAIL returned.", __func__);
	return HTTPC_RET_ERROR_ALLOC_FAIL;
}

static int httpc_conn(con_handle_t *handle, char *host, char *port)
{
	struct addrinfo hints, *addr_list, *cur;
	int ret = 0;
	int socket = -1;

	memset( &hints, 0, sizeof(hints) );
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	if (getaddrinfo(host, port, &hints, &addr_list) != 0) {
		HTTPC_LOGE("[%s] Error!! HTTPC_RET_ERROR_RESOLVING_DNS returned.", __func__);
		return HTTPC_RET_ERROR_RESOLVING_DNS;
	}

	/* Try the sockaddrs until a connection succeeds */
	ret = HTTPC_RET_ERROR_RESOLVING_DNS;
	for (cur = addr_list; cur != NULL; cur = cur->ai_next) {
		socket = (int)socket(cur->ai_family, cur->ai_socktype, cur->ai_protocol);
		if (socket < 0) {
			ret = HTTPC_RET_ERROR_SOCKET_FAIL;
			continue;
		}

		if (connect(socket, cur->ai_addr, (int)cur->ai_addrlen) == 0) {
			ret = HTTPC_RET_OK;
			break;
		}

		close(socket);
		ret = HTTPC_RET_ERROR_CONNECTION;
	}

	freeaddrinfo(addr_list);

	*handle = (con_handle_t)socket;
	return ret;
}

static void httpc_debug( void *ctx, int level, const char *file, int line, const char *str)
{
	printf("[%s:%d] %s", file, line, str);
}

#if defined(SUPPORT_HTTPS_CLIENT)
static int httpc_ssl_conn(con_handle_t *handle, http_info_t *info, ssl_certs_t *certs)
{
	const char *pers = "httpc_ssl";
	http_ssl_t *http_ssl = NULL;
	uint32_t flags;
	int ret = 0;

	http_ssl = (http_ssl_t*)nrc_mem_malloc(sizeof(http_ssl_t));
	if (http_ssl == NULL) {
		HTTPC_LOGE("memory malloc error.");
		ret = HTTPC_RET_ERROR_ALLOC_FAIL;
		goto exit;
	}

	memset(http_ssl, 0, sizeof(http_ssl_t));
	*handle = (con_handle_t) http_ssl;
#if defined(MBEDTLS_DEBUG_C)
	int http_client_debug_level = httpc_get_log_level();
	mbedtls_debug_set_threshold( http_client_debug_level );
#endif

	/*
	 * 0. Initialize the RNG and the session data
	 */
	mbedtls_net_init( &http_ssl->net_ctx );
	mbedtls_ssl_init( &http_ssl->ssl_ctx );
	mbedtls_ssl_config_init( &http_ssl->ssl_conf );
	mbedtls_x509_crt_init( &http_ssl->cacert );
	mbedtls_x509_crt_init( &http_ssl->clicert );
	mbedtls_pk_init( &http_ssl->pkey );
	mbedtls_ctr_drbg_init( &http_ssl->ctr_drbg );

	HTTPC_LOGD( "  . Seeding the random number generator..." );

	mbedtls_entropy_init( &http_ssl->entropy );
	if( ( ret = mbedtls_ctr_drbg_seed( &http_ssl->ctr_drbg, mbedtls_entropy_func,
			&http_ssl->entropy, (const unsigned char *) pers,  strlen( pers ) ) ) != 0 ) {
		HTTPC_LOGE(" failed\n  ! mbedtls_ctr_drbg_seed returned -0x%x\n", -ret);
		goto exit;
	}

	HTTPC_LOGD( " ok" );

	/*
	 * 0. Initialize certificates
	 */
	HTTPC_LOGD( "  . Loading the CA root certificate ..." );

	ret = mbedtls_x509_crt_parse( &http_ssl->cacert, (const unsigned char *) certs->ca_cert,
								  certs->ca_cert_length );
	if( ret < 0 ) {
		HTTPC_LOGE(" failed\n  !  mbedtls_x509_crt_parse returned -0x%x while parsing root cert", -ret);
		goto exit;
	}

	HTTPC_LOGD( " ok (%d skipped)", ret );

	HTTPC_LOGD( "  . Loading the Client certificate ..." );

	ret = mbedtls_x509_crt_parse( &http_ssl->clicert, (const unsigned char *) certs->client_cert,
								certs->client_cert_length );
	if( ret < 0 ) {
		HTTPC_LOGE(" failed\n  !  mbedtls_x509_crt_parse returned -0x%x while parsing device cert", -ret);
		goto exit;
	}

	HTTPC_LOGD( " ok (%d skipped)", ret );

	HTTPC_LOGD( "  . Loading the Client private key ..." );

	ret = mbedtls_pk_parse_key( &http_ssl->pkey, (const unsigned char *) certs->client_pk,
								certs->client_pk_length, NULL, 0 );
	if( ret < 0 ) {
		HTTPC_LOGE(" failed\n  !  mbedtls_pk_parse_key returned -0x%x while parsing private key\n\n", -ret);
		goto exit;
	}

	HTTPC_LOGD( " ok (%d skipped)", ret );

	/*
	 * 1. Start the connection
	 */
	HTTPC_LOGD( "  . Connecting to tcp/%s/%s...", info->host, info->port );

	if( ( ret = mbedtls_net_connect( &http_ssl->net_ctx, info->host,
								info->port, MBEDTLS_NET_PROTO_TCP ) ) != 0 ) {
		HTTPC_LOGE(" failed\n  ! mbedtls_net_connect returned -0x%x", -ret);
		goto exit;
	}

	HTTPC_LOGD( " ok" );

	/*
	 * 2. Setup stuff
	 */
	HTTPC_LOGD( "  . Setting up the SSL/TLS structure..." );

	if( ( ret = mbedtls_ssl_config_defaults( &http_ssl->ssl_conf,
					MBEDTLS_SSL_IS_CLIENT,
					MBEDTLS_SSL_TRANSPORT_STREAM,
					MBEDTLS_SSL_PRESET_DEFAULT ) ) != 0 ) {
		HTTPC_LOGE(" failed\n  ! mbedtls_ssl_config_defaults returned -0x%x", -ret);
		goto exit;
	}

	HTTPC_LOGD( " ok" );

	/* OPTIONAL is not optimal for security,
	 * but makes interop easier in this simplified example */
	mbedtls_ssl_conf_authmode( &http_ssl->ssl_conf, MBEDTLS_SSL_VERIFY_OPTIONAL );
	mbedtls_ssl_conf_ca_chain( &http_ssl->ssl_conf, &http_ssl->cacert, NULL );
	mbedtls_ssl_conf_own_cert( &http_ssl->ssl_conf, &http_ssl->clicert, &http_ssl->pkey);
	mbedtls_ssl_conf_rng( &http_ssl->ssl_conf, mbedtls_ctr_drbg_random, &http_ssl->ctr_drbg );
	mbedtls_ssl_conf_dbg( &http_ssl->ssl_conf, httpc_debug, stdout );
	mbedtls_ssl_conf_read_timeout( &http_ssl->ssl_conf, HTTP_SSL_READ_TIMEOUT);

	HTTPC_LOGD( "  . Setting up SSL..." );
	if( ( ret = mbedtls_ssl_setup( &http_ssl->ssl_ctx, &http_ssl->ssl_conf ) ) != 0 ) {
		HTTPC_LOGE(" failed\n  ! mbedtls_ssl_setup returned -0x%x", -ret);
		goto exit;
	}

	HTTPC_LOGD( " ok" );

	HTTPC_LOGD( "  . Setting up SSL hostname..." );
	if( ( ret = mbedtls_ssl_set_hostname( &http_ssl->ssl_ctx, info->host ) ) != 0 ) {
		HTTPC_LOGE(" failed\n  ! mbedtls_ssl_set_hostname returned %d", ret);
		goto exit;
	}

	HTTPC_LOGD( " ok" );

	mbedtls_ssl_set_bio( &http_ssl->ssl_ctx, &http_ssl->net_ctx, mbedtls_net_send, mbedtls_net_recv, mbedtls_net_recv_timeout );

	/*
	 * 4. Handshake
	 */
	HTTPC_LOGD( "  . Performing the SSL/TLS handshake..." );

	while( ( ret = mbedtls_ssl_handshake( &http_ssl->ssl_ctx ) ) != 0 ) {
		if( ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE ) {
			HTTPC_LOGE(" failed\n  ! mbedtls_ssl_handshake returned -0x%x\n", -ret );
			goto exit;
		}
	}

	HTTPC_LOGD( " ok\n    [ Protocol is %s ]\n    [ Ciphersuite is %s ]",
			mbedtls_ssl_get_version( &http_ssl->ssl_ctx ),
			mbedtls_ssl_get_ciphersuite( &http_ssl->ssl_ctx ));

	/*
	 * 5. Verify the server certificate
	 */
#ifdef VERIFY_SERVER_CERT
	HTTPC_LOGD( "  . Verifying peer X.509 certificate..." );

	/* In real life, we probably want to bail out when ret != 0 */
	if( ( flags = mbedtls_ssl_get_verify_result( &http_ssl->ssl_ctx ) ) != 0 ) {
		char vrfy_buf[512];

		HTTPC_LOGE(" failed" );

		mbedtls_x509_crt_verify_info( vrfy_buf, sizeof( vrfy_buf ), "  ! ", flags );

		HTTPC_LOGE("%s\n", vrfy_buf);
	}
	else
		HTTPC_LOGD( " ok" );
#endif
exit:
	return ret;
}

static int httpc_ssl_close(con_handle_t *ssl_handle)
{
	http_ssl_t *http_ssl = (http_ssl_t *)(*ssl_handle);
	int ret = 0;

	do {
		ret = mbedtls_ssl_close_notify(&http_ssl->ssl_ctx);
	} while(ret == MBEDTLS_ERR_SSL_WANT_WRITE);

	mbedtls_net_free( &http_ssl->net_ctx );

	mbedtls_x509_crt_free( &http_ssl->cacert );
	mbedtls_x509_crt_free( &http_ssl->clicert );
	mbedtls_pk_free( &http_ssl->pkey );
	mbedtls_ssl_free( &http_ssl->ssl_ctx );
	mbedtls_ssl_config_free( &http_ssl->ssl_conf );
	mbedtls_ctr_drbg_free( &http_ssl->ctr_drbg );
	mbedtls_entropy_free( &http_ssl->entropy );
	nrc_mem_free(http_ssl);

	return 0;
}

/* This function will attempt to check if the network condition is OK to send ssl data. */
/* If the data cannot be sent without blocking, it will return HTTPC_RET_ERROR_TLS_SEND_FAIL. */
static int httpc_ssl_write(con_handle_t *ssl_handle, const unsigned char *buf, size_t len)
{
	http_ssl_t *http_ssl = (http_ssl_t *)(*ssl_handle);
	mbedtls_ssl_context *ssl = (mbedtls_ssl_context*)(*ssl_handle);
	size_t bytesSent = 0;
	int frags = 0, ret = 0;

	for(bytesSent = 0, frags = 0; bytesSent < len; bytesSent += ret, frags++) {
		while((ret = mbedtls_ssl_write(ssl, buf + bytesSent, len - bytesSent)) <= 0) {
			if(ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
				HTTPC_LOGE("failed\n  ! mbedtls_ssl_write returned -0x%x", -ret);
				return HTTPC_RET_ERROR_TLS_SEND_FAIL;
			}
		}
	}
	HTTPC_LOGI("[%s] bytesSent %d", __func__, bytesSent);

	return HTTPC_RET_OK;
}
#endif

static int httpc_connect(con_handle_t *handle, const char *url, http_info_t *info, ssl_certs_t *certs)
{
	int ret = HTTPC_RET_OK;

	ret = httpc_parse_url(url, info);
	if (ret != HTTPC_RET_OK) {
		HTTPC_LOGE("httpc_parse_url() returned %d", ret);
		return ret;
	}
	*handle = INVALID_HANDLE;
	if (info->scheme == HTTP) {
		ret = httpc_conn(handle, info->host, info->port);
	}
#if defined(SUPPORT_HTTPS_CLIENT)
	else {
		if (httpc_ssl_conn(handle, info, certs) != 0) {
			ret = HTTPC_RET_ERROR_CONNECTION;
		}
	}
#endif
	HTTPC_LOGI("[%s] result:%d, handle:0x%08x", __func__, ret, *handle);
	if (ret != HTTPC_RET_OK) {
		HTTPC_LOGE("HTTP Client connection failed.");
	}
	return ret;
}

static int httpc_send_header(con_handle_t *handle, int method, http_info_t *info)
{
	int ret = -1;
	const char *method_str = NULL;
	size_t default_header_len = 0;
	size_t header_len = 0;
	char *default_header = NULL;

	switch (method) {
		case METHOD_GET:
			method_str = "GET";
			break;
		case METHOD_POST:
			method_str = "POST";
			break;
		case METHOD_PUT:
			method_str = "PUT";
			break;
		case METHOD_DELETE:
			method_str = "DELETE";
			break;
		default:
			HTTPC_LOGE("[%s] Invalid HTTP method: %d", __func__, method);
			return HTTPC_RET_ERROR_INVALID_METHOD;
	}

	default_header_len = strlen(method_str) + strlen(info->uri) +
			strlen(info->host) + strlen(info->port) + 32;
	default_header = (char *)nrc_mem_malloc(default_header_len);
	if (!default_header) {
		HTTPC_LOGE("[%s] allocating memory for default_header failed.", __func__);
		goto exit;
	}
	memset(default_header, 0, default_header_len);

	HTTPC_LOGI("--- Request Header ---\n");
	/* Send Request-Line of the Request Header */
	header_len = snprintf((char *)default_header, default_header_len, "%s %s HTTP/1.1\r\nHost: %s:%s\r\n",
			method_str, info->uri, info->host, info->port);

	if (header_len > default_header_len) {
		HTTPC_LOGE("[%s] Header length exceeds buffer size.", __func__);
		ret = HTTPC_RET_ERROR_HEADER_SEND_FAIL;
		goto exit;
	}

	HTTPC_LOGI("%s", default_header);
	if (*handle < MEMP_NUM_NETCONN) {
		ret = send(*handle, default_header, strlen(default_header), 0);
	}
#if defined(SUPPORT_HTTPS_CLIENT)
	else {
		ret = httpc_ssl_write(handle, (unsigned char *) default_header, strlen(default_header));
	}
#endif
	if (ret < HTTPC_RET_OK) {
		HTTPC_LOGE("[%s] send request-line and host fail.", __func__);
		goto exit;
	}

	if (info->header != NULL) {
		HTTPC_LOGI("%s", info->header);
		if (*handle < MEMP_NUM_NETCONN) {
			ret = send(*handle, info->header, strlen(info->header), 0);
		}
#if defined(SUPPORT_HTTPS_CLIENT)
		else {
			ret = httpc_ssl_write(handle, (unsigned char *) info->header, strlen(info->header));
		}
#endif
		if (ret < HTTPC_RET_OK) {
			HTTPC_LOGE("[%s] send custom header fail.", __func__);
			goto exit;
		}
	}

	if (*handle < MEMP_NUM_NETCONN) {
		/* Close header */
		ret = send(*handle, "\r\n", 2, 0);
	}
#if defined(SUPPORT_HTTPS_CLIENT)
	else {
		ret = httpc_ssl_write(handle, (const unsigned char*)"\r\n", 2);
	}
#endif
	if (ret < HTTPC_RET_OK) {
		HTTPC_LOGE("[%s] closing header fail.", __func__);
		goto exit;
	}

exit:
	if (default_header) {
		nrc_mem_free(default_header);
	}

	if (ret < HTTPC_RET_OK){
		return HTTPC_RET_ERROR_HEADER_SEND_FAIL;
	}
	return HTTPC_RET_OK;
}

static int httpc_send_body(con_handle_t *handle, http_info_t *info) {
	const unsigned char *data = (const unsigned char *)info->data->data_out;
	size_t data_len = info->data->data_out_length;
	int ret = HTTPC_RET_OK;

	if (*handle < MEMP_NUM_NETCONN) {
		ret = send(*handle, data, data_len, 0);
	}
#if defined(SUPPORT_HTTPS_CLIENT)
	else {
		ret = httpc_ssl_write(handle, data, data_len);
	}
#endif
	if (ret < HTTPC_RET_OK) {
		HTTPC_LOGE("[%s] send request body fail. %d", __func__, ret);
		return HTTPC_RET_ERROR_BODY_SEND_FAIL;
	}
	return HTTPC_RET_OK;
}

static int httpc_send_req(con_handle_t *handle, int method, http_info_t *info)
{
	int ret = HTTPC_RET_ERROR_CONNECTION;

	if (*handle == INVALID_HANDLE) {
		return HTTPC_RET_ERROR_INVALID_HANDLE;
	}

	ret = httpc_send_header(handle, method, info);
	if (ret != HTTPC_RET_OK) {
		return ret;
	}

	if (method == METHOD_POST || method == METHOD_PUT) {
		ret = httpc_send_body(handle, info);
	}
	return ret;
}

void httpc_close(con_handle_t *handle)
{
	if (*handle != INVALID_HANDLE) {
		if (*handle < MEMP_NUM_NETCONN) {
			close(*handle);
		}
#if defined(SUPPORT_HTTPS_CLIENT)
		else {
			httpc_ssl_close(handle);
		}
#endif
		*handle = INVALID_HANDLE;
	}
	HTTPC_LOGI("[%s] httpc_close()", __func__);
}

static httpc_ret_e httpc_common(con_handle_t *handle, const char *url, int method, const char *header, httpc_data_t *data, ssl_certs_t *certs)
{
	httpc_ret_e ret = HTTPC_RET_ERROR_CONNECTION;
	http_info_t info;

	memset(&info, 0x0, sizeof(http_info_t));

	ret = httpc_connect(handle, url, &info, certs);
	if(ret != HTTPC_RET_OK)
		return ret;

	info.header = header;
	info.data = data;
	ret = httpc_send_req(handle, method, &info);
	if (ret != HTTPC_RET_OK)
		goto exit;

	ret = nrc_httpc_recv_response(handle, data);

exit:
	return ret;
}

httpc_ret_e nrc_httpc_get(con_handle_t *handle, const char *url, const char *custom_header, httpc_data_t *data, ssl_certs_t *certs)
{
	return httpc_common(handle, url, METHOD_GET, custom_header, data, certs);
}

httpc_ret_e nrc_httpc_post(con_handle_t *handle, const char *url, const char *custom_header, httpc_data_t *data, ssl_certs_t *certs)
{
	return httpc_common(handle, url, METHOD_POST, custom_header, data, certs);
}

httpc_ret_e nrc_httpc_put(con_handle_t *handle, const char *url, const char *custom_header, httpc_data_t *data, ssl_certs_t *certs)
{
	return httpc_common(handle, url, METHOD_PUT, custom_header, data, certs);
}

httpc_ret_e nrc_httpc_delete(con_handle_t *handle, const char *url, const char *custom_header, httpc_data_t *data, ssl_certs_t *certs)
{
	return httpc_common(handle, url, METHOD_DELETE, custom_header, data, certs);
}

httpc_ret_e nrc_httpc_recv_response(con_handle_t *handle, httpc_data_t *data)
{
	int size = 0;
	httpc_ret_e ret = HTTPC_RET_OK;
	data->recved_size = 0;

	if (*handle < MEMP_NUM_NETCONN) {
		size = recv(*handle, data->data_in, data->data_in_length, 0);
	}
#if defined(SUPPORT_HTTPS_CLIENT)
	else {
		http_ssl_t *ssl = (http_ssl_t*)(*handle);
		size = mbedtls_ssl_read(&ssl->ssl_ctx, (unsigned char*)data->data_in, data->data_in_length);
	}
#endif

	HTTPC_LOGI("[%s] size=%d", __func__, size);
	if (size > 0) {
		data->recved_size = size;
		ret = HTTPC_RET_OK;
	} else if (size == 0) {
		ret = HTTPC_RET_CON_CLOSED;
	} else {
		ret = HTTPC_RET_ERROR_CONNECTION;
	}
	return ret;
}

void nrc_httpc_close(con_handle_t *handle)
{
	httpc_close(handle);
}
