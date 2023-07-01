/*
 * MIT License
 *
 * Copyright (c) 2023 Newracom, Inc.
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
#include "lwip/netdb.h"

#include "nrc_tcp_server.h"

static int run_loop = 1;
static int maxdesc = 0;

int write_to_socket(int sock, char *buffer, size_t length)
{
	int written = 0;
	int wrote = 0;

	do {
		wrote = write(sock, buffer + wrote, length - wrote);
		if (wrote < 0) {
			return -1;
		} else {
			written += wrote;
		}
	} while (written < length);

	return written;
}

static int new_connection(int sock)
{
	struct sockaddr_in incoming;
	socklen_t len = 0;
	int client_sock;

	len = sizeof(incoming);
	getsockname(sock, (struct sockaddr *) &incoming, &len);
	if ((client_sock = accept(sock, (struct sockaddr *) &incoming, &len)) < 0) {
		return -1;
	}

	if (fcntl(client_sock, F_SETFL, O_NDELAY) == -1) {
		return -1;
	}

	nrc_usr_print("New connection made : %d\n", client_sock);
	return client_sock;
}

static int new_client(tcp_client_t **client_list, int sock)
{
	int client = -1;;
	tcp_client_t *c_data = NULL;

	if ((client = new_connection(sock)) < 0) {
		return -1;
	}

	c_data = (tcp_client_t *) nrc_mem_malloc(sizeof(tcp_client_t));
	if (!c_data) {
		return -1;
	}
	memset(c_data, 0, sizeof(tcp_client_t));

	c_data->sock = client;
	c_data->next = *client_list;
	*client_list = c_data;

	if (client > maxdesc) {
		maxdesc = client;
	}

	nrc_usr_print("new client created (%d)...\n", client);
	return 0;
}

static int process_input(tcp_client_t *client, tcp_input_handler input_handler)
{
	int received = 0;
	char buffer[1024];

	memset(buffer, 0, sizeof(buffer));
	if ((received = read(client->sock, buffer, sizeof(buffer))) > 0) {
		input_handler(client->sock, buffer, received);
	} else {
		return -1;
	}
	return 0;
}

void close_socket(tcp_client_t **client_list, tcp_client_t *client)
{
	tcp_client_t *tmp;

	if (!client) {
		return;
	}

	nrc_usr_print("Closing socket %d...\n", client->sock);
	close(client->sock);

	if (client->sock == maxdesc) {
		maxdesc--;
	}

	if (client == *client_list) {
		*client_list = (*client_list)->next;
	} else {
		for (tmp = *client_list; (tmp->next != client) && tmp; tmp = tmp->next);
		tmp->next = client->next;
	}

	nrc_mem_free(client);
}

static void server_loop(tcp_client_t **client_list, int sock, tcp_input_handler input_handler)
{
	fd_set input_set, output_set, exec_set;
	tcp_client_t *client;
	tcp_client_t *next_client;

	maxdesc = sock;
	while (run_loop) {
		FD_ZERO(&input_set);
		FD_ZERO(&output_set);
		FD_ZERO(&exec_set);
		FD_SET(sock, &input_set);

		for (client = *client_list; client; client = next_client) {
			next_client = client->next;
			FD_SET(client->sock, &input_set);
			FD_SET(client->sock, &output_set);
			FD_SET(client->sock, &output_set);
		}

		if (select(maxdesc + 1, &input_set, &output_set, &exec_set, NULL) < 0) {
			return;
		}

		if (FD_ISSET(sock, &input_set)) {
			if (new_client(client_list, sock) < 0) {
				nrc_usr_print("error new connection\n");
			}
		}

		for (client = *client_list; client; client = next_client) {
			next_client = client->next;
			if (FD_ISSET(client->sock, &input_set)) {
				if (process_input(client, input_handler) < 0) {
					FD_CLR(client->sock, &input_set);
					FD_CLR(client->sock, &output_set);
					close_socket(client_list, client);
				}
			}
		}
	}
}

static int init_server_socket(unsigned short port)
{
	int sock;
	int reuse = 1;
	struct sockaddr_in server_addr;

	if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		return -1;
	}

	memset(&server_addr, 0, sizeof(struct sockaddr_in));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) < 0) {
		return -1;
	}
	if (bind(sock, (struct sockaddr *) &server_addr, sizeof(struct sockaddr_in)) < 0) {
		return -1;
	}

	if (listen(sock, 3) < 0) {
		return -1;
	}

	nrc_usr_print("Server socket created : %d\n", sock);
	return sock;
}

void start_server(unsigned short port, tcp_client_t **clients, tcp_input_handler input_handler)
{
	int server_socket = 0;

	if ((server_socket = init_server_socket(port)) < 0) {
		return;
	}
	server_loop(clients, server_socket, input_handler);
}
