#include "utils/includes.h"
#include "utils/common.h"
#include "config.h"
#include "wpa_supplicant_i.h"
#include "ctrl_iface.h"
#include "ctrl_iface_freeRTOS.h"
#include "system_common.h"
#include "driver_nrc.h"
#include "eloop_freeRTOS.h"


#ifdef CONFIG_NO_STDOUT_DEBUG
#ifdef wpa_printf
#undef wpa_printf
extern int wpa_debug_level;
static void wpa_printf(int level, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	system_vprintf(fmt, ap);
	system_printf("\n");
	va_end(ap);
}
#endif
#endif

/***********************************************************************************************************/

struct ctrl_iface_priv {
	struct wpa_supplicant *wpa_s;
};

struct ctrl_iface_global_priv {
	struct ctrl_iface_priv 	*ctrl_if[NRC_WPA_NUM_INTERFACES];
};

static struct ctrl_iface_global_priv* global_ctrl_if = NULL;


static struct wpa_supplicant *wpa_get_wpa_from_vif(int vif_id)
{

	if (vif_id < 0 || vif_id >= NRC_WPA_NUM_INTERFACES ||
	    !global_ctrl_if || !global_ctrl_if->ctrl_if[vif_id]) {
		return NULL;
	}
	return global_ctrl_if->ctrl_if[vif_id]->wpa_s;
}

/***********************************************************************************************************/

static SemaphoreHandle_t g_ctrl_if_lock = NULL;

static int _ctrl_iface_lock_init (void)
{
	if (!g_ctrl_if_lock) {
		g_ctrl_if_lock = xSemaphoreCreateMutex();
		if (!g_ctrl_if_lock) {
			wpa_printf(MSG_ERROR, "WPA: %s() failed\n", __func__);
			return -1;
		}
	}

	return 0;
}

static void _ctrl_iface_lock_deinit (void)
{
	if (g_ctrl_if_lock) {
		vSemaphoreDelete(g_ctrl_if_lock);
		g_ctrl_if_lock = NULL;
	}
}

static bool _ctrl_iface_lock (void)
{
	const int timeout_msec = 60 * 1000;

	if (!g_ctrl_if_lock) {
		wpa_printf(MSG_ERROR, "WPA: %s(), null\n", __func__);
		return false;
	}

	if (!xSemaphoreTake(g_ctrl_if_lock, pdMS_TO_TICKS(timeout_msec)))	{
		wpa_printf(MSG_ERROR, "WPA: %s(), timeout, %d-msec\n", __func__, timeout_msec);
		return false;
	}

	return true;
}

static bool _ctrl_iface_unlock (void)
{
	if (!g_ctrl_if_lock) {
		wpa_printf(MSG_ERROR, "WPA: %s(), null\n", __func__);
		return false;
	}

	if (!xSemaphoreGive(g_ctrl_if_lock)) {
		wpa_printf(MSG_ERROR, "WPA: %s(), no space\n", __func__);
		return false;
	}

	return true;
}

/***********************************************************************************************************/

int ctrl_iface_receive(int vif_id, char *cmd)
{
	struct wpa_supplicant *wpa_s = wpa_get_wpa_from_vif(vif_id);
	int ret = -1;

	if (wpa_s) {
		char *reply;
		size_t reply_len;
		int i;

		for (i = 0 ; cmd[i] != ' ' && cmd[i] != '\0' ; i++)
			cmd[i] = toupper(cmd[i]);

		reply = wpa_supplicant_ctrl_iface_process(wpa_s, cmd, &reply_len);

		if (!reply || reply_len <= 0)
			wpa_printf(MSG_DEBUG, "WPA: Control interface response 'INVALID'");
		else {
			if (reply[reply_len - 1] == '\n')
				reply[--reply_len] = '\0'; /* remove new line */
			else
				reply[reply_len] = '\0';

			if (strcmp(reply, "OK") == 0 || (strcmp(reply, "FAIL") != 0 && strcmp(reply, "UNKNOWN COMMAND") != 0))
				ret = 0;

			if (reply_len <= 30)
				wpa_printf(MSG_DEBUG, "WPA: Control interface response '%s' (%d)", reply, reply_len);
			else {
				wpa_printf(MSG_DEBUG, "WPA: Control interface response (%d)", reply_len);

				char temp;

				for (i = 0 ; i < reply_len ; i += PRINT_BUFFER_SIZE) {
					if ((reply_len - i) < PRINT_BUFFER_SIZE)
						wpa_printf(MSG_DEBUG, "%s\n", reply + i);
					else {
						temp = reply[i + PRINT_BUFFER_SIZE];
						reply[i + PRINT_BUFFER_SIZE] = '\0';
						wpa_printf(MSG_DEBUG, "%s", reply + i);
						reply[i + PRINT_BUFFER_SIZE] = temp;
					}
				}
			}
		}

		if (reply)
			os_free(reply);
	}

	return ret;
}

int wpa_cmd_receive(int vif_id, int argc, char *argv[])
{
	int i = 0;

	if (argc <= 1)
	{
		return -1;
	}

	eloop_msg_t* eloop_msg = os_zalloc(ELOOP_MESSAGE_SIZE);
	char *buf = os_zalloc(ELOOP_MESSAGE_BUF_SIZE);

	if ((eloop_msg == NULL) || (buf == NULL))
	{
		return -2;
	}

	eloop_msg->vif_id = vif_id;
	eloop_msg->wait_rsp = 0;
	eloop_msg->buf = buf;
	sprintf(buf, "%s", argv[1]);

	//If there are additional strings separated by spaces,
	//input them starting from 2.
	for (i = 2; i < argc; i++) {
		sprintf(buf + strlen(buf), " %s", argv[i]);
	}
	xQueueSend(eloop_message_queue_req, &eloop_msg, 0);
	eloop_run_signal();

	return 0;
}

/***********************************************************************************************************/

ctrl_iface_resp_t *ctrl_iface_receive_response(int vif_id, const char *fmt, ...)
{
	struct wpa_supplicant *wpa_s = wpa_get_wpa_from_vif(vif_id);

	if (wpa_s)
	{
		va_list ap;
		va_start(ap, fmt);

		va_list ap_copy;
		va_copy(ap_copy, ap);
		int cmd_len = os_strlen(fmt) + os_strlen(va_arg(ap_copy, char*)); /* for certificate length(256 and above) */
		va_end(ap_copy);

		cmd_len = cmd_len > 256 ? cmd_len : 256;
		char cmd[cmd_len];
		ASSERT(_ctrl_iface_lock());

		if (vsnprintf(cmd, sizeof(cmd), fmt, ap) > 0)
		{
			size_t reply_len;
			char *reply;
			int i;

#if defined(INCLUDE_TRACE_WAKEUP)
#if !defined(INCLUDE_MEASURE_AIRTIME)
			wpa_printf(MSG_EXCESSIVE, "[%s] cmd: %s", __func__, cmd);
#endif /* !defined(INCLUDE_MEASURE_AIRTIME) */
#endif /* INCLUDE_TRACE_WAKEUP */

			for (i = 0 ; cmd[i] != ' ' && cmd[i] != '\0' ; i++)
				cmd[i] = toupper(cmd[i]);

			reply = wpa_supplicant_ctrl_iface_process(wpa_s, cmd, &reply_len);
			//wpa_printf(MSG_EXCESSIVE, "[%s] rep:0x%x len:%d", __func__, reply, reply_len);
			if (reply && reply_len > 0)
			{
				if (reply[reply_len - 1] == '\n')
					reply[--reply_len] = '\0'; /* remove new line */
				else
					reply[reply_len] = '\0';

#if defined(INCLUDE_TRACE_WAKEUP)
#if !defined(INCLUDE_MEASURE_AIRTIME)
				wpa_printf(MSG_EXCESSIVE, "reply_len: %d\n", reply_len);

				if (reply_len <= PRINT_BUFFER_SIZE){
					wpa_printf(MSG_EXCESSIVE, "reply: %s\n", reply);
				} else {
					char temp;

					wpa_printf(MSG_EXCESSIVE, "reply: ");

					for (i = 0 ; i < reply_len ; i += PRINT_BUFFER_SIZE) {
						if ((reply_len - i) < PRINT_BUFFER_SIZE)
							wpa_printf(MSG_INFO, "%s\n", reply + i);
						else {
							temp = reply[i + PRINT_BUFFER_SIZE];
							reply[i + PRINT_BUFFER_SIZE] = '\0';
							wpa_printf(MSG_INFO, "%s", reply + i);
							reply[i + PRINT_BUFFER_SIZE] = temp;
						}
					}
				}
#endif /* !defined(INCLUDE_MEASURE_AIRTIME) */
#endif /* INCLUDE_TRACE_WAKEUP */

				if (memcmp(reply, "FAIL", 4) != 0 && strcmp(reply, "UNKNOWN COMMAND") != 0)
				{
					static ctrl_iface_resp_t resp;

					if (strcmp(reply, "OK") == 0) {
						os_free(reply);
						reply_len = 0;
						reply = NULL;
					}

					resp.len = reply_len;
					resp.msg = reply;

					va_end(ap);
					ASSERT(_ctrl_iface_unlock());

					return &resp;
				}
			}

			if (reply) {
				os_free(reply);
			}
		}

		va_end(ap);
		ASSERT(_ctrl_iface_unlock());
	}

	return NULL;
}

bool CTRL_IFACE_RESP_OK (ctrl_iface_resp_t *resp)
{
	if (resp && resp->len == 0)
		return true;

	CTRL_IFACE_RESP_FREE(resp);
	return false;
}

bool CTRL_IFACE_RESP_MSG (ctrl_iface_resp_t *resp)
{
	if (resp && resp->len > 0 && resp->msg)
		return true;

	CTRL_IFACE_RESP_FREE(resp);
	return false;
}

bool CTRL_IFACE_RESP_ERR (ctrl_iface_resp_t *resp)
{
	if (!resp || resp->len < 0)
		return true;

	CTRL_IFACE_RESP_FREE(resp);
	return false;
}

void CTRL_IFACE_RESP_FREE (ctrl_iface_resp_t *resp)
{
	if (resp) {
		if (resp->len > 0 && resp->len < 4096 && resp->msg) {
			//wpa_printf(MSG_ERROR, "[1] free msg (len:%d msg:0x%x)", resp->len, resp->msg);
			os_free(resp->msg);
		}
		//wpa_printf(MSG_ERROR, "[2] free resp (resp:0x%x)\n", resp);
		os_free(resp);
	}
}

/***********************************************************************************************************/

struct ctrl_iface_priv* wpa_supplicant_ctrl_iface_init(struct wpa_supplicant *wpa_s)
{
	struct ctrl_iface_priv* priv = NULL;
	int vif_id = 0;

	if (!global_ctrl_if) {
		wpa_printf(MSG_ERROR, "Failed to init ctrl_iface.");
		return NULL;
	}

	if (_ctrl_iface_lock_init() != 0)
		return NULL;

	if (os_strcmp(NRC_WPA_INTERFACE_NAME_0, wpa_s->ifname) == 0)
		vif_id = 0;
	else if (os_strcmp(NRC_WPA_INTERFACE_NAME_1, wpa_s->ifname) == 0)
		vif_id = 1;
	else {
		wpa_printf(MSG_ERROR, "%s: Unknown interface name (%s)", __func__,
			wpa_s->ifname);
	}

	if(wpa_s->conf->ctrl_interface) {
		os_free(wpa_s->conf->ctrl_interface);
		wpa_s->conf->ctrl_interface = NULL;
	}

	priv = (struct ctrl_iface_priv *) os_zalloc(sizeof(*priv));

	if (!priv) {
		wpa_printf(MSG_ERROR, "Failed to allocate ctrl_iface");
		return NULL;
	}
	priv->wpa_s = wpa_s;
	global_ctrl_if->ctrl_if[vif_id] = priv;
	wpa_s->conf->ctrl_interface = (char *) priv;

	return priv;
}

void wpa_supplicant_ctrl_iface_deinit(struct wpa_supplicant *wpa_s,
														struct ctrl_iface_priv *priv)
{
	os_free(priv);

	_ctrl_iface_lock_deinit();
}

void wpa_supplicant_ctrl_iface_wait(struct ctrl_iface_priv *priv)
{
}

struct ctrl_iface_global_priv* wpa_supplicant_global_ctrl_iface_init(struct wpa_global *global)
{
	if (global_ctrl_if) {
		os_free(global_ctrl_if);
		global_ctrl_if = NULL;
	}

	global_ctrl_if = (struct ctrl_iface_global_priv *) os_zalloc(sizeof(*global_ctrl_if));

	if(!global_ctrl_if) {
		wpa_printf(MSG_ERROR, "Failed to allocate ctrl_iface global");
		return NULL;
	}

	if(global->params.ctrl_interface) {
		os_free(global->params.ctrl_interface);
		global->params.ctrl_interface = NULL;
	}
	global->params.ctrl_interface = (char *) global_ctrl_if;

	return global_ctrl_if;
}

void wpa_supplicant_global_ctrl_iface_deinit(struct ctrl_iface_global_priv *priv)
{
	os_free(priv);
}
