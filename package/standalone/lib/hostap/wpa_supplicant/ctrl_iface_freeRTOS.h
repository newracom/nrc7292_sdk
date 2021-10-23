#ifndef CTRL_IFACE_FREERTOS_H
#define CTRL_IFACE_FREERTOS_H

int wpa_cmd_receive(int vif_id, int argc, char *argv[]);
size_t ctrl_iface_receive_response(int vif_id, char *cmd, char *ret);
int ctrl_iface_receive(int vif_id, char *cmd);

#endif /* CTRL_IFACE_FREERTOS_H */
