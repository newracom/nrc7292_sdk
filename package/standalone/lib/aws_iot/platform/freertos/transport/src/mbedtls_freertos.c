#include "mbedtls_freertos.h"
#include "aws_iot_error.h"
#include "mbedtls/ctr_drbg.h"

#ifdef MBEDTLS_DEBUG_C
#include "system.h"

#define IOT_SSL_DEBUG_LEVEL	0
static void _iot_tls_debug( void *ctx, int level,
					const char *file, int line, const char *str )
{
	system_printf( "%s", str );
}
#endif /* MBEDTLS_DEBUG_C */

static int _iot_tls_verify_cert(void *data, mbedtls_x509_crt *crt, int depth, uint32_t *flags) {
	char buf[1024];
	((void) data);

	LogDebug(("\nVerify requested for (Depth %d):\n", depth));
	mbedtls_x509_crt_info(buf, sizeof(buf) - 1, "", crt);
	LogDebug(("%s", buf));

	if((*flags) == 0) {
		LogDebug(("  This certificate has no flags\n"));
	} else {
		LogDebug((buf, sizeof(buf), "  ! ", *flags));
		LogDebug(("%s\n", buf));
	}

	return 0;
}

struct NetworkContext
{
	MbedTLSParams_t * pParams;
};

/*-----------------------------------------------------------*/
MbedTLSStatus_t MbedTLS_Connect(NetworkContext_t * pNetworkContext,
								const ServerInfo_t * pServerInfo,
								const MbedTLSCredentials_t * pMbedTLSCredentials,
								uint32_t sendTimeoutMs,
								uint32_t recvTimeoutMs )
{
    MbedTLSParams_t *pMbedTLSParams = NULL;
	const char *pers = "aws_iot_tls_wrapper";
	char portBuffer[6];
	char vrfy_buf[512];
	int ret = 0;

	/* Validate parameters. */
	if( ( pNetworkContext == NULL ) || ( pNetworkContext->pParams == NULL ) )
	{
		LogError(( "Parameter check failed: pNetworkContext is NULL." ));
		return MBEDTLS_INVALID_PARAMETER;
	}
	else if( pMbedTLSCredentials == NULL )
	{
		LogError(( "Parameter check failed: pMbedTLSCredentials is NULL." ));
		return MBEDTLS_INVALID_PARAMETER;
	}

	pMbedTLSParams = pNetworkContext->pParams;

	mbedtls_net_init(&(pMbedTLSParams->server_fd));
	mbedtls_ssl_init(&(pMbedTLSParams->ssl));
	mbedtls_ssl_config_init(&(pMbedTLSParams->conf));
	mbedtls_ctr_drbg_init(&(pMbedTLSParams->ctr_drbg));
	mbedtls_x509_crt_init(&(pMbedTLSParams->cacert));
	mbedtls_x509_crt_init(&(pMbedTLSParams->clicert));
	mbedtls_pk_init(&(pMbedTLSParams->pkey));

#if defined(MBEDTLS_DEBUG_C)
	mbedtls_debug_set_threshold(IOT_SSL_DEBUG_LEVEL);
	mbedtls_ssl_conf_dbg(&(pMbedTLSParams->conf), _iot_tls_debug, NULL);
#endif /* MBEDTLS_DEBUG_C */

	LogDebug(("\n  . Seeding the random number generator..."));
	mbedtls_entropy_init(&(pMbedTLSParams->entropy));
	if((ret = mbedtls_ctr_drbg_seed(&(pMbedTLSParams->ctr_drbg), mbedtls_entropy_func, &(pMbedTLSParams->entropy),
									(const unsigned char *) pers, strlen(pers))) != 0) {
		LogError((" failed\n  ! mbedtls_ctr_drbg_seed returned -0x%x\n", -ret));
		return NETWORK_MBEDTLS_ERR_CTR_DRBG_ENTROPY_SOURCE_FAILED;
	}

	LogDebug(("  . Loading the CA root certificate ..."));

	ret = mbedtls_x509_crt_parse(&(pMbedTLSParams->cacert),
								 (const unsigned char *)pMbedTLSCredentials->pRootCa,
								 strlen(pMbedTLSCredentials->pRootCa) + 1);

	if(ret < 0) {
		LogError((" failed\n  !  mbedtls_x509_crt_parse returned -0x%x while parsing root cert\n\n", -ret));
		return NETWORK_X509_ROOT_CRT_PARSE_ERROR;
	}
	LogDebug((" ok (%d skipped)\n", ret));

	LogDebug(("  . Loading the client cert. and key..."));
	ret = mbedtls_x509_crt_parse(&(pMbedTLSParams->clicert),
								 (const unsigned char *)pMbedTLSCredentials->pClientCert,
								 strlen(pMbedTLSCredentials->pClientCert) + 1);
	if(ret != 0) {
		LogError((" failed\n  !  mbedtls_x509_crt_parse returned -0x%x while parsing device cert\n\n", -ret));
		return NETWORK_X509_DEVICE_CRT_PARSE_ERROR;
	}

	ret = mbedtls_pk_parse_key(&(pMbedTLSParams->pkey),
							   (const unsigned char *)pMbedTLSCredentials->pPrivateKey,
							   strlen(pMbedTLSCredentials->pPrivateKey) + 1,
							   (const unsigned char *)"", strlen(""));

	if(ret != 0) {
		LogError((" failed\n  !  mbedtls_pk_parse_key returned -0x%x while parsing private key\n\n", -ret));
		LogDebug((" private key pem : %s ", pMbedTLSCredentials->pPrivateKey));
		return NETWORK_PK_PRIVATE_KEY_PARSE_ERROR;
	}
	LogDebug((" ok\n"));
	snprintf(portBuffer, 6, "%d", pServerInfo->port);
	LogDebug(("  . Connecting to %s/%s...", pServerInfo->pHostName, portBuffer));
	if((ret = mbedtls_net_connect(&(pMbedTLSParams->server_fd), pServerInfo->pHostName,
								  portBuffer, MBEDTLS_NET_PROTO_TCP)) != 0) {
		LogError((" failed\n  ! mbedtls_net_connect returned -0x%x\n\n", -ret));
		switch(ret) {
			case MBEDTLS_ERR_NET_SOCKET_FAILED:
				return NETWORK_ERR_NET_SOCKET_FAILED;
			case MBEDTLS_ERR_NET_UNKNOWN_HOST:
				return NETWORK_ERR_NET_UNKNOWN_HOST;
			case MBEDTLS_ERR_NET_CONNECT_FAILED:
			default:
				return NETWORK_ERR_NET_CONNECT_FAILED;
		};
	}

	ret = mbedtls_net_set_block(&(pMbedTLSParams->server_fd));
	if(ret != 0) {
		LogError((" failed\n  ! net_set_(non)block() returned -0x%x\n\n", -ret));
		return SSL_CONNECTION_ERROR;
	} LogDebug((" ok\n"));

	LogDebug(("  . Setting up the SSL/TLS structure..."));
	if((ret = mbedtls_ssl_config_defaults(&(pMbedTLSParams->conf), MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM,
										  MBEDTLS_SSL_PRESET_DEFAULT)) != 0) {
		LogError((" failed\n  ! mbedtls_ssl_config_defaults returned -0x%x\n\n", -ret));
		return SSL_CONNECTION_ERROR;
	}

	mbedtls_ssl_conf_verify(&(pMbedTLSParams->conf), _iot_tls_verify_cert, NULL);
	if(pMbedTLSCredentials->ServerVerificationFlag == true) {
		mbedtls_ssl_conf_authmode(&(pMbedTLSParams->conf), MBEDTLS_SSL_VERIFY_REQUIRED);
	} else {
		mbedtls_ssl_conf_authmode(&(pMbedTLSParams->conf), MBEDTLS_SSL_VERIFY_OPTIONAL);
	}
	mbedtls_ssl_conf_rng(&(pMbedTLSParams->conf), mbedtls_ctr_drbg_random, &(pMbedTLSParams->ctr_drbg));

	mbedtls_ssl_conf_ca_chain(&(pMbedTLSParams->conf), &(pMbedTLSParams->cacert), NULL);
	if((ret = mbedtls_ssl_conf_own_cert(&(pMbedTLSParams->conf), &(pMbedTLSParams->clicert), &(pMbedTLSParams->pkey))) != 0) {
		LogError((" failed\n  ! mbedtls_ssl_conf_own_cert returned %d\n\n", ret));
		return SSL_CONNECTION_ERROR;
	}

	mbedtls_ssl_conf_read_timeout(&(pMbedTLSParams->conf), recvTimeoutMs);

	/* Use the AWS IoT ALPN extension for MQTT if port 443 is requested. */
	if(443 == pServerInfo->port) {
		if((ret = mbedtls_ssl_conf_alpn_protocols(&(pMbedTLSParams->conf),
												  (const char **) &(pMbedTLSCredentials->pAlpnProtos))) != 0) {
			LogError((" failed\n  ! mbedtls_ssl_conf_alpn_protocols returned -0x%x\n\n", -ret));
			return SSL_CONNECTION_ERROR;
		}
	}

	/* Assign the resulting configuration to the SSL context. */
	if((ret = mbedtls_ssl_setup(&(pMbedTLSParams->ssl), &(pMbedTLSParams->conf))) != 0) {
		LogError((" failed\n  ! mbedtls_ssl_setup returned -0x%x\n\n", -ret));
		return SSL_CONNECTION_ERROR;
	}
	if((ret = mbedtls_ssl_set_hostname(&(pMbedTLSParams->ssl), pServerInfo->pHostName)) != 0) {
		LogError((" failed\n  ! mbedtls_ssl_set_hostname returned %d\n\n", ret));
		return SSL_CONNECTION_ERROR;
	}
	LogDebug(("\n\nSSL state connect : %d ", pMbedTLSParams->ssl.state));
	mbedtls_ssl_set_bio(&(pMbedTLSParams->ssl), &(pMbedTLSParams->server_fd), mbedtls_net_send, NULL,
						mbedtls_net_recv_timeout);
	LogDebug((" ok\n"));

	LogDebug(("\n\nSSL state connect : %d ", pMbedTLSParams->ssl.state));
	LogDebug(("  . Performing the SSL/TLS handshake..."));
	while((ret = mbedtls_ssl_handshake(&(pMbedTLSParams->ssl))) != 0) {
		if(ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
			LogError((" failed\n  ! mbedtls_ssl_handshake returned -0x%x\n", -ret));
			if(ret == MBEDTLS_ERR_X509_CERT_VERIFY_FAILED) {
				LogError(("    Unable to verify the server's certificate. "
						  "Either it is invalid,\n"
						  "    or you didn't set ca_file or ca_path "
						  "to an appropriate value.\n"
						  "    Alternatively, you may want to use "
						  "auth_mode=optional for testing purposes.\n"));
			}
			return SSL_CONNECTION_ERROR;
		}
	}

	LogDebug((" ok\n    [ Protocol is %s ]\n    [ Ciphersuite is %s ]\n", mbedtls_ssl_get_version(&(pMbedTLSParams->ssl)),
			  mbedtls_ssl_get_ciphersuite(&(pMbedTLSParams->ssl))));
	if((ret = mbedtls_ssl_get_record_expansion(&(pMbedTLSParams->ssl))) >= 0) {
		LogDebug(("    [ Record expansion is %d ]\n", ret));
	} else {
		LogDebug(("    [ Record expansion is unknown (compression) ]\n"));
	}

	LogDebug(("  . Verifying peer X.509 certificate..."));

	if(pMbedTLSCredentials->ServerVerificationFlag == true) {
		if((pMbedTLSParams->flags = mbedtls_ssl_get_verify_result(&(pMbedTLSParams->ssl))) != 0) {
			LogError((" failed\n"));
			mbedtls_x509_crt_verify_info(vrfy_buf, sizeof(vrfy_buf), "  ! ", pMbedTLSParams->flags);
			LogError(("%s\n", vrfy_buf));
			ret = SSL_CONNECTION_ERROR;
		} else {
			LogDebug((" ok\n"));
			ret = SUCCESS;
		}
	} else {
		LogDebug((" Server Verification skipped\n"));
		ret = SUCCESS;
	}

	mbedtls_ssl_conf_read_timeout(&(pMbedTLSParams->conf), recvTimeoutMs);

	return MBEDTLS_SUCCESS;
}

MbedTLSStatus_t MbedTLS_Disconnect(const NetworkContext_t * pNetworkContext)
{
	MbedTLSParams_t *pMbedTLSParams = pNetworkContext->pParams;
	mbedtls_ssl_context *ssl = &(pMbedTLSParams->ssl);
	int ret = 0;

	do {
		ret = mbedtls_ssl_close_notify(ssl);
	} while(ret == MBEDTLS_ERR_SSL_WANT_WRITE);

	/* clean up mbedtls data structure */
	mbedtls_net_free(&(pMbedTLSParams->server_fd));
	mbedtls_x509_crt_free(&(pMbedTLSParams->clicert));
	mbedtls_x509_crt_free(&(pMbedTLSParams->cacert));
	mbedtls_pk_free(&(pMbedTLSParams->pkey));
	mbedtls_ssl_free(ssl);
	mbedtls_ssl_config_free(&(pMbedTLSParams->conf));
	mbedtls_ctr_drbg_free(&(pMbedTLSParams->ctr_drbg));
	mbedtls_entropy_free(&(pMbedTLSParams->entropy));
	return MBEDTLS_SUCCESS;
}

int32_t MbedTLS_Recv(NetworkContext_t * pNetworkContext,
					 void *pBuffer,
					 size_t bytesToRecv)
{
	mbedtls_ssl_context *ssl = &(pNetworkContext->pParams->ssl);
	int bytesReceived = 0;

	// This read will timeout after IOT_SSL_READ_TIMEOUT if there's no data to be read
	bytesReceived = mbedtls_ssl_read(ssl, pBuffer, bytesToRecv);

	if (bytesReceived <= 0) {
		if (bytesReceived == MBEDTLS_ERR_SSL_WANT_READ) {
			bytesReceived = 0;
		} else if (bytesReceived == MBEDTLS_ERR_SSL_TIMEOUT) {
			bytesReceived = 0;
		} else {
			bytesReceived = -1;
		}
	}

	return bytesReceived;
}

int32_t MbedTLS_Send(NetworkContext_t * pNetworkContext,
					 const void *pBuffer,
					 size_t bytesToSend)
{
	size_t bytesSent = 0;
	int frags = 0;
	int ret = 0;

	mbedtls_ssl_context *ssl = &(pNetworkContext->pParams->ssl);

	for(bytesSent = 0, frags = 0;
		bytesSent < bytesToSend; bytesSent += ret, frags++) {
		while((ret = mbedtls_ssl_write(ssl, pBuffer + bytesSent, bytesToSend - bytesSent)) <= 0) {
			if(ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
				LogError((" failed\n  ! mbedtls_ssl_write returned -0x%x\n\n", -ret));
				/* All other negative return values indicate connection needs to be reset.
				 * Will be caught in ping request so ignored here */
				return -1;
			}
		}
	}

	return bytesSent;
}
