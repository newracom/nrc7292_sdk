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

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>

#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>


#define CONFIG_IPV4_ONLY

/***************************************************************************************/

#define ATCMD_DATA_LEN_MAX		(4 * 1024)

#define STR_IFNAME_LEN_MAX		16

#define STR_IP4ADDR_LEN_MIN		7
#define STR_IP4ADDR_LEN_MAX		15

#define STR_IP6ADDR_LEN_MIN		3
#define STR_IP6ADDR_LEN_MAX		45

#define STR_IPADDR_LEN_MIN		STR_IP6ADDR_LEN_MIN
#define STR_IPADDR_LEN_MAX		STR_IP6ADDR_LEN_MAX

#define TCP_LISTEN_BACKLOG		1
#define TCP_CONNECTION_MAX		1

typedef struct
{
#define REMOTE_DEFAULT_PORT		50000
	union
	{
		uint32_t flags;

		struct
		{
			uint32_t udp:1;
			uint32_t client:1;
			uint32_t ipv4:1;
			uint32_t ipv6:1;
			uint32_t echo:1;
			uint32_t verbose:1;
			uint32_t reserved:26;
		};
	};

	union
	{
		struct
		{
			int fd;
			uint16_t port;
		};

		struct
		{
			int server_fd;
			uint16_t listen_port;
			int clients_fd[TCP_CONNECTION_MAX];
		};

		struct
		{
			int client_fd;
			uint16_t server_port;
			char server_ip[STR_IPADDR_LEN_MAX + 1];
		   	char ifname[STR_IFNAME_LEN_MAX + 1];
		};
	};
} remote_info_t;

/***************************************************************************************/

static void remote_loop (remote_info_t *info)
{
	union
	{
		struct sockaddr_in ip4;
		struct sockaddr_in6 ip6;
	} from;

	int fd = (info->udp || info->client) ? info->fd : info->clients_fd[1];
   	bool ipv4_only = (info->ipv4 && !info->ipv6);
	int from_len = ipv4_only ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6);

	char addr[STR_IPADDR_LEN_MAX + 1];
	uint16_t port;
	uint8_t *buf;
	uint32_t buf_len;
	uint32_t cnt;
	uint32_t len;
	int ret;

	buf_len = ATCMD_DATA_LEN_MAX;
	buf = malloc(buf_len);
	if (!buf)
	{
		perror("malloc()");
		return;
	}

	if (!info->udp)
	{
		ret = getpeername(fd, (struct sockaddr *)&from, (socklen_t *)&from_len);
		if (ret < 0)
		{
			perror("getpeername()");
			return;
		}
	}

	for (cnt = len = 0 ; ; len += ret)
	{
		if (info->udp)
			ret = recvfrom(fd, buf, buf_len, 0, (struct sockaddr *)&from, (socklen_t *)&from_len);
		else
			ret = recv(fd, buf, buf_len, 0);

		if (ret <= 0)
		{
			if (errno == EAGAIN)
				ret = 0;
			else
			{
				perror("recvfrom()");
				break;
			}
		}

		if (ipv4_only)
		{
			inet_ntop(AF_INET, &from.ip4.sin_addr, addr, sizeof(addr));
			port = ntohs(from.ip4.sin_port);
		}
		else
		{
			inet_ntop(AF_INET6, &from.ip6.sin6_addr, addr, sizeof(addr));
			port = ntohs(from.ip6.sin6_port);
		}

		cnt++;

		if (info->verbose)
			printf("RECV: addr=%s port=%u len=%d (%u, %u)\n", addr, port, ret, cnt, len + ret);
		else
			printf("RECV: %d (%u)\n", ret, len + ret);

		if (info->echo)
		{
			if (info->udp)
				ret = sendto(fd, buf, ret, 0, (struct sockaddr *)&from, (socklen_t)from_len);
			else
				ret = send(fd, buf, ret, 0);

			if (ret < 0)
			{
				perror("sendto()");
				break;
			}

			if (info->verbose)
				printf("SEND: addr=%s port=%u len=%d\n", addr, port, ret);
			else
				printf("SEND: %d\n", ret);
		}
	}
}

static void remote_connected (struct sockaddr *addr, bool ipv4_only)
{
	if (ipv4_only)
	{
		struct sockaddr_in *_addr = (struct sockaddr_in *)addr;
		char ipaddr[STR_IP4ADDR_LEN_MAX + 1];

		inet_ntop(AF_INET, &_addr->sin_addr, ipaddr, sizeof(ipaddr));

		printf("CONNECT: ipaddr=%s port=%d\n\n", ipaddr, ntohs(_addr->sin_port));
	}
	else
	{
		struct sockaddr_in6 *_addr = (struct sockaddr_in6 *)addr;
		char ipaddr[STR_IP6ADDR_LEN_MAX + 1];
		char *p;
		int i;

		inet_ntop(AF_INET6, &_addr->sin6_addr, ipaddr, sizeof(ipaddr));

//		if (memcmp(ipaddr, "::ffff:", 7) == 0)
//			strcpy(ipaddr, ipaddr + 7);

		for (p = ipaddr, i = 0 ; i < 3 ; i++)
		{
			p = strchr(p, '.');
			if (!p)
				break;
		}

		if (i == 3)
		{
			p = strrchr(ipaddr, ':');
			if (p)
				strcpy(ipaddr, p + 1);
		}

		printf("CONNECT: ipaddr=%s port=%d\n\n", ipaddr, ntohs(_addr->sin6_port));
	}
}

static int remote_create_server (remote_info_t *info)
{
	bool ipv4_only = info->ipv4 && !info->ipv6;
	struct sockaddr *addr = NULL;
	socklen_t addrlen;
	int ret = -1;
	int i;

	for (i = 0 ; i < TCP_CONNECTION_MAX ; i++)
		info->clients_fd[i] = -1;

	if (ipv4_only)
	{
		addrlen = sizeof(struct sockaddr_in);
		addr = (struct sockaddr *)malloc(sizeof(struct sockaddr_in));
		if (addr)
		{
			struct sockaddr_in *_addr = (struct sockaddr_in *)addr;

			memset(_addr, 0, addrlen);

			_addr->sin_family = AF_INET;
			_addr->sin_port = htons(info->listen_port);
			_addr->sin_addr.s_addr = htonl(INADDR_ANY);
		}
	}
	else
	{
		addrlen = sizeof(struct sockaddr_in6);
		addr = (struct sockaddr *)malloc(sizeof(struct sockaddr_in6));
		if (addr)
		{
			struct sockaddr_in6 *_addr = (struct sockaddr_in6 *)addr;

			memset(_addr, 0, addrlen);

			_addr->sin6_family = AF_INET6;
			_addr->sin6_port = htons(info->listen_port);
			_addr->sin6_addr = in6addr_any;
		}
	}

	if (!addr)
		perror("malloc()");
	else
	{
		if (bind(info->fd, addr, addrlen) < 0)
			perror("bind()");
		else if (info->udp)
			ret = 0;
		else if (listen(info->fd, TCP_LISTEN_BACKLOG) < 0)
			perror("listen()");
		else
		{
			printf("LISTEN ...\n");

			info->clients_fd[1] = accept(info->fd, addr, &addrlen);
			if (info->clients_fd[1] < 0)
				perror("accept()");
			else
			{
				remote_connected(addr, ipv4_only);
				ret = 0;
			}
		}

		free(addr);
	}

	return ret;
}

static int remote_create_client (remote_info_t *info)
{
	bool ipv4_only = info->ipv4 && !info->ipv6;
	struct sockaddr *addr = NULL;
	socklen_t addrlen;
	int ret = -1;

	if (ipv4_only)
	{
		addrlen = sizeof(struct sockaddr_in);
		addr = (struct sockaddr *)malloc(sizeof(struct sockaddr_in));
		if (addr)
		{
			struct sockaddr_in *_addr = (struct sockaddr_in *)addr;

			memset(_addr, 0, addrlen);

			_addr->sin_family = AF_INET;
			_addr->sin_port = htons(info->server_port);
			inet_pton(AF_INET, info->server_ip, &_addr->sin_addr);
		}
	}
	else
	{
		addrlen = sizeof(struct sockaddr_in6);
		addr = (struct sockaddr *)malloc(sizeof(struct sockaddr_in6));
		if (addr)
		{
			struct sockaddr_in6 *_addr = (struct sockaddr_in6 *)addr;

			memset(_addr, 0, addrlen);

			_addr->sin6_family = AF_INET6;
			_addr->sin6_port = htons(info->server_port);
			inet_pton(AF_INET6, info->server_ip, &_addr->sin6_addr);
			if (strlen(info->ifname) > 0)
				_addr->sin6_scope_id = if_nametoindex(info->ifname);
		}
	}

	if (!addr)
		perror("malloc()");
	else
	{
		if (connect(info->fd, addr, addrlen) < 0)
			perror("connect()");
		else
		{
			remote_connected(addr, ipv4_only);
			ret = 0;
		}

		free(addr);
	}

	return ret;
}

static int remote_create_socket (remote_info_t *info)
{
	bool ipv4_only = info->ipv4 && !info->ipv6;
	bool ipv6_only = !info->ipv4 && info->ipv6;

	int fd = -1;
	int domain = ipv4_only ? AF_INET : AF_INET6;
	int type = info->udp ? SOCK_DGRAM : SOCK_STREAM;
	int protocol = info->udp ? IPPROTO_UDP : IPPROTO_TCP;

	int optval = 1;
	int optlen = sizeof(int);

	fd = socket(domain, type, protocol);
	if (fd < 0)
	{
		perror("socket()");
		return -1;
	}

	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optval, (socklen_t)optlen) < 0)
	{
		perror("setsockopt(), REUSEADDR");
		close(fd);
		return -1;
	}

	if (ipv6_only)
	{
		if (setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, &optval, (socklen_t)optlen) < 0)
		{
			perror("setsockopt(), IPV6_ONLY");
			close(fd);
			return -1;
		}
	}

	info->fd = fd;

	return 0;
}

static int remote_create (remote_info_t *info)
{
	if (remote_create_socket(info) < 0)
		return -1;

	if ((info->client ? remote_create_client(info) : remote_create_server(info)) != 0)
		return -1;

	return 0;
}

static int remote_delete (remote_info_t *info)
{
	if (info->fd >= 0)
	{
		if (!info->udp && !info->client)
		{
			int i;

			for (i = 0 ; i < TCP_CONNECTION_MAX ; i++)
			{
				if (info->clients_fd[i] >= 0)
					close(info->clients_fd[i]);
			}
		}

		close(info->fd);
	}

	return 0;
}

/***************************************************************************************/

static void remote_print_version (void)
{
	const char *version = "1.2.0";

	printf("raspi-atcmd-remote version %s\n", version);
 	printf("Copyright (c) 2019-2023  <NEWRACOM LTD>\n");
}

static void remote_print_help (char *cmd)
{
	remote_print_version();

	printf("\n");
	printf("Usage:\n");
#if !defined(CONFIG_IPV4_ONLY)
	printf("  $ %s -s [-p <listen_port>] [-u] [-4|-6] [-e]\n", cmd);
#else
	printf("  $ %s -s [-p <listen_port>] [-u] [-e]\n", cmd);
#endif
	printf("  $ %s -c <server_ip> [-p <server_port>] [-u] [-e]\n", cmd);
	printf("\n");

	printf("Options:\n");
	printf("  -s, --server             run in server mode\n");
	printf("  -c, --client #           run in client mode\n");
	printf("  -p, --port #             set server port to listen on or connect to (default: 50000)\n");
	printf("  -u, --udp                use UDP\n");
#if !defined(CONFIG_IPV4_ONLY)
	printf("  -4, --ipv4               use only IPv4 in server mode\n");
	printf("  -6, --ipv6               use only IPv6 in server mode\n");
#endif
	printf("  -e, --echo               enable echo for received packets (default: off)\n");
	printf("  -v, --version            print version information and quit\n");
	printf("  -h, --help               print this message and quit\n");
}

static int remote_get_options (int argc, char *argv[], remote_info_t *info)
{
	struct option opt_info[] =
	{
		{ "server",		no_argument,		0,	's' },
		{ "client",		required_argument,	0,	'c' },
		{ "port",		required_argument,	0,	'p' },
		{ "udp",		no_argument,		0,	'u' },
#if !defined(CONFIG_IPV4_ONLY)
		{ "ipv4",		no_argument,		0,	'4' },
		{ "ipv6",		no_argument,		0,	'6' },
#endif
		{ "echo",		no_argument,		0,	'e' },
		{ "version",	no_argument,		0,	'v' },
		{ "help",		no_argument,		0,	'h' },

		{ 0, 0, 0, 0 }
	};

	int opt_idx;
	int ret;

	if (argc < 2)
	{
		remote_print_help(argv[0]);
		return -1;
	}

	while (1)
	{
#if !defined(CONFIG_IPV4_ONLY)
		ret = getopt_long(argc, argv, "sc:p:u46evh", opt_info, &opt_idx);
#else
		ret = getopt_long(argc, argv, "sc:p:uevh", opt_info, &opt_idx);
#endif

		switch (ret)
		{
			case -1: // end
				if (!info->ipv4 && !info->ipv6)
					return -1;

				if (info->client)
				{
					printf("[ %s_CLIENT ]\n", info->udp ? "UDP" : "TCP");
					if (strlen(info->ifname) > 0)
						printf("  - server_ip : %s (%s)\n", info->server_ip, info->ifname);
					else
						printf("  - server_ip : %s\n", info->server_ip);
					printf("  - server_port : %u\n", info->server_port);
				}
				else
				{
					printf("[ %s_SERVER ]\n", info->udp ? "UDP" : "TCP");
					printf("  - listen_port : %u\n", info->listen_port);
				}

#if !defined(CONFIG_IPV4_ONLY)
				printf("  - ipv4 : %s\n", info->ipv4 ? "on" : "off");
				printf("  - ipv6 : %s\n", info->ipv6 ? "on" : "off");
#endif
				printf("  - echo : %s\n", info->echo ? "on" : "off");
				printf("\n");

				return 0;

			case 0:
				break;

			case 'u':
				info->udp = true;
				break;

			case 's':
				info->client = false;
				break;

			case 'c':
			{
#if !defined(CONFIG_IPV4_ONLY)
				char *ifname = NULL;
				int ret;

				ifname = strchr(optarg, '%');
				if (ifname)
				{
					*ifname = '\0';
					strcpy(info->ifname, ++ifname);
				}

				ret = inet_pton(AF_INET, optarg, info->server_ip);
				if (ret == 1)
					info->ipv6 = false;
				else if (ret == 0)
				{
					ret = inet_pton(AF_INET6, optarg, info->server_ip);
					if (ret == 1)
						info->ipv4 = false;
				}

				if (ret <= 0)
				{
					perror("inet_pton()");
					return -1;
				}
#endif
				info->client = true;;
				strcpy(info->server_ip, optarg);
				break;
			}

			case 'p':
				info->port = atoi(optarg);
				break;

#if !defined(CONFIG_IPV4_ONLY)
			case '4':
				info->ipv6 = false;
				break;

			case '6':
				info->ipv4 = false;
				break;
#endif
			case 'e':
				info->echo = true;
				break;

			case 'v':
				remote_print_version();
				return 1;

			case 'h':
				remote_print_help(argv[0]);
				return 1;

			default:
				return -1;
		}
	}
}

static void remote_init_info (remote_info_t *info)
{
	memset(info, 0, sizeof(remote_info_t));

	info->udp = false;
	info->client = false;
	info->ipv4 = true;
#if defined(CONFIG_IPV4_ONLY)
	info->ipv6 = false;
#else
	info->ipv6 = true;
#endif
	info->echo = false;
#ifdef CONFIG_VERBOSE
	info->verbose = true;
#else
	info->verbose = false;
#endif

	info->fd = -1;
	info->port = REMOTE_DEFAULT_PORT;

#if 0
	printf("fd=%d server_fd=%d client_fd=%d\n", info->fd, info->server_fd, info->client_fd);
	printf("port=%u listen_port=%u server_port=%u\n", info->port, info->listen_port, info->server_port);

	if (1)
	{
		char *v = (char *)info;
		int i;

		for (i = 0 ; i < sizeof(remote_info_t) ; i++)
		{
			printf(" %02X", v[i]);
			if ((i % 16) == 15)
				printf("\r\n");
		}

		printf("\r\n");
	}

	printf("bool=%d int=%d\n", sizeof(bool), sizeof(int));
#endif
}

/***************************************************************************************/

int main (int argc, char *argv[])
{
	remote_info_t info;

	remote_init_info(&info);

	if (remote_get_options(argc, argv, &info) == 0)
	{
		if (remote_create(&info) == 0)
			remote_loop(&info);

		remote_delete(&info);
	}

	return 0;
}

