#include "nrc_sdk.h"
#include "nrc_lwip.h"
#include "wifi_config_setup.h"
#include "wifi_connect_common.h"
#include "wifi_config.h"
#include "nrc_eth_if.h"

nrc_err_t start_softap(WIFI_CONFIG* param)
{
	int i = 0;
	int count = 0;
	int network_index = 0;
	int dhcp_server = 0;
	tWIFI_STATE_ID wifi_state = WIFI_STATE_INIT;

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

	nrc_usr_print ("\033[31m [%s] dhcp_server start? %d \033[39m\n", __func__, dhcp_server);
	if (dhcp_server == 1) {
		nrc_usr_print("\033[31m [%s] Trying to start DHCP Server \033[39m\n",	__func__);
		if(nrc_wifi_softap_start_dhcp_server(0) != WIFI_SUCCESS) {
			nrc_usr_print("[%s] Fail to start dhcp server\n", __func__);
			return NRC_FAIL;
		}
	}

	return NRC_SUCCESS;
}

nrc_err_t connect_to_ap(WIFI_CONFIG* param)
{
	int i = 0;
	int count = 0;
	int dhcp_server = 0;

	dhcp_server = param->dhcp_server;

	if (wifi_init(param)!= WIFI_SUCCESS) {
		nrc_usr_print ("[%s] ASSERT! Fail for init\n", __func__);
		return NRC_FAIL;
	}

	netif_set_default(nrc_netif_get_by_idx(0));

	/* Try to connect */
	if (wifi_connect(param)!= WIFI_SUCCESS) {
		nrc_usr_print ("[%s] Fail for Wi-Fi connection (results:%d)\n", __func__);
		return NRC_FAIL;
	}

	if (nrc_eth_get_network_mode() == NRC_NETWORK_MODE_BRIDGE) {
		/* ignore wifi state for bridge mode */
		return WIFI_SUCCESS;
	}

	/* check if IP is ready */
	if (nrc_wait_for_ip(0, param->dhcp_timeout) == NRC_FAIL) {
		return NRC_FAIL;
	}

	return NRC_SUCCESS;
}
