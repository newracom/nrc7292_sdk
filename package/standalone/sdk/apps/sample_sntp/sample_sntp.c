/*
 * MIT License
 *
 * Copyright (c) 2022 Newracom, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "nrc_sdk.h"
#include "nrc_lwip.h"
#include "lwip/sys.h"
#include "lwip/sockets.h"
#include "lwip/errno.h"
#include "wifi_config_setup.h"
#include "wifi_connect_common.h"

#ifndef MAX_RETRY
#define MAX_RETRY 30
#endif

#define SNTP_SERVER "pool.ntp.org"
#define SNTP_TIMEOUT 60

/****************************************************************************
 * FunctionName : run_sample_sntp
 * Description  : Aging test for tcp client
 * Parameters   : WIFI_CONFIG
 * Returns	  : 0 or -1 (0: success, -1: fail)
 *****************************************************************************/
nrc_err_t run_sample_sntp(WIFI_CONFIG *param)
{
	int count;
	u64_t utc_time = 0;
	char utc_buf[26];

	nrc_usr_print("==========================\n");
	nrc_usr_print("SNTP Test - Client\n");
	nrc_usr_print("==========================\n");

	/* set initial wifi configuration */
	if (wifi_init(param)!= WIFI_SUCCESS) {
		nrc_usr_print ("[%s] Error Wifi initialzation failed.\n", __func__);
		return -1;
	}

	/* connect to AP */
	for(count = 0 ; count < MAX_RETRY ; ){
		if (wifi_connect(param)== WIFI_SUCCESS){
			nrc_usr_print ("[%s] Successfully connected to '%s' !! \n", __func__, param->ssid);
			break;
		}

		if (++count == MAX_RETRY){
			nrc_usr_print ("[%s] Failed to connect to '%s'\n", __func__, param->ssid);
			return -1;
		}

		_delay_ms(1000);
	}

	/* check if IP is ready */
	if (nrc_wait_for_ip(0, param->dhcp_timeout) == NRC_FAIL) {
		return NRC_FAIL;
	}

	if (initialize_sntp(SNTP_SERVER, SNTP_TIMEOUT) != 0) {
		nrc_usr_print("%s: timeout %ds", __func__, SNTP_TIMEOUT);
		return -1;
	}

	for(int i = 0; i<10; i++) {
		nrc_usr_print("%s: UTC %s\n", __func__, get_utc_time_str(utc_buf, sizeof(utc_buf)));
		_delay_ms(2000);
	}

	return 0;
}


/****************************************************************************
 * FunctionName : user_init
 * Description  : Start Code for User Application, Initialize User function
 * Parameters   : none
 * Returns      : none
 *****************************************************************************/
WIFI_CONFIG wifi_config;
WIFI_CONFIG* param = &wifi_config;

void user_init(void)
{

	memset(param, 0x0, WIFI_CONFIG_SIZE);

	nrc_wifi_set_config(param);
	run_sample_sntp(param);
}

