/*
 * MIT License
 *
 * Copyright (c) 2022 Teledatics, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
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

/**
 * @file teledatics_gui_http.c
 * @author James Ewing
 * @date 27 Mar 2022
 * @brief Teledatics HTML GUI for Halo TD-XPAH
 */

#include "base64_html.h"
#include "lzo_decompress.h"
#include "esp_http_server.h"

#include "mbedtls/base64.h"

#include "nrc_sdk.h"
#include "standalone.h"
#include "wifi_config_setup.h"
#include "favicon.h"

httpd_handle_t http_server = NULL;
static char* html_buffer=NULL;
static char* html_orig=NULL;

#define HTML_BUF_LEN 14 * 1024
#define COMPRESSED_HTML_BUF_LEN 7 * 1024

static const char rebooting_element[] = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
	<title>Rebooting...</title>
</head>
<body>
	<h1>Rebooting...</h1>
	<p>Close the browser and reconnect...</p>
</body>
</html>
)rawliteral";

static const char setup_complete[] = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
	<title>Configuration complete</title>
</head>
<body>
	<h1>Setup completed!</h1>
    <p> <button value="submit" type="submit" onclick="reboot()">Reboot</button> </p>

    <script>
        function reboot() {
            window.location.href = '/rebooting';
        }
    </script>
</body>
</html>
)rawliteral";

/**
 * @brief insert new string in place of old string within destination string
 *
 * String substitution utility
 *
 * @param destination string
 * @param string to replace
 * @param string to insert
 * @returns pointer to destination string on success, NULL on failure
 */
static char*
subst_string(char* dest, char* old, const char* new)
{
	char* p;
	size_t dl, ol, nl, tl;

	if (!dest || !old || !new)
		return NULL;

	p = strstr(dest, old);

	if (!p)
		return NULL;

	dl = strlen(dest) + 1;
	ol = strlen(old);
	nl = strlen(new);
	tl = dl - (p - dest) - ol;

	if (dl == 1 || dl < ol)
		return NULL;

	memmove(p + nl, p + ol, tl);
	memcpy(p, new, nl);

	return dest;
}


/**
 * @brief substitute settings in HTML
 *
 * Macro substitution of current settings
 *
 * @param HTML buffer
 * @param Wifi settings
 * @returns int, 0 on success
 */
int
subst_wifi_values(char* html, WIFI_CONFIG* wifi_config)
{
	char val[256] = { 0 };
	char* subst = NULL;
	int i = 0;

	for (i = 0; i < WIFI_SUBST_SIZE; i++) {
		subst = strstr(html, html_subst[i]);
		if (subst) {
			//nrc_usr_print("[%s] found %s in html\n", __func__, html_subst[i]);
			switch (i) {
				case WIFI_MODE_SUBST:
					if (WIFI_MODE_STATION == wifi_config->device_mode) {
						snprintf(val, sizeof(val) - 1, "\"%s\"", "sta");
					} else {
						snprintf(val, sizeof(val) - 1, "\"%s\"", "ap");
					}
					break;
				case WIFI_SSID_SUBST:
					snprintf(val, sizeof(val) - 1, "\"%s\"", wifi_config->ssid);
					break;
				case WIFI_SECURITY_SUBST:
					if (WIFI_SEC_WPA2 == wifi_config->security_mode) {
						snprintf(val, sizeof(val) - 1, "%s", "\"wpa2\"");
					} else if (WIFI_SEC_WPA3_OWE ==
						wifi_config->security_mode) {
						snprintf(val, sizeof(val) - 1, "%s", "\"wpa3_owe\"");
					} else if (WIFI_SEC_WPA3_SAE ==
						wifi_config->security_mode) {
						snprintf(val, sizeof(val) - 1, "%s", "\"wpa3_sae\"");
					} else {
						snprintf(val, sizeof(val) - 1, "%s", "\"none\"");
					}
					break;
				case WIFI_PASSWORD_SUBST:
					if (strlen((char*)wifi_config->password) > 0) {
						snprintf(val, sizeof(val) - 1, 	"\"%s\"", wifi_config->password);
					} else {
						snprintf(val, sizeof(val) - 1, "\"\"");
					}
					break;
				case WIFI_COUNTRY_SUBST:
					snprintf(val, sizeof(val) - 1, "\"%s\"", wifi_config->country);
					break;
				case WIFI_CHANNEL_SUBST:
					snprintf(val, sizeof(val) - 1, "%d", wifi_config->channel);
					break;
				case WIFI_IPMODE_SUBST:
					if (WIFI_STATIC_IP == wifi_config->ip_mode) {
						snprintf(val, sizeof(val) - 1, "\"%s\"", "static_ip");
					} else {
						snprintf(val, sizeof(val) - 1, "\"%s\"", "dynamic_ip");
					}
					break;
				case WIFI_STATICIP_SUBST:
					snprintf(val, 	sizeof(val) - 1, "\"%s\"",  wifi_config->static_ip);
					break;
				case WIFI_DHCPSERVER_SUBST:
					snprintf(val, sizeof(val) - 1, "%d", wifi_config->dhcp_server);
					break;
				case WIFI_BEACON_INTERVAL_SUBST:
					snprintf(val, sizeof(val) - 1, "%d", wifi_config->bcn_interval);
					break;
				case WIFI_TXPOWER_SUBST:
					snprintf(val, sizeof(val) - 1, "%d", wifi_config->tx_power);
					break;
				case WIFI_BSS_MAX_IDLE_SUBST:
					snprintf(val, sizeof(val) - 1, "%d", (int)wifi_config->bss_max_idle);
					break;
				case WIFI_BSS_RETRY_CNT_SUBST:
					snprintf(val, sizeof(val) - 1, "%d", (int)wifi_config->bss_retry_cnt);
					break;
				case WIFI_CONN_TIMEOUT_SUBST:
					snprintf(val, sizeof(val) - 1, "%d", (int)wifi_config->conn_timeout);
					break;
				case WIFI_DISCONN_TIMEOUT_SUBST:
					snprintf(val, sizeof(val) - 1, "%d", (int)wifi_config->disconn_timeout);
					break;
				case WIFI_TXPOWER_TYPE_SUBST:
					if (WIFI_TXPOWER_LIMIT == wifi_config->tx_power_type) {
						snprintf(val, sizeof(val) - 1, "\"%s\"", "limit");
					} else if (WIFI_TXPOWER_FIXED == wifi_config->tx_power_type) {
						snprintf(val, sizeof(val) - 1, "\"%s\"", "fixed");
					} else {
						snprintf(val, sizeof(val) - 1, "\"%s\"", "auto");
					}
					break;
				case WIFI_NETMASK_SUBST:
					snprintf(val, 	sizeof(val) - 1, "\"%s\"",  wifi_config->netmask);
					break;
				case WIFI_GATEWAY_SUBST:
					snprintf(val, 	sizeof(val) - 1, "\"%s\"",  wifi_config->gateway);
					break;
				default:
					break;
			}
			//nrc_usr_print("[%s] subst %s in html\n", __func__, val);
			subst_string(html, (char*)html_subst[i], val);
		}
	}
	return 0;
}

/**
 * @brief http configuration handler callback
 *
 * Callback function that sends configuration page
 *
 * @param http request
 * @returns esp error type
 */
esp_err_t
setup_page_http(httpd_req_t* req)
{
	WIFI_CONFIG* wifi_config;
	esp_err_t  ret_httpd_resp = ESP_OK;

	wifi_config = (WIFI_CONFIG*)req->user_ctx;
	memcpy(html_buffer, html_orig, HTML_BUF_LEN );
	subst_wifi_values(html_buffer, wifi_config);

	ret_httpd_resp = httpd_resp_send(req, html_buffer, strlen(html_buffer));
	_delay_ms(1000);

	if (ret_httpd_resp != ESP_OK) {
		nrc_usr_print("[%s] Failed to send HTTP response: 0x%04x\n", __func__, ret_httpd_resp);
		return ESP_FAIL;
	} else {
//		nrc_usr_print("[%s] HTTP response sent successfully\n", __func__);
		return ESP_OK;
	}
}

httpd_uri_t setup_page = {
	.uri = "/",
	.method = HTTP_GET,
	.handler = setup_page_http,
	.user_ctx = (char *) setup_complete
};

/**
 * @brief http update settings handler callback
 *
 * Callback function that updates settings and saves to NV flash
 *
 * @param http request
 * @returns esp error type
 */
esp_err_t
update_settings_handler(httpd_req_t* req)
{
	WIFI_CONFIG config;
	WIFI_CONFIG* wifi_config = &config;
	char tmp[256] = { 0 };
	int req_len;
	int network_index = -1;

	req_len = httpd_req_get_url_query_len(req) + 1;

	wifi_config = (WIFI_CONFIG*)req->user_ctx;

	if (req_len < HTML_BUF_LEN) {
		if (httpd_req_get_url_query_str(req, html_buffer, req_len) == ESP_OK) {
			if (httpd_query_key_value(html_buffer, "mode", (char*)tmp, sizeof(tmp)) == ESP_OK) {
				nrc_usr_print("[%s] Found URL query parameter => mode=%s\n", __func__, tmp);
				if (!strncasecmp(tmp, "ap", sizeof("ap") - 1))
					wifi_config->device_mode = WIFI_MODE_AP;
				else if (!strncasecmp(tmp, "sta", sizeof("sta") - 1))
					wifi_config->device_mode = WIFI_MODE_STATION;
			}

			if (httpd_query_key_value(html_buffer, "ssid", (char*)wifi_config->ssid,
					sizeof(wifi_config->ssid)) == ESP_OK) {
				nrc_usr_print("[%s] Found URL query parameter => ssid=%s\n", __func__, wifi_config->ssid);
			}

			if (httpd_query_key_value(html_buffer, "security", tmp, sizeof(tmp)) == ESP_OK) {
				nrc_usr_print("[%s] Found URL query parameter => security=%s\n", __func__, tmp);

				if (strncasecmp(tmp, "wpa2", sizeof("wpa2") - 1) == 0) {
					wifi_config->security_mode = WIFI_SEC_WPA2;
				} else if (strncasecmp(tmp, "wpa3_owe", sizeof("wpa3_owe") - 1) == 0) {
					wifi_config->security_mode = WIFI_SEC_WPA3_OWE;
				} else if (strncasecmp(tmp, "wpa3_sae", sizeof("wpa3_sae") - 1) == 0) {
					wifi_config->security_mode = WIFI_SEC_WPA3_SAE;
				} else {
					wifi_config->security_mode = WIFI_SEC_OPEN;
				}
			}

			if (httpd_query_key_value(html_buffer, "password", (char*)wifi_config->password,
				sizeof(wifi_config->password)) == ESP_OK) {
				nrc_usr_print("[%s] Found URL query parameter => password=%s\n", __func__, wifi_config->password);
			}

			if (httpd_query_key_value(html_buffer, "country", (char*)wifi_config->country,
				sizeof(wifi_config->country)) == ESP_OK) {
				nrc_usr_print("[%s] Found URL query parameter => country=%s\n", __func__, wifi_config->country);
			}

			if (httpd_query_key_value(
				html_buffer, "channel", (char*)tmp, sizeof(tmp)) == ESP_OK) {
				nrc_usr_print("[%s] Found URL query parameter => channel=%s\n", __func__, tmp);
				wifi_config->channel = atoi(tmp);
			}

			if (httpd_query_key_value(html_buffer, "ip_mode", (char*)tmp, sizeof(tmp)) == ESP_OK) {
				nrc_usr_print("[%s] Found URL query parameter => ip_mode=%s\n", __func__, tmp);
				if (strncasecmp(tmp, "dynamic_ip", sizeof("dynamic_ip") - 1) == 0) {
					wifi_config->ip_mode = WIFI_DYNAMIC_IP;
				} else {
					wifi_config->ip_mode = WIFI_STATIC_IP;
				}
			}

			if (httpd_query_key_value(html_buffer, "static_ip1", (char*)tmp, sizeof(tmp)) == ESP_OK) {
				char ipv4[16] = "";
				strcat(ipv4, tmp);
				strcat(ipv4, ".");

				httpd_query_key_value(html_buffer, "static_ip2", (char*)tmp, sizeof(tmp));
				strcat(ipv4, tmp);
				strcat(ipv4, ".");

				httpd_query_key_value(html_buffer, "static_ip3", (char*)tmp, sizeof(tmp));
				strcat(ipv4, tmp);
				strcat(ipv4, ".");

				httpd_query_key_value(html_buffer, "static_ip4", (char*)tmp, sizeof(tmp));
				strcat(ipv4, tmp);

				nrc_usr_print("[%s] Found URL query parameter => static_ip=%s\n", __func__, ipv4);
				snprintf((char*)wifi_config->static_ip, sizeof(wifi_config->static_ip), "%s", ipv4);
			}

			// checkbox is present for 1, absent for 0
			if (httpd_query_key_value(html_buffer, "dhcp_server", (char*)tmp, sizeof(tmp)) == ESP_OK) {
				nrc_usr_print("[%s] Found URL query parameter => dhcp_server=%s\n", __func__, tmp);
				wifi_config->dhcp_server = 1;
			} else {
				nrc_usr_print("[%s] URL query parameter dhcp_server missing, set to zero\n", __func__, tmp);
				wifi_config->dhcp_server = 0;
			}

			if (httpd_query_key_value(html_buffer, "bcn_interval", (char*)tmp, sizeof(tmp)) == ESP_OK) {
				nrc_usr_print("[%s] Found URL query parameter => bcn_interval=%s\n", __func__, tmp);
				wifi_config->bcn_interval= atoi(tmp);
			}

			if (httpd_query_key_value(html_buffer, "txpower", (char*)tmp, sizeof(tmp)) == ESP_OK) {
				nrc_usr_print("[%s] Found URL query parameter => txpower=%s\n", __func__, tmp);
				wifi_config->tx_power = atoi(tmp);
			}

			if (httpd_query_key_value(html_buffer, "bss_max_idle", (char*)tmp, sizeof(tmp)) == ESP_OK) {
				nrc_usr_print("[%s] Found URL query parameter => bss_max_idle=%s\n", __func__, tmp);
				wifi_config->bss_max_idle = atoi(tmp);
			}

			if (httpd_query_key_value(html_buffer, "bss_retry_cnt", (char*)tmp, sizeof(tmp)) == ESP_OK) {
				nrc_usr_print("[%s] Found URL query parameter => bss_retry_cnt=%s\n", __func__, tmp);
				wifi_config->bss_retry_cnt = atoi(tmp);
			}

			if (httpd_query_key_value(html_buffer, "conn_timeout", (char*)tmp, sizeof(tmp)) == ESP_OK) {
				nrc_usr_print("[%s] Found URL query parameter => conn_timeout=%s\n", __func__, tmp);
				wifi_config->conn_timeout = atoi(tmp);
			}

			if (httpd_query_key_value(html_buffer, "disconn_timeout", (char*)tmp, sizeof(tmp)) == ESP_OK) {
				nrc_usr_print("[%s] Found URL query parameter => disconn_timeout=%s\n", __func__, tmp);
				wifi_config->disconn_timeout = atoi(tmp);
			}

			if (httpd_query_key_value(html_buffer, "txpower_type", tmp, sizeof(tmp)) == ESP_OK) {
				nrc_usr_print("[%s] Found URL query parameter => txpower_type=%s\n", __func__, tmp);
				if (strncasecmp(tmp, "limit", sizeof("limit") - 1) == 0) {
					wifi_config->tx_power_type = WIFI_TXPOWER_LIMIT;
				} else if (strncasecmp(tmp, "fixed", sizeof("fixed") - 1) == 0) {
					wifi_config->tx_power_type = WIFI_TXPOWER_FIXED;
				} else {
					wifi_config->tx_power_type = WIFI_TXPOWER_AUTO;
				}
			}

			if (httpd_query_key_value(html_buffer, "netmask1", (char*)tmp, sizeof(tmp)) == ESP_OK) {
				char nm[16] = "";
				strcat(nm, tmp);
				strcat(nm, ".");

				httpd_query_key_value(html_buffer, "netmask2", (char*)tmp, sizeof(tmp));
				strcat(nm, tmp);
				strcat(nm, ".");

				httpd_query_key_value(html_buffer, "netmask3", (char*)tmp, sizeof(tmp));
				strcat(nm, tmp);
				strcat(nm, ".");

				httpd_query_key_value(html_buffer, "netmask4", (char*)tmp, sizeof(tmp));
				strcat(nm, tmp);

				nrc_usr_print("[%s] Found URL query parameter => netmask=%s\n", __func__, nm);
				snprintf((char*)wifi_config->netmask, sizeof(wifi_config->netmask), 	"%s", nm);
			}

			if (httpd_query_key_value(html_buffer, "gateway1", (char*)tmp, sizeof(tmp)) == ESP_OK) {
				char gw[16] = "";
				strcat(gw, tmp);
				strcat(gw, ".");

				httpd_query_key_value(html_buffer, "gateway2", (char*)tmp, sizeof(tmp));
				strcat(gw, tmp);
				strcat(gw, ".");

				httpd_query_key_value(html_buffer, "gateway3", (char*)tmp, sizeof(tmp));
				strcat(gw, tmp);
				strcat(gw, ".");

				httpd_query_key_value(html_buffer, "gateway4", (char*)tmp, sizeof(tmp));
				strcat(gw, tmp);

				nrc_usr_print("[%s] Found URL query parameter => gateway=%s\n", __func__, gw);
				snprintf((char*)wifi_config->gateway, sizeof(wifi_config->gateway), 	"%s", gw);
			}

		}
		wifi_config->network_mode = WIFI_NETWORK_MODE_BRIDGE;

		nrc_usr_print("[%s] Saving configuration...\n", __func__);
		nrc_save_wifi_config(wifi_config, 1);

		httpd_resp_send(req, setup_complete, strlen(setup_complete));
		return ESP_OK;
	}

	return ESP_FAIL;
}

httpd_uri_t update_settings = {
	.uri = "/update_settings",
	.method = HTTP_GET,
	.handler = update_settings_handler,
	.user_ctx = NULL
};

static esp_err_t reboot_page_handler(httpd_req_t *req)
{
	esp_err_t ret = httpd_resp_send(req, rebooting_element, strlen(rebooting_element));

	_delay_ms(2000);
	nrc_sw_reset();

	return ESP_OK;
}

static httpd_uri_t rebooting = {
	.uri       = "/rebooting",
	.method    = HTTP_GET,
	.handler   = reboot_page_handler,
	.user_ctx  = NULL
};

/**
 * @brief http configuration handler callback
 *
 * Callback function that sends favicon
 *
 * @param http request
 * @returns esp error type
 */
esp_err_t
setup_favicon(httpd_req_t* req)
{
	esp_err_t  ret_httpd_resp = ESP_OK;

	// Set the Content-Type header to indicate that this is a favicon file
	httpd_resp_set_type(req, "image/x-icon");

	// Load the favicon file from disk and send it to the client
	ret_httpd_resp = httpd_resp_send(req, (const char *)favicon_png, favicon_png_len);
	vTaskDelay(pdMS_TO_TICKS(100));

	if (ret_httpd_resp != ESP_OK) {
		nrc_usr_print("[%s] Failed to send HTTP response: 0x%04x\n", __func__, ret_httpd_resp);
		return ESP_FAIL;
	} else {
		nrc_usr_print("[%s] HTTP response sent successfully\n", __func__);
		return ESP_OK;
	}
}


httpd_uri_t favicon_setup = {
	.uri = "/favicon.ico",
	.method = HTTP_GET,
	.handler = setup_favicon,
	.user_ctx = NULL
};

esp_err_t setup_setting_html(void)
{
	size_t html_len = 0, buf_len = 0;
	uint8_t buf[COMPRESSED_HTML_BUF_LEN];
	int ret;

	ret = mbedtls_base64_decode((uint8_t*)buf, sizeof(buf), &buf_len,
			(uint8_t*)html_base64, strlen(html_base64));
	nrc_usr_print("[%s] base64 decode required buf_len %d, html_base64 length  %d\n", __func__, buf_len, strlen(html_base64));

	if (ret || buf_len <= 0) {
		nrc_usr_print("[%s] base64 decode error ret %d buf_len %d\n", __func__, ret, buf_len);
		return ESP_FAIL;
	}

	html_len = HTML_BUF_LEN;
	nrc_usr_print("[%s] allocating html_buffer of size %d.\n", __func__, HTML_BUF_LEN);
	html_orig = nrc_mem_malloc(HTML_BUF_LEN);
	if(!html_orig)
		return ESP_FAIL;
	memset(html_orig, 0, HTML_BUF_LEN);

	html_buffer = nrc_mem_malloc(HTML_BUF_LEN);
	if(!html_buffer){
		nrc_mem_free(html_orig);
		return ESP_FAIL;
	}
	memset(html_buffer, 0, HTML_BUF_LEN);

	ret = lzop_decompress(buf, buf_len, (unsigned char *)html_orig, &html_len);

	if (ret || html_len <= 0) {
		nrc_usr_print("[%s] lzo decompress error ret %d html_len %d\n", __func__, ret, html_len);
		nrc_mem_free(html_orig);
		nrc_mem_free(html_buffer);
		return ESP_FAIL;
	}
	return ESP_OK;
}


/**
 * @brief start http server
 *
 * Run configuration GUI http server
 *
 * @param ptr to configuration struct
 * @returns handle to running server
 */
httpd_handle_t
run_http_server(WIFI_CONFIG* wifi_config)
{
	httpd_config_t conf = HTTPD_DEFAULT_CONFIG();

	if(setup_setting_html() != ESP_OK){
		return NULL;
	}

	if (http_server) {
		httpd_stop(http_server);
		http_server = NULL;
	}

	if (httpd_start(&http_server, &conf) == ESP_OK) {
		nrc_usr_print("httpd server on port : %d\n", conf.server_port);
	}

	if (http_server) {
		nrc_usr_print("[%s]: set up callbacks\n", __func__);

		setup_page.user_ctx = update_settings.user_ctx = (void*)wifi_config;

		httpd_register_uri_handler(http_server, &setup_page);
		httpd_register_uri_handler(http_server, &update_settings);
		httpd_register_uri_handler(http_server, &favicon_setup);
		httpd_register_uri_handler(http_server, &rebooting);

		nrc_usr_print("[%s]: http server started\n", __func__);

		return http_server;
	}

	nrc_usr_print("[%s]: error, returning NULL\n", __func__);

	return NULL;
}
