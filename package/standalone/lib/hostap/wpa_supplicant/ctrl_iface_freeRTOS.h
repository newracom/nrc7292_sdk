#ifndef CTRL_IFACE_FREERTOS_H
#define CTRL_IFACE_FREERTOS_H

typedef struct
{
	int len;
	char *msg;
} ctrl_iface_resp_t;

int ctrl_iface_receive(int vif_id, char *cmd);
int wpa_cmd_receive(int vif_id, int argc, char *argv[]);

ctrl_iface_resp_t *ctrl_iface_receive_response(int vif_id, const char *fmt, ...);
bool CTRL_IFACE_RESP_OK (ctrl_iface_resp_t *resp);
bool CTRL_IFACE_RESP_MSG (ctrl_iface_resp_t *resp);
bool CTRL_IFACE_RESP_ERR (ctrl_iface_resp_t *resp);
void CTRL_IFACE_RESP_FREE (ctrl_iface_resp_t *resp);

#endif /* CTRL_IFACE_FREERTOS_H */
