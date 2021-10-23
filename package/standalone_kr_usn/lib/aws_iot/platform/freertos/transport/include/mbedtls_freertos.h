#ifndef MBEDTLS_FREERTOS_H_
#define MBEDTLS_FREERTOS_H_

/**************************************************/
/******* DO NOT CHANGE the following order ********/
/**************************************************/

/* Logging related header files are required to be included in the following order:
 * 1. Include the header file "logging_levels.h".
 * 2. Define LIBRARY_LOG_NAME and  LIBRARY_LOG_LEVEL.
 * 3. Include the header file "logging_stack.h".
 */

/* Include header that defines log levels. */
#include "logging_levels.h"

/* Logging configuration for the transport interface implementation which uses
 * OpenSSL and Sockets. */
#ifndef LIBRARY_LOG_NAME
    #define LIBRARY_LOG_NAME     "Transport_MbedTLS_Sockets"
#endif
#ifndef LIBRARY_LOG_LEVEL
    #define LIBRARY_LOG_LEVEL    LOG_ERROR
#endif

#include "logging_stack.h"

/************ End of logging configuration ****************/

#include <stdbool.h>

#include "mbedtls/platform.h"
#include "mbedtls/net.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/certs.h"
#include "mbedtls/x509.h"
#include "mbedtls/error.h"
#include "mbedtls/debug.h"
#include "mbedtls/timing.h"

#include "sockets_freertos.h"

typedef struct MbedTLSParams
{
    int32_t socketDescriptor;
    int flags;
    mbedtls_ssl_context ssl;
    mbedtls_ssl_config conf;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_x509_crt cacert;
	mbedtls_x509_crt clicert;
	mbedtls_pk_context pkey;
	mbedtls_net_context server_fd;
} MbedTLSParams_t;

typedef enum MbedTLSStatus
{
    MBEDTLS_SUCCESS = 0,         /**< Function successfully completed. */
    MBEDTLS_INVALID_PARAMETER,   /**< At least one parameter was invalid. */
    MBEDTLS_INSUFFICIENT_MEMORY, /**< Insufficient memory required to establish connection. */
    MBEDTLS_INVALID_CREDENTIALS, /**< Provided credentials were invalid. */
    MBEDTLS_HANDSHAKE_FAILED,    /**< Performing TLS handshake with server failed. */
    MBEDTLS_API_ERROR,           /**< A call to a system API resulted in an internal error. */
    MBEDTLS_DNS_FAILURE,         /**< Resolving hostname of the server failed. */
    MBEDTLS_CONNECT_FAILURE      /**< Initial connection to the server failed. */
} MbedTLSStatus_t;

/**
 * @brief Contains the credentials to establish a TLS connection.
 */
typedef struct MbedTLSCredentials
{
    bool ServerVerificationFlag;
    /**
     * @brief An array of ALPN protocols. Set to NULL to disable ALPN.
     *
     * See [this link]
     * (https://aws.amazon.com/blogs/iot/mqtt-with-tls-client-authentication-on-port-443-why-it-is-useful-and-how-it-works/)
     * for more information.
     */
    const char * pAlpnProtos;

    /**
     * @brief Length of the ALPN protocols array.
     */
    uint32_t alpnProtosLen;

    /**
     * @brief Set a host name to enable SNI. Set to NULL to disable SNI.
     *
     * @note This string must be NULL-terminated because the OpenSSL API requires it to be.
     */
    const char * sniHostName;

    /**
     * @brief Set the value for the TLS max fragment length (TLS MFLN)
     *
     * OpenSSL allows this value to be in the range of:
     * [512, 16384 (SSL3_RT_MAX_PLAIN_LENGTH)]
     *
     * @note By setting this to 0, OpenSSL uses the default value,
     * which is 16384 (SSL3_RT_MAX_PLAIN_LENGTH).
     */
    uint16_t maxFragmentLength;

    /**
     * @brief Filepaths to certificates and private key that are used when
     * performing the TLS handshake.
     *
     * @note These strings must be NULL-terminated because the OpenSSL API requires them to be.
     */
    const char * pRootCa;     /**< @brief the trusted server root CA string. */
    const char * pClientCert; /**< @brief the client certificate string. */
    const char * pPrivateKey; /**< @brief the client certificate's private key string. */
} MbedTLSCredentials_t;


MbedTLSStatus_t MbedTLS_Connect(NetworkContext_t * pNetworkContext,
                                 const ServerInfo_t * pServerInfo,
                                 const MbedTLSCredentials_t * pMbedTLSCredentials,
                                 uint32_t sendTimeoutMs,
                                uint32_t recvTimeoutMs );

MbedTLSStatus_t MbedTLS_Disconnect(const NetworkContext_t * pNetworkContext);

int32_t MbedTLS_Recv(NetworkContext_t * pNetworkContext,
                     void *pBuffer,
                     size_t bytesToRecv);

int32_t MbedTLS_Send(NetworkContext_t * pNetworkContext,
                     const void *pBuffer,
                     size_t bytesToSend);
#endif /* ifndef MBEDTLS_FREERTOS_H_ */
