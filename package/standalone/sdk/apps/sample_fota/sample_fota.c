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

#include <stdlib.h>
#include <string.h>

#include "nrc_sdk.h"
#include "nrc_lwip.h"
#include "wifi_config_setup.h"
#include "wifi_connect_common.h"
#include "sample_fota_version.h"

#include "nrc_http_client.h"

#include "cJSON.h"

#if defined( SUPPORT_MBEDTLS )
#include "fota_certs.h"
#endif

//#define RUN_HTTPS
#if defined( SUPPORT_HTTPS_CLIENT ) && defined( RUN_HTTPS )
#define SERVER_URL "https://192.168.200.1:4443/"
#else
#define SERVER_URL "http://192.168.200.1:8080/"
#endif

#define CHECK_VER_URL SERVER_URL "version" // Unique URL for retrieving the version information
#define CHUNK_SIZE 2048

typedef struct {
	VERSION_T version;
	uint8_t force;
	uint32_t crc;
	char fw_url[128];
} update_info_t;

update_info_t update_info;


static void parseVersionString(const char* versionString, size_t versionStringLen, VERSION_T* parsedVersion) {
    char versionCopy[128];
    size_t copyLen = versionStringLen < sizeof(versionCopy) ? versionStringLen : sizeof(versionCopy) - 1;
    strncpy(versionCopy, versionString, copyLen);
    versionCopy[copyLen] = '\0';

    char* token = strtok(versionCopy, ".");
    if (token != NULL)
        parsedVersion->major = atoi(token);

    token = strtok(NULL, ".");
    if (token != NULL)
        parsedVersion->minor = atoi(token);

    token = strtok(NULL, ".");
    if (token != NULL)
        parsedVersion->patch = atoi(token);
}

static  bool isCurrentVersionLower(VERSION_T* parsedVersion) {
	 VERSION_T targetVersion = *parsedVersion;
	 VERSION_T currentVersion = *nrc_get_app_version();

	 if (currentVersion.major < targetVersion.major)
		 return true;
	 else if (currentVersion.major == targetVersion.major && currentVersion.minor < targetVersion.minor)
		 return true;
	 else if (currentVersion.major == targetVersion.major && currentVersion.minor == targetVersion.minor && currentVersion.patch < targetVersion.patch)
		 return true;
	 else
		 return false;
 }

int get_json_str_value(cJSON *cjson, char *key, char **value)
{
	cJSON *cjson_obj = NULL;

	cjson_obj = cJSON_GetObjectItem(cjson, key);
	if (cjson_obj && cjson_obj->valuestring) {
		*value = strdup(cjson_obj->valuestring);
		return 1;
	} else {
		nrc_usr_print("[%s] %s not found in json\n", __func__, key);
		return 0;
	}
}

void parse_response_version (char * data, uint32_t length)
{
	cJSON *cjson = NULL;

	char *version = NULL;
	char *crc = NULL;
	char *fw_url = NULL;
	char *force = NULL;

	char* token = strtok(data, "\r\n");
	uint32_t body_length = 0;

	while (token != NULL) {
		if (strncmp(token, "Content-Length: ", 16) == 0) {
			char* length_str;
			char* temp = strtok_r(token, " ", &length_str);
			body_length = atoi(length_str);
			break;
		} else {
			token = strtok(NULL, "\r\n");
		}
	}

	nrc_usr_print("[%s] version data :\n %s\n", __func__, (char *) (data + (length - body_length)));
	cjson = cJSON_Parse((char *) (data + (length - body_length)));

	if (cjson) {
		if (get_json_str_value(cjson, "version", &version)) {
			nrc_usr_print("[%s] version : %s\n", __func__, version);
			parseVersionString(version, strlen(version), &update_info.version);
		} else {
			goto exit;
		}

		if (get_json_str_value(cjson, "crc", &crc)) {
			nrc_usr_print("[%s] crc : %s\n", __func__, crc);
			update_info.crc = strtoul(crc, NULL, 16);
		} else {
			goto exit;
		}

		if (get_json_str_value(cjson, "fw_name", &fw_url)) {
			nrc_usr_print("[%s] URL : %s\n", __func__, fw_url);
			sprintf(update_info.fw_url, "%s%s", SERVER_URL, fw_url);
		} else {
			goto exit;
		}

		if (get_json_str_value(cjson, "force", &force)) {
			nrc_usr_print("[%s] force : %s\n", __func__, force);
			update_info.force = atoi(force);
		} else {
			/* set default update mode */
			update_info.force = 0;
		}
	} else{
		nrc_usr_print("[%s] JSON parse error\n", __func__);
	}

exit:
	if (cjson) {
		cJSON_Delete(cjson);

		if (version)
			free(version);

		if (crc)
			free(crc);

		if (fw_url)
			free(fw_url);

		if (force)
			free(force);

		nrc_usr_print("[%s] version: %d.%d.%d,  crc: %x  fw_url: %s force: %s\n",
			__func__,  update_info.version.major, update_info.version.minor,update_info.version.patch,
			update_info.crc, update_info.fw_url, update_info.force);
	}
}

uint32_t fw_len = 0;
static void parse_response_content (char * data )
{
	char* token = strtok(data, "\r\n");
	while (token != NULL) {
		if (strncmp(token, "Content-Length: ", 16) == 0) {
			char* length;
			char* temp = strtok_r(token, " ", &length);
			fw_len = atoi(length);
			nrc_usr_print("[%s] fw_len = %u\n\n",__func__, fw_len);
			break;
		} else {
			token = strtok(NULL, "\r\n");
		}
	}
}

static void connect_to_ap(WIFI_CONFIG *param)
{
	SCAN_RESULTS results;

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
		return;
	}

}

/******************************************************************************
 * FunctionName : run_sample_fota
 * Description  : sample test for fota (firmware over the air)
 * Parameters   : WIFI_CONFIG
 * Returns      : 0 or -1 (0: success, -1: fail)
 *******************************************************************************/
nrc_err_t run_sample_fota()
{
	con_handle_t handle;
	httpc_data_t data;
	httpc_ret_e ret;
	ssl_certs_t *certs_ptr = NULL;

	char buf[CHUNK_SIZE];

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

	nrc_usr_print("[%s] Sample App for fota (firmware over the air) \n",__func__);

	if (!nrc_fota_is_support()) {
		nrc_usr_print("=========================================\n");
		nrc_usr_print("   Serial Flash does not support FOTA.\n");
		nrc_usr_print("=========================================\n");
		return NRC_FAIL;
	}

	data.data_in_length = CHUNK_SIZE;

	nrc_usr_print("=========================================\n");
	nrc_usr_print("STEP 1. Check firmware version in server (%s).\n", CHECK_VER_URL);
	nrc_usr_print("=========================================\n");
	while (1) {
		uint32_t data_size = 0;
		data.data_in = buf;
		memset(buf, 0, CHUNK_SIZE);
		ret = nrc_httpc_get(&handle, CHECK_VER_URL, NULL, &data, certs_ptr);

		while (ret == HTTPC_RET_OK) {
			data.data_in += data.recved_size;
			data_size += data.recved_size;
			ret = nrc_httpc_recv_response(&handle, &data);

			if (data.recved_size == 0) {
				break;
			}
		}
		nrc_httpc_close(&handle);

		nrc_usr_print("[HTTP Response Length] %d\n", data_size);
		nrc_usr_print("-----Recvd Data-----\n");
		nrc_usr_print("%s\n", buf);
		parse_response_version(buf, data_size);

		bool isLower = isCurrentVersionLower(&update_info.version);
		if (isLower || update_info.force) {
			break;
		}
		_delay_ms(10000);
	}

	nrc_usr_print("======================================================\n");
	nrc_usr_print("STEP 2. Erase flash area for OTA firmware to be downloaded.\n");
	nrc_usr_print("======================================================\n");
	nrc_usr_print("Erasing......");
	nrc_fota_erase();
	nrc_usr_print("Done.\n\n");

	nrc_usr_print("===========================================\n");
	nrc_usr_print("STEP 3. Download new firmware (%s) and update.\n", update_info.fw_url);
	nrc_usr_print("===========================================\n\n");
	data.data_in = buf;
	memset(buf, 0, CHUNK_SIZE);

	uint32_t total_length = 0;
	uint32_t body_content_length = 0;

	ret = nrc_httpc_get(&handle, update_info.fw_url, NULL, &data, certs_ptr);

	char* http_start_index = NULL;
	if ((http_start_index = strstr((char*)buf, "\r\n\r\n")) != NULL) {
		http_start_index += 4;
		body_content_length = data.recved_size - (http_start_index - data.data_in);

		nrc_usr_print("-- head content length: %d\n\n", http_start_index - buf);

		if (body_content_length > 0 ) { //In case of HTTP Server
			nrc_usr_print("-- body content length: %d\n\n", body_content_length);
			nrc_fota_write(total_length, (uint8_t*)http_start_index, body_content_length);
			//_delay_ms(50);
			total_length += body_content_length;
		}
	} else {
	//Read until all header data is received.
		while (ret == HTTPC_RET_OK) {
			data.data_in += data.recved_size;
			ret = nrc_httpc_recv_response(&handle, &data);

			if (data.recved_size == 2 && memcmp(data.data_in,"\r\n",2) == 0) {
				break;
			}
		}
	}

	nrc_usr_print("[HTTP Response Length] %d\n", data.data_in);
	nrc_usr_print("-----Recvd Data-----\n");
	nrc_usr_print("%s\n", buf);

	parse_response_content(buf);

	data.data_in = buf;
	memset(buf, 0, CHUNK_SIZE);

	FOTA_INFO fota_info;
	fota_info.fw_length = fw_len;
	fota_info.crc = update_info.crc;
	nrc_fota_set_info(fota_info.fw_length, fota_info.crc);

	system_wdt_deinit();
	uint32_t counter = 0;
	while (ret == HTTPC_RET_OK) {
		++counter;
		ret = nrc_httpc_recv_response(&handle, &data);

		if (data.recved_size > 0) {
			nrc_fota_write(total_length, (uint8_t*)data.data_in, data.recved_size);
			//_delay_ms(50);
			total_length += data.recved_size;
			data.recved_size = 0;
			nrc_usr_print("[%d] %d\n", counter, total_length);
			if (strstr(data.data_in, "\r\n\r\n") != NULL) {
				nrc_usr_print("last %d\n", counter);
				break;
			}
		}
	}
	system_wdt_init();
	nrc_httpc_close(&handle);

	nrc_usr_print("=================================================\n");
	nrc_usr_print("STEP 4. Firmware update to new one and reboot.\n");
	nrc_usr_print("=================================================\n\n");
	nrc_fota_update_done();

	while(1);

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
	VERSION_T app_version;
	nrc_uart_console_enable(true);

	app_version.major = SAMPLE_FOTA_MAJOR;
	app_version.minor = SAMPLE_FOTA_MINOR;
	app_version.patch = SAMPLE_FOTA_PATCH;
	nrc_set_app_version(&app_version);
	nrc_set_app_name(SAMPLE_FOTA_APP_NAME);

	memset(param, 0x0, WIFI_CONFIG_SIZE);
	nrc_wifi_set_config(param);
	connect_to_ap(param);
	run_sample_fota();
}

