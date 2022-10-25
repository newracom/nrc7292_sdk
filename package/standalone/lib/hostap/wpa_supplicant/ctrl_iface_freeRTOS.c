#include "utils/includes.h"
#include "utils/common.h"
#include "config.h"
#include "wpa_supplicant_i.h"
#include "ctrl_iface.h"
#include "system_common.h"
#include "driver_nrc.h"

#define PRINT_BUFFER_SIZE 1024

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

int ctrl_iface_receive(int vif_id, char *cmd);

struct ctrl_iface_priv {
	struct wpa_supplicant *wpa_s;
};

struct ctrl_iface_global_priv {
	struct ctrl_iface_priv 	*ctrl_if[NRC_WPA_NUM_INTERFACES];
};


static struct ctrl_iface_global_priv* global_ctrl_if = NULL;

//struct wpa_supplicant *ctrl_if_wpa_s = NULL;

static struct wpa_supplicant *wpa_get_wpa_from_vif(int vif_id)
{

	if (vif_id < 0 || vif_id >= NRC_WPA_NUM_INTERFACES ||
	    !global_ctrl_if || !global_ctrl_if->ctrl_if[vif_id]) {
		return NULL;
	}
	return global_ctrl_if->ctrl_if[vif_id]->wpa_s;
}

int wpa_cmd_receive(int vif_id, int argc, char *argv[]) {
	int i = 0;
	char buf[512] = {0,};

	if (argc <= 1)
		return -1;

	sprintf(buf, "%s", argv[1]);
	for (i = 2; i < argc; i++) {
		sprintf(buf, "%s %s", buf, argv[i]);
	}

	return ctrl_iface_receive(vif_id, buf);
}

size_t ctrl_iface_receive_response(int vif_id, char *cmd, char *ret)
{
	char *reply = NULL, *p = cmd;
	size_t reply_len = 0;
	struct wpa_supplicant *wpa_s;

	wpa_printf(MSG_INFO, "[%s] cmd: %s", __func__, cmd);

	wpa_s = wpa_get_wpa_from_vif(vif_id);
	if (!wpa_s)
		return 0;

	while(*p != ' ' && *p != 0) {
		*p = (char) toupper((int) *p);
		p++;
	}

	reply = wpa_supplicant_ctrl_iface_process(
		wpa_s, cmd, &reply_len);
	if (reply_len <= 0) {
		os_free(reply);
		return 0;
	}

	if (reply[reply_len - 1] == '\n')
		reply[--reply_len] = '\0'; /* remove new line */
	else
		reply[reply_len] = '\0';

	wpa_printf(MSG_INFO, "reply_len: %d\nreply: %s\n", reply_len, reply);

	memcpy(ret, reply, reply_len);
	ret[reply_len] = '\0';

	os_free(reply);

	return reply_len;
}

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

				if (wpa_debug_level <= MSG_DEBUG) {
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
		}

		if (reply)
			os_free(reply);
	}

	return ret;
}

struct ctrl_iface_priv* wpa_supplicant_ctrl_iface_init(struct wpa_supplicant *wpa_s)
{
	struct ctrl_iface_priv* priv = NULL;
	int vif_id = 0;

	if (!global_ctrl_if) {
		wpa_printf(MSG_ERROR, "Failed to init ctrl_iface.");
		return NULL;
	}

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

void wpa_supplicant_ctrl_iface_deinit(struct ctrl_iface_priv *priv)
{
	os_free(priv);
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
