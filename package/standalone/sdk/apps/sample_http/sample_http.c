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

#include <string.h>
#include "nrc_sdk.h"
#include "nrc_lwip.h"
#include "wifi_config_setup.h"
#include "wifi_connect_common.h"

#include "nrc_http_client.h"

#if defined( SUPPORT_MBEDTLS )
#include "http_certs.h"
#endif

#define RUN_HTTPS
#if defined( SUPPORT_HTTPS_CLIENT ) && defined( RUN_HTTPS )
char *url = "https://192.168.200.1:4443/";
#else
char *url = "http://192.168.10.199";
#endif

#define ENABLE_POST_REQUEST 1
#if ENABLE_POST_REQUEST
char *url_post = "https://192.168.200.1:4443/";
#endif

/******************************************************************************
 * FunctionName : run_sample_http
 * Description  : sample test for http
 * Parameters   : WIFI_CONFIG
 * Returns      : 0 or -1 (0: success, -1: fail)
 *******************************************************************************/
nrc_err_t run_sample_http(WIFI_CONFIG *param)
{
	SCAN_RESULTS results;
	ssl_certs_t *certs_ptr = NULL;

	nrc_usr_print("[%s] Sample App for http \n",__func__);

	int i = 0;
	int ssid_found =false;

	/* set initial wifi configuration */
	while(1){
		if (wifi_init(param)== WIFI_SUCCESS) {
			nrc_usr_print ("[%s] wifi_init Success !! \n", __func__);
			break;
		} else {
			nrc_usr_print ("[%s] wifi_init Failed !! \n", __func__);
			_delay_ms(1000);
		}
	}

	/* find AP */
	while(1){
		if (nrc_wifi_scan(0) == WIFI_SUCCESS){
			if (nrc_wifi_scan_results(0, &results)== WIFI_SUCCESS) {
				/* Find the ssid in scan results */
				for(i=0; i<results.n_result ; i++){
					if((strcmp((char*)param->ssid, (char*)results.result[i].ssid)== 0)
					   && (results.result[i].security == param->security_mode)){
						ssid_found = true;
						break;
					}
				}

				if(ssid_found){
					nrc_usr_print ("[%s] %s is found \n", __func__, param->ssid);
					break;
				}
			}
		} else {
			nrc_usr_print ("[%s] Scan fail !! \n", __func__);
			_delay_ms(1000);
		}
	}

	/* connect to AP */
	while(1) {
		if (wifi_connect(param)== WIFI_SUCCESS) {
			nrc_usr_print ("[%s] connect to %s successfully !! \n", __func__, param->ssid);
			break;
		} else{
			nrc_usr_print ("[%s] Fail for connection %s\n", __func__, param->ssid);
			_delay_ms(1000);
		}
	}

	/* check if IP is ready */
	if (nrc_wait_for_ip(0, param->dhcp_timeout) == NRC_FAIL) {
		return NRC_FAIL;
	}

#if defined( SUPPORT_HTTPS_CLIENT ) && defined( RUN_HTTPS )
	ssl_certs_t certs;
	certs.client_cert = ssl_client_crt;
	certs.client_cert_length = sizeof(ssl_client_crt);
	certs.client_pk = ssl_client_key;
	certs.client_pk_length = sizeof(ssl_client_key);
	certs.ca_cert = ssl_ca_crt;
	certs.ca_cert_length = sizeof(ssl_ca_crt);
	certs_ptr = &certs;
#endif

	con_handle_t handle;
	httpc_data_t data;

	char buf[512] = {0,};
	data.data_in = buf;
	data.data_in_length = 512;

	httpc_ret_e ret;
	ret = nrc_httpc_get(&handle, url, NULL, &data, certs_ptr);
	nrc_usr_print("%s\n", data.data_in);

	while (ret == HTTPC_RET_OK) {
		ret = nrc_httpc_recv_response(&handle, &data);
		nrc_usr_print("%s\n", data.data_in);
		if (strstr(data.data_in, "\r\n0\r\n") != NULL) {
			nrc_httpc_close(&handle);
			return NRC_FAIL;
		}
		memset(buf, 0, 512);
	}
	nrc_httpc_close(&handle);

#if ENABLE_POST_REQUEST
	con_handle_t handle2;
	char *ct = "Content-Type: application/x-www-form-urlencoded\r\nContent-Length: 8\r\n";
	char *body = "onoff=on";
	data.data_out = body;
	data.data_out_length = 8;
	memset(buf, 0, 512);
	ret = nrc_httpc_post(&handle2, url_post, ct, &data, certs_ptr);
	nrc_usr_print("%s\n", data.data_in);

	while (ret == HTTPC_RET_OK) {
		ret = nrc_httpc_recv_response(&handle2, &data);
		nrc_usr_print("%s\n", data.data_in);
		memset(buf, 0, 512);
	}
	nrc_httpc_close(&handle2);

#endif
	if (nrc_wifi_get_state(0) == WIFI_STATE_CONNECTED) {
		nrc_usr_print("[%s] Trying to DISCONNECT... for exit\n",__func__);
		if (nrc_wifi_disconnect(0, 5000) != WIFI_SUCCESS) {
			nrc_usr_print ("[%s] Fail for Wi-Fi disconnection (results:%d)\n", __func__);
			return NRC_FAIL;
		}
	}
	nrc_usr_print("[%s] End of run_sample_http!! \n",__func__);
	return NRC_SUCCESS;
}

/******************************************************************************
 * FunctionName : user_init
 * Description  : Start Code for User Application, Initialize User function
 * Parameters   : none
 * Returns      : none
 *******************************************************************************/
WIFI_CONFIG wifi_config;
WIFI_CONFIG* param = &wifi_config;
void user_init(void)
{
	nrc_uart_console_enable(true);

	memset(param, 0x0, WIFI_CONFIG_SIZE);

	nrc_wifi_set_config(param);
	run_sample_http(param);
}

