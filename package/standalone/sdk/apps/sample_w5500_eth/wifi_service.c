#include "nrc_sdk.h"

#include "wifi_config_setup.h"
#include "wifi_connect_common.h"
#include "wifi_config.h"
#include "nrc_lwip.h"
#include "nrc_eth_if.h"
#include "lwip/netif.h"

extern struct netif* nrc_netif[MAX_IF];

nrc_err_t start_softap(WIFI_CONFIG* param)
{
	/* set initial wifi configuration */
	while(1) {
		if (wifi_init(param) == WIFI_SUCCESS) {
			nrc_usr_print ("\033[31m [%s] wifi_init Success !! \033[39m\n", __func__);
			break;
		} else {
			nrc_usr_print ("\033[31m [%s] wifi_init Failed !! \033[39m\n", __func__);
			_delay_ms(1000);
		}
	}
	nrc_usr_print ("\033[31m [%s] calling wifi_start_softap \033[39m\n", __func__);
	if (wifi_start_softap(param) != WIFI_SUCCESS) {
		nrc_usr_print ("[%s] ASSERT! Fail to start softap\n", __func__);
		return NRC_FAIL;
	}

	return NRC_SUCCESS;
}

nrc_err_t connect_to_ap(WIFI_CONFIG* param)
{
	int i = 0;
	int count = 0;
	int dhcp_server = 0;

	count = 10;
	dhcp_server = param->dhcp_server;

	if (wifi_init(param)!= WIFI_SUCCESS) {
		nrc_usr_print ("[%s] ASSERT! Fail for init\n", __func__);
		return NRC_FAIL;
	}

	netif_set_default(nrc_netif[0]);

	/* Try to connect */
	if (wifi_connect(param)!= WIFI_SUCCESS) {
		nrc_usr_print ("[%s] Fail for Wi-Fi connection (results:%d)\n", __func__);
		return NRC_FAIL;
	}

	if (param->network_mode == WIFI_NETWORK_MODE_BRIDGE) {
		/* ignore wifi state for bridge mode */
		return WIFI_SUCCESS;
	}

	/* check if IP is ready */
	while(1){
		if (nrc_addr_get_state(0) == NET_ADDR_SET) {
			nrc_usr_print("[%s] IP ...\n",__func__);
			break;
		} else {
			nrc_usr_print("[%s] IP Address setting State : %d != NET_ADDR_SET(%d) yet...\n",
						  __func__, nrc_addr_get_state(0), NET_ADDR_SET);
		}
		_delay_ms(1000);
	}

//	nrc_usr_print("[%s] setting wifi interface to promiscuous mode...\n",__func__);
//	system_api_set_promiscuous_mode(true);
	return NRC_SUCCESS;
}
