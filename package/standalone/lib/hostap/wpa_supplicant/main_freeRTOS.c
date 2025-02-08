/*
 * WPA Supplicant / Example program entrypoint
 * Copyright (c) 2003-2005, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#include "includes.h"
#include "common.h"
#include "wpa_supplicant_i.h"
#include "FreeRTOS.h"
#include "system_common.h"
#include "lmac_common.h"
#include "driver_nrc.h"
#include "wpa_debug.h"
#include "ctrl_iface_freeRTOS.h"
#include "config.h"
#include "eap_peer/eap_i.h"

#define IFNAME_PREFIX "wlan"
struct wpa_global *global = NULL;

static inline void wpas_set_blob(struct wpa_supplicant *wpa_s, const char *name, const char *data)
{
	struct wpa_config_blob *blob = os_zalloc(sizeof(struct wpa_config_blob));
	blob->name = os_strdup(name);
	blob->data = (u8 *)os_strdup(data);
	blob->len = os_strlen(data) + 1;
	blob->next = wpa_s->conf->blobs;
	wpa_s->conf->blobs = blob;
}

char * wpas_config_get(int vif_id, const char *name)
{
	struct wpa_supplicant *wpa_s = wpas_iface_init(vif_id);
	if (!wpa_s) {
		return NULL;
	}

	struct wpa_ssid *ssid = wpa_config_get_network(wpa_s->conf, vif_id);
	/* Caution: some returned memory must be freed by you */
	return wpa_config_get(ssid, name);
}

int wpas_config_set(int vif_id, const char *name, char *value)
{
	struct wpa_supplicant *wpa_s = wpas_iface_init(vif_id);
	if (!wpa_s) {
		return -1;
	}

	struct wpa_ssid *ssid = wpa_config_get_network(wpa_s->conf, vif_id);
	return wpa_config_set(ssid, name, value, 0);
}

int wpas_config_global_get(int vif_id, const char *name, char *buf, size_t buflen)
{
	struct wpa_supplicant *wpa_s = wpas_iface_init(vif_id);
	if (!wpa_s) {
		return -1;
	}

	return wpa_config_get_value(name, wpa_s->conf, buf, buflen);
}

#define MAX_CONFIG_LINE_SIZE 512
int wpas_config_global_set(int vif_id, const char *name, char *value)
{
	struct wpa_supplicant *wpa_s = wpas_iface_init(vif_id);
	if (!wpa_s) {
		return -1;
	}

	char config_line[MAX_CONFIG_LINE_SIZE];
	os_snprintf(config_line, sizeof(config_line), "%s=%s", name, value);

	return wpa_config_process_global(wpa_s->conf, config_line, 0);
}

bool wpas_recovery(int vif_id, ctrl_iface_recovery_t *config, void (*set_network_id)(int vif_id, int net_id))
{
	struct wpa_supplicant *wpa_s = NULL;
	struct wpa_ssid *ssid = NULL;

	if (!config) {
		return false;
	}

	/* Get interface */
	wpa_s = wpas_iface_init(vif_id);
	if (!wpa_s) {
		return false;
	}

	/* Add network */
	ssid = wpa_supplicant_add_network(wpa_s);
	if (!ssid) {
		wpa_printf(MSG_ERROR, "Failed to add network(%d)", vif_id);
		return false;
	}

	/* Set network id */
	if (set_network_id) {
		set_network_id(vif_id, ssid->id);
	}

	/* Set country */
	wpa_s->conf->country[0] = config->country[0];
	wpa_s->conf->country[1] = config->country[1];
	wpa_s->conf->changed_parameters |= CFG_CHANGED_COUNTRY;
	wpa_supplicant_update_config(wpa_s);

	/* Set passive scan */
	if (config->scan_mode == 1) {
		wpa_s->conf->passive_scan = 1;
	}

	/* Set mode */
	ssid->mode = WPAS_MODE_INFRA;

	/* Set ssid */
	ssid->ssid_len = os_strlen((const char *)config->ssid);
	ssid->ssid = (u8 *) os_zalloc(ssid->ssid_len + 1);
	os_strlcpy((char *)ssid->ssid, (const char *)config->ssid, ssid->ssid_len + 1);
	ssid->scan_ssid = 1;

	/* Set bssid */
	memcpy(ssid->bssid, config->bssid, sizeof(ssid->bssid));

	/* Set frequency */
	ssid->frequency = config->nons1g_freq;

	/* Set security */
	if (config->security == WPA_KEY_MGMT_NONE) {
		ssid->key_mgmt = WPA_KEY_MGMT_NONE;
		ssid->ieee80211w = NO_MGMT_FRAME_PROTECTION;
	} else {
		switch(config->akm) {
		case 2:  // RSN_AUTH_KEY_MGMT_PSK_OVER_802_1X
			if (config->eap_type == 0) {
				ssid->key_mgmt = WPA_KEY_MGMT_PSK;
				ssid->ieee80211w = NO_MGMT_FRAME_PROTECTION;
				ssid->proto = WPA_PROTO_RSN;
				wpa_config_update_psk(ssid);
				/* WPA2-PSK passphrase can't be recovered due to lack of retention.
				 * ssid->passphrase = os_strdup((const char *)config->passphrase) */
			}
#if defined(IEEE8021X_EAPOL) && defined(EAPOL_8021X_RETENTION) // for future use
			else {
				ssid->key_mgmt = WPA_KEY_MGMT_IEEE8021X;
				ssid->eap.identity = (u8 *)os_strdup((const char *)config->identity);
				ssid->eap.password = (u8 *)os_strdup((const char *)config->password);
				ssid->eap.password_len = os_strlen((const char *)config->password);
				ssid->eap.flags &= ~EAP_CONFIG_FLAGS_PASSWORD_NTHASH;
				ssid->eap.flags &= ~EAP_CONFIG_FLAGS_EXT_PASSWORD;
			}
			switch (config->eap_type) {
				case 1: // WIFI_EAP_TLS
					ssid->eap.eap_methods = os_zalloc(sizeof(struct eap_method_type));
					ssid->eap.eap_methods->method = EAP_TYPE_TLS;
					ssid->eap.eap_methods->vendor = EAP_VENDOR_IETF;
					ssid->eap.cert.ca_cert = os_strdup("\"blob://ca_cert\"");
					ssid->eap.cert.client_cert = os_strdup("\"blob://client_cert\"");
					ssid->eap.cert.private_key = os_strdup("\"blob://private_key\"");
					ssid->eap.cert.private_key_passwd = os_strdup((const char *)config->private_key_password);
					wpas_set_blob(wpa_s, "ca_cert", config->eap_ca_cert);
					wpas_set_blob(wpa_s, "client_cert", config->eap_client_cert);
					wpas_set_blob(wpa_s, "private_key", config->eap_private_key);
					break;
				case 3: //WIFI_EAP_PEAP
					ssid->eap.eap_methods = os_zalloc(sizeof(struct eap_method_type));
					ssid->eap.eap_methods->method = EAP_TYPE_PEAP;
					ssid->eap.eap_methods->vendor = EAP_VENDOR_IETF;
					ssid->eap.phase1 = os_strdup("\"peapver=0\"");
					ssid->eap.phase2 = os_strdup("\"MSCHAPV2\"");
					break;
				case 2: // WIFI_EAP_TTLS
					ssid->eap.eap_methods = os_zalloc(sizeof(struct eap_method_type));
					ssid->eap.eap_methods->method = EAP_TYPE_TTLS;
					ssid->eap.eap_methods->vendor = EAP_VENDOR_IETF;
					ssid->eap.phase2 = os_strdup("\"MSCHAPV2\"");
					break;
				default:
					goto recovery_fail;
			}
#else
			else {
				goto recovery_fail;
			}
#endif /* IEEE8021X_EAPOL && EAPOL_8021X_RETENTION */
			break;
#if defined(CONFIG_SAE)
		case 8:  // RSN_AUTH_KEY_MGMT_SAE
			ssid->key_mgmt = WPA_KEY_MGMT_SAE;
			ssid->ieee80211w = MGMT_FRAME_PROTECTION_REQUIRED;
			ssid->proto = WPA_PROTO_RSN;
			ssid->sae_password = os_strdup((const char *)config->password);
			if (config->sae_pwe > 0) {
				/* SAE: Special test mode (Use hunting-and-pecking loop)
				 * hash-to-element makes PT in recovery process but takes
				 * long time. Use test mode in recovery process and recover
				 * sae_pwe option in port authorization. (wpa_ps_hook_handle_port)
				 * https://w1.fi/cgit/hostap/commit/?id=641d79f165750f3c6af5f9565ff317f14fffa114
				 */
				wpa_s->conf->sae_pwe = 3;
			}
			break;
#endif
#if defined(CONFIG_OWE)
		case 18: // RSN_AUTH_KEY_MGMT_OWE
			ssid->key_mgmt = WPA_KEY_MGMT_OWE;
			ssid->ieee80211w = MGMT_FRAME_PROTECTION_REQUIRED;
			ssid->proto = WPA_PROTO_RSN;
			break;
#endif
		default:
			goto recovery_fail;
		}
	}

	/* Enable network */
	wpa_supplicant_enable_network(wpa_s, ssid);

	return true;

recovery_fail:
	os_free(ssid->ssid);

	/* Remove network */
	wpa_supplicant_remove_network(wpa_s, ssid->id);

	return false;
}

struct wpa_supplicant * wpas_iface_init(int vif_id)
{
	char ifname[6];
	struct wpa_supplicant *wpa_s = NULL;
	struct wpa_interface iface;

	if (!global) {
		global = wpas_global_init();
		if (!global) {
			return NULL;
		}
	}

	snprintf(ifname, sizeof(ifname), IFNAME_PREFIX"%d", vif_id);
	wpa_s = wpa_supplicant_get_iface(global, ifname);
	if (wpa_s) {
		return wpa_s;
	}

	memset(&iface, 0, sizeof(struct wpa_interface));
	iface.confname = iface.ifname = ifname;
	wpa_s = wpa_supplicant_add_iface(global, &iface, NULL);
	if (!wpa_s) {
		wpa_printf(MSG_ERROR, "Failed to add iface(%d)", vif_id);
		return NULL;
	}

	return wpa_s;
}

int wpas_iface_deinit(int vif_id)
{
	char ifname[6];
	struct wpa_supplicant *wpa_s = NULL;

	if (!global) {
		return -1;
	}

	snprintf(ifname, sizeof(ifname), IFNAME_PREFIX"%d", vif_id);
	wpa_s = wpa_supplicant_get_iface(global, ifname);
	if (!wpa_s) {
		return -1;
	}

	return wpa_supplicant_remove_iface(global, wpa_s, 0);
}

struct wpa_global * wpas_global_init(void)
{
	struct wpa_params params;

	if (global) {
		return global;
	}

	memset(&params, 0, sizeof(params));
	params.wpa_debug_level = wpa_debug_level;

	global = wpa_supplicant_init(&params);

	if (!global) {
		wpa_printf(MSG_ERROR, "Failed to init wpa_supplicant");
		return NULL;
	}

	return global;
}

void wpas_task_main(void *pvParams)
{
	global = wpas_global_init();
	if (!global) {
		return;
	}

	wpas_iface_init(0);

#if !defined(NRC7291_SDK_DUAL_CM3) && !defined(NRC7292_SDK_DUAL_CM3)
	lmac_start();
#endif

	wpa_supplicant_run(global);
	wpa_supplicant_deinit(global);
}
