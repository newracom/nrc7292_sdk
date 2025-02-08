/* Copyright (c) 2020 Cameron Harper
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * */
#include "transport.h"
#include "log.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>
#include "sockets_freertos.h"
#include "mbedtls_freertos.h"


struct NetworkContext
{
    MbedTLSParams_t * pParams;
};

static NetworkContext_t *pNetworkContext = NULL;

#define TRANSPORT_SEND_RECV_TIMEOUT_MS    ( 5000 )

bool transport_open_client(enum wic_schema schema, const char *host,
		uint16_t port, int *s, struct wic_inst *inst)
{
	struct addrinfo hints;
	int returnStatus = 0;
	ServerInfo_t serverInfo;

	memset( &serverInfo, 0, sizeof(ServerInfo_t) );
	memset( &hints, 0, sizeof( struct addrinfo ) );

	serverInfo.pHostName = host;
	serverInfo.hostNameLength = strlen(host);
	serverInfo.port = port;

	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

    switch (schema) {
	    case WIC_SCHEMA_HTTPS:
	    case WIC_SCHEMA_WSS:
	        {
				pNetworkContext = pvPortMalloc(sizeof(NetworkContext_t));
				if(!pNetworkContext)
					return false;
				memset( pNetworkContext, 0, sizeof( NetworkContext_t ) );

				pNetworkContext->pParams  = pvPortMalloc(sizeof(MbedTLSParams_t));
				if(!pNetworkContext->pParams){
					vPortFree(pNetworkContext);
					return false;
				}
				memset( pNetworkContext->pParams, 0, sizeof( MbedTLSParams_t ) );

				MbedTLSCredentials_t mbedTLSCredentials;
				memset( &mbedTLSCredentials, 0, sizeof( MbedTLSCredentials_t ) );
				mbedTLSCredentials.pRootCa = inst->root_ca;
				mbedTLSCredentials.pClientCert = inst->client_cert;
				mbedTLSCredentials.pPrivateKey = inst->private_key;

				returnStatus = MbedTLS_Connect(pNetworkContext,
									&serverInfo,
									&mbedTLSCredentials,
									TRANSPORT_SEND_RECV_TIMEOUT_MS,
									TRANSPORT_SEND_RECV_TIMEOUT_MS );
	        }
	        break;

		default:
			{
				returnStatus = Sockets_Connect((int32_t*)s,
									&serverInfo,
									TRANSPORT_SEND_RECV_TIMEOUT_MS,
									TRANSPORT_SEND_RECV_TIMEOUT_MS );
			}
    }

	return (returnStatus == 0) ? true : false;

}

void transport_write(int s, const void *data, size_t size, struct wic_inst *inst)
{
	enum wic_schema schema = inst->schema;

	int returnStatus = 0;

    switch (schema) {
	    case WIC_SCHEMA_HTTPS:
	    case WIC_SCHEMA_WSS:
	        {
				returnStatus = MbedTLS_Send(pNetworkContext, data, size );
	        }
	        break;

	    default:
	        {
				returnStatus = Sockets_Send(s, data,size);
	        }
    }
}

bool transport_recv(int s, struct wic_inst *inst)
{
	enum wic_schema schema = inst->schema;
    static uint8_t buffer[1000U];
    int bytes;
    size_t retval, pos;

	int returnStatus = 0;

    switch (schema) {
	    case WIC_SCHEMA_HTTPS:
	    case WIC_SCHEMA_WSS:
	        {
				bytes = MbedTLS_Recv(pNetworkContext, buffer, sizeof(buffer));
	        }
	        break;

	    default:
	        {
				bytes = Sockets_Recv(s, buffer, sizeof(buffer));
	        }
    }

    if (bytes > 0) {
        for (pos = 0U; pos < bytes; pos += retval) {
            retval = wic_parse(inst, &buffer[pos], bytes - pos);
        }
    }
#if 0
    if (bytes <= 0) {
        wic_close_with_reason(inst, WIC_CLOSE_ABNORMAL_2, NULL, 0);
    }
#endif
	return (bytes > 0);
}

void transport_close(int *s, struct wic_inst *inst)
{
	enum wic_schema schema = inst->schema;

    switch (schema) {
	    case WIC_SCHEMA_HTTPS:
	    case WIC_SCHEMA_WSS:
	        {
				if(pNetworkContext){
					MbedTLS_Disconnect(pNetworkContext);
					vPortFree(pNetworkContext->pParams);
					vPortFree(pNetworkContext);
					pNetworkContext = NULL;
				}
	        }
	        break;

	    default:
	        {
				if(*s >= 0) {
					Sockets_Disconnect((int32_t)*s );
					*s = -1;
				}
	        }
    }
}

