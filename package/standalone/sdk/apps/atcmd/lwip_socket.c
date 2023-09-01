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


#include "atcmd.h"
#include "lwip_socket.h"
#include "lwip/netdb.h"


static lwip_socket_info_t *g_lwip_socket_info = NULL;

/**********************************************************************************************/

static int __lwip_socket_error (const char *func, const int line, int _errno_)
{
	switch (_errno_)
	{
		case 0:
		case EAGAIN:
		case ENOBUFS:
			break;

		default:
			_lwip_socket_log("%s::%d, %s (%d)\n", func, line, strerror(_errno_), _errno_);
	}

	errno = _errno_;

	return -errno;
}

#define _lwip_socket_error(_errno_)	__lwip_socket_error(__func__, __LINE__, _errno_)

#ifndef gai_strerror
const char * __lwip_gai_strerror (int ecode)
{
	const char *result = "Unknown error";

	switch(ecode)
	{
		case EAI_NONAME:
			return("Name or service not known");
		case EAI_SERVICE:
			return("Servname not supported for ai_socktype");
		case EAI_FAIL:
			return("Non-recoverable failure in name resolution");
		case EAI_MEMORY:
			return("Memory allocation failure");
		case EAI_FAMILY:
			return("ai_family not supported");
		case HOST_NOT_FOUND:
			return("Host not found");
		case NO_DATA:
			return("No address associated with hostname");
		case NO_RECOVERY:
			return("No recovery");
		case TRY_AGAIN:
			return("Try again");
		default:
			return("Unknown error");
	}
}

#define gai_strerror(ecode)		__lwip_gai_strerror(ecode)
#endif

/**********************************************************************************************/

static int _lwip_socket_get_type (int fd, int *type)
{
	socklen_t len = sizeof(*type);

	return getsockopt(fd, SOL_SOCKET, SO_TYPE, type, &len);
}

static int _lwip_socket_get_error (int fd, int *error)
{
	socklen_t len = sizeof(*error);
	int ret;

	ret	= getsockopt(fd, SOL_SOCKET, SO_ERROR, error, &len);
	if (ret < 0)
		return _lwip_socket_error(errno);

	return 0;
}

static int _lwip_socket_get_info (int fd, ip_addr_t *ipaddr, uint16_t *port, bool peer)
{
	lwip_sockaddr_in_t addr;
	socklen_t addrlen;
	int ret;

	memset(&addr, 0, sizeof(lwip_sockaddr_in_t));
	addrlen = sizeof(lwip_sockaddr_in_t);

	if (peer)
		ret = getpeername(fd, (struct sockaddr *)&addr, &addrlen);
	else
		ret = getsockname(fd, (struct sockaddr *)&addr, &addrlen);

	if (ret < 0)
		return _lwip_socket_error(errno);

	if (addrlen == sizeof(struct sockaddr_in))
	{
		struct sockaddr_in *_addr = &addr.ip4;

		if (ipaddr)
			inet_addr_to_ip4addr(ip_2_ip4(ipaddr), &_addr->sin_addr);

		if (port)
			*port = ntohs(_addr->sin_port);
	}
#ifdef CONFIG_ATCMD_IPV6
	else if (addrlen == sizeof(struct sockaddr_in6))
	{
		struct sockaddr_in6 *_addr = &addr.ip6;

		if (ipaddr)
			inet6_addr_to_ip6addr(ip_2_ip6(ipaddr), &_addr->sin6_addr);

		if (port)
			*port = ntohs(_addr->sin6_port);
	}
#endif
	else
	{
		_lwip_socket_log("SOCK_GET_INFO: invalid addrlen (%d)\n", addrlen);
		return -1;
	}

	return 0;
}

static int _lwip_socket_get_recvlen (int fd, int *len)
{
	if (!len)
		return _lwip_socket_error(EINVAL);

	*len = 0;

	if (ioctl(fd, FIONREAD, len) < 0)
		return _lwip_socket_error(errno);

	return 0;
}

static int _lwip_socket_set_block (int fd)
{
	int flags;
	int ret;

	flags = fcntl(fd, F_GETFL, 0) & ~O_NONBLOCK;

	ret = fcntl(fd, F_SETFL, flags);
	if (ret < 0)
		return _lwip_socket_error(errno);

	return 0;
}

static int _lwip_socket_set_nonblock (int fd)
{
	int flags;
   	int ret;

	flags = fcntl(fd, F_GETFL, 0) | O_NONBLOCK;

	ret = fcntl(fd, F_SETFL, flags);
	if (ret < 0)
		return _lwip_socket_error(errno);

	return 0;
}

static int _lwip_socket_get_reuse_addr (int fd, bool *enabled)
{
	int reuse_addr;
	socklen_t len = sizeof(int);
	int ret;

	ret = getsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, &len);
	if (ret < 0)
		return _lwip_socket_error(errno);

	if (enabled)
		*enabled = !!reuse_addr;

	_lwip_socket_log("reuse_addr_get: %d, %s\n", fd, !!reuse_addr ? "on" : "off");

	return 0;
}

static int _lwip_socket_set_reuse_addr (int fd, bool enable)
{
	int reuse_addr = enable ? 1 : 0;
	socklen_t len = sizeof(int);
	int ret;

	ret = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, len);
	if (ret < 0)
		return _lwip_socket_error(errno);

	_lwip_socket_log("reuse_addr_set: %d, %s\n", fd, enable ? "on" : "off");

	return 0;
}

int _lwip_socket_tcp_get_keepalive (int fd, int *keepalive, int *keepidle, int *keepcnt, int *keepintvl)
{
	socklen_t len = sizeof(int);
	int _keepalive = -1;
	int _keepidle = -1;
   	int _keepcnt = -1;
   	int _keepintvl = -1;
	int ret;

	if (fd < 0)
		return _lwip_socket_error(EINVAL);

	ret = getsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &_keepalive, &len);
	if (ret < 0)
		return _lwip_socket_error(errno);

	_keepalive = _keepalive ? 1 : 0;

	ret = getsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, &_keepidle, &len);
	if (ret < 0)
		return _lwip_socket_error(errno);

	ret = getsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, &_keepcnt, &len);
	if (ret < 0)
		return _lwip_socket_error(errno);

	ret = getsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, &_keepintvl, &len);
	if (ret < 0)
		return _lwip_socket_error(errno);

/*	_lwip_socket_log("tcp_keepalive_get: fd=%d keepalive=%d keepidle=%d keepcnt=%d keepintvl=%d\n",
						fd, _keepalive, _keepidle, _keepcnt, _keepintvl); */

	if (keepalive) *keepalive = _keepalive;
	if (keepidle) *keepidle = _keepidle;
	if (keepcnt) *keepcnt = _keepcnt;
	if (keepintvl) *keepintvl = _keepintvl;

	return 0;
}

int _lwip_socket_tcp_set_keepalive (int fd, int keepalive, int keepidle, int keepcnt, int keepintvl)
{
	int ret;

	ret = setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, &keepidle, sizeof(int));
	if (ret < 0)
		return _lwip_socket_error(errno);

	ret = setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, &keepcnt, sizeof(int));
	if (ret < 0)
		return _lwip_socket_error(errno);

	ret = setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, &keepintvl, sizeof(int));
	if (ret < 0)
		return _lwip_socket_error(errno);

	ret = setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &keepalive, sizeof(int));
	if (ret < 0)
		return _lwip_socket_error(errno);

/* 	_lwip_socket_log("tcp_keepalive_set: fd=%d keepalive=%d keepidle=%d keepcnt=%d keepintvl=%d\n",
						fd, keepalive, keepidle, keepcnt, keepintvl); */

	return 0;
}

int _lwip_socket_tcp_get_nodelay (int fd, bool *enabled)
{
	int tcp_nodelay;
	socklen_t len = sizeof(int);
	int ret;

	ret = getsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &tcp_nodelay, &len);
	if (ret < 0)
		return _lwip_socket_error(errno);

	if (enabled)
		*enabled = !!tcp_nodelay;

/*	_lwip_socket_log("tcp_nodelay_get: %d, %s\n", fd, !!tcp_nodelay ? "on" : "off"); */

	return 0;
}

int _lwip_socket_tcp_set_nodelay (int fd, bool enable)
{
	int tcp_nodelay = enable ? 1 : 0;
	socklen_t len = sizeof(int);
	int ret;

	ret = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &tcp_nodelay, len);
	if (ret < 0)
		return _lwip_socket_error(errno);

/*	_lwip_socket_log("tcp_nodelay_set: %d, %s\n", fd, enable ? "on" : "off"); */

	return 0;
}

/**********************************************************************************************/

static int _lwip_socket_bind (int fd, uint16_t port, bool ipv6)
{
	lwip_sockaddr_in_t addr;
	socklen_t addrlen;

	memset(&addr, 0, sizeof(lwip_sockaddr_in_t));
	addrlen = 0;

	if (ipv6)
	{
#ifdef CONFIG_ATCMD_IPV6
		struct sockaddr_in6 *_addr = &addr.ip6;

		_addr->sin6_len = addrlen;
		_addr->sin6_family = AF_INET6;
		_addr->sin6_port = htons(port);
		_addr->sin6_flowinfo = 0;
		_addr->sin6_addr = in6addr_any;
		_addr->sin6_scope_id = SIN6_SCOPE_ID;

		addrlen = sizeof(struct sockaddr_in6);
#endif
	}
	else
	{
		struct sockaddr_in *_addr = &addr.ip4;

		_addr->sin_len = addrlen;
		_addr->sin_family = AF_INET;
		_addr->sin_port = htons(port);
		_addr->sin_addr.s_addr = htonl(INADDR_ANY);

		addrlen = sizeof(struct sockaddr_in);
	}

	if (bind(fd, (struct sockaddr *)&addr, addrlen) < 0)
		return _lwip_socket_error(errno);

	return 0;
}

static int _lwip_socket_listen (int fd)
{
	/*
	 * TCP_LISTEN_BACKLOG=5, lib/lwip/contrib/port/lwipopts.h
	 */

	if (listen(fd, TCP_LISTEN_BACKLOG) < 0)
		return _lwip_socket_error(errno);

	return 0;
}

static int _lwip_socket_accept (int fd, ip_addr_t *remote_addr, uint16_t *remote_port)
{
	lwip_sockaddr_in_t addr;
	socklen_t addrlen;
	int type;

	if (_lwip_socket_get_type(fd, &type) != 0)
		return _lwip_socket_error(errno);

	if (type != SOCK_STREAM)
		return _lwip_socket_error(EPROTOTYPE);

	memset(&addr, 0, sizeof(lwip_sockaddr_in_t));
	addrlen = sizeof(lwip_sockaddr_in_t);

	fd = accept(fd, (struct sockaddr *)&addr, &addrlen);
	if (fd < 0)
		return _lwip_socket_error(errno);

	if (addrlen == sizeof(struct sockaddr_in))
	{
		struct sockaddr_in *_addr = &addr.ip4;

		inet_addr_to_ip4addr(ip_2_ip4(remote_addr), &_addr->sin_addr);
		IP_SET_TYPE(remote_addr, IPADDR_TYPE_V4);
		*remote_port = ntohs(_addr->sin_port);
	}
#ifdef CONFIG_ATCMD_IPV6
	else if (addrlen == sizeof(struct sockaddr_in6))
	{
		struct sockaddr_in6 *_addr = &addr.ip6;

		inet6_addr_to_ip6addr(ip_2_ip6(remote_addr), &_addr->sin6_addr);
		IP_SET_TYPE(remote_addr, IPADDR_TYPE_V6);
		*remote_port = ntohs(_addr->sin6_port);
	}
#endif

	return fd;
}

static int _lwip_socket_connect (int fd, ip_addr_t *remote_addr, uint16_t remote_port,
								int timeout_msec, bool ipv6)
{
	lwip_sockaddr_in_t addr;
	socklen_t addrlen;
	struct timeval timeout;
	fd_set fds;
	int ret;

	memset(&addr, 0, sizeof(lwip_sockaddr_in_t));
	addrlen = 0;

	if (ipv6)
	{
#ifdef CONFIG_ATCMD_IPV6
		struct sockaddr_in6 *_addr = &addr.ip6;

		_addr->sin6_len = addrlen;
		_addr->sin6_family = AF_INET6;
		_addr->sin6_port = htons(remote_port);
		inet6_addr_from_ip6addr(&_addr->sin6_addr, ip_2_ip6(remote_addr));
		_addr->sin6_scope_id = SIN6_SCOPE_ID;

		addrlen = sizeof(struct sockaddr_in6);
#endif
	}
	else
	{
		struct sockaddr_in *_addr = &addr.ip4;

		_addr->sin_len = addrlen;
		_addr->sin_family = AF_INET;
		_addr->sin_port = htons(remote_port);
		inet_addr_from_ip4addr(&_addr->sin_addr, ip_2_ip4(remote_addr));

		addrlen = sizeof(struct sockaddr_in);
	}

	if (_lwip_socket_set_nonblock(fd) < 0)
		return _lwip_socket_error(errno);

	if (connect(fd, (struct sockaddr *)&addr, addrlen) == 0)
		return 0;

	if (_lwip_socket_set_block(fd) < 0)
		return _lwip_socket_error(errno);

	if (errno != EINPROGRESS)
		return _lwip_socket_error(errno);

	if (timeout_msec <= 0)
		timeout_msec = 10000; // 10sec

	timeout.tv_sec = timeout_msec / 1000;
    timeout.tv_usec = (timeout_msec % 1000) * 1000;

	FD_ZERO(&fds);
	FD_SET(fd, &fds);

	ret = select(fd + 1, NULL, &fds, NULL, &timeout);
	if (ret < 0)
		return _lwip_socket_error(errno);

	if (ret > 0)
	{
		int err = EAGAIN;

	 	if (FD_ISSET(fd, &fds))
		{
			if (_lwip_socket_get_error(fd, &err) != 0)
				return _lwip_socket_error(errno);
		}

		return _lwip_socket_error(err);
	}

	return _lwip_socket_error(ETIMEDOUT);
}

/**********************************************************************************************/

#define _lwip_socket_fds_debug		/* _lwip_socket_log */

static bool _lwip_socket_fds_mutex_take (lwip_socket_info_t *info)
{
	if (xSemaphoreTake(info->fds.mutex, LWIP_SOCKET_FDS_MUTEX_TIMEOUT) == pdFALSE)
	{
		_lwip_socket_error(EPERM);
		return false;
	}

	return true;
}
#define LWIP_SOCKET_FDS_LOCK(info)	ASSERT(_lwip_socket_fds_mutex_take(info))

static bool _lwip_socket_fds_mutex_give (lwip_socket_info_t *info)
{
	if (xSemaphoreGive(info->fds.mutex) == pdFALSE)
	{
		_lwip_socket_error(EPERM);
		return false;
	}

	return true;
}
#define LWIP_SOCKET_FDS_UNLOCK(info)	ASSERT(_lwip_socket_fds_mutex_give(info))

static int _lwip_socket_fds_get (lwip_socket_info_t *info, fd_set *read, fd_set *write)
{
	bool set_fd;
	int nfds = 0;
	int fd;

	LWIP_SOCKET_FDS_LOCK(info);

	if (read)
		FD_ZERO(read);

	if (write)
		FD_ZERO(write);

	for (fd = 0 ; fd < FD_SETSIZE ; fd++)
	{
		set_fd = false;

		if (read && FD_ISSET(fd, &info->fds.read))
		{
			FD_SET(fd, read);
			set_fd = true;
		}

		if (write && FD_ISSET(fd, &info->fds.write))
		{
			FD_SET(fd, write);
			set_fd = true;
		}

		if (set_fd)
			nfds = fd + 1;
	}

	LWIP_SOCKET_FDS_UNLOCK(info);

	return nfds;
}

static int _lwip_socket_fds_set (lwip_socket_info_t *info, int fd, bool listen)
{
	_lwip_socket_fds_debug("SOCK_FDS_SET: fd=%d listen=%d\n", fd, listen);

	LWIP_SOCKET_FDS_LOCK(info);

	FD_SET(fd, &info->fds.read);
	FD_CLR(fd, &info->fds.write);

	if (listen)
		FD_SET(fd, &info->fds.listen);

	LWIP_SOCKET_FDS_UNLOCK(info);

	return 0;
}

static int _lwip_socket_fds_clear (lwip_socket_info_t *info, int fd)
{
	_lwip_socket_fds_debug("SOCK_FDS_CLR: fd=%d\n", fd);

	LWIP_SOCKET_FDS_LOCK(info);

	FD_CLR(fd, &info->fds.read);
	FD_CLR(fd, &info->fds.write);
	FD_CLR(fd, &info->fds.listen);

	LWIP_SOCKET_FDS_UNLOCK(info);

	return 0;
}

static void _lwip_socket_task (void *arg)
{
	const uint32_t default_timeout = (1 * 1000); /* usec */
	lwip_socket_info_t *info = (lwip_socket_info_t *)arg;
	fd_set fds_read, fds_write;
	struct timeval timeout;
	int nfds;
	int ret;
	int fd;

	FD_ZERO(&fds_read);
	FD_ZERO(&fds_write);

	if (info->timeout.task == 0)
		info->timeout.task = default_timeout;

	while (1)
	{
		nfds = _lwip_socket_fds_get(info, &fds_read, &fds_write);
		if (nfds <= 0)
		{
/*			if (info->log.task)
				_lwip_socket_log("SOCK_TASK: no fds\n"); */

			_delay_ms(10);
			continue;
		}

		if (info->log.task)
		{
			_lwip_socket_log("SOCK_TASK: get, %d 0x%X 0x%X\n",
					nfds, fds_read.__fds_bits[0], fds_write.__fds_bits[0]);
		}

		timeout.tv_sec = info->timeout.task / 1000000;
		timeout.tv_usec = info->timeout.task % 1000000;

		ret = select(nfds,
					(fds_read.__fds_bits[0]) ? &fds_read : NULL,
					(fds_write.__fds_bits[0]) ? &fds_write : NULL,
					NULL,
					&timeout);

		switch (ret)
		{
			case -1:
				_lwip_socket_error(errno);
				break;

			case 0:
				if (info->log.task)
					_lwip_socket_log("SOCK_TASK: timeout, %uus\n", info->timeout.task);
				break;

			default:
				if (info->log.task)
				{
					_lwip_socket_log("SOCK_TASK: set, %d 0x%X 0x%X\n",
							nfds, fds_read.__fds_bits[0], fds_write.__fds_bits[0]);
				}
		}

		for (fd = 0 ; ret > 0 && fd < nfds ; fd++)
		{
			if (FD_ISSET(fd, &fds_read))
			{
				ret--;

				if (FD_ISSET(fd, &info->fds.listen))
				{
					_lwip_socket_fds_debug("SOCK_FDS_LISTEN: fd=%d\n", fd);

					if (info->cb.tcp_connect)
					{
						ip_addr_t remote_addr;
						uint16_t remote_port = 0;
						int _fd;

						_fd = _lwip_socket_accept(fd, &remote_addr, &remote_port);

						if (_fd >= 0)
						{
							_lwip_socket_fds_set(info, _fd, false);

							info->cb.tcp_connect(_fd, &remote_addr, remote_port);
						}
					}
				}
				else
				{
					_lwip_socket_fds_debug("SOCK_FDS_READ: fd=%d\n", fd);

					if (info->cb.recv_ready)
						info->cb.recv_ready(fd);
				}
			}

			if (FD_ISSET(fd, &fds_write))
			{
				ret--;

				_lwip_socket_fds_debug("SOCK_FDS_WRITE: fd=%d\n", fd);

				if (info->cb.send_ready)
					info->cb.send_ready(fd);

//				_lwip_socket_send_done(fd);
			}
		}

		if (ret > 0)
			_lwip_socket_log("SOCK_TASK: ret=%d\n", ret);
	}
}

/**********************************************************************************************/

int _lwip_socket_init (lwip_socket_cb_t *cb)
{
	lwip_socket_info_t *info;
	int i;

	if (!cb)
		return _lwip_socket_error(EPERM);

	if (g_lwip_socket_info)
		return _lwip_socket_error(EBUSY);

	info = _lwip_socket_malloc(sizeof(lwip_socket_info_t));
	if (!info)
		goto _lwip_socket_init_fail;

	memset(info, 0, sizeof(lwip_socket_info_t));
	memcpy(&info->cb, cb, sizeof(lwip_socket_cb_t));

	info->send_done_event = xEventGroupCreate();
	if (!info->send_done_event)
		goto _lwip_socket_init_fail;

	info->fds.mutex = xSemaphoreCreateMutex();
	if (!info->fds.mutex)
		goto _lwip_socket_init_fail;

	FD_ZERO(&info->fds.read);
	FD_ZERO(&info->fds.write);
	FD_ZERO(&info->fds.listen);

	if (xTaskCreate(_lwip_socket_task, "atcmd_lwip_socket",
			LWIP_SOCKET_TASK_STACK_SIZE, info,
			LWIP_SOCKET_TASK_PRIORITY, &info->task) != pdPASS)
		goto _lwip_socket_init_fail;

	info->timeout.task = 0;

	info->log.task = 0;
	info->log.send = 0;
	info->log.recv = 0;

	g_lwip_socket_info = info;

	return 0;

_lwip_socket_init_fail:

	if (info->fds.mutex)
		vSemaphoreDelete(info->fds.mutex);

	if (info->send_done_event)
		vEventGroupDelete(info->send_done_event);

	if (info)
		_lwip_socket_mfree(info);

	return _lwip_socket_error(ENOMEM);
}

int _lwip_socket_deinit (void)
{
	lwip_socket_info_t *info = g_lwip_socket_info;

	if (!info)
		return _lwip_socket_error(EPERM);

	vTaskDelete(info->task);

	if (info->send_done_event)
		vEventGroupDelete(info->send_done_event);

	vSemaphoreDelete(info->fds.mutex);

	_lwip_socket_mfree(info);

	g_lwip_socket_info = NULL;

	return 0;
}

static int _lwip_socket_open (int type, int *fd,
							ip_addr_t *remote_addr, uint16_t remote_port, uint16_t local_port,
							int timeout_msec, bool ipv6, bool reuse_addr)
{
	if (g_lwip_socket_info)
	{
		bool listen = false;
		int ret;

		if (!fd)
			return _lwip_socket_error(EINVAL);

		*fd = -1;

		switch (type)
		{
			case SOCK_DGRAM:
				if (remote_addr || remote_port || !local_port)
					return _lwip_socket_error(EINVAL);

				ret = socket((ipv6 ? AF_INET6 : AF_INET), SOCK_DGRAM, IPPROTO_UDP);
				if (ret < 0)
					return _lwip_socket_error(errno);

				*fd = ret;

				ret = _lwip_socket_set_reuse_addr(*fd, reuse_addr);
				if (ret < 0)
					return _lwip_socket_error(errno);

				ret = _lwip_socket_bind(*fd, local_port, ipv6);
				break;

			case SOCK_STREAM:
				if (remote_port && local_port)
					return _lwip_socket_error(EINVAL);

				ret = socket((ipv6 ? AF_INET6 : AF_INET), SOCK_STREAM, IPPROTO_TCP);
				if (ret < 0)
					return _lwip_socket_error(errno);

				*fd = ret;

				ret = _lwip_socket_set_reuse_addr(*fd, reuse_addr);
				if (ret < 0)
					return _lwip_socket_error(errno);

				if (remote_addr)
					ret = _lwip_socket_connect(*fd, remote_addr, remote_port, timeout_msec, ipv6);
				else
				{
					ret = _lwip_socket_bind(*fd, local_port, ipv6);
					if (ret	== 0)
					{
						ret = _lwip_socket_listen(*fd);
						if (ret == 0)
							listen = true;
					}
				}
				break;

			default:
				return _lwip_socket_error(EINVAL);
		}

		switch (ret)
		{
			case 0:
			case -EINPROGRESS:
				_lwip_socket_fds_set(g_lwip_socket_info, *fd, listen);
				break;

			default:
				close(*fd);
				*fd = -1;
		}

		return ret;
	}

	return _lwip_socket_error(EPERM);
}

int _lwip_socket_open_udp (int *fd, uint16_t local_port, bool ipv6, bool reuse_addr)
{
	return _lwip_socket_open(SOCK_DGRAM, fd, NULL, 0, local_port, 0, ipv6, reuse_addr);
}

int _lwip_socket_open_tcp_server (int *fd, uint16_t local_port, bool ipv6, bool reuse_addr)
{
	return _lwip_socket_open(SOCK_STREAM, fd, NULL, 0, local_port, 0, ipv6, reuse_addr);
}

int _lwip_socket_open_tcp_client (int *fd, ip_addr_t *remote_addr, uint16_t remote_port,
								int timeout_msec, bool ipv6, bool reuse_addr)
{
	return _lwip_socket_open(SOCK_STREAM, fd, remote_addr, remote_port, 0, timeout_msec, ipv6, reuse_addr);
}

int _lwip_socket_get_peer (int fd, ip_addr_t *ipaddr, uint16_t *port)
{
	return _lwip_socket_get_info(fd, ipaddr, port, true);
}

int _lwip_socket_get_local (int fd, ip_addr_t *ipaddr, uint16_t *port)
{
	return _lwip_socket_get_info(fd, ipaddr, port, false);
}

int _lwip_socket_close (int fd)
{
	if (g_lwip_socket_info)
	{
		int ret;

		_lwip_socket_fds_clear(g_lwip_socket_info, fd);

		ret = close(fd);
		if (ret < 0)
			return _lwip_socket_error(errno);

		return 0;
	}

	return _lwip_socket_error(EPERM);
}

int _lwip_socket_send_request (int fd)
{
	lwip_socket_info_t *info = g_lwip_socket_info;

	if (info && info->send_done_event)
	{
		if (FD_ISSET(fd, &info->fds.write))
			_atcmd_error("already set fd, fd=%d\n", fd);
		else
		{
			const int timeout = 1000; /* msec */
			const int retry_max = 10;
			int retry;
			EventBits_t event;

			if (info->log.send)
				_lwip_socket_log("_lwip_socket_send: request, evt=0x%X\n", xEventGroupGetBits(info->send_done_event));

			LWIP_SOCKET_FDS_LOCK(info);

			FD_SET(fd, &info->fds.write);

			LWIP_SOCKET_FDS_UNLOCK(info);

//			xEventGroupClearBits(info->send_done_event, (1 << fd));

			for (retry = event = 0 ; retry < retry_max ; retry++)
			{
				event = xEventGroupWaitBits(info->send_done_event, (1 << fd),
											pdTRUE, pdFALSE, timeout / portTICK_PERIOD_MS);
				if (event & (1 << fd))
				   return 0;
				else if (event)
					_lwip_socket_log("%s: invalid, 0x%X\n", __func__, event);
				else
					_lwip_socket_log("%s: timeout, %dms\n", __func__, timeout);
			}

			return _lwip_socket_error(ETIMEDOUT);
		}

		return _lwip_socket_error(EBUSY);
	}

	return _lwip_socket_error(EPERM);
}

int _lwip_socket_send_done (int fd)
{
	lwip_socket_info_t *info = g_lwip_socket_info;

	if (info && info->send_done_event)
	{
		if (info->log.send)
			_lwip_socket_log("_lwip_socket_send: done, evt=0x%X\n", xEventGroupGetBits(info->send_done_event));

		LWIP_SOCKET_FDS_LOCK(info);

		FD_CLR(fd, &info->fds.write);

		LWIP_SOCKET_FDS_UNLOCK(info);

		xEventGroupSetBits(info->send_done_event, 1 << fd);

		return 0;
	}

	return _lwip_socket_error(EPERM);
}

int _lwip_socket_send (int fd, ip_addr_t *remote_addr, uint16_t remote_port,
						char *data, int len)
{
	if (g_lwip_socket_info)
	{
		int flags = MSG_DONTWAIT;
		int type;
		int ret;

		if (!data || !len)
			return _lwip_socket_error(EINVAL);

		if (_lwip_socket_get_type(fd, &type) != 0)
			return _lwip_socket_error(errno);

		switch (type)
		{
			case SOCK_STREAM:
				ret = send(fd, data, len, flags);
				break;

			case SOCK_DGRAM:
				if (!remote_addr || !remote_port)
					return _lwip_socket_error(EINVAL);
				else
				{
					const int max_len = 1470;
					lwip_sockaddr_in_t addr;
					socklen_t addrlen;
					int _len;
					int i;

					memset(&addr, 0, sizeof(lwip_sockaddr_in_t));

					if (IP_IS_V6(remote_addr))
					{
#ifdef CONFIG_ATCMD_IPV6
						struct sockaddr_in6 *_addr = &addr.ip6;

						addrlen = sizeof(struct sockaddr_in6);

						_addr->sin6_len = addrlen;
						_addr->sin6_family = AF_INET6;
						_addr->sin6_port = htons(remote_port);
						inet6_addr_from_ip6addr(&_addr->sin6_addr, ip_2_ip6(remote_addr));
						_addr->sin6_scope_id = SIN6_SCOPE_ID;
#endif
					}
					else
					{
						struct sockaddr_in *_addr = &addr.ip4;

						addrlen = sizeof(struct sockaddr_in);

						_addr->sin_len = addrlen;
						_addr->sin_family = AF_INET;
						_addr->sin_port = htons(remote_port);
						inet_addr_from_ip4addr(&_addr->sin_addr, ip_2_ip4(remote_addr));
					}

					for (ret = i = 0 ; i < len ; i += ret)
					{
						_len = len - i;
						if (_len > max_len)
							_len = max_len;

						ret = sendto(fd, data + i, _len, flags,
								(struct sockaddr *)&addr, addrlen);

						if (ret > 0)
							continue;
						else if (ret == 0)
							ret = i;

						break;
					}

					if (i == len)
						ret = len;
				}
				break;

			default:
				return _lwip_socket_error(EINVAL);
		}

		if (ret < 0)
			return _lwip_socket_error(errno);

		return ret;
	}

	return _lwip_socket_error(EBUSY);
}

int _lwip_socket_recv_request (int fd)
{
	lwip_socket_info_t *info = g_lwip_socket_info;

	if (info)
	{
		if (info->log.recv)
			_lwip_socket_log("_lwip_socket_recv: request\n");

		LWIP_SOCKET_FDS_LOCK(info);

		FD_SET(fd, &info->fds.read);

		LWIP_SOCKET_FDS_UNLOCK(info);

		return 0;
	}

	return _lwip_socket_error(EPERM);
}

int _lwip_socket_recv_done (int fd)
{
	lwip_socket_info_t *info = g_lwip_socket_info;

	if (info)
	{
		if (info->log.recv)
			_lwip_socket_log("_lwip_socket_recv: done\n");

		LWIP_SOCKET_FDS_LOCK(info);

		FD_CLR(fd, &info->fds.read);

		LWIP_SOCKET_FDS_UNLOCK(info);

		return 0;
	}

	return _lwip_socket_error(EPERM);
}

int _lwip_socket_recv_len (int fd)
{
	int len;
	int ret;

	ret = _lwip_socket_get_recvlen(fd, &len);
	if (ret == 0)
		return len;

	return ret;
}

int _lwip_socket_recv (int fd, ip_addr_t *remote_addr, uint16_t *remote_port,
						char *data, int len)
{
	if (g_lwip_socket_info)
	{
		int flags = MSG_DONTWAIT;
		int type;
		int ret;

		if (!data)
			return _lwip_socket_error(EINVAL);

		if (_lwip_socket_get_type(fd, &type) != 0)
			return _lwip_socket_error(errno);

		switch (type)
		{
			case SOCK_STREAM:
				ret = recv(fd, data, len, flags);
				break;

			case SOCK_DGRAM:
			{
				lwip_sockaddr_in_t addr;
				socklen_t addrlen;

				if (!remote_addr && !remote_port)
					addrlen = 0;
				else
					addrlen = sizeof(lwip_sockaddr_in_t);

				ret = recvfrom(fd, data, len, flags, (struct sockaddr *)&addr, &addrlen);

				if (ret > 0)
				{
					if (addrlen == sizeof(struct sockaddr_in))
					{
						struct sockaddr_in *_addr = &addr.ip4;

						inet_addr_to_ip4addr(ip_2_ip4(remote_addr), &_addr->sin_addr);
						*remote_port = ntohs(_addr->sin_port);
					}
#ifdef CONFIG_ATCMD_IPV6
					else if (addrlen == sizeof(struct sockaddr_in6))
					{
						struct sockaddr_in6 *_addr = &addr.ip6;

						inet6_addr_to_ip6addr(ip_2_ip6(remote_addr), &_addr->sin6_addr);
						*remote_port = ntohs(_addr->sin6_port);
					}
#endif
				}

				break;
			}

			default:
				errno = EPROTOTYPE;
				ret = -1;
		}

		if (ret < 0)
			return _lwip_socket_error(errno);

		return ret;
	}

	return _lwip_socket_error(EBUSY);
}

/**********************************************************************************************/

int _lwip_socket_addr_info_1 (const char *host, char *addr, int addrlen)
{
#define LOG_AI_HOSTENT	1 /* 0:unused, 1:terse, 2:verbose */

	lwip_socket_info_t *info = g_lwip_socket_info;
	struct hostent *hostent;

	if (!host || !addr || addrlen < IP4ADDR_STRLEN_MAX)
		return _lwip_socket_error(EINVAL);

	if (strcmp(host, info->addrinfo.name) == 0)
	{
		strcpy(addr, info->addrinfo.addr);
		return 0;
	}

	hostent = gethostbyname(host);
	if (!hostent)
	{
		const char *str_h_errno[] = { "HOST_NOT_FOUND", "NO_DATA", "NO_RECOVERY", "TRY_AGAIN" };

		switch (h_errno)
		{
			case HOST_NOT_FOUND:
			case NO_DATA:
			case NO_RECOVERY:
			case TRY_AGAIN:
				_lwip_socket_log("%s: %s\n", __func__, str_h_errno[h_errno - HOST_NOT_FOUND]);
				break;

			default:
				_lwip_socket_log("%s: unknown error\n", __func__);
		}

		return _lwip_socket_error(EPERM);
	}

#if LOG_AI_HOSTENT == 1
	_lwip_socket_log("%s:\n", __func__);
	_lwip_socket_log(" - h_name: %s\n", hostent->h_name);
	_lwip_socket_log(" - h_addrtype: %d\n", hostent->h_addrtype);
	_lwip_socket_log(" - h_addr: %s\n", inet_ntoa(*(struct in_addr *)hostent->h_addr));
#elif LOG_AI_HOSTENT == 2
	{
		int i;

		_lwip_socket_log("%s:\n", __func__);
		_lwip_socket_log(" - h_name: %s\n", hostent->h_name);
		for (i = 0 ; hostent->h_aliases[i] ; i++)
			_lwip_socket_log(" - h_aliases[%d]: %s\n", i, hostent->h_aliases[i]);
		_lwip_socket_log(" - h_addrtype: %d\n", hostent->h_addrtype);
		_lwip_socket_log(" - h_length: %d\n", hostent->h_length);
		for (i = 0 ; hostent->h_addr_list[i] ; i++)
			_lwip_socket_log(" - h_addr_list[%d]: %s\n", i, inet_ntoa(*(struct in_addr *)hostent->h_addr_list[i]));
	}
#endif

	if (!inet_ntop(hostent->h_addrtype, (struct in_addr *)hostent->h_addr, addr, addrlen))
		return _lwip_socket_error(errno);

	return 0;
}

int _lwip_socket_addr_info_2 (const char *host, const char *port, char *addr, int addrlen)
{
#define USE_AI_HINTS	0 /* 0:unused, 1:used */
#define LOG_AI_RESULT	1 /* 0:unused, 1:terse, 2:verbose */

	lwip_socket_info_t *info = g_lwip_socket_info;
	struct addrinfo *hints = NULL;
	struct addrinfo *res;
	int ecode;

	if (!host || !port || !addr || addrlen < IP4ADDR_STRLEN_MAX)
		return _lwip_socket_error(EINVAL);

	if (strcmp(host, info->addrinfo.name) == 0)
	{
		strcpy(addr, info->addrinfo.addr);
		return 0;
	}

#if USE_AI_HINTS
	hints = _lwip_socket_malloc(sizeof(struct addrinfo));
	if (!hints)
		_lwip_socket_error(ENOMEM);
	else
	{
		memset(hints, 0, sizeof(struct addrinfo));

		hints->ai_flags = AI_NUMERICSERV | AI_ADDRCONFIG | AI_CANONNAME;
		hints->ai_family = AF_UNSPEC;
		hints->ai_socktype = 0;
		hints->ai_protocol = 0;
	}
#endif

	_lwip_socket_log("%s::%d\n", __func__, __LINE__);

	ecode = getaddrinfo(host, port, hints, &res);
	if (ecode != 0)
	{
		_lwip_socket_log("%s::%d, %s (%d)\n", __func__, __LINE__, gai_strerror(ecode), ecode);

		switch (ecode)
		{
			case EAI_NONAME:
			case EAI_SERVICE:
				return _lwip_socket_error(EINVAL);

			case EAI_MEMORY:
				return _lwip_socket_error(ENOMEM);

			default:
				return _lwip_socket_error(EPERM);
		}
	}

#if LOG_AI_RESULT == 1
	_lwip_socket_log("%s:\n", __func__);
	_lwip_socket_log(" - ai_family: %d\n", res->ai_family);
	_lwip_socket_log(" - ai_addr\n");
	_lwip_socket_log("   - ip: %s\n", inet_ntoa(((struct sockaddr_in *)res->ai_addr)->sin_addr));
	_lwip_socket_log("   - port: %u\n", ntohs(((struct sockaddr_in *)res->ai_addr)->sin_port));
	_lwip_socket_log(" - ai_canonname: %s\n", res->ai_canonname);
#elif LOG_AI_RESULT == 2
	_lwip_socket_log("%s:\n", __func__);
	_lwip_socket_log(" - ai_flags: 0x%X\n", res->ai_flags);
	_lwip_socket_log(" - ai_family: %d\n", res->ai_family);
	_lwip_socket_log(" - ai_socktype: %d\n", res->ai_socktype);
	_lwip_socket_log(" - ai_protocol: %d\n", res->ai_protocol);
	_lwip_socket_log(" - ai_addrlen: %d\n", res->ai_addrlen);
	_lwip_socket_log(" - ai_addr: %p\n", res->ai_addr);
	_lwip_socket_log("   - ip: %s\n", inet_ntoa(((struct sockaddr_in *)res->ai_addr)->sin_addr));
	_lwip_socket_log("   - port: %u\n", ntohs(((struct sockaddr_in *)res->ai_addr)->sin_port));
	_lwip_socket_log(" - ai_canonname: %s\n", res->ai_canonname);
	_lwip_socket_log(" - ai_next: %p\n", res->ai_next);
#endif

	if (!inet_ntop(res->ai_family, &((struct sockaddr_in *)res->ai_addr)->sin_addr, addr, addrlen))
		return _lwip_socket_error(errno);

	freeaddrinfo(res);

#if USE_AI_HINTS
	if (hints)
		_lwip_socket_mfree(hints);
#endif

	return 0;
}

/**********************************************************************************************/

static int cmd_atcmd_lwip_log (cmd_tbl_t *t, int argc, char *argv[])
{
	lwip_socket_info_t *info = g_lwip_socket_info;

	if (!info)
		return CMD_RET_FAILURE;

	switch (argc)
	{
		case 0:
			A("task: %d\n", info->log.task);
			A("send: %d\n", info->log.send);
			A("recv: %d\n", info->log.recv);
			break;

		case 2:
		{
			int log = atoi(argv[1]);

			if (log == 0 || log == 1)
			{
				if (strcmp(argv[0], "task") == 0)
				{
					info->log.task = log;
					break;
				}
				else if (strcmp(argv[0], "send") == 0)
				{
					info->log.send = log;
					break;
				}
				else if (strcmp(argv[0], "recv") == 0)
				{
					info->log.recv = log;
					break;
				}
				else if (strcmp(argv[0], "all") == 0)
				{
					info->log.task = log;
					info->log.send = log;
					info->log.recv = log;
					break;
				}
			}
		}

		default:
			return CMD_RET_USAGE;
	}

	return CMD_RET_SUCCESS;
}

static int cmd_atcmd_lwip_fds (cmd_tbl_t *t, int argc, char *argv[])
{
	lwip_socket_info_t *info = g_lwip_socket_info;

	if (!info)
		return CMD_RET_FAILURE;

	switch (argc)
	{
		case 0:
			A("read: %X\n", info->fds.read.__fds_bits[0]);
			A("write: %X\n", info->fds.write.__fds_bits[0]);
			A("listen: %X\n", info->fds.listen.__fds_bits[0]);
			break;

		default:
			return CMD_RET_USAGE;
	}

	return CMD_RET_SUCCESS;
}

static int cmd_atcmd_lwip_timeout (cmd_tbl_t *t, int argc, char *argv[])
{
	lwip_socket_info_t *info = g_lwip_socket_info;

	if (!info)
		return CMD_RET_FAILURE;

	switch (argc)
	{
		case 0:
			A("task: %u\n", info->timeout.task);
			break;

		case 1:
		{
			int timeout = atoi(argv[0]);

			if (timeout >= 0)
			{
				info->timeout.task = timeout;
				break;
			}
		}

		default:
			return CMD_RET_USAGE;
	}

	return CMD_RET_SUCCESS;
}

static int cmd_atcmd_addrinfo (cmd_tbl_t *t, int argc, char *argv[])
{
	lwip_socket_info_t *info = g_lwip_socket_info;

	if (!info)
		return CMD_RET_FAILURE;

	switch (argc)
	{
		case 0:
			A(" - name: %s\n", info->addrinfo.name);
			A(" - addr: %s\n", info->addrinfo.addr);
			break;

		case 2:
		{
			char *name = argv[0];
			char *addr = argv[1];

			if (strlen(name) < sizeof(info->addrinfo.name))
			{
				if (strlen(addr) < sizeof(info->addrinfo.addr))
				{
					if (ip4addr_aton(addr, NULL))
					{
						strcpy(info->addrinfo.name, name);
						strcpy(info->addrinfo.addr, addr);

						A(" - name: %s\n", info->addrinfo.name);
						A(" - addr: %s\n", info->addrinfo.addr);
						break;
					}
				}
			}
		}

		default:
			return CMD_RET_USAGE;
	}

	return CMD_RET_SUCCESS;
}

static int cmd_atcmd_lwip (cmd_tbl_t *t, int argc, char *argv[])
{
	int ret = CMD_RET_USAGE;

	if (argc >= 2)
	{
		if (strcmp(argv[1], "log") == 0)
			ret = cmd_atcmd_lwip_log(t, argc - 2, argv + 2);
		else if (strcmp(argv[1], "fds") == 0)
			ret = cmd_atcmd_lwip_log(t, argc - 2, argv + 2);
		else if (strcmp(argv[1], "timeout") == 0)
			ret = cmd_atcmd_lwip_timeout(t, argc - 2, argv + 2);
		else if (strcmp(argv[1], "addrinfo") == 0)
			ret = cmd_atcmd_lwip_log(t, argc - 2, argv + 2);
		else if (strcmp(argv[1], "help") == 0)
		{
			A("atcmd lwip log {task|send|recv} {0|1}\n");
			A("atcmd lwip fds\n");
			A("atcmd lwip timeout <usec>\n");
			A("atcmd lwip addrinfo <name> <addr>\n");

			return CMD_RET_SUCCESS;
		}
	}

	return ret;
}

SUBCMD_MAND(atcmd,
		lwip,
		cmd_atcmd_lwip,
		"lwip_socket",
		"atcmd lwip help");
