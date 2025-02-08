/*
 * MIT License
 *
 * Copyright (c) 2024 Newracom, Inc.
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
#include "lwip/sys.h"
#include "lwip/sockets.h"
#include "lwip/errno.h"

static int sockfd = -1;

#define SOCK_TIMEOUT_SECONDS 1
#define MAX_RECV_LEN (4*1024)
#define MAX_RETRY 5
#define MAX_SEND_RETRY 2

/****************************************************************************
 * FunctionName : open_connection
 * Description  : create tcp sockets and connect to server
 * Parameters   : void
 * Returns	    : true or false
 *****************************************************************************/
bool open_connection(char *remote_address, uint16_t port)
{
#ifdef CONFIG_IPV6
	struct sockaddr_in6 dest_addr;
#else
	struct sockaddr_in dest_addr;
#endif
	struct sockaddr_in *addr_in = (struct sockaddr_in *) &dest_addr;
	ip_addr_t remote_addr;
	int ret = 0;

	if (ipaddr_aton((char *) remote_address, &remote_addr)) {
		if (IP_IS_V4(&remote_addr)) {
			nrc_usr_print("[%s] IPv4...\n", __func__);
			struct sockaddr_in *d_addr = (struct sockaddr_in *) &dest_addr;
			addr_in->sin_family = AF_INET;
			addr_in->sin_len = sizeof(struct sockaddr_in);
			addr_in->sin_port = htons(port);
			addr_in->sin_addr.s_addr = inet_addr(remote_address);
		} else {
#ifdef CONFIG_IPV6
			nrc_usr_print("[%s] IPv6...\n", __func__);
			struct sockaddr_in6 *d_addr = (struct sockaddr_in6 *) &dest_addr;
			dest_addr.sin6_family = AF_INET6;
			dest_addr.sin6_len = sizeof(struct sockaddr_in6);
			dest_addr.sin6_port = htons(port);
			dest_addr.sin6_flowinfo = 0;
			dest_addr.sin6_scope_id = ip6_addr_zone(ip_2_ip6(&remote_addr));
			inet6_addr_from_ip6addr(&dest_addr.sin6_addr, ip_2_ip6(&remote_addr));
#else
		nrc_usr_print("[%s] Unknown address type %s\n", __func__, remote_address);
		nrc_usr_print("[%s] Enable IPv6 to handle IPv6 remote address...\n", __func__);
		return false;
#endif
		}
	} else {
		nrc_usr_print("[%s] Address %s not valid or enable IPv6 support.\n", __func__, remote_address);
		nrc_usr_print("[%s] use nvs set remote_address <ip address>\n", __func__);
		return false;
	}

	nrc_usr_print("[%s] Connecting to %s port %d...\n", __func__,
					remote_address, port);

	sockfd = socket(addr_in->sin_family, SOCK_STREAM, IPPROTO_TCP);
	if (sockfd < 0) {
		nrc_usr_print("Create socket failed\n");
		return false;
	}

	ret = connect(sockfd, (struct sockaddr *) &dest_addr, sizeof(dest_addr));
	if (ret < 0) {
		nrc_usr_print("open_connection failed\n");
		close(sockfd);
		sockfd = -1;
		return false;
	}

	return true;
}

void close_connection(void)
{
	if(sockfd >= 0) {
		nrc_usr_print("close_connection :%d\n", sockfd);
		shutdown(sockfd, SHUT_RDWR);
		close(sockfd);
		sockfd = -1;
	}
}

/****************************************************************************
 * FunctionName : retry_tcp_connection
 * Description  : retry tcp connection for given address and port
 * Parameters   : remote address, port
 * Returns	    : true or false
 *****************************************************************************/
static bool retry_tcp_connection(char *remote_address, uint16_t port)
{
	int bSuccess = true;

	close_connection();

	bSuccess = open_connection(remote_address, port);
	if (bSuccess == false) {
		return false;
	}

	nrc_usr_print("retry_tcp_connection success!!! \n");
	return true;
}

static int send_socket_data(char* data, int data_length)
{
	int ret = 0;
	int send_retry_count = 0;
	int send_len = 0;
	int remain_len = data_length;

	struct timeval select_timeout;
	fd_set fdWrite;

	/* setting select_timeout */
	select_timeout.tv_sec = 2;
	select_timeout.tv_usec = 0;

	while (remain_len > 0) {
		FD_ZERO(&fdWrite);
		FD_SET(sockfd, &fdWrite);
		ret = select(sockfd + 1, NULL, &fdWrite, NULL, &select_timeout);
		if (ret > 0) {
			if (FD_ISSET(sockfd, &fdWrite)) {
				FD_CLR(sockfd, &fdWrite);
//				nrc_usr_print("[%s] sending data with size (%d)\n", __func__, remain_len);
				send_len = send(sockfd, data, remain_len, 0);
				if (send_len < 0) {
					if(send_retry_count <  MAX_SEND_RETRY) {
						send_retry_count++;
						continue;
					} else {
						nrc_usr_print("[%s] MAX retry (%d) reached, returning false\n",
									  __func__, send_retry_count);
						return 0;
					}
				}
//				nrc_usr_print("[%s] %d bytes sent completed\n", __func__, send_len);
				send_retry_count = 0;
				data += send_len;
				remain_len -= send_len;
			}
		} else if (ret == 0) {
			nrc_usr_print("[%s] couldn't send...\n", __func__);
			return 0;
		} else {
			nrc_usr_print("[%s] 'select' failed inside send method\n", __func__);
			return -1;
		}
	}

//	nrc_usr_print("[%s] %d bytes sent...\n", __func__, send_len);
	return send_len;
}

static int receive_socket_data(char* recv_buffer, int buffer_len)
{
	int ret = 0;
	int recv_len = 0;
	struct timeval select_timeout;
	fd_set fdRead;

	/* setting select_timeout */
	select_timeout.tv_sec = 2;
	select_timeout.tv_usec = 0;

	FD_ZERO(&fdRead);
	FD_SET(sockfd, &fdRead);
	ret = select(sockfd + 1, &fdRead, NULL, NULL, &select_timeout);

	if (ret > 0) {
		if (FD_ISSET(sockfd, &fdRead)) {
			FD_CLR(sockfd, &fdRead);
			recv_len = recv(sockfd, recv_buffer, buffer_len-1, 0);
			if (recv_len <= 0) {
				nrc_usr_print("recv failed\n");
				return -1;
			}
		}
		return recv_len;
	} else if (ret == 0) {
		nrc_usr_print("nothing to receive, continue...\n");
		return 0;
	} else {
		nrc_usr_print("'select' failed inside recv method\n");
		return -1;
	}
}

/****************************************************************************
 * FunctionName : upload_data_packet
 * Description  : close connection and open connection again
 * Parameters   : char* data      - data pointer
 *                int data_length - data length
 * Returns	    : true or false
 *****************************************************************************/
bool upload_data_packet(char *remote_address, uint16_t port, char* data, int data_length)
{
	int retryCount = 0;

	while (retryCount < MAX_RETRY) {
		if (send_socket_data(data, data_length) > 0) {
			return true;
		} else {
			nrc_usr_print("upload_data_packet attempting retry %d\n", retryCount);
			if(retry_tcp_connection(remote_address, port)){
				nrc_usr_print("Restart upload_data_packet!!!\n");
				retryCount = 0;
			}else{
				retryCount++;
			}
		}
	}
	return false;
}
