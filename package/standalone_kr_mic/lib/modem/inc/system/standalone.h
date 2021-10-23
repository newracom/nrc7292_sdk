#ifndef STANDALONE_H
#define STANDALONE_H


int standalone_main();
void get_standalone_macaddr(int vif_id, uint8_t *mac);
void set_standalone_ipaddr(int vif_id, uint32_t ipaddr, uint32_t netmask, uint32_t gwaddr);
int set_standalone_hook_dhcp(int vif_id);

#endif // STANDALONE_H
