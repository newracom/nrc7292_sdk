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

#include "raspi-hif.h"
#include "nrc-atcmd.h"

#define atcmd_log(fmt, ...)			if (g_atcmd_info.log) log_info(fmt, ##__VA_ARGS__)

#define atcmd_log_send(fmt, ...)	atcmd_log("SEND: " fmt, ##__VA_ARGS__)
#define atcmd_log_recv(fmt, ...)	atcmd_log("RECV: " fmt, ##__VA_ARGS__)
#define atcmd_log_data(fmt, ...)	atcmd_log("DATA: " fmt, ##__VA_ARGS__)

/**********************************************************************************************/

static struct
{
	bool log;
	bool ready;

	struct
	{
		uint32_t send;
		uint32_t recv;

		bool print_send;
		bool print_recv;
	} data;

	struct
	{
		int val; /* -1: error, 0: ok, 1: don't care */

		pthread_mutex_t mutex;
		pthread_cond_t cond;
	} ret;

	struct
	{
		atcmd_boot_cb_t boot;
		atcmd_info_cb_t info;
		atcmd_event_cb_t event;
		atcmd_rxd_cb_t rxd;
	} cb;
} g_atcmd_info =
{
	.log = true,
	.ready = false,
	
	.data =
	{
		.send = 0,
		.recv = 0,
		.print_send = false,
		.print_recv = false,
	},

	.ret =
	{
		.val = 1,
		.mutex = PTHREAD_MUTEX_INITIALIZER,
		.cond  = PTHREAD_COND_INITIALIZER,
	},

	.cb =
	{
		.boot = NULL,
		.info = NULL,
		.event = NULL,
		.rxd = NULL,
	}
};

char *nrc_atcmd_param_to_str (const char *param, char *str, int len)
{
	int param_len;

	if (!param || !str || !len)
		return NULL;

	param_len = strlen(param);

	if (param_len <= 2 || (param_len - 2) > (len - 1))
		return NULL;

	if (param[0] != '"' && param[param_len - 1] != '"')
		return NULL;

	memcpy(str, &param[1], param_len - 2);
	str[param_len - 2] = '\0';

	return str;
}

static void nrc_atcmd_print_data (char *data, int len)
{
	char buf[2][36];
	int i;

	memset(buf, 0, sizeof(buf));

	for (i = 0 ; i < len ; i++)
	{
		sprintf(buf[0] + (3 * (i % 10)), "%02X ", data[i]);

		if (data[i] >= 0x20 && data[i] <= 0x7E)
			sprintf(buf[1] + (i % 10), "%c", data[i]);
		else
			sprintf(buf[1] + (i % 10), ".");

		if ((i % 10) == 9)
		{
			atcmd_log_data("%s %s\n", buf[0], buf[1]);
			memset(buf, 0, sizeof(buf));
		}
	}
		
	if (((i - 1) % 10) < 9)
	{
		for (i %= 10 ; i < 10 ; i++)
			sprintf(buf[0] + (3 * (i % 10)), "   ");

		atcmd_log_data("%s %s\n", buf[0], buf[1]);
	}
}

bool nrc_atcmd_is_ready (void)
{
	return g_atcmd_info.ready;
}

static void nrc_atcmd_init_return (void)
{
	pthread_mutex_lock(&g_atcmd_info.ret.mutex);

	g_atcmd_info.ret.val = ATCMD_RET_NONE;

	pthread_mutex_unlock(&g_atcmd_info.ret.mutex);
}

static int nrc_atcmd_get_return (void)
{
	int ret;

	pthread_mutex_lock(&g_atcmd_info.ret.mutex);

	ret = g_atcmd_info.ret.val;

	pthread_mutex_unlock(&g_atcmd_info.ret.mutex);

	return ret;
}

static void nrc_atcmd_set_return (int ret)
{
	pthread_mutex_lock(&g_atcmd_info.ret.mutex);

	g_atcmd_info.ret.val = ret;

	pthread_cond_signal(&g_atcmd_info.ret.cond);
	pthread_mutex_unlock(&g_atcmd_info.ret.mutex);
}

static void nrc_atcmd_wait_return (char *cmd)
{
	uint8_t timeout = 0;
	int ret = 0;

	if (strcmp(cmd, "ATZ\r\n") == 0)
	{
		g_atcmd_info.ready = false;	
		timeout = 5;
	}
	else if (strlen(cmd) > 8 && memcmp(cmd, "AT+UART=", 8) == 0)
		timeout = 1;
	else if (strlen(cmd) > 14 && memcmp(cmd, "AT+WDEEPSLEEP=", 14) == 0)
		g_atcmd_info.ready = false;	

	pthread_mutex_lock(&g_atcmd_info.ret.mutex);

	if (timeout == 0)
		ret = pthread_cond_wait(&g_atcmd_info.ret.cond, &g_atcmd_info.ret.mutex);
	else
	{
		struct timeval curtime;
		struct timespec abstime;

		gettimeofday(&curtime, NULL);

		abstime.tv_sec = curtime.tv_sec + timeout;
		abstime.tv_nsec = curtime.tv_usec * 1000;

		ret = pthread_cond_timedwait(&g_atcmd_info.ret.cond, &g_atcmd_info.ret.mutex, &abstime);
	}

	pthread_mutex_unlock(&g_atcmd_info.ret.mutex);

	switch (ret)
	{
		case 0:
			break;

		case EPERM:
		case EINVAL:
			log_error("%s\n", strerror(ret));
			break;

		case ETIMEDOUT:
			log_info("NO RETURN\n");
			break;

		default:
			log_error("%swait\n", timeout > 0 ? "timed" : "");
	}
}

int nrc_atcmd_send (char *buf, int len)
{
	int retry;
	int ret;
	int i;

	for (retry = 0, i = 0 ; i < len ; i += ret)
	{
		ret = raspi_hif_write(buf + i, len - i);

		if (ret < 0)
		{
			if (ret == -EAGAIN)
				ret = 0;
			else
			{
				log_error("raspi_hif_write, %s\n", strerror(-ret));
				return ret;
			}
		}

		if (ret == 0)
		{
			retry++;
			usleep(1000);
		}
	}

/*	if (retry > 0)
		atcmd_log_send("retry=%d\n", retry); */

	return len;
}

int nrc_atcmd_send_cmd (const char *fmt, ...)
{
	va_list ap;
	char name[20 + 1] = { '\0', };
	char cmd[ATCMD_MSG_LEN_MAX + 1];
	int len;
	int ret;

	va_start(ap, fmt);

	len = vsnprintf(cmd, ATCMD_MSG_LEN_MAX, fmt, ap);
	if (len < (ATCMD_MSG_LEN_MIN - 2) || len > (ATCMD_MSG_LEN_MAX - 2))
		return -1;

	va_end(ap);

	if (memcmp(cmd, "AT", 2) != 0)
		return -1;

	if (cmd[2] == '+')
	{
		int i;

		for (i = 0 ; i < 20 && cmd[3 + i] != '\0' ; i++)
		{
			name[i] = cmd[3 + i];

			if (name[i] == '?' || name[i] == '=')
				break;
		}

		name[i] = '\0';
	}

	len += snprintf(cmd + len, ATCMD_MSG_LEN_MAX - len, "\r\n");

	nrc_atcmd_init_return();

	ret = nrc_atcmd_send(cmd, len);
	if (ret < 0)
		return -1;

	atcmd_log_send("%s", cmd);

	nrc_atcmd_wait_return(cmd);

	ret = nrc_atcmd_get_return();

	return ret;
}

int nrc_atcmd_send_data (char *data, int len)
{
	if (nrc_atcmd_send(data, len) < 0)
		return -1;

	atcmd_log_send("DATA %d\n", len);
	if (g_atcmd_info.data.print_send)
		nrc_atcmd_print_data(data, len);

	g_atcmd_info.data.send += len;

	return 0;
}

static atcmd_rxd_t *nrc_atcmd_alloc_rxd (enum ATCMD_DATA_TYPE type)
{
	atcmd_rxd_t *rxd;

	rxd = malloc(sizeof(atcmd_rxd_t));
	if (!rxd)
		log_error("%s\n", strerror(errno));		
	else
	{
		memset(rxd, 0, sizeof(atcmd_rxd_t));

		switch (type)
		{
			case ATCMD_DATA_SOCKET:
				rxd->id = -1;
				strcpy(rxd->remote_addr, "0.0.0.0");
				rxd->verbose = false;
				break;

			case ATCMD_DATA_SFUSER:
			case ATCMD_DATA_SFSYSUSER:
				break;

			default:
				free(rxd);
				return NULL;
		}

		rxd->type = type;
	}

	return rxd;
}

static int nrc_atcmd_recv_boot (char *msg, int len)
{
	int reason = 0;

	if (!msg || !len)
		return -EINVAL;

	if (memcmp(msg, "+BOOT:", 6) != 0)
		return -EINVAL;
	else
	{
		msg += 6;
		len -= 6;
	}

	if (!g_atcmd_info.ready)
	{
		g_atcmd_info.ready = true;	
		nrc_atcmd_set_return(ATCMD_RET_NONE);
	}

	if (strstr(msg, "POR"))
		reason |= ATCMD_BOOT_POR;
	
	if (strstr(msg, "WDT"))
		reason |= ATCMD_BOOT_WDT;
	
	if (strstr(msg, "PMC"))
		reason |= ATCMD_BOOT_PMC;
	
	if (strstr(msg, "HSPI"))
		reason |= ATCMD_BOOT_HSPI;
	
/*	log_debug("reason: 0x%X (%s)\n", reason, msg); */

	if (g_atcmd_info.cb.boot)
		g_atcmd_info.cb.boot(reason);

	return 0;
}

/* static int nrc_atcmd_recv_info (char *msg, int len)
{
	if (!msg || !len)
		return -1;

	if (g_atcmd_info.cb.info)
		g_atcmd_info.cb.info(0, 1, &msg);

	return 0;
} */

static int nrc_atcmd_recv_event (char *msg, int len)
{
	const char *name[ATCMD_EVENT_NUM] =
	{
		[ATCMD_BEVENT_FWBINDL_IDLE] = "FWBINDL_IDLE",
		[ATCMD_BEVENT_FWBINDL_DROP] = "FWBINDL_DROP",
		[ATCMD_BEVENT_FWBINDL_FAIL] = "FWBINDL_FAIL",
		[ATCMD_BEVENT_FWBINDL_DONE] = "FWBINDL_DONE",

		[ATCMD_BEVENT_SFUSER_IDLE] = "SFUSER_IDLE",
		[ATCMD_BEVENT_SFUSER_DROP] = "SFUSER_DROP",
		[ATCMD_BEVENT_SFUSER_FAIL] = "SFUSER_FAIL",
		[ATCMD_BEVENT_SFUSER_DONE] = "SFUSER_DONE",

		/* Wi-Fi Events */
		[ATCMD_WEVENT_CONNECT_SUCCESS] = "CONNECT_SUCCESS",
		[ATCMD_WEVENT_DISCONNECT] = "DISCONNECT",

		[ATCMD_WEVENT_DHCP_START] = "DHCP_START",
		[ATCMD_WEVENT_DHCP_STOP] = "DHCP_STOP",
		[ATCMD_WEVENT_DHCP_BUSY] = "DHCP_BUSY",
		[ATCMD_WEVENT_DHCP_FAIL] = "DHCP_FAIL",
		[ATCMD_WEVENT_DHCP_SUCCESS] = "DHCP_SUCCESS",
		[ATCMD_WEVENT_DHCP_TIMEOUT] = "DHCP_TIMEOUT",

		[ATCMD_WEVENT_FOTA_VERSION] = "FOTA_VERSION",
		[ATCMD_WEVENT_FOTA_BINARY] = "FOTA_BINARY",
		[ATCMD_WEVENT_FOTA_DOWNLOAD] = "FOTA_DOWNLOAD",
		[ATCMD_WEVENT_FOTA_UPDATE] = "FOTA_UPDATE",
		[ATCMD_WEVENT_FOTA_FAIL] = "FOTA_FAIL",

		[ATCMD_WEVENT_DEEPSLEEP_WAKEUP] = "DEEPSLEEP_WAKEUP",

		/* Socket Events */
		[ATCMD_SEVENT_CONNECT] = "CONNECT",
		[ATCMD_SEVENT_CLOSE] = "CLOSE",
		[ATCMD_SEVENT_SEND_IDLE] = "SEND_IDLE",
		[ATCMD_SEVENT_SEND_DROP] = "SEND_DROP",
		[ATCMD_SEVENT_SEND_EXIT] = "SEND_EXIT",
		[ATCMD_SEVENT_SEND_ERROR] = "SEND_ERROR",
		[ATCMD_SEVENT_RECV_READY] = "RECV_READY",
		[ATCMD_SEVENT_RECV_ERROR] = "RECV_ERROR",
	};
	char *argv[10];
	int argc;
	int event;
	int i;

	if (!msg || !len)
		return -1;

	msg += 8;
	len -= 8;

	argv[0] = msg;
	argc = 1;

	for (i = 0 ; i < len ; i++)
	{
		if (msg[i] == ',')
		{
			msg[i] = '\0';

			if (argc < 10)
				argv[argc] = &msg[i + 1];

			argc++;
		}
	}

	nrc_atcmd_param_to_str(argv[0], argv[0], strlen(argv[0]));

	for (event = ATCMD_EVENT_START ; event < ATCMD_EVENT_END ; event++)
	{
		if (strcmp(argv[0], "TCP_ERROR") == 0)
			strcpy(argv[0], "RECV_ERROR");

		if (strcmp(argv[0], name[event]) == 0)
		{
			if (event == ATCMD_WEVENT_DEEPSLEEP_WAKEUP)
			{
				if (!g_atcmd_info.ready)
				{
					g_atcmd_info.ready = true;
					nrc_atcmd_set_return(ATCMD_RET_NONE);
				}
			}

			if (g_atcmd_info.cb.event)
				g_atcmd_info.cb.event(event, argc - 1, argv + 1);

			break;
		}
	}

	return 0;
}

static atcmd_rxd_t *nrc_atcmd_recv_rxd (char *msg)
{
	atcmd_rxd_t *rxd = NULL;
	char *argv[4];
	int argc;
	int i;

	if (!msg)
		return NULL;

	rxd = nrc_atcmd_alloc_rxd(ATCMD_DATA_SOCKET);
	if (!rxd)
		return NULL;

	if (memcmp(msg, "+RXD:", 5) != 0)
		goto invalid_rxd;

	msg += 5;

	for (argv[0] = msg, argc = 1 ; *msg != '\0' ; msg++)
	{
		if (argc > 4)
			goto invalid_rxd;

		if (*msg == ',')
		{
			*msg = '\0';
			argv[argc++] = msg + 1;
		}
	}

	if (argc != 2 && argc != 4)
		goto invalid_rxd;

	rxd->verbose = argc == 4 ? true : false;

	for (i = 0 ; i < argc ; i++)
	{
		switch (i)
		{
			case 0:
				rxd->id = atoi(argv[i]);
				if (rxd->id < 0)
					goto invalid_rxd;
				break;

			case 1:
				rxd->length = atoi(argv[i]);
				if (rxd->length < 0)
					goto invalid_rxd;
				break;

			case 2:
			{
				int ip_len = strlen(argv[i]);

				if (ip_len	< ATCMD_IPADDR_LEN_MIN || ip_len > ATCMD_IPADDR_LEN_MAX)
					goto invalid_rxd;

				strcpy(rxd->remote_addr, argv[i]);
				break;
			}

			case 3:
				rxd->remote_port = atoi(argv[i]);
				if (rxd->remote_port < 0)
					goto invalid_rxd;
		}
	}

/*	if (rxd->verbose)
		log_debug("rxd_msg_rxd: socket, id=%d len=%d remote=%s,%d\n",
				rxd->id, rxd->len, rxd->remote_addr, rxd->remote_port);
	else
		log_debug("rxd_msg_info: socket, id=%d len=%d\n", rxd->id, rxd->len); */

	return rxd;

invalid_rxd:

	free(rxd);

	return NULL;
}

static atcmd_rxd_t *nrc_atcmd_recv_rxd_sfuser (char *msg)
{
	atcmd_rxd_t *rxd = NULL;
	char *argv[2];
	int argc;

	if (!msg)
		return NULL;

	rxd = nrc_atcmd_alloc_rxd(ATCMD_DATA_SFUSER);
	if (!rxd)
		return NULL;

	if (memcmp(msg, "+RXD_SFUSER:", 12) != 0)
		goto invalid_rxd_sfuser;

	msg += 12;

	for (argv[0] = msg, argc = 1 ; *msg != '\0' ; msg++)
	{
		if (argc > 2)
			goto invalid_rxd_sfuser;

		if (*msg == ',')
		{
			*msg = '\0';
			argv[argc++] = msg + 1;
		}
	}

	if (argc != 2)
		goto invalid_rxd_sfuser;

	rxd->offset = atoi(argv[0]);
	if (rxd->offset < 0)
		goto invalid_rxd_sfuser;

	rxd->length = atoi(argv[1]);
	if (rxd->length < 0)
		goto invalid_rxd_sfuser;

/*	log_debug("rxd_msg_info: sf_user, offset=%d length=%d\n", rxd->offset, rxd->length); */

	return rxd;

invalid_rxd_sfuser:

	free(rxd);

	return NULL;
}

static atcmd_rxd_t *nrc_atcmd_recv_rxd_sfsysuser (char *msg)
{
	atcmd_rxd_t *rxd = NULL;
	char *argv[2];
	int argc;

	if (!msg)
		return NULL;

	rxd = nrc_atcmd_alloc_rxd(ATCMD_DATA_SFSYSUSER);
	if (!rxd)
		return NULL;

	if (memcmp(msg, "+RXD_SFSYSUSER:", 15) != 0)
		goto invalid_rxd_sfsysuser;

	msg += 15;

	for (argv[0] = msg, argc = 1 ; *msg != '\0' ; msg++)
	{
		if (argc > 2)
			goto invalid_rxd_sfsysuser;

		if (*msg == ',')
		{
			*msg = '\0';
			argv[argc++] = msg + 1;
		}
	}

	if (argc != 2)
		goto invalid_rxd_sfsysuser;

	rxd->offset = atoi(argv[0]);
	if (rxd->offset < 0)
		goto invalid_rxd_sfsysuser;

	rxd->length = atoi(argv[1]);
	if (rxd->length < 0)
		goto invalid_rxd_sfsysuser;

/*	log_debug("rxd_msg_info: sf_sys_user, offset=%d length=%d\n", rxd->offset, rxd->length); */

	return rxd;
		
invalid_rxd_sfsysuser:
		
	free(rxd);
	
	return NULL;
}

void nrc_atcmd_recv (char *buf, int len)
{
	enum ATCMD_MSG_TYPE
	{
		ATCMD_MSG_ERROR = -1,
		ATCMD_MSG_OK = 0,
		ATCMD_MSG_INFO,
		ATCMD_MSG_BOOT,
		ATCMD_MSG_BEVENT,
		ATCMD_MSG_WEVENT,
		ATCMD_MSG_SEVENT,
		ATCMD_MSG_DATA_SOCKET,
		ATCMD_MSG_DATA_SFUSER,
		ATCMD_MSG_DATA_SFSYSUSER,

		ATCMD_MSG_NONE = 255,
	};

	static struct
	{
		int type;

		int cnt;
		char buf[ATCMD_MSG_LEN_MAX + 1];
	}  msg =
	{
		.type = ATCMD_MSG_NONE,
		.cnt = 0,
		.buf = { 0, }
	};

	static struct
	{
		atcmd_rxd_t *rxd;

		int cnt;
		char buf[ATCMD_DATA_LEN_MAX + 1];		
	} data =
	{
		.rxd = NULL,
		.cnt = 0,
		.buf = { 0, }
	};
	int i;

	for (i = 0 ; i < len ; i++)
	{
		if (data.rxd)
		{
			data.buf[data.cnt] = buf[i];

			if (++data.cnt == data.rxd->length)
			{
				atcmd_log_recv("DATA %d\n", data.cnt);
				if (g_atcmd_info.data.print_recv)
					nrc_atcmd_print_data(data.buf, data.cnt);

				g_atcmd_info.data.recv += data.cnt;

				if (g_atcmd_info.cb.rxd)
					g_atcmd_info.cb.rxd(data.rxd, data.buf);

				free(data.rxd);
				data.rxd = NULL;
				data.cnt = 0;
			}

			continue;
		}

		if (msg.cnt >= ATCMD_MSG_LEN_MAX)
		{
			if (msg.type != ATCMD_MSG_NONE)
				log_error("message length > %d\n", ATCMD_MSG_LEN_MAX);

			msg.type = ATCMD_MSG_NONE;
			msg.cnt = 0;
		}

		msg.buf[msg.cnt++] = buf[i];

		switch (msg.type)
		{
			case ATCMD_MSG_NONE:
				if (msg.cnt > 1)
				{
					log_error("invaid count (%d)\n", msg.cnt);
					msg.buf[0] = msg.buf[msg.cnt - 1];
					msg.cnt = 1;
				}

				switch (msg.buf[0])
				{
					case 'O':
						msg.type = ATCMD_MSG_OK;
						break;

					case 'E':
						msg.type = ATCMD_MSG_ERROR;
						break;

					case '+':
						msg.type = ATCMD_MSG_INFO;
						break;

					default:
						msg.cnt = 0;
				}

				continue;

			case ATCMD_MSG_OK:
				if (msg.cnt < 4)
					continue;
				else if (memcmp(msg.buf, "OK\r\n", msg.cnt) != 0)
				{
					msg.type = ATCMD_MSG_NONE;
					msg.cnt = 0;
					continue;
				}

				break;

			case ATCMD_MSG_ERROR:
				if (msg.cnt < 7)
					continue;
				else if (memcmp(msg.buf, "ERROR\r\n", msg.cnt) != 0)
				{
					msg.type = ATCMD_MSG_NONE;
					msg.cnt = 0;
					continue;
				}

				break;

			case ATCMD_MSG_INFO:
				if (msg.cnt == 6 && memcmp(msg.buf, "+BOOT:", msg.cnt) == 0)
				{
					msg.type = ATCMD_MSG_BOOT;
					continue;
				}
				else if (msg.cnt == 8)
				{
					if (memcmp(msg.buf, "+BEVENT:", msg.cnt) == 0)
					{			
						msg.type = ATCMD_MSG_BEVENT;
						continue;
					}
					else if (memcmp(msg.buf, "+WEVENT:", msg.cnt) == 0)
					{
						msg.type = ATCMD_MSG_WEVENT;
						continue;
					}
					else if (memcmp(msg.buf, "+SEVENT:", msg.cnt) == 0)
					{
						msg.type = ATCMD_MSG_SEVENT;
						continue;
					}
				}
				else if (msg.cnt == 5 && memcmp(msg.buf, "+RXD:", msg.cnt) == 0)
				{
					msg.type = ATCMD_MSG_DATA_SOCKET;
					continue;
				}
				else if (msg.cnt == 12 && memcmp(msg.buf, "+RXD_SFUSER:", msg.cnt) == 0)
				{
					msg.type = ATCMD_MSG_DATA_SFUSER;
					continue;
				}
				else if (msg.cnt == 15 && memcmp(msg.buf, "+RXD_SFSYSUSER:", msg.cnt) == 0)
				{
					msg.type = ATCMD_MSG_DATA_SFSYSUSER;
					continue;
				}
		}

		if (msg.cnt >= 2 && memcmp(&msg.buf[msg.cnt - 2], "\r\n", 2) == 0)
		{
			msg.cnt -= 2;
			msg.buf[msg.cnt] = '\0';

			atcmd_log_recv("%s\n", msg.buf);

			switch (msg.type)
			{
				case ATCMD_MSG_OK:
					nrc_atcmd_set_return(ATCMD_RET_OK);
					break;

				case ATCMD_MSG_ERROR:
					nrc_atcmd_set_return(ATCMD_RET_ERROR);
					break;

				case ATCMD_MSG_INFO:
					/* nrc_atcmd_recv_info(msg.buf, msg.cnt); */
					break;

				case ATCMD_MSG_BOOT:
					nrc_atcmd_recv_boot(msg.buf, msg.cnt);
					break;

				case ATCMD_MSG_BEVENT:
				case ATCMD_MSG_WEVENT:
				case ATCMD_MSG_SEVENT:
					nrc_atcmd_recv_event(msg.buf, msg.cnt);
					break;

				case ATCMD_MSG_DATA_SOCKET:
					data.rxd = nrc_atcmd_recv_rxd(msg.buf);
					if (!data.rxd)
						atcmd_log_recv("!!! RXD FAIL !!!\n");
					break;

				case ATCMD_MSG_DATA_SFUSER:
					data.rxd = nrc_atcmd_recv_rxd_sfuser(msg.buf);
					if (!data.rxd)
						atcmd_log_recv("!!! RXD_SFUSER FAIL !!!\n");
					break;

				case ATCMD_MSG_DATA_SFSYSUSER:
					data.rxd = nrc_atcmd_recv_rxd_sfsysuser(msg.buf);
					if (!data.rxd)
						atcmd_log_recv("!!! RXD_SFSYSUSER FAIL !!!\n");
					break;

				default:
					log_error("invalid message type (%d)\n", msg.type);
			}

			msg.type = ATCMD_MSG_NONE;
			msg.cnt = 0;
		}
	}
}

int nrc_atcmd_register_callback (int type, void *func)
{
	switch (type)
	{
		case ATCMD_CB_BOOT:
			if (!g_atcmd_info.cb.boot)
			{
				g_atcmd_info.cb.boot = func;
				return 0;
			}
			break;

		case ATCMD_CB_INFO:
			if (!g_atcmd_info.cb.info)
			{
				g_atcmd_info.cb.info = func;
				return 0;
			}
			break;

		case ATCMD_CB_EVENT:
			if (!g_atcmd_info.cb.event)
			{
				g_atcmd_info.cb.event = func;
				return 0;
			}
			break;

		case ATCMD_CB_RXD:
			if (!g_atcmd_info.cb.rxd)
			{
				g_atcmd_info.cb.rxd = func;
				return 0;
			}
	}

	return -1;
}

int nrc_atcmd_unregister_callback (int type)
{
	switch (type)
	{
		case ATCMD_CB_BOOT:
			g_atcmd_info.cb.boot = NULL;
			break;

		case ATCMD_CB_INFO:
			g_atcmd_info.cb.info = NULL;
			break;

		case ATCMD_CB_EVENT:
			g_atcmd_info.cb.event = NULL;
			break;

		case ATCMD_CB_RXD:
			g_atcmd_info.cb.rxd = NULL;
			break;

		default:
			return -1;
	}

	return 0;
}

bool nrc_atcmd_log_print (int log)
{
	if (log >= 0)
		g_atcmd_info.log = !!log;

	return g_atcmd_info.log;
}

void nrc_atcmd_data_reset (void)
{
	g_atcmd_info.data.send = 0;
	g_atcmd_info.data.recv = 0;
}

void nrc_atcmd_data_info (uint64_t *send, uint64_t *recv)
{
	if (send)
		*send = g_atcmd_info.data.send;

	if (recv)
		*recv = g_atcmd_info.data.recv;
}

int nrc_atcmd_data_print (int send, int recv)
{
	int print = 0;

	if (send >= 0)
		g_atcmd_info.data.print_send = !!send;

	if (recv >= 0)
		g_atcmd_info.data.print_recv = !!recv;

	if (g_atcmd_info.data.print_send)
		print |= (1 << 0);

	if (g_atcmd_info.data.print_recv)
		print |= (1 << 1);

	return print & 0x3;
}

static union
{
	uint8_t flags;

	struct
	{
		uint8_t idle:1;
		uint8_t drop:1;
		uint8_t fail:1;
		uint8_t done:1;
		uint8_t reserved:4;
	};
} g_atcmd_firmware_download_event =
{
	.flags = 0,
};

static int nrc_atcmd_firmware_download_event_callback (enum ATCMD_EVENT event, int argc, char *argv[])
{
	switch (event)
	{
		case ATCMD_BEVENT_FWBINDL_IDLE:
			g_atcmd_firmware_download_event.idle = 1;
			break;

		case ATCMD_BEVENT_FWBINDL_DROP:
			g_atcmd_firmware_download_event.drop = 1;
			break;

		case ATCMD_BEVENT_FWBINDL_FAIL:
			g_atcmd_firmware_download_event.fail = 1;
			break;

		case ATCMD_BEVENT_FWBINDL_DONE:
			g_atcmd_firmware_download_event.done = 1;
			break;

		default:
			return -1;
	}

	return 0;
}

int nrc_atcmd_firmware_download (char *bin_data, int bin_size, uint32_t bin_crc32, int verify)
{
	uint32_t offset;
	uint32_t length;
	int ret = -1;

	if (nrc_atcmd_register_callback (ATCMD_CB_EVENT, nrc_atcmd_firmware_download_event_callback) != 0)
		return -1;

	if (nrc_atcmd_send_cmd("AT+FWUPDATE=0") != ATCMD_RET_OK)
		goto firmware_download_done;

	if (nrc_atcmd_send_cmd("AT+FWUPDATE=%u,0x%X,%u", bin_size, bin_crc32, verify) != ATCMD_RET_OK)
		goto firmware_download_done;

	if (nrc_atcmd_send_cmd("AT+FWUPDATE?") != ATCMD_RET_OK)
		goto firmware_download_done;

	if (nrc_atcmd_send_cmd("AT+FWBINDL?") != ATCMD_RET_OK)
		goto firmware_download_done;

	for (offset = 0 ; offset < bin_size ; offset += length)
	{
		if ((bin_size - offset) >= 4096)
			length = 4096;
		else
			length = bin_size - offset;

		g_atcmd_firmware_download_event.flags = 0;

		if (nrc_atcmd_send_cmd("AT+FWBINDL=%u,%u", offset, length) != ATCMD_RET_OK)
			goto firmware_download_done;

		if (nrc_atcmd_send_data(bin_data + offset, length) != 0)
			goto firmware_download_done;

		while (g_atcmd_firmware_download_event.flags == 0)
			usleep(1000);

		if (!g_atcmd_firmware_download_event.done)
		{
			nrc_atcmd_send_cmd("AT");
			goto firmware_download_done;
		}
	}

	if (nrc_atcmd_send_cmd("AT+FWBINDL?") != ATCMD_RET_OK)
		goto firmware_download_done;

	if (nrc_atcmd_send_cmd("AT+FWUPDATE?") != ATCMD_RET_OK)
		goto firmware_download_done;

	ret = 0;

firmware_download_done:

	nrc_atcmd_unregister_callback(ATCMD_CB_EVENT);

	return ret;
}

