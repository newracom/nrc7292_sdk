
#include "raspi-hif.h"
#include "nrc-atcmd.h"

#define atcmd_error(fmt, ...)		log_error(fmt, ##__VA_ARGS__)
#define atcmd_log(fmt, ...)			if (g_atcmd_info.log) log_info(fmt, ##__VA_ARGS__)

#define atcmd_log_send(fmt, ...)	atcmd_log("SEND: " fmt, ##__VA_ARGS__)
#define atcmd_log_recv(fmt, ...)	atcmd_log("RECV: " fmt, ##__VA_ARGS__)

/**********************************************************************************************/

static struct
{
	bool log;

	struct
	{
		int val; /* -1: error, 0: ok, 1: don't care */

		pthread_mutex_t mutex;
		pthread_cond_t cond;
	} ret;

	struct
	{
		atcmd_info_cb_t info;
		atcmd_event_cb_t event;
		atcmd_rxd_cb_t rxd;
	} cb;
} g_atcmd_info =
{
	.log = true,

	.ret =
	{
		.val = 1,
		.mutex = PTHREAD_MUTEX_INITIALIZER,
		.cond  = PTHREAD_COND_INITIALIZER,
	},

	.cb =
	{
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
		timeout = 3;
	else if (strlen(cmd) > 11 && memcmp(cmd, "AT+UART=", 8) == 0)
		timeout = 1;

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

static int nrc_atcmd_send (char *buf, int len)
{
	int ret;
	int i;

	for (i = 0 ; i < len ; i += ret)
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
			usleep(1000);
	}

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

	return nrc_atcmd_get_return();
}

int nrc_atcmd_send_data (char *data, int len)
{
	if (nrc_atcmd_send(data, len) < 0)
		return -1;

	atcmd_log_send("DATA %d\n", len);

	return 0;
}

static void nrc_atcmd_init_rxd (atcmd_rxd_t *rxd)
{
	if (rxd)
	{
		memset(rxd, 0, sizeof(atcmd_rxd_t));
		rxd->id = -1;
		strcpy(rxd->remote_addr, "0.0.0.0");
	}
}

static int nrc_atcmd_recv_info (char *msg, int len)
{
	if (!msg || !len)
		return -1;

	if (g_atcmd_info.cb.info)
	{
		g_atcmd_info.cb.info(0, 1, &msg);
	}

	return 0;
}

static int nrc_atcmd_recv_event (char *msg, int len)
{
	const char *name[ATCMD_EVENT_NUM] =
	{
		[ATCMD_WEVENT_SCAN_DONE] = "SCAN_DONE",
		[ATCMD_WEVENT_CONNECT_SUCCESS] = "CONNECT_SUCCESS",
		[ATCMD_WEVENT_CONNECT_FAIL] = "CONNECT_FAIL",
		[ATCMD_WEVENT_DISCONNECT] = "DISCONNECT",

		[ATCMD_SEVENT_CONNECT] = "CONNECT",
		[ATCMD_SEVENT_CLOSE] = "CLOSE",
		[ATCMD_SEVENT_SEND_IDLE] = "SEND_IDLE",
		[ATCMD_SEVENT_SEND_DROP] = "SEND_DROP",
		[ATCMD_SEVENT_SEND_EXIT] = "SEND_EXIT",
		[ATCMD_SEVENT_SEND_ERROR] = "SEND_ERROR",
		[ATCMD_SEVENT_RECV_ERROR] = "RECV_ERROR",

		[ATCMD_HEVENT_SEND_IDLE] = "SEND_IDLE",
		[ATCMD_HEVENT_SEND_DROP] = "SEND_DROP",
		[ATCMD_HEVENT_SEND_EXIT] = "SEND_EXIT",
		[ATCMD_HEVENT_SEND_ERROR] = "SEND_ERROR",
	};
	char *argv[3];
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

			argv[argc++] = &msg[i + 1];
		}
	}

	nrc_atcmd_param_to_str(argv[0], argv[0], strlen(argv[0]));

	for (event = ATCMD_EVENT_START ; event < ATCMD_EVENT_END ; event++)
	{
		if (strcmp(argv[0], "TCP_ERROR") == 0)
			strcpy(argv[0], "RECV_ERROR");

		if (strcmp(name[event], argv[0]) == 0)
		{
			if (g_atcmd_info.cb.event)
				g_atcmd_info.cb.event(event, argc - 1, argv + 1);

			break;
		}
	}

	return 0;
}

static int nrc_atcmd_recv_rxd (atcmd_rxd_t *rxd, char *msg)
{
	char *argv[4];
	int argc;
	int i;

	if (!rxd || !msg)
		return -1;

	nrc_atcmd_init_rxd(rxd);

	if (memcmp(msg, "+RXD:", 5) == 0)
		msg += 5;
	else if (memcmp(msg, "+HRXD:", 6) == 0)
		msg += 6;
	else
		return -1;

	for (argv[0] = msg, argc = 1 ; *msg != '\0' ; msg++)
	{
		if (argc > 4)
			return -1;

		if (*msg == ',')
		{
			*msg = '\0';
			argv[argc++] = msg + 1;
		}
	}

	if (argc != 2 && argc != 4)
		return -1;

	rxd->verbose = argc == 4 ? true : false;

	for (i = 0 ; i < argc ; i++)
	{
		switch (i)
		{
			case 0:
				rxd->id = atoi(argv[i]);
				if (rxd->id < 0)
					return -1;
				break;

			case 1:
				rxd->len = atoi(argv[i]);
				if (rxd->len < 0)
					return -1;
				break;

			case 2:
			{
				int ip_len = strlen(argv[i]);

				if (ip_len	< ATCMD_IPADDR_LEN_MIN || ip_len > ATCMD_IPADDR_LEN_MAX)
					return -1;

				strcpy(rxd->remote_addr, argv[i]);
				break;
			}

			case 3:
				rxd->remote_port = atoi(argv[i]);
				if (rxd->remote_port < 0)
					return -1;
		}
	}

/*	if (rxd->verbose)
		log_debug("rxd_msg_rxd: id=%d len=%d remote=%s,%d\n",
				rxd->id, rxd->len, rxd->remote_addr, rxd->remote_port);
	else
		log_debug("rxd_msg_info: id=%d len=%d\n", rxd->id, rxd->len); */

	return 0;
}

void nrc_atcmd_recv (char *buf, int len)
{
//#define CONFIG_RXD_PRINT

	enum ATCMD_MSG_TYPE
	{
		ATCMD_MSG_ERROR = -1,
		ATCMD_MSG_OK = 0,
		ATCMD_MSG_INFO,
		ATCMD_MSG_WEVENT,
		ATCMD_MSG_SEVENT,
		ATCMD_MSG_HEVENT,
		ATCMD_MSG_DATA,

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
		atcmd_rxd_t rxd;

		int cnt;
		char buf[ATCMD_DATA_LEN_MAX + 1];
	} data =
	{
		.rxd =
		{
			.verbose = false,
			.id = -1,
			.len = 0,
			.remote_addr = { 0, },
			.remote_port = 0
		},
		.cnt = 0,
		.buf = { 0, }
	};
	int i;

	for (i = 0 ; i < len ; i++)
	{
		if (data.rxd.len > 0)
		{
			data.buf[data.cnt] = buf[i];

			if (++data.cnt == data.rxd.len)
			{
#ifdef CONFIG_RXD_PRINT
				data.buf[data.cnt] = '\0';
				atcmd_log_recv("%s\n", data.buf);
#endif

				if (g_atcmd_info.cb.rxd)
					g_atcmd_info.cb.rxd(&data.rxd, data.buf);

				nrc_atcmd_init_rxd(&data.rxd);
				data.cnt = 0;
			}

			continue;
		}

		if (msg.cnt >= ATCMD_MSG_LEN_MAX)
		{
			if (msg.type != ATCMD_MSG_NONE)
				atcmd_error("message length > %d\n", ATCMD_MSG_LEN_MAX);

			msg.type = ATCMD_MSG_NONE;
			msg.cnt = 0;
		}

		msg.buf[msg.cnt++] = buf[i];

		if (msg.cnt == 1)
		{
			switch (msg.buf[0])
			{
				case '+':
					msg.type = ATCMD_MSG_INFO;
					break;

				case 'O':
					msg.type = ATCMD_MSG_OK;
					break;

				case 'E':
					msg.type = ATCMD_MSG_ERROR;
					break;

				default:
					msg.cnt = 0;
			}

			continue;
		}

		switch (msg.type)
		{
			case ATCMD_MSG_OK:
				if (msg.cnt > 4 || memcmp(msg.buf, "OK\r\n", msg.cnt) != 0)
				{
					msg.cnt = 0;
					msg.type = ATCMD_MSG_NONE;
					continue;
				}

				if (msg.cnt < 4)
					continue;

				break;

			case ATCMD_MSG_ERROR:
				if (msg.cnt > 7 || memcmp(msg.buf, "ERROR\r\n", msg.cnt) != 0)
				{
					msg.cnt = 0;
					msg.type = ATCMD_MSG_NONE;
					continue;
				}

				if (msg.cnt < 7)
					continue;

				break;

			case ATCMD_MSG_INFO:
				if (memcmp(msg.buf, "+RXD:", msg.cnt) == 0)
				{
					if (msg.cnt == 5)
						msg.type = ATCMD_MSG_DATA;
					continue;
				}
				else if (memcmp(msg.buf, "+SEVENT:", msg.cnt) == 0)
				{
					if (msg.cnt == 8)
						msg.type = ATCMD_MSG_SEVENT;
					continue;
				}

				if (memcmp(msg.buf, "+WEVENT:", msg.cnt) == 0)
				{
					if (msg.cnt == 8)
						msg.type = ATCMD_MSG_WEVENT;
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
					nrc_atcmd_recv_info(msg.buf, msg.cnt);
					break;

				case ATCMD_MSG_WEVENT:
				case ATCMD_MSG_SEVENT:
				case ATCMD_MSG_HEVENT:
					nrc_atcmd_recv_event(msg.buf, msg.cnt);
					break;

				case ATCMD_MSG_DATA:
					if (nrc_atcmd_recv_rxd(&data.rxd, msg.buf) != 0)
						atcmd_log_recv("!!! RXD FAIL !!!\n");
					break;

				default:
					atcmd_error("invalid message type (%d)\n", msg.type);
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

void nrc_atcmd_log_on (void)
{
	g_atcmd_info.log = true;
}

void nrc_atcmd_log_off (void)
{
	g_atcmd_info.log = false;
}

bool nrc_atcmd_log_is_on (void)
{
	return g_atcmd_info.log;
}

