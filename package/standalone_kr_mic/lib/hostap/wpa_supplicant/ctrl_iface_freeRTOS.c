#include "utils/includes.h"
#include "utils/common.h"
#include "config.h"
#include "wpa_supplicant_i.h"
#include "ctrl_iface.h"
#include "system_common.h"
#include "driver_nrc.h"

#ifdef CONFIG_NO_STDOUT_DEBUG
#ifdef wpa_printf
#undef wpa_printf
static void wpa_printf(int level, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	system_vprintf(fmt, ap);
	system_printf("\n");
}
#endif
#endif

static void processCommand(char* cmd);
int ctrl_iface_receive(struct wpa_supplicant *wpa_s, char *cmd);

struct ctrl_iface_priv {
	struct wpa_supplicant *wpa_s;
};

struct ctrl_iface_global_priv {
	struct ctrl_iface_priv 	*ctrl_if[NRC_WPA_NUM_INTERFACES];
};


static struct ctrl_iface_global_priv* global_ctrl_if = NULL;

//struct wpa_supplicant *ctrl_if_wpa_s = NULL;

int wpa_cmd_receive(int vif_id, int argc, char *argv[]) {
	int i = 0;
	char buf[512] = {0,};

	if (argc <= 1)
		return -1;

	sprintf(buf, "%s", argv[1]);
	for (i = 2; i < argc; i++) {
		sprintf(buf, "%s %s", buf, argv[i]);
	}

	if (!global_ctrl_if || !global_ctrl_if->ctrl_if[vif_id])
		return -1;

	return ctrl_iface_receive(global_ctrl_if->ctrl_if[vif_id]->wpa_s, buf);
}

size_t ctrl_iface_receive_response(char *cmd, char *ret)
{
	int i = 0;
	char *reply = NULL, *p = cmd;
	size_t reply_len = 0;

	wpa_printf(MSG_DEBUG, "[%s] cmd: %s", __func__, cmd);

	while(*p != ' ' && *p != 0) {
		*p = (char) toupper((int) *p);
		p++;
	}

	reply = wpa_supplicant_ctrl_iface_process(
		global_ctrl_if->ctrl_if[0]->wpa_s, cmd, &reply_len);

	reply[reply_len] = 0;
	wpa_printf(MSG_ERROR, "reply:%s reply_len: %d \n", reply, reply_len);

	memcpy(ret, reply, reply_len - 1);
	os_free(reply);

	return reply_len;
}

int ctrl_iface_receive(struct wpa_supplicant *wpa_s, char *cmd)
{
	int i = 0;
	char *reply = NULL, *p = cmd;
	size_t reply_len = 0;

	wpa_printf(MSG_DEBUG, "%s() cmd: %s", __func__, cmd);

	if (!wpa_s)
		return 0;

	while(*p != ' ' && *p != 0 ) {
		*p = (char) toupper((int) *p);
		p++;
	}

	reply = wpa_supplicant_ctrl_iface_process(wpa_s, cmd, &reply_len);

	reply[reply_len] = 0;
	wpa_printf(MSG_ERROR, "reply_len: %d ", (int)reply_len);

	if(reply_len == 1) {
		wpa_printf(MSG_DEBUG, "FAIL");
	} else if(reply_len == 2) {
		wpa_printf(MSG_DEBUG, "OK");
		os_free(reply);
		return 0;
	} else if(reply) {
	    if (reply_len > PRINT_BUFFER_SIZE ) {
    		int pos = 0;
	        char atom[PRINT_BUFFER_SIZE+1];
    	    while (pos < reply_len) {
        		int len = reply_len - pos;
	            if (len > PRINT_BUFFER_SIZE)
    	        	len = PRINT_BUFFER_SIZE;
        	    memcpy(atom, reply + pos, len);
	    	    atom[len] = '\0';
    	        wpa_printf(MSG_DEBUG, atom);
        	    pos += len;
	        }
    	} else {
        	reply[reply_len] = 0;
            wpa_printf(MSG_DEBUG, reply);
        }
	} else {
		wpa_printf(MSG_DEBUG, "UNKNOWN");
	}

    if(reply != NULL ) {
    	os_free(reply);
    }

	return 0;
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
