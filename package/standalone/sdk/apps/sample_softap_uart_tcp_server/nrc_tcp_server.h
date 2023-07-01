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

#ifndef __NRC_TCP_SERVER_H__
#define __NRC_TCP_SERVER_H__

struct tcp_client {
	int sock;
	struct tcp_client *next;
};

typedef struct tcp_client tcp_client_t;

typedef void (*tcp_input_handler)(int sock, char *buffer, unsigned int size);

int write_to_socket(int sock, char *buffer, size_t length);
void close_socket(tcp_client_t **client_list, tcp_client_t *client);
void start_server(unsigned short port, tcp_client_t **clients, tcp_input_handler input_handler);

#endif /* __NRC_TCP_SERVER_H__ */
