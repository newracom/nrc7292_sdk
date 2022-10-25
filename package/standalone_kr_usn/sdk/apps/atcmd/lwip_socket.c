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


#include "atcmd.h"
#include "lwip_socket.h"

static lwip_socket_info_t *g_lwip_socket_info = NULL;

/**********************************************************************************************/

static int __lwip_socket_error (const char *func, const int line, int _errno_)
{
	if (_errno_ != ENOBUFS)
		_lwip_socket_log("%s::%d, %s (%d)\n", func, line, strerror(_errno_), _errno_);

	errno = _errno_;

	return -errno;
}

#define _lwip_socket_error(_errno_)	__lwip_socket_error(__func__, __LINE__, _errno_)

/**********************************************************************************************/

static int _lwip_socket_get_type (int fd, int *type)
{
	socklen_t len = sizeof(*type);
	int ret;

	ret	= getsockopt(fd, SOL_SOCKET, SO_TYPE, type, &len);
	if (ret < 0)
		return _lwip_socket_error(errno);

	return 0;
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

static int _lwip_socket_get_info (int fd, char *addr, uint16_t *port, bool remote)
{
	struct sockaddr_in sock;
	int ret;

	if (remote)
		ret = getpeername(fd, (struct sockaddr *)&sock, NULL);
	else
		ret = getsockname(fd, (struct sockaddr *)&sock, NULL);

	if (ret < 0)
		return _lwip_socket_error(errno);

	strcpy(addr, inet_ntoa(sock.sin_addr));
	*port = ntohs(sock.sin_port);

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

static int _lwip_socket_bind (int fd, uint16_t port)
{
	struct sockaddr_in addr;

	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
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

static int _lwip_socket_accept (int fd, char *remote_addr, uint16_t *remote_port)
{
	struct sockaddr_in client;
	int len = sizeof(client);
	int type;

	if (_lwip_socket_get_type(fd, &type) != 0)
		return _lwip_socket_error(errno);

	if (type != SOCK_STREAM)
		return _lwip_socket_error(EPROTOTYPE);

	fd = accept(fd, (struct sockaddr *)&client, (socklen_t *)&len);
	if (fd < 0)
		return _lwip_socket_error(errno);

	strcpy(remote_addr, inet_ntoa(client.sin_addr));
	*remote_port = ntohs(client.sin_port);

	return fd;
}

static int _lwip_socket_connect (int fd, const char *remote_addr, uint16_t remote_port,
									int timeout_msec)
{
	struct sockaddr_in server;
	struct timeval timeout;
	fd_set fds;

	server.sin_family = AF_INET;
	server.sin_port = htons(remote_port);
	server.sin_addr.s_addr = inet_addr(remote_addr);

	if (_lwip_socket_set_nonblock(fd) < 0)
		return _lwip_socket_error(errno);

	if (connect(fd, (struct sockaddr *)&server, sizeof(server)) == 0)
		return 0;

	if (errno != EINPROGRESS)
		return _lwip_socket_error(errno);

	if (timeout_msec <= 0)
		timeout_msec = 10000; // 10sec

	timeout.tv_sec = timeout_msec / 1000;
    timeout.tv_usec = (timeout_msec % 1000) * 1000;

	FD_SET(fd, &fds);

	if (select(fd + 1, NULL, &fds, NULL, &timeout) < 0)
		return _lwip_socket_error(errno);

	if (_lwip_socket_set_block(fd) < 0)
		return _lwip_socket_error(errno);

	if (FD_ISSET(fd, &fds))
	{
		int err;

		if (_lwip_socket_get_error(fd, &err) != 0)
			return _lwip_socket_error(errno);

		return -err;
	}

	return _lwip_socket_error(ETIMEDOUT);
}

/**********************************************************************************************/

#define _lwip_socket_fds_debug		/*_lwip_socket_log */

static bool _lwip_socket_fds_mutex_take (lwip_socket_info_t *info)
{
	atcmd_trace_mutex_take(ATCMD_TRACE_MUTEX_FDS);

	if (xSemaphoreTake(info->fds.mutex, LWIP_SOCKET_FDS_MUTEX_TIMEOUT) == pdFALSE)
	{
		_lwip_socket_error(EPERM);
		return false;
	}

	return true;
}

static bool _lwip_socket_fds_mutex_give (lwip_socket_info_t *info)
{
	atcmd_trace_mutex_give(ATCMD_TRACE_MUTEX_FDS);

	if (xSemaphoreGive(info->fds.mutex) == pdFALSE)
	{
		_lwip_socket_error(EPERM);
		return false;
	}

	return true;
}

static int _lwip_socket_fds_get (lwip_socket_info_t *info, fd_set *read, fd_set *write)
{
	if (_lwip_socket_fds_mutex_take(info))
	{
		int nfds = 0;
		int fd;

		if (read)
			FD_ZERO(read);

		if (write)
			FD_ZERO(write);

		for (fd = 0 ; fd < FD_SETSIZE ; fd++)
		{
			if (FD_ISSET(fd, &info->fds.read))
			{
				nfds = fd + 1;

				if (read)
					FD_SET(fd, read);

				if (write && FD_ISSET(fd, &info->fds.write))
					FD_SET(fd, write);
			}
		}

		if (_lwip_socket_fds_mutex_give(info))
			return nfds;
	}

	return _lwip_socket_error(EPERM);
}

static int _lwip_socket_fds_set (lwip_socket_info_t *info, int fd, bool listen)
{
	if (_lwip_socket_fds_mutex_take(info))
	{
		_lwip_socket_log("SOCK_FDS_SET: fd=%d listen=%d\n", fd, listen);

		FD_SET(fd, &info->fds.read);
		FD_SET(fd, &info->fds.write);

		if (listen)
			FD_SET(fd, &info->fds.listen);

		if (_lwip_socket_fds_mutex_give(info))
			return 0;
	}

	return _lwip_socket_error(EPERM);
}

static int _lwip_socket_fds_clear (lwip_socket_info_t *info, int fd)
{
	if (_lwip_socket_fds_mutex_take(info))
	{
		_lwip_socket_log("SOCK_FDS_CLR: fd=%d\n", fd);

		FD_CLR(fd, &info->fds.read);
		FD_CLR(fd, &info->fds.write);
		FD_CLR(fd, &info->fds.listen);
		FD_CLR(fd, &info->fds.send);

		if (_lwip_socket_fds_mutex_give(info))
			return 0;
	}

	return _lwip_socket_error(EPERM);
}

static void _lwip_socket_task (void *arg)
{
	lwip_socket_info_t *info = (lwip_socket_info_t *)arg;
	fd_set fds_read, fds_write;
	struct timeval timeout;
	int nfds;
	int ret;
	int fd;

	FD_ZERO(&fds_read);
	FD_ZERO(&fds_write);

#ifdef CONFIG_ATCMD_XPUT_IMPROVEMENT
	timeout.tv_sec = LWIP_SOCKET_TASK_TIMEOUT / 1000000;
	timeout.tv_usec = LWIP_SOCKET_TASK_TIMEOUT % 1000000;
#endif

	while (1)
	{
#ifdef CONFIG_ATCMD_SOCKET_EVENT_SEND
		nfds = _lwip_socket_fds_get(info, &fds_read, &fds_write);
#else
		nfds = _lwip_socket_fds_get(info, &fds_read, NULL);
#endif
		if (nfds <= 0)
		{
			_delay_ms(100);
			continue;
		}

#ifndef CONFIG_ATCMD_XPUT_IMPROVEMENT
		timeout.tv_sec = LWIP_SOCKET_TASK_TIMEOUT / 1000000;
		timeout.tv_usec = LWIP_SOCKET_TASK_TIMEOUT % 1000000;
#endif

		atcmd_trace_func_call(ATCMD_TRACE_FUNC_SELECT);

#ifdef CONFIG_ATCMD_SOCKET_EVENT_SEND
		ret = select(nfds, &fds_read, &fds_write, NULL, &timeout);
#else
		ret = select(nfds, &fds_read, NULL, NULL, &timeout);
#endif

		atcmd_trace_func_return(ATCMD_TRACE_FUNC_SELECT);

		switch (ret)
		{
			case -1:
				_lwip_socket_error(errno);
				break;

			case 0: // time expired
//				_lwip_socket_log("SOCK_TASK: timeout (%d usec)\n", LWIP_SOCKET_TASK_TIMEOUT);
				break;
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
						char remote_addr[15 + 1];
						uint16_t remote_port = 0;
						int _fd;

						_fd = _lwip_socket_accept(fd, remote_addr, &remote_port);

						if (_fd >= 0)
						{
							_lwip_socket_fds_set(info, _fd, false);

							info->cb.tcp_connect(_fd, remote_addr, remote_port);
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

#ifdef CONFIG_ATCMD_SOCKET_EVENT_SEND
			if (FD_ISSET(fd, &fds_write))
			{
				ret--;

				if (FD_ISSET(fd, &info->fds.send))
				{
					if (_lwip_socket_fds_mutex_take(info))
					{
						_lwip_socket_fds_debug("SOCK_FDS_WRITE: fd=%d\n", fd);

						FD_CLR(fd, &info->fds.send);

						_lwip_socket_fds_mutex_give(info);

						if (info->cb.send_ready)
							info->cb.send_ready(fd);

						atcmd_trace_func_call(ATCMD_TRACE_FUNC_EVTSET);

						xEventGroupSetBits(info->event, 1 << fd);

						atcmd_trace_func_return(ATCMD_TRACE_FUNC_EVTSET);
					}
				}
			}
#endif
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

	if (g_lwip_socket_info)
		return _lwip_socket_error(EBUSY);

	info = _lwip_socket_malloc(sizeof(lwip_socket_info_t));
	if (!info)
		goto _lwip_socket_init_fail;

	memset(info, 0, sizeof(lwip_socket_info_t));

	FD_ZERO(&info->fds.read);
	FD_ZERO(&info->fds.write);
	FD_ZERO(&info->fds.listen);
	FD_ZERO(&info->fds.send);

	if (cb)
		memcpy(&info->cb, cb, sizeof(lwip_socket_cb_t));

	info->fds.mutex = xSemaphoreCreateMutexStatic(&info->fds.mutex_buffer);
	if (!info->fds.mutex)
		goto _lwip_socket_init_fail;

	info->event = xEventGroupCreate();
	if (!info->event)
		goto _lwip_socket_init_fail;

	if (xTaskCreate(_lwip_socket_task, "_lwip_socket",
			LWIP_SOCKET_TASK_STACK_SIZE, info,
			LWIP_SOCKET_TASK_PRIORITY, &info->task) != pdPASS)
		goto _lwip_socket_init_fail;

	g_lwip_socket_info = info;

	return 0;

_lwip_socket_init_fail:

	if (info->fds.mutex)
		vSemaphoreDelete(info->fds.mutex);

	if (info->event)
		vEventGroupDelete(info->event);

	if (info)
		_lwip_socket_mfree(info);

	return _lwip_socket_error(ENOMEM);
}

int _lwip_socket_exit (void)
{
	if (g_lwip_socket_info)
	{
		vTaskDelete(g_lwip_socket_info->task);
		vEventGroupDelete(g_lwip_socket_info->event);
		vSemaphoreDelete(g_lwip_socket_info->fds.mutex);
		_lwip_socket_mfree(g_lwip_socket_info);
		g_lwip_socket_info = NULL;

		return 0;
	}

	return _lwip_socket_error(EBUSY);
}

static int _lwip_socket_open (int type, int *fd,
							const char *remote_addr, uint16_t remote_port,
							uint16_t local_port, int timeout_msec)
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

				ret = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
				if (ret < 0)
					return _lwip_socket_error(errno);

				*fd = ret;
				ret = _lwip_socket_bind(*fd, local_port);
				break;

			case SOCK_STREAM:
				if (remote_port && local_port)
					return _lwip_socket_error(EINVAL);

				ret = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
				if (ret < 0)
					return _lwip_socket_error(errno);

				*fd = ret;

				if (remote_addr)
					ret = _lwip_socket_connect(*fd, remote_addr, remote_port, timeout_msec);
				else
				{
					ret = _lwip_socket_bind(*fd, local_port);
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

int _lwip_socket_open_udp (int *fd, uint16_t local_port)
{
	return _lwip_socket_open(SOCK_DGRAM, fd, NULL, 0, local_port, 0);
}

int _lwip_socket_open_tcp_server (int *fd, uint16_t local_port)
{
	return _lwip_socket_open(SOCK_STREAM, fd, NULL, 0, local_port, 0);
}

int _lwip_socket_open_tcp_client (int *fd, const char *remote_addr, uint16_t remote_port,
									int timeout_msec)
{
	return _lwip_socket_open(SOCK_STREAM, fd, remote_addr, remote_port, 0, timeout_msec);
}

int _lwip_socket_close (int fd)
{
	if (g_lwip_socket_info)
	{
		int ret;

		ret = close(fd);
		if (ret < 0)
			return _lwip_socket_error(errno);

		_lwip_socket_fds_clear(g_lwip_socket_info, fd);

		return 0;
	}

	return _lwip_socket_error(EPERM);
}

int _lwip_socket_send_start (int fd)
{
	if (g_lwip_socket_info)
	{
		if (_lwip_socket_fds_mutex_take(g_lwip_socket_info))
		{
			FD_SET(fd, &g_lwip_socket_info->fds.send);

			if (_lwip_socket_fds_mutex_give(g_lwip_socket_info))
				return 0;
		}
	}

	return _lwip_socket_error(EPERM);
}

int _lwip_socket_send_wait (int fd)
{
	if (g_lwip_socket_info)
	{
		EventBits_t event;

		atcmd_trace_func_call(ATCMD_TRACE_FUNC_EVTWAIT);

		event = xEventGroupWaitBits(g_lwip_socket_info->event, 1 << fd,
									pdTRUE, pdFALSE, portMAX_DELAY);

		atcmd_trace_func_return(ATCMD_TRACE_FUNC_EVTWAIT);

		if (event == 0)
		{
			_lwip_socket_send_stop(fd);

			return _lwip_socket_error(ETIME);
		}

		return 0;
	}

	return _lwip_socket_error(EPERM);
}

int _lwip_socket_send_stop (int fd)
{
	if (g_lwip_socket_info)
	{
		if (_lwip_socket_fds_mutex_take(g_lwip_socket_info))
		{
			FD_CLR(fd, &g_lwip_socket_info->fds.send);

			if (_lwip_socket_fds_mutex_give(g_lwip_socket_info))
				return 0;
		}
	}

	return _lwip_socket_error(EPERM);
}

int _lwip_socket_send (int fd, const char *remote_addr, uint16_t remote_port,
						char *data, int len)
{
	if (g_lwip_socket_info)
	{
		bool flags = MSG_DONTWAIT;
		int type;
		int ret;

		if (!data || !len)
			return _lwip_socket_error(EINVAL);

		if (_lwip_socket_get_type(fd, &type) != 0)
			return _lwip_socket_error(errno);

		switch (type)
		{
			case SOCK_STREAM:
				atcmd_trace_func_call(ATCMD_TRACE_FUNC_SEND);

				ret = send(fd, data, len, flags);

				atcmd_trace_func_return(ATCMD_TRACE_FUNC_SEND);
				break;

			case SOCK_DGRAM:
				if (!remote_addr || !remote_port)
					return _lwip_socket_error(EINVAL);
				else
				{
					const int max_len = 1470;
					struct sockaddr_in dest_addr;
					int addrlen = sizeof(dest_addr);
					int _len;
					int i;

					dest_addr.sin_family = AF_INET;
					dest_addr.sin_port = htons(remote_port);
					dest_addr.sin_addr.s_addr = ipaddr_addr(remote_addr);

					atcmd_trace_func_call(ATCMD_TRACE_FUNC_SENDTO);

					for (ret = i = 0 ; i < len ; i += ret)
					{
						_len = len - i;
						if (_len > max_len)
							_len = max_len;

						ret = sendto(fd, data + i, _len, flags,
								(struct sockaddr*)&dest_addr, addrlen);

						if (ret > 0)
							continue;
						else if (ret == 0)
							ret = i;

						break;
					}

					if (i == len)
						ret = len;

					atcmd_trace_func_return(ATCMD_TRACE_FUNC_SENDTO);
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

int _lwip_socket_recv (int fd, char *remote_addr, uint16_t *remote_port,
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
				atcmd_trace_func_call(ATCMD_TRACE_FUNC_RECV);

				ret = recv(fd, data, len, flags);

				atcmd_trace_func_return(ATCMD_TRACE_FUNC_RECV);
				break;

			case SOCK_DGRAM:
				if (!remote_addr || !remote_port)
					return _lwip_socket_error(EINVAL);
				else
				{
					struct sockaddr_in src_addr;
					int addrlen;

					atcmd_trace_func_call(ATCMD_TRACE_FUNC_RECVFROM);

					ret = recvfrom(fd, data, len, flags,
							(struct sockaddr *)&src_addr, (socklen_t *)&addrlen);

					atcmd_trace_func_return(ATCMD_TRACE_FUNC_RECVFROM);

					if (ret > 0)
					{
#ifdef CONFIG_ATCMD_XPUT_IMPROVEMENT
						inet_ntoa_r(src_addr.sin_addr, remote_addr, ATCMD_IP4_ADDR_LEN_MAX + 1);
#else
						strcpy(remote_addr, inet_ntoa(src_addr.sin_addr));
#endif
						*remote_port = ntohs(src_addr.sin_port);
					}
				}
				break;

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

