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

#ifndef __NRC_HTTPC_API_H__
#define __NRC_HTTPC_API_H__

/** This macro defines the deault HTTP port.  */
#define HTTP_PORT   80

/** This macro defines the deault HTTPS port.  */
#define HTTPS_PORT 443

/** This macro defines the INVALID_HANDLE value.  */
#define INVALID_HANDLE 0xFFFFFFFF


/** This enumeration defines the API return type.  */
typedef enum {
	HTTPC_RET_ERROR_TLS_CONNECTION = -14,     /**< TLS connection fail. */
	HTTPC_RET_ERROR_PK_LOADING_FAIL = -13,    /**< Private key loading fail. */
	HTTPC_RET_ERROR_CERT_LOADING_FAIL = -12,  /**< Certificate loading fail. */
	HTTPC_RET_ERROR_SEED_FAIL = -11,          /**< Seed creation fail. */
	HTTPC_RET_ERROR_BODY_SEND_FAIL = -10,     /**< Request body send fail. */
	HTTPC_RET_ERROR_HEADER_SEND_FAIL = -9,    /**< Request Header send fail. */
	HTTPC_RET_ERROR_INVALID_HANDLE = -8,      /**< Invalid handle. */
	HTTPC_RET_ERROR_ALLOC_FAIL = -7,          /**< Memory allocation fail. */
	HTTPC_RET_ERROR_SCHEME_NOT_FOUND = -6,    /**< Scheme(http:// or https://) not found */
	HTTPC_RET_ERROR_SOCKET_FAIL = -5,         /**< Socket creation fail. */
	HTTPC_RET_ERROR_RESOLVING_DNS = -4,       /**< Cannot resolve the hostname. */
	HTTPC_RET_ERROR_CONNECTION = -3,          /**< Connection fail. */
	HTTPC_RET_ERROR_UNKNOWN = -2,             /**< Unknown error. */
	HTTPC_RET_CON_CLOSED = -1,                /**< Connection closed by remote. */
	HTTPC_RET_OK = 0,                         /**< Success */
} httpc_ret_e;

/** This typedef defines the con_handle_t variable type.  */
typedef int con_handle_t;

/** This structure defines the ssl_certs_t structure.  */
typedef struct {
	const char *server_cert;            /**< Server certification. */
	const char *client_cert;            /**< Client certification. */
	const char *client_pk;              /**< Client private key. */
	int server_cert_length;             /**< Server certification lenght, server_cert buffer size. */
	int client_cert_length;             /**< Client certification lenght, client_cert buffer size. */
	int client_pk_length;               /**< Client private key lenght, client_pk buffer size. */
} ssl_certs_t;

/** This structure defines the httpc_data_t structure.  */
typedef struct {
	char *data_out;                     /**< Pointer of the output buffer for data sending. */
	uint32_t data_out_length;           /**< Output buffer length. */
	char *data_in;                      /**< Pointer of the input buffer for data receiving. */
	uint32_t data_in_length;            /**< Input buffer length. */
	int recved_size;                    /**< Actuall received data size. */
} httpc_data_t;

/**********************************************
 * @brief Executes a GET request on a given URL
 *
 * @param handle: Connection handle
 *
 * @param url: URL for the request
 *
 * @param custom_header: Customized request header. the request-line("<method> <uri> HTTP/1.1") and "Host: <host-name>" will be sent in default internally. Other headers can be set as null-terminated string format.
 *
 * @param data: It's a pointer to the #httpc_data_t to manage the data sending and receiving.
 *
 * @param certs: It's a pointer to the #ssl_certs_t for the certificates.
 *
 * @return Please refer to #httpc_ret_e
 ***********************************************/
httpc_ret_e nrc_httpc_get(con_handle_t *handle, const char *url, const char *custom_header, httpc_data_t *data, ssl_certs_t *certs);

/**********************************************
 * @brief Executes a POST request on a given URL
 *
 * @param handle: Connection handle
 *
 * @param url: URL for the request
 *
 * @param custom_header: Customized request header. the request-line("<method> <uri> HTTP/1.1") and "Host: <host-name>" will be sent in default internally. Other headers can be set as null-terminated string format.
 *
 * @param data: It's a pointer to the #httpc_data_t to manage the data sending and receiving.
 *
 * @param certs: It's a pointer to the #ssl_certs_t for the certificates.
 *
 * @return Please refer to #httpc_ret_e
 ***********************************************/
httpc_ret_e nrc_httpc_post(con_handle_t *handle, const char *url, const char *custom_header, httpc_data_t *data, ssl_certs_t *certs);

/**********************************************
 * @brief Executes a PUT request on a given URL
 *
 * @param handle: Connection handle
 *
 * @param url: URL for the request
 *
 * @param custom_header: Customized request header. the request-line("<method> <uri> HTTP/1.1") and "Host: <host-name>" will be sent in default internally. Other headers can be set as null-terminated string format.
 *
 * @param data: It's a pointer to the #httpc_data_t to manage the data sending and receiving.
 *
 * @param certs: It's a pointer to the #ssl_certs_t for the certificates.
 *
 * @return Please refer to #httpc_ret_e
 ***********************************************/
httpc_ret_e nrc_httpc_put(con_handle_t *handle, const char *url, const char *custom_header,httpc_data_t *data, ssl_certs_t *certs);

/**********************************************
 * @brief Executes a DELETE request on a given URL
 *
 * @param handle: Connection handle
 *
 * @param url: URL for the request
 *
 * @param custom_header: Customized request header. the request-line("<method> <uri> HTTP/1.1") and "Host: <host-name>" will be sent in default internally. Other headers can be set as null-terminated string format.
 *
 * @param data: It's a pointer to the #httpc_data_t to manage the data sending and receiving.
 *
 * @param certs: It's a pointer to the #ssl_certs_t for the certificates.
 *
 * @return Please refer to #httpc_ret_e
 ***********************************************/
httpc_ret_e nrc_httpc_delete(con_handle_t *handle, const char *url, const char *custom_header, httpc_data_t *data, ssl_certs_t *certs);

/**********************************************
 * @brief Retrieves the response data when there're remains after executing the request functions
 *
 * @param handle: Connection handle
 *
 * @param data: It's a pointer to the #httpc_data_t to manage the data sending and receiving.
 *
 * @return Please refer to #httpc_ret_e
 ***********************************************/
httpc_ret_e nrc_httpc_recv_response(con_handle_t *handle, httpc_data_t *data);

/**********************************************
 * @brief Close connection
 *
 * @param handle: Connection handle
 *
 * @return N/A
 ***********************************************/
void nrc_httpc_close(con_handle_t *handle);


#endif // __NRC_HTTPC_API_H__
