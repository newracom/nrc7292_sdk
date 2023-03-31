/*
 * Copyright (c) 2016-2019 Newracom, Inc.
 *
 * Command-line interface for Wi-Fi API test
 *
 */

#ifndef __UTIL_CMD_API_WIFI_TEST_H__
#define __UTIL_CMD_API_WIFI_TEST_H__

typedef struct wifi_test_cmd {
	const char *cmd;
	const char *usage;
} wifi_test_cmd_tbl_t;

typedef enum {
	WIFI_CMD_INFO =0,
	WIFI_CMD_CONNECT,
	WIFI_CMD_DISCONNECT,
	WIFI_CMD_SET_IP_ADDRESS,
	WIFI_CMD_SET_DNS,
	WIFI_CMD_SET_MTU = 5,
	WIFI_CMD_GET_MTU,
	WIFI_CMD_GET_IP_MODE,
	WIFI_CMD_GET_RSSI,
	WIFI_CMD_GET_SNR,
	WIFI_CMD_GET_CHANNEL_BANDWIDTH = 10,
	WIFI_CMD_SCAN,
	WIFI_CMD_ABORT_SCAN,
	WIFI_CMD_SCAN_RESULTS,
	WIFI_CMD_SCAN_FREQUENCY,
	WIFI_CMD_SOFTAP_SET_CONF = 15,
	WIFI_CMD_SOFTAP_SET_IP,
	WIFI_CMD_SOFTAP_START,
	WIFI_CMD_SOFTAP_STOP,
	WIFI_CMD_DHCP_SERVER_START,
	WIFI_CMD_DHCP_SERVER_STOP = 20,
	WIFI_CMD_DISASSOCIATION,
	WIFI_CMD_SET_TX_POWER ,
	WIFI_CMD_GET_TX_POWER,
	WIFI_CMD_SET_COUNTRY,
	WIFI_CMD_GET_COUNTRY = 25,
	WIFI_CMD_GET_CHANNEL_FREQUENCY,
	WIFI_CMD_GET_MAC_ADDRESS,
	WIFI_CMD_GET_IP_ADDRESS,
	WIFI_CMD_GET_BSSID,
	WIFI_CMD_GET_STATE = 30,
	WIFI_CMD_REGISTER_CALLBACK,
	WIFI_CMD_SET_SSID,
	WIFI_CMD_SET_SECURITY,
	WIFI_CMD_SET_BSSID,
	WIFI_CMD_ADD_NETWORK = 35,
	WIFI_CMD_REMOVE_NETWORK,
	WIFI_CMD_MAX,
} tWIFI_TEST_CMD;

#define CMD_WIFI_GET(cmd, type)	{ type value; ret = nrc_wifi_##cmd(&value); if (ret == 0) system_printf("%s:%d\n", #cmd, value);}
#define CMD_WIFI_GET2(vif_id, cmd, type)	{ type value; ret = nrc_wifi_##cmd(vif_id, &value); if (ret == 0) system_printf("%s:%d [vif_id:%d]\n", #cmd, value, vif_id);}
#define CMD_WIFI_GET_ADDR(cmd, netif)	{ char *value; ret = nrc_wifi_##cmd(netif, &value); if (ret == 0) system_printf("%s:%s\n", #cmd, value);}
#define CMD_WIFI_GET_ADDR2(cmd,mode)	{ char *value; ret = nrc_wifi_##cmd(mode,&value); if (ret == 0) system_printf("%s(mode:%d):%s\n", #cmd, mode, value);}

int cmd_wifi_api_test(int argc, char *argv[]);

#endif /* __UTIL_CMD_API_WIFI_TEST_H__ */
