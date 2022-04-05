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

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>


/***************************************************************************************/

typedef struct
{
#define REMOTE_UDP 		(1 << 0)
#define REMOTE_TCP 		(1 << 1)
#define REMOTE_SERVER	(1 << 2)
#define REMOTE_CLIENT	(1 << 3)

	int mode;

	union
	{
		uint16_t port;

		struct
		{
			uint16_t bind_port;
		} udp;

		struct
		{
			uint16_t listen_port;
		} tcp_server;

		struct
		{
			uint16_t server_port;
			char server_ip[15 + 1];
		} tcp_client;
	};

	bool echo;
	bool verbose;
} remote_opt_t;

/***************************************************************************************/

static int socket_reuse_address (int sockfd)
{
	int optval = 1;
	int optlen = sizeof(int);

	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, (socklen_t)optlen) < 0)
	{
		perror("setsockopt()");
		return -1;
	}

	return 0;
}

static int remote_create (remote_opt_t *opt)
{
#define TCP_SERVER_LISTEN_BACKLOG	1

	struct sockaddr_in addr;
	int addr_len = sizeof(addr);
	int sockfd;

	switch (opt->mode)
	{
		case REMOTE_UDP:
			sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
			if (sockfd < 0)
			{
				perror("socket()");
				return -1;
			}

			if (socket_reuse_address(sockfd) < 0)
				return -1;

			addr.sin_family = AF_INET;
			addr.sin_port = htons(opt->udp.bind_port);
			addr.sin_addr.s_addr = htonl(INADDR_ANY);

			if (bind(sockfd, (struct sockaddr *)&addr, (socklen_t)addr_len) < 0)
			{
				perror("bind()");
				return -1;
			}

			break;

		case REMOTE_TCP | REMOTE_SERVER:
			sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			if (sockfd < 0)
			{
				perror("socket()");
				return -1;
			}

			if (socket_reuse_address(sockfd) < 0)
				return -1;

			addr.sin_family = AF_INET;
			addr.sin_port = htons(opt->tcp_server.listen_port);
			addr.sin_addr.s_addr = htonl(INADDR_ANY);

			if (bind(sockfd, (struct sockaddr *)&addr, (socklen_t)addr_len) < 0)
			{
				perror("bind()");
				return -1;
			}

			if (listen(sockfd, TCP_SERVER_LISTEN_BACKLOG) < 0)
			{
				perror("listen()");
				return -1;
			}

			printf("LISTEN ...\n");

			sockfd = accept(sockfd, (struct sockaddr *)&addr, (socklen_t *)&addr_len);
			if (sockfd < 0)
			{
				perror("accept()");
				return -1;
			}

			printf("CONNECT: addr=%s port=%d\n\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));

			break;

		case REMOTE_TCP | REMOTE_CLIENT:
			sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			if (sockfd < 0)
			{
				perror("socket()");
				return -1;
			}

			if (socket_reuse_address(sockfd) < 0)
				return -1;

			addr.sin_family = AF_INET;
			addr.sin_port = htons(opt->tcp_client.server_port);
			addr.sin_addr.s_addr = inet_addr(opt->tcp_client.server_ip);

			if (connect(sockfd, (struct sockaddr *)&addr, (socklen_t)addr_len) < 0)
			{
				perror("connect()");
				return -1;
			}

			printf("CONNECT: addr=%s port=%d\n\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
			break;

		default:
			return -1;
	}

	return sockfd;
}

static int remote_delete (int sockfd)
{
	close(sockfd);

	return 0;
}

static void remote_loop (int sockfd, remote_opt_t *opt)
{
	bool udp = !!(opt->mode & REMOTE_UDP);
	struct sockaddr_in from;
	int from_len = sizeof(struct sockaddr_in);
	char buf[4096];
	uint32_t cnt;
	uint32_t len;
	int ret;

	if (!udp)
	{
		ret = getpeername(sockfd, (struct sockaddr *)&from, (socklen_t *)&from_len);
		if (ret < 0)
		{
			perror("getpeername()");
			return;
		}
	}

	for (cnt = len = 0 ; ; )	
	{
		if (udp)
			ret = recvfrom(sockfd, buf, sizeof(buf), 0, (struct sockaddr *)&from, (socklen_t *)&from_len);
		else
			ret = recv(sockfd, buf, sizeof(buf), 0);

		if (ret == 0)
			continue;
		else if (ret < 0)
		{
			if (errno == EAGAIN)
				continue;
			else if (errno != 0)
				perror("recvfrom()");

			break;
		}

		cnt++;
		len += ret;

		if (opt->verbose)
			printf("RECV: addr=%s port=%d len=%d (%u, %u)\n", inet_ntoa(from.sin_addr), ntohs(from.sin_port), ret, cnt, len);
		else
			printf("RECV: %d (%u)\n", ret, len);

		if (opt->echo)
		{
			if (udp)
				ret = sendto(sockfd, buf, ret, 0, (struct sockaddr *)&from, (socklen_t)from_len);
			else
				ret = send(sockfd, buf, ret, 0);

			if (ret < 0)
			{
				perror("sendto()");
				break;
			}

			if (opt->verbose)
				printf("SEND: addr=%s port=%d len=%d\n", inet_ntoa(from.sin_addr), ntohs(from.sin_port), ret);
			else
				printf("SEND: %d\n", ret);
		}
	}
}

/***************************************************************************************/

static void remote_version (void)
{
	const char *version = "1.1.0";

	printf("raspi-atcmd-remote version %s\n", version);
 	printf("Copyright (c) 2019-2020  <NEWRACOM LTD>\n");
}

static void remote_help (char *cmd)
{
	remote_version();

	printf("\n");
	printf("Usage:\n");
	printf("  $ %s -u [-p <bind_port>] [-e]\n", cmd);
	printf("  $ %s -t -s [-p <listen_port>] [-e]\n", cmd);
	printf("  $ %s -t -c <server_ip> [-p <server_port>] [-e]\n", cmd);
	printf("\n");

	printf("UDP:\n");
	printf("  -u, --udp             use UDP.\n");
	printf("\n");

	printf("TCP:\n");
	printf("  -t, --tcp             use TCP\n");
	printf("  -s, --server          run in server mode\n");
	printf("  -c, --client #        run in client mode\n");
	printf("\n");

	printf("UDP/TCP:\n");
	printf("  -p, --port #          set port number (default: 50000)\n");
	printf("  -e, --echo            enable echo for received packets (default: disable)\n");
	printf("\n");

	printf("  -v, --version         print version information and quit.\n");
	printf("  -h, --help            print this message and quit.\n");
}

static int remote_option (int argc, char *argv[], remote_opt_t *opt)
{
	struct option opt_info[] =
	{
		{ "udp",		no_argument,		0,	'u' },

		{ "tcp",		no_argument,		0,	't' },
		{ "server",		no_argument,		0,	's' },
		{ "client",		required_argument,	0,	'c' },

		{ "port",		required_argument,	0,	'p' },
		{ "echo",		no_argument,		0,	'e' },

		{ "version",	no_argument,		0,	'v' },
		{ "help",		no_argument,		0,	'h' },

		{ 0, 0, 0, 0 }
	};

	int opt_idx;
	int ret;

	if (argc < 2)
	{
		remote_help(argv[0]);
		return -1;
	}

	opt->mode = 0;
	opt->port = 50000;
	opt->echo = false;
#ifdef CONFIG_VERBOSE	
	opt->verbose = true;
#else
	opt->verbose = false;
#endif

	while (1)
	{
		ret = getopt_long(argc, argv, "utsc:p:evh", opt_info, &opt_idx);

		switch (ret)
		{
			case -1: // end
				switch (opt->mode)
				{
					case REMOTE_UDP:
					case REMOTE_UDP|REMOTE_SERVER:
					case REMOTE_UDP|REMOTE_CLIENT:
						printf("[ UDP ]\n");
						printf("  - bind_port : %u\n", opt->udp.bind_port);

						opt->mode &= ~(REMOTE_SERVER | REMOTE_CLIENT);
						break;

					case REMOTE_TCP|REMOTE_SERVER:
						printf("[ TCP_SERVER ]\n");
						printf("  - listen_port : %u\n", opt->tcp_server.listen_port);
						break;

					case REMOTE_TCP|REMOTE_CLIENT:
						printf("[ TCP_CLIENT ]\n");
						printf("  - server_ip : %s\n", opt->tcp_client.server_ip);
						printf("  - server_port : %u\n", opt->tcp_client.server_port);
						break;

					default:
						printf("invalid options\n");
						return -1;
				}

				printf("  - echo : %s\n", opt->echo ? "on" : "off");
				return 0;

			case 0:
				break;

			case 'u':
				opt->mode |= REMOTE_UDP;
				break;

			case 't':
				opt->mode |= REMOTE_TCP;
				break;

			case 's':
				opt->mode |= REMOTE_SERVER;
				break;

			case 'c':
				opt->mode |= REMOTE_CLIENT;
				strcpy(opt->tcp_client.server_ip, optarg);
				break;

			case 'p':
				opt->port = atoi(optarg);
				break;

			case 'e':
				opt->echo = true;
				break;

			case 'v':
				remote_version();
				return 1;

			case 'h':
				remote_help(argv[0]);
				return 1;

			default:
				return -1;
		}
	}
}

/***************************************************************************************/

int main (int argc, char *argv[])
{
	int sockfd = -1;
	remote_opt_t opt;

	if (remote_option(argc, argv, &opt) == 0)
	{
		sockfd = remote_create(&opt);
		if (sockfd < 0)
			return -1;

		remote_loop(sockfd, &opt);

		remote_delete(sockfd);
	}

	return 0;
}

