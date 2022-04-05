
#include <stdint.h>

typedef enum
{
	false = 0,
	true
} bool;

extern void atcmd_socket_recv_data (char *buf, int len);

void wifi_lwip_init( void )
{

}

bool wifi_ifconfig(int num_param, char *params[])
{
	return false;
}

int setup_wifi_ap_mode(int vif, int updated_lease_time)
{
	return -1;
}

int ping_run(char *cmd)
{
	return -1;
}

int iperf_run(char *cmd, void *report_cb)
{
	return -1;
}

int dhcp_run(int vif)
{
	return -1;
}

int dhcps_status(void)
{
	return -1;
}

void reset_ip_address(int vif)
{

}

void lwif_input(uint8_t vif_id, void *buffer, int data_len, bool is_ap)
{
	atcmd_socket_recv_data((char *)buffer, data_len);
}

