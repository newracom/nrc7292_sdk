#include "nrc_sdk.h"
#include "wifi_config_setup.h"
#include "wifi_connect_common.h"

static bool ap_connected = false;

static uint16_t nrc_wifi_get_s1g_channel(uint16_t freq)
{
	const CHANNEL_MAPPING_TABLE *ch_mapping_table = get_s1g_channel_item_by_nons1g_freq(freq);

	if (ch_mapping_table) {
		return ch_mapping_table->s1g_freq;
	}

	return 0;
}

nrc_err_t connect_to_ap(WIFI_CONFIG *param)
{
	const int max_tries = 10;
	bool result = false;
	int i = 0, j = 0;
	SCAN_RESULTS scan_output;

	if (ap_connected) {
		return NRC_SUCCESS;
	}

	if (wifi_init(param) != WIFI_SUCCESS) {
		nrc_usr_print ("[%s] wifi initialization failed.\n", __func__);
		return NRC_FAIL;
	}
	/* find AP */
	for (i = 0; i < max_tries; i++) {
		if (nrc_wifi_scan(0) == WIFI_SUCCESS) {
			if (nrc_wifi_scan_results(0, &scan_output) == WIFI_SUCCESS) {
				/* Find the ssid in scan results */
				for (j = 0; j < scan_output.n_result; j++) {
					if ((strcmp((char*) param->ssid, (char*) scan_output.result[j].ssid) == 0)
						&& (scan_output.result[i].security == param->security_mode)) {
						result = true;
						nrc_usr_print("[%s] channel frequency = %s\n", __func__, scan_output.result[j].freq);
						nrc_usr_print("[%s] s1g freq = %d\n", __func__, nrc_wifi_get_s1g_channel(atoi(scan_output.result[j].freq)));
						/* help connecting AP by setting scan_freq_list to one we found */
						uint16_t s1g_freq = nrc_wifi_get_s1g_channel(atoi(scan_output.result[j].freq));
						if (s1g_freq) {
							param->scan_freq_list[0] = s1g_freq;
							param->scan_freq_num = 1;
						}
						break;
					}
				}

				if (result) {
					nrc_usr_print("[%s] %s found \n", __func__, param->ssid);
					break;
				}
			}
		} else {
			nrc_usr_print("[%s] Couldn't find AP with ssid \"%s\" \n", __func__, param->ssid);
			_delay_ms(1000);
		}
	}

	if (!result) {
		return NRC_FAIL;
	}

	result = false;
	nrc_usr_print ("[%s] Connecting to AP - %s...\n", __func__, param->ssid);
	for (i = 0; i < max_tries; i++) {
		if (wifi_connect(param) == WIFI_SUCCESS) {
			nrc_usr_print ("[%s] Wi-Fi connection successful...\n", __func__);
			result = true;
			break;
		} else {
			nrc_usr_print ("[%s] Fail for connection %s\n", __func__, param->ssid);
			_delay_ms(1000);
		}
	}

	if (!result) {
		return NRC_FAIL;
	}

	result = false;
	/* check if IP is ready */
	for (i = 0; i < max_tries; i++) {
		if (nrc_addr_get_state(0) == NET_ADDR_SET) {
			nrc_usr_print("[%s] IP ...\n",__func__);
			result = true;
			break;
		} else {
			nrc_usr_print("[%s] IP Address setting State : %d != NET_ADDR_SET(%d) yet...\n",
				__func__, nrc_addr_get_state(0), NET_ADDR_SET);
		}
		_delay_ms(1000);
	}

	if (!result) {
		return NRC_FAIL;
	}

	nrc_usr_print("[%s] Device is online connected to %s\n",__func__, param->ssid);
	/* set ap_connected to true */
	ap_connected = true;

	return NRC_SUCCESS;
}
