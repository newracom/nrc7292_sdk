/*
 * MIT License
 *
 * Copyright (c) 2020 Newracom, Inc.
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

#include "MQTTNrcImpl.h"

#include "lwip/opt.h"
#include "lwip/netif.h"
#include "lwip/tcpip.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"

#if LWIP_SOCKET
#include "lwip/sockets.h"
#endif

#include "system.h"

#if defined( SUPPORT_MBEDTLS )
#include "mbedtls/net.h"
#include "mbedtls/ssl.h"
#include "mbedtls/certs.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/debug.h"
#endif

#if defined( SUPPORT_MBEDTLS )
typedef struct {
	mbedtls_ssl_context ssl_ctx;        /* mbedtls ssl context */
	mbedtls_net_context net_ctx;        /* Fill in socket id */
	mbedtls_ssl_config ssl_conf;        /* SSL configuration */
	mbedtls_x509_crt cacert;
	mbedtls_x509_crt clicert;
	mbedtls_pk_context pkey;
} mqtt_ssl_t;
#endif

int ThreadStart(Thread* thread, void (*fn)(void*), void* arg)
{
	int rc = 0;
	uint16_t usTaskStackSize = (configMINIMAL_STACK_SIZE * 1);
	UBaseType_t uxTaskPriority = uxTaskPriorityGet(NULL); /* set the priority as the same as the calling task*/

	rc = xTaskCreate(fn,    /* The function that implements the task. */
		"MQTTTask",         /* Just a text name for the task to aid debugging. */
		usTaskStackSize,    /* The stack size is defined in FreeRTOSIPConfig.h. */
		arg,                /* The task parameter, not used in this case. */
		uxTaskPriority,     /* The priority assigned to the task is defined in FreeRTOSConfig.h. */
		&thread->task);     /* The task handle is not used. */

	return rc;
}

void MutexInit(Mutex* mutex)
{
	mutex->sem = xSemaphoreCreateMutex();
}

int MutexLock(Mutex* mutex)
{
	return xSemaphoreTake(mutex->sem, portMAX_DELAY);
}

int MutexUnlock(Mutex* mutex)
{
	return xSemaphoreGive(mutex->sem);
}

void TimerCountdownMS(Timer* timer, unsigned int timeout_ms)
{
	timer->xTicksToWait = timeout_ms / portTICK_PERIOD_MS; /* convert milliseconds to ticks */
	vTaskSetTimeOutState(&timer->xTimeOut); /* Record the time at which this function was entered. */
}

void TimerCountdown(Timer* timer, unsigned int timeout)
{
	TimerCountdownMS(timer, timeout * 1000);
}

int TimerLeftMS(Timer* timer)
{
	xTaskCheckForTimeOut(&timer->xTimeOut, &timer->xTicksToWait); /* updates xTicksToWait to the number left */
	return (timer->xTicksToWait < 0) ? 0 : (timer->xTicksToWait * portTICK_PERIOD_MS);
}

char TimerIsExpired(Timer* timer)
{
	return xTaskCheckForTimeOut(&timer->xTimeOut, &timer->xTicksToWait) == pdTRUE;
}

void TimerInit(Timer* timer)
{
	timer->xTicksToWait = 0;
	memset(&timer->xTimeOut, '\0', sizeof(timer->xTimeOut));
}

int nrc_sock_read(Network* n, unsigned char* buffer, int len, int timeout_ms)
{
	int rc = 0;
	int recvlen = 0;
	int ret = -1;
	fd_set fdset;
	struct timeval tv;

	FD_ZERO(&fdset);
	FD_SET(n->my_socket, &fdset);

	tv.tv_sec = timeout_ms / 1000;
	tv.tv_usec = (timeout_ms % 1000) * 1000;

	ret = select(n->my_socket + 1, &fdset, NULL, NULL, &tv);

	if (ret < 0) {
		return -10;
	} else if (ret == 0) {
		/* timeout */
		return ret;
	} else if (ret == 1) {
		do {
			rc = recv(n->my_socket, buffer + recvlen, len - recvlen, 0);
			if (rc > 0) {
				recvlen += rc;
			} else {
				return -30;
			}
		} while (recvlen < len);
	}
	return recvlen;
}

int nrc_sock_write(Network* n, unsigned char* buffer, int len, int timeout_ms)
{
	int rc = 0;
	int ret = -1;
	fd_set fdset;
	struct timeval tv;

	FD_ZERO(&fdset);
	FD_SET(n->my_socket, &fdset);

	tv.tv_sec = timeout_ms / 1000;
	tv.tv_usec = (timeout_ms % 1000) * 1000;

	ret = select(n->my_socket + 1, NULL, &fdset, NULL, &tv);

	if (ret < 0) {
		/* error */
		return -1;
	} else if (ret == 0) {
		/* timeout */
		return ret;
	} else if (ret == 1) {
		rc = write(n->my_socket, buffer, len);
	}

	return rc;
}

void NetworkInit(Network* n)
{
	n->my_socket = -1;
	n->mqttread = nrc_sock_read;
	n->mqttwrite = nrc_sock_write;
}

int NetworkConnect(Network* n, char* addr, int port)
{
	struct sockaddr_in address;
	int rc = -1;
	struct addrinfo *result = NULL;
	struct addrinfo hints = {0, AF_UNSPEC, SOCK_STREAM, IPPROTO_TCP, 0, NULL, NULL, NULL};

	if ((rc = getaddrinfo(addr, NULL, &hints, &result)) == 0) {
		struct addrinfo *res = result;

		/* prefer ip4 addresses */
		while (res) {
			if (res->ai_family == AF_INET) {
				result = res;
				break;
			}
			res = res->ai_next;
		}

		if (result->ai_family == AF_INET) {
			address.sin_port = htons(port);
			address.sin_family = AF_INET;
			address.sin_addr = ((struct sockaddr_in *)(result->ai_addr))->sin_addr;
			system_printf("[%s]ip address found: %s\n", __func__, inet_ntoa(address.sin_addr));
		} else {
			rc = -1;
		}
		freeaddrinfo(result);
	}

	/* create client socket */
	if (rc == 0) {
		int opval = 1;
		n->my_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (n->my_socket < 0)
			return -1;

		/* connect remote servers*/
		rc = connect(n->my_socket, (struct sockaddr *)&address, sizeof(address));
		if (rc < 0) {
			close(n->my_socket);
			system_printf("[%s] error: mqtt socket connect fail: rc=%d\n", __func__, rc);
			return -2;
		}

		setsockopt(n->my_socket, IPPROTO_TCP, TCP_NODELAY, &opval, sizeof(opval));
	}

	return rc;
}

int NetworkDisconnect(Network* n)
{
	int sock = n->my_socket;

	shutdown(sock, SHUT_RDWR);
	close(sock);

	return 0;
}

#if defined( SUPPORT_MBEDTLS )
u32_t mqtt_avRandom()
{
	return (((u32_t)rand() << 16) + rand());
}

static int mqtt_ssl_random(void *p_rng, unsigned char *output, size_t output_len)
{
	uint32_t rnglen = output_len;
	uint8_t   rngoffset = 0;

	while (rnglen > 0) {
		*(output + rngoffset) = (unsigned char)mqtt_avRandom() ;
		rngoffset++;
		rnglen--;
	}
	return 0;
}

static void mqtt_ssl_debug( void *ctx, int level, const char *file, int line, const char *str )
{
	system_printf("[%s:%d] %s\n", file, line, str);
}

int mqtt_real_confirm(int verify_result)
{
#define VERIFY_ITEM(Result, Item, ErrMsg) \
	do { \
		if (((Result) & (Item)) != 0) { \
			system_printf(ErrMsg); \
		} \
	} while (0)

	system_printf("certificate verification result: 0x%02x\n", verify_result);
	VERIFY_ITEM(verify_result, MBEDTLS_X509_BADCERT_EXPIRED, " ! fail ! server certificate has expired\n");
	VERIFY_ITEM(verify_result, MBEDTLS_X509_BADCERT_REVOKED, " ! fail ! server certificate has been revoked\n");
	VERIFY_ITEM(verify_result, MBEDTLS_X509_BADCERT_CN_MISMATCH, " ! fail ! CN mismatch\n");
	VERIFY_ITEM(verify_result, MBEDTLS_X509_BADCERT_NOT_TRUSTED, " ! fail ! self-signed or not signed by a trusted CA\n");

	return 0;
}

#if !defined( RELEASE )
static int ssl_parse_crt(mbedtls_x509_crt *crt)
{
	char buf[1024];
	mbedtls_x509_crt *local_crt = crt;
	int i = 0;
	while (local_crt) {
		system_printf("\n########## %d\n", i);
		mbedtls_x509_crt_info(buf, sizeof(buf) - 1, "", local_crt);
		char str[512];
		const char *start, *cur;
		start = buf;
		for (cur = buf; *cur != '\0'; cur++) {
			if (*cur == '\n') {
				size_t len = cur - start + 1;
				if (len > 511) {
					len = 511;
				}
				memcpy(str, start, len);
				str[len] = '\0';
				start = cur + 1;
				system_printf("- %s", str);
			}
		}
		system_printf("crt content:%d!\n####################\n", strlen(buf));
		local_crt = local_crt->next;
		i++;
	}
	return i;
}
#endif

int mqtt_ssl_read_all(Network *n, unsigned char *buffer, int len, int timeout_ms)
{
	size_t readLen = 0;
	int ret = -1;
	mqtt_ssl_t *ssl = (mqtt_ssl_t *)(n->my_socket);

	mbedtls_ssl_conf_read_timeout(&ssl->ssl_conf, timeout_ms);
	while (readLen < len) {
		ret = mbedtls_ssl_read(&ssl->ssl_ctx, (unsigned char *)(buffer + readLen), (len - readLen));
		if (ret > 0) {
			readLen += ret;
		} else if (ret == 0) {
			return -2; // eof
		} else if (ret == MBEDTLS_ERR_SSL_TIMEOUT) {
			return 0;
		} else {
			if (ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) {
				return -2;
			}
			return -1; //Connnection error
		}
	}
	return readLen;
}

int mqtt_ssl_write_all(Network *n, unsigned char *buffer, int len, int timeout_ms)
{
	size_t writtenLen = 0;
	int ret = -1;

	while (writtenLen < len) {
		ret = mbedtls_ssl_write((mbedtls_ssl_context*)(n->my_socket), (unsigned char *)(buffer + writtenLen), (len - writtenLen));
		if (ret > 0) {
			writtenLen += ret;
			continue;
		} else if (ret == 0) {
			return writtenLen;
		} else {
			return -1; //Connnection error
		}
	}
	return writtenLen;
}

void mqtt_ssl_disconnect(Network *n)
{
	mqtt_ssl_t *ssl = (mqtt_ssl_t *)(n->my_socket);
	if (ssl == NULL)
		return;

	mbedtls_ssl_close_notify(&ssl->ssl_ctx);
	mbedtls_net_free(&ssl->net_ctx);
#if defined(MBEDTLS_X509_CRT_PARSE_C)
	mbedtls_x509_crt_free( &ssl->cacert);
	if ((ssl->pkey).pk_info != NULL) {
		mbedtls_x509_crt_free(&ssl->clicert);
		mbedtls_pk_free(&ssl->pkey);
	}
#endif
	mbedtls_ssl_free(&ssl->ssl_ctx);
	mbedtls_ssl_config_free(&ssl->ssl_conf);
}

int NetworkConnectTLS(Network *n, const char *addr, int po, Certs *certs)
{
	int ret = -1;
	char port[10] = {0,};
	snprintf(port, 10, "%d", po);

	mqtt_ssl_t *ssl = (mqtt_ssl_t*)pvPortMalloc(sizeof(mqtt_ssl_t));
	if (ssl == NULL) {
		system_printf("[%s] memory malloc error.\n", __func__);
		return -10;
	}

	/*
	* Initialize the RNG and the session data
	*/
#if defined(MBEDTLS_DEBUG_C)
	mbedtls_debug_set_threshold(0);
#endif
	mbedtls_net_init(&ssl->net_ctx);
	mbedtls_ssl_init(&ssl->ssl_ctx);
	mbedtls_ssl_config_init(&ssl->ssl_conf);
#if defined(MBEDTLS_X509_CRT_PARSE_C)
	mbedtls_x509_crt_init(&ssl->cacert);
	mbedtls_x509_crt_init(&ssl->clicert);
	mbedtls_pk_init(&ssl->pkey);
#endif

	/*
	* Initialize certificates
	*/
#if defined(MBEDTLS_X509_CRT_PARSE_C)
	system_printf("  . Loading the CA root certificate ...");
	if (certs->ca_cert != NULL) {
#if defined(MBEDTLS_CERTS_C)
		ret = mbedtls_x509_crt_parse(&ssl->cacert, (const unsigned char *)certs->ca_cert, certs->ca_cert_length);
		if (ret != 0) {
			system_printf(" failed! x509parse_crt returned -0x%04x\n\n", -ret);
			return ret;
		}
#endif
	}
#if !defined( RELEASE )
	ssl_parse_crt(&ssl->cacert);
#endif
	system_printf(" ok (%d skipped)\n", ret);

	/*
	* Setup Client Cert/Key
	*/
	if ( certs->client_cert != NULL && certs->client_pk != NULL) {
#if defined(MBEDTLS_CERTS_C)
		system_printf("  . Loading the client certificate ...");
		ret = mbedtls_x509_crt_parse(&ssl->clicert, (const unsigned char *)certs->client_cert, certs->client_cert_length);
#else
		{
			ret = 1;
			system_printf("MBEDTLS_CERTS_C not defined.\n");
		}
#endif
		if ( ret != 0 ) {
			system_printf(" failed! mbedtls_x509_crt_parse returned -0x%x\n\n", -ret);
			return ret;
		}
#if !defined( RELEASE )
		ssl_parse_crt(&ssl->clicert);
#endif
		system_printf(" ok\n");

#if defined(MBEDTLS_CERTS_C)
		system_printf("  . Parsing the client private key[%s] ...", certs->client_pk_pwd);
			ret = mbedtls_pk_parse_key(&ssl->pkey,
			(const unsigned char *)certs->client_pk, certs->client_pk_length,
			(const unsigned char *)certs->client_pk_pwd, certs->client_pk_pwd_length);
#else
		{
			ret = 1;
			system_printf("MBEDTLS_CERTS_C not defined.\n");
		}
#endif
		if ( ret != 0 ) {
			system_printf(" failed! mbedtls_pk_parse_key returned -0x%x\n\n", -ret);
			return ret;
		}
		system_printf(" ok\n");
	}
#endif /* MBEDTLS_X509_CRT_PARSE_C */

	/*
	* Start the connection
	*/
	system_printf("  . Connecting to tcp/%s/%s...", addr, port);
	if (0 != (ret = mbedtls_net_connect(&ssl->net_ctx, addr, port, MBEDTLS_NET_PROTO_TCP))) {
		system_printf(" failed! net_connect returned -0x%04x\n\n", -ret);
		return ret;
	}
	system_printf(" ok\n");

	/*
	* Setup stuff
	*/
	system_printf("  . Setting up the SSL/TLS structure...");
	if ( ( ret = mbedtls_ssl_config_defaults(&ssl->ssl_conf,
				MBEDTLS_SSL_IS_CLIENT,
				MBEDTLS_SSL_TRANSPORT_STREAM,
				MBEDTLS_SSL_PRESET_DEFAULT ) ) != 0 ) {
		system_printf(" failed! mbedtls_ssl_config_defaults returned %d\n\n", ret);
		return ret;
	}
	system_printf(" ok\n");

	/*
	* OPTIONAL is not optimal for security,
	* but makes interop easier in this simplified example
	*/
	if (certs->ca_cert != NULL) {
		mbedtls_ssl_conf_authmode(&ssl->ssl_conf, MBEDTLS_SSL_VERIFY_OPTIONAL );
	} else {
		mbedtls_ssl_conf_authmode(&ssl->ssl_conf, MBEDTLS_SSL_VERIFY_NONE);
	}

#if defined(MBEDTLS_X509_CRT_PARSE_C)
	mbedtls_ssl_conf_ca_chain(&ssl->ssl_conf, &ssl->cacert, NULL);

	if ( ( ret = mbedtls_ssl_conf_own_cert(&ssl->ssl_conf, &ssl->clicert, &ssl->pkey ) ) != 0 ) {
		system_printf(" failed! mbedtls_ssl_conf_own_cert returned %d\n\n", ret);
		return ret;
	}
#endif
	mbedtls_ssl_conf_rng(&ssl->ssl_conf, mqtt_ssl_random, NULL );
	mbedtls_ssl_conf_dbg(&ssl->ssl_conf, mqtt_ssl_debug, NULL );


	if ( ( ret = mbedtls_ssl_setup(&ssl->ssl_ctx, &ssl->ssl_conf) ) != 0 ) {
		system_printf(" failed! mbedtls_ssl_setup returned %d\n\n", ret);
		return ret;
	}
	mbedtls_ssl_set_hostname(&ssl->ssl_ctx, addr);
	mbedtls_ssl_set_bio( &ssl->ssl_ctx, &ssl->net_ctx, mbedtls_net_send, mbedtls_net_recv, mbedtls_net_recv_timeout);

	/*
	* Handshake
	*/
	system_printf("  . Performing the SSL/TLS handshake...");
	while ((ret = mbedtls_ssl_handshake(&ssl->ssl_ctx)) != 0) {
		if ((ret != MBEDTLS_ERR_SSL_WANT_READ) && (ret != MBEDTLS_ERR_SSL_WANT_WRITE)) {
			system_printf(" failed! mbedtls_ssl_handshake returned -0x%04x\n\n", -ret);
			return ret;
		}
	}
	system_printf(" ok\n");

#if 1
	/*
	* Verify the server certificate
	*/
	system_printf("  . Verifying peer X.509 certificate...");
	if (0 != (ret = mqtt_real_confirm(mbedtls_ssl_get_verify_result(&ssl->ssl_ctx)))) {
		system_printf(" failed! verify result not confirmed.\n\n");
		return ret;
	}
#else
#if defined(MBEDTLS_X509_CRT_PARSE_C)
	/*
	* Verify the server certificate
	*/
	system_printf( "  . Verifying peer X.509 certificate..." );

	uint32_t flags;
	if( ( flags = mbedtls_ssl_get_verify_result(&ssl->ssl_ctx) ) != 0 )
	{
		char vrfy_buf[512];

		system_printf( " failed\n" );

		mbedtls_x509_crt_verify_info( vrfy_buf, sizeof( vrfy_buf ),
		"  ! ", flags );

		system_printf( "%s\n", vrfy_buf );
	} else {
		system_printf( " ok\n" );
	}

	if( mbedtls_ssl_get_peer_cert(&ssl->ssl_ctx) != NULL ) {
		char buf[1024];
		system_printf( "  . Peer certificate information    ...\n" );
		mbedtls_x509_crt_info( (char *) buf, sizeof( buf ) - 1, "      ",
		mbedtls_ssl_get_peer_cert(&ssl->ssl_ctx) );
		system_printf( "%s\n", buf );
	}
#endif /* MBEDTLS_X509_CRT_PARSE_C */
#endif

	n->my_socket = (int)(ssl);
	n->mqttread = mqtt_ssl_read_all;
	n->mqttwrite = mqtt_ssl_write_all;
	n->disconnect = mqtt_ssl_disconnect;

	return 0;
}

int NetworkDisconnectTLS(Network* n)
{
	mqtt_ssl_t *ssl = (mqtt_ssl_t *)(n->my_socket);
	int sock = n->my_socket;

	shutdown(sock, SHUT_RDWR);
	close(sock);

	if (ssl != NULL) {
		vPortFree(ssl);
	}

	return 0;
}
#endif
