#ifndef CTRL_IFACE_FREERTOS_H
#define CTRL_IFACE_FREERTOS_H

typedef struct
{
	int len;
	char *msg;
} ctrl_iface_resp_t;

typedef struct
{
	uint8_t security;
	uint8_t akm;
	uint8_t eap_type;
	uint8_t scan_mode;
	uint8_t sae_pwe;
	uint16_t channel;
	uint16_t nons1g_freq;
	uint32_t ip_addr;
	uint32_t net_mask:31;
	uint32_t ip_mode:1;
	uint32_t gw_addr;
	const char *country;
	const char *ssid;
	const char *bssid;
	const char *password;
	const char *pmk;
#if defined(EAPOL_8021X_RETENTION)
	const char *identity;
	const char *private_key_password;
	const char *eap_ca_cert;
	const char *eap_client_cert;
	const char *eap_private_key;
#endif
} ctrl_iface_recovery_t;

int ctrl_iface_receive(int vif_id, char *cmd);
int wpa_cmd_receive(int vif_id, int argc, char *argv[]);
struct wpa_global * wpas_global_init(void);
struct wpa_supplicant * wpas_iface_init(int vif_id);
int wpas_iface_deinit(int vif_id);
bool wpas_recovery(int vif_id, ctrl_iface_recovery_t *config, void (*set_network_id)(int vif_id, int net_id));
char * wpas_config_get(int vif_id, const char *name);
int wpas_config_set(int vif_id, const char *name, char *value);
int wpas_config_global_get(int vif_id, const char *name, char *buf, size_t buflen);
int wpas_config_global_set(int vif_id, const char *name, char *value);
void nrc_ps_get_rconf(ctrl_iface_recovery_t *config);

ctrl_iface_resp_t *ctrl_iface_receive_response(int vif_id, const char *fmt, ...);
bool CTRL_IFACE_RESP_OK (ctrl_iface_resp_t *resp);
bool CTRL_IFACE_RESP_MSG (ctrl_iface_resp_t *resp);
bool CTRL_IFACE_RESP_ERR (ctrl_iface_resp_t *resp);
void CTRL_IFACE_RESP_FREE (ctrl_iface_resp_t *resp);

#endif /* CTRL_IFACE_FREERTOS_H */
