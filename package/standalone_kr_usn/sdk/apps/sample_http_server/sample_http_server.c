/*
 * MIT License
 *
 * Copyright (c) 2020 Newracom, Inc.
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

#include <esp_http_server.h>
#include <esp_err.h>
#include "wifi_config_setup.h"
#include "wifi_connect_common.h"
#include "wifi_config.h"
#include <lwip.h>
#include <dhcpserver.h>

#define AP_SSID "halow_setup"
#define SECURITY_MODE_SIZE 10

const char input_element[] = R"rawliteral(
<!DOCTYPE html>
<html>
<body>

<h1>Configure Wi-Fi</h1>

<form action="/hello">
  <label for="ssid">Wi-Fi name:</label>
  <input type="text" id="ssid" name="ssid"><br><br>
  <label for="passwd">Wi-Fi password:</label>
  <input type="text" id="passwd" name="passwd"><br><br>
  <label for="security">Security mode:</label>
  <select id="security" name="security">
    <option value="open">OPEN</option>
    <option value="wpa2">WPA2</option>
    <option value="wpa3-sae">WPA3-SAE</option>
    <option value="wpa3-owe">WPA3-OWE</option>
  </select>
  <input type="submit" value="Submit">
</form>

<p>Input desired values and click the "Submit" button.</p>

</body>
</html>

)rawliteral";

nrc_err_t start_softap(WIFI_CONFIG* param)
{
	int i = 0;
	int count = 0;
	int network_index = 0;
	int dhcp_server = 0;
	tWIFI_STATE_ID wifi_state = WIFI_STATE_INIT;

	count = param->count;
	dhcp_server = param->dhcp_server;

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
	nrc_usr_print ("\033[31m [%s] calling nrc_wifi_softap_set_ip \033[39m\n", __func__);
	if (nrc_wifi_softap_set_ip((char *)&param->ap_ip) != WIFI_SUCCESS) {
		nrc_usr_print("[%s] Fail set AP's IP\n", __func__);
		return NRC_FAIL;
	}

	nrc_usr_print ("\033[31m [%s] dhcp_server start? %d \033[39m\n", __func__, dhcp_server);
	if (dhcp_server == 1) {
		nrc_usr_print("\033[31m [%s] Trying to start DHCP Server \033[39m\n",	__func__);
		if(nrc_wifi_softap_start_dhcp_server() != WIFI_SUCCESS) {
			nrc_usr_print("[%s] Fail to start dhcp server\n", __func__);
			return NRC_FAIL;
		}
	}

	return NRC_SUCCESS;
}

int connect_to_ap(WIFI_CONFIG* param)
{
	int i = 0;
	int count =0;
	int network_index = 0;
	int dhcp_server = 0;
	tWIFI_STATE_ID wifi_state = WIFI_STATE_INIT;

	count = param->count;
	dhcp_server = param->dhcp_server;

	if (wifi_init(param)!= WIFI_SUCCESS) {
		nrc_usr_print ("[%s] ASSERT! Fail for init\n", __func__);
		return NRC_FAIL;
	}

	nrc_usr_print ("[%s] Trying to connect to AP - %s...\n", __func__, param->ssid);
	do {
		if (wifi_connect(param) == WIFI_SUCCESS) {
			nrc_usr_print ("[%s] Wi-Fi connection successful...\n", __func__);
			break;
		}
		nrc_usr_print ("[%s] Connect to AP(%s) timed out, trying again...\n", __func__, param->ssid);
		i++;
	} while (i < 10);

	if (i >= 10) {
		nrc_usr_print ("[%s] Wi-Fi connection failed. Check AP availability and SSID, and try again...\n", __func__);
		return NRC_FAIL;
	}

	nrc_wifi_get_network_index(&network_index );
	nrc_usr_print ("[%s] network_index = %d\n", __func__, network_index);

	i = 0;
	do {
		nrc_wifi_get_state(&wifi_state);
		if (wifi_state == WIFI_STATE_GET_IP) {
			nrc_usr_print("[%s] IP received!\n",__func__);
			break;
		}
		_delay_ms(1000);
		i++;
	} while (i < 10);

	if (wifi_state != WIFI_STATE_GET_IP) {
		nrc_usr_print("[%s] Fail to connect or get IP !\n",__func__);
		return NRC_FAIL;
	}

	nrc_usr_print("[%s] Device is online connected to %s\n",__func__, param->ssid);
	return NRC_SUCCESS;
}

/* An HTTP GET handler */
esp_err_t input_get_handler(httpd_req_t *req)
{
	char*  buf;
	size_t buf_len;
	WIFI_CONFIG param;
	int network_index = -1;

	/* Get header value string length and allocate memory for length + 1,
	 * extra byte for null termination */
	buf_len = httpd_req_get_hdr_value_len(req, "Host") + 1;
	if (buf_len > 1) {
		buf = malloc(buf_len);
		/* Copy null terminated value string into buffer */
		if (httpd_req_get_hdr_value_str(req, "Host", buf, buf_len) == ESP_OK) {
			nrc_usr_print ("Found header => Host: %s\n", buf);
		}
		free(buf);
	}

	buf_len = httpd_req_get_hdr_value_len(req, "Test-Header-2") + 1;
	if (buf_len > 1) {
		buf = malloc(buf_len);
		if (httpd_req_get_hdr_value_str(req, "Test-Header-2", buf, buf_len) == ESP_OK) {
			nrc_usr_print ("Found header => Test-Header-2: %s\n", buf);
		}
		free(buf);
	}

	buf_len = httpd_req_get_hdr_value_len(req, "Test-Header-1") + 1;
	if (buf_len > 1) {
		buf = malloc(buf_len);
		if (httpd_req_get_hdr_value_str(req, "Test-Header-1", buf, buf_len) == ESP_OK) {
			nrc_usr_print ("Found header => Test-Header-1: %s\n", buf);
		}
		free(buf);
	}

	/* Read URL query string length and allocate memory for length + 1,
	 * extra byte for null termination */
	buf_len = httpd_req_get_url_query_len(req) + 1;
	if (buf_len > 1) {
		buf = malloc(buf_len);
		if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
			nrc_usr_print ("Found URL query => %s\n", buf);
			char security_mode[SECURITY_MODE_SIZE];
			memset(&param, 0x0, WIFI_CONFIG_SIZE);
			set_wifi_config(&param);
			memset(security_mode, 0x0, SECURITY_MODE_SIZE);

			/* Get value of expected key from query string */
			if (httpd_query_key_value(buf, "ssid", (char *) param.ssid, sizeof(param.ssid)) == ESP_OK) {
				nrc_usr_print ("Found URL query parameter => ssid=%s\n", param.ssid);
			}
			if (httpd_query_key_value(buf, "passwd", (char *) param.password, sizeof(param.password)) == ESP_OK) {
				nrc_usr_print ("Found URL query parameter => passwd=%s\n", param.password);
			}
			if (httpd_query_key_value(buf, "security", security_mode, sizeof(security_mode)) == ESP_OK) {
				nrc_usr_print ("Found URL query parameter => security=%s\n", security_mode);
				if (strcmp(security_mode, "open") == 0) {
					param.security_mode = WIFI_SEC_OPEN;
					memset(param.password, 0x0, sizeof(param.password));
				} else if (strcmp(security_mode, "wpa2") == 0) {
					param.security_mode = WIFI_SEC_WPA2;
				} else if (strcmp(security_mode, "wpa3-owe") == 0) {
					param.security_mode = WIFI_SEC_WPA3_OWE;
                } else if (strcmp(security_mode, "wpa3-sae") == 0) {
					param.security_mode = WIFI_SEC_WPA3_SAE;
				}
			}
		}
		free(buf);
	}

	/* Set some custom headers */
	httpd_resp_set_hdr(req, "Custom-Header-1", "Custom-Value-1");
	httpd_resp_set_hdr(req, "Custom-Header-2", "Custom-Value-2");

	/* Send response with custom headers and body set as the
	 * string passed in user context*/
	const char* resp_str = (const char*) req->user_ctx;
	httpd_resp_send(req, resp_str, strlen(resp_str));

	/* After sending the HTTP response the old HTTP request
	 * headers are lost. Check if HTTP request headers can be read now. */
	if (httpd_req_get_hdr_value_len(req, "Host") == 0) {
		nrc_usr_print ("Request headers lost\n");
	}

	nrc_wifi_get_network_index(&network_index);
	nrc_usr_print ("[%s] network_index = %d before calling connect_to_ap\n", __func__, network_index);
	reset_ip_address(network_index);
	nrc_wifi_remove_network(0);
	dhcps_stop();
	connect_to_ap(&param);
	return ESP_OK;
}

httpd_uri_t hello = {
	.uri       = "/hello",
	.method    = HTTP_GET,
	.handler   = input_get_handler,
	/* Let's pass response string in user
	 * context to demonstrate it's usage */
	.user_ctx  = "Setup completed!"
};

esp_err_t input_handler(httpd_req_t *req)
{
	/* Recv, Process and Send */
	return httpd_resp_send(req, input_element, strlen(input_element));
}

httpd_uri_t input = {
	.uri = "/",
	.method = HTTP_GET,
	.handler = input_handler,
	.user_ctx = NULL
};

/******************************************************************************
 * FunctionName : run_http_server
 * Description  : Start http server
 * Parameters   :
 * Returns	    : return httpd handler
 *******************************************************************************/
httpd_handle_t run_http_server()
{
	httpd_handle_t handle;
	httpd_config_t conf = HTTPD_DEFAULT_CONFIG();

	if (httpd_start(&handle, &conf) == ESP_OK) {
		nrc_usr_print("\033[31m[httpd] server on port : %d\033[39m\n", conf.server_port);
		return handle;
	}
	return NULL;
}

/******************************************************************************
 * FunctionName : user_init
 * Description  : Start Code for User Application, Initialize User function
 * Parameters   : none
 * Returns	    : none
 *******************************************************************************/
void user_init(void)
{
	httpd_handle_t server = NULL;

	WIFI_CONFIG *param;

	nrc_uart_console_enable(true);

	param = nrc_mem_malloc(WIFI_CONFIG_SIZE);
	memset(param, 0x0, WIFI_CONFIG_SIZE);

	nrc_usr_print("\033[31m[httpd] calling set_wifi_softap_config...\033[39m");
	set_wifi_softap_config(param);
	strcpy((char *)param->ssid, AP_SSID);
	param->security_mode = WIFI_SEC_WPA2;
	nrc_usr_print("\033[31m[httpd] calling start_softap...\033[39m\n");
	start_softap(param);

	nrc_usr_print("\033[31m[httpd] freeing param...\033[39m\n");
	if(param){
		nrc_mem_free(param);
	}

	/* start HTTP server */
	nrc_usr_print("\033[31m[httpd] Starting http server...\033[39m\n");
	server = run_http_server();

	if (server) {
		httpd_register_uri_handler(server, &input);
		httpd_register_uri_handler(server, &hello);
	}

	nrc_usr_print("\033[31m [%s] End of user_init!! \033[39m\n",__func__);

	while(1) {
		_delay_ms(1);
	}
}
