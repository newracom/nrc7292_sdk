/*
 * MIT License
 *
 * Copyright (c) 2023 Newracom, Inc.
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
#include "lwip/ip_addr.h"
#include "wifi_config_setup.h"
#include "wifi_connect_common.h"
#include "sample_softap_uart_tcp_server_version.h"

#include "cJSON.h"

#include "nrc_fota.h"

typedef struct{
	uint8_t major;
	uint8_t minor;
	uint8_t patch;
} version_t;

typedef struct {
	version_t version;
	uint8_t force_update;
	uint32_t crc;
	char fw_url[128];
} update_info_t;

update_info_t update_info;

version_t getCurrentVersion() {
    version_t currentVersion;

    currentVersion.major = SAMPLE_SOFTAP_UART_TCP_SERVER_MAJOR;
    currentVersion.minor = SAMPLE_SOFTAP_UART_TCP_SERVER_MINOR;
    currentVersion.patch = SAMPLE_SOFTAP_UART_TCP_SERVER_PATCH;
    return currentVersion;
}

void parseVersionString(const char* versionString, size_t versionStringLen, version_t* parsedVersion) {
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

 bool isCurrentVersionLower(version_t* parsedVersion) {
	 version_t targetVersion = *parsedVersion;
	 version_t currentVersion = getCurrentVersion();

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
	char *force_update = NULL;

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

		if (get_json_str_value(cjson, "force_update", &force_update)) {
			nrc_usr_print("[%s] force_update : %s\n", __func__, force_update);
			update_info.force_update = atoi(force_update);
		} else {
			goto exit;
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

		if (force_update)
			free(force_update);

		nrc_usr_print("[%s] version: %d.%d.%d,  crc: %x  fw_url: %s force_update: %s\n",
			__func__,  update_info.version.major, update_info.version.minor,update_info.version.patch,
			update_info.crc, update_info.fw_url, update_info.force_update);
	}
}

uint32_t fw_len = 0;
void parse_response_content (char * data )
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

/******************************************************************************
 * FunctionName : nrc_fota_start
 * Description  : firmware over the air
 * Returns      : 0 success, -1 failure
 *******************************************************************************/
int nrc_fota_start(char *url, unsigned int length)
{
	con_handle_t httpc_handle;
	httpc_data_t data;
	httpc_ret_e ret;
	uint32_t data_size = 0;
	char* uri = NULL;
	char* buf = (char*)nrc_mem_malloc(CHUNK_SIZE);

	nrc_usr_print("[%s] Starting fota (firmware over the air) \n",__func__);
	memset(&update_info, 0, sizeof(update_info));

#if defined( SUPPORT_MBEDTLS ) && defined( RUN_HTTPS )
	ssl_certs_t certs;
	certs.client_cert = ssl_client_ca_crt;
	certs.client_cert_length = sizeof(ssl_client_ca_crt);
	certs.client_pk = ssl_client_key;
	certs.client_pk_length = sizeof(ssl_client_key);
	certs.ca_cert = ssl_server_ca_crt;
	certs.ca_cert_length = sizeof(ssl_server_ca_crt);
#endif

	if (!nrc_fota_is_support()) {
		nrc_usr_print("=========================================\n");
		nrc_usr_print("   Serial Flash does not support FOTA.\n");
		nrc_usr_print("=========================================\n");
		return -1;
	}

	memset(buf, 0x0, CHUNK_SIZE);

	data.data_in_length = CHUNK_SIZE;

	nrc_usr_print("=========================================\n");
	nrc_usr_print("STEP 1. Check firmware version in server.\n");
	nrc_usr_print("=========================================\n");
	data.data_in = buf;
	memset(buf, 0, CHUNK_SIZE);

	uri = (char *) nrc_mem_malloc(length + strlen(FW_SERVER_VERSION) + 2);
	if (!uri) {
		return -1;
	}

	sprintf(uri, "%s/%s", url, FW_SERVER_VERSION);

#if defined( SUPPORT_MBEDTLS ) && defined( RUN_HTTPS )
	ret = nrc_httpc_get(&httpc_handle, uri, NULL, &data, &certs);
#else
	ret = nrc_httpc_get(&httpc_handle, uri, NULL, &data, NULL);
#endif

	nrc_mem_free(uri);

	if (ret < 0) {
		return -1;
	}

	while (ret == HTTPC_RET_OK) {
		data.data_in += data.recved_size;
		data_size += data.recved_size;
		ret = nrc_httpc_recv_response(&httpc_handle, &data);

		if (data.recved_size == 0) {
			break;
		}
	}
	nrc_httpc_close(&httpc_handle);

	nrc_usr_print("[HTTP Response Length] %d\n", data_size);
	nrc_usr_print("-----Recvd Data-----\n");
	nrc_usr_print("%s\n", buf);
	parse_response_version(buf, data_size);

	bool isLower = isCurrentVersionLower(&update_info.version);
	if (!isLower && !update_info.force_update) {
		nrc_usr_print("Current version is same or higher than the target version.\n");
		return -1;
	}

	nrc_usr_print("======================================================\n");
	nrc_usr_print("STEP 2. Erase flash area for OTA firmware to be downloaded.\n");
	nrc_usr_print("======================================================\n");
	nrc_usr_print("Erasing......");
	nrc_fota_erase();
	nrc_usr_print("Done.\n\n");

	nrc_usr_print("===========================================\n");
	nrc_usr_print("STEP 3. Download new firmware and update.\n");
	nrc_usr_print("===========================================\n\n");
	data.data_in = buf;
	memset(buf, 0, CHUNK_SIZE);

	uint32_t total_length = 0;
	uint32_t body_content_length = 0;

#if defined( SUPPORT_MBEDTLS ) && defined( RUN_HTTPS )
	ret = nrc_httpc_get(&httpc_handle, update_info.fw_url, NULL, &data, &certs);
#else //HTTP
	ret = nrc_httpc_get(&httpc_handle, update_info.fw_url, NULL, &data, NULL);
#endif

	char* http_start_index = NULL;
	if ((http_start_index = strstr((char*)buf, "\r\n\r\n")) != NULL) {
		http_start_index += 4;
		body_content_length = data.recved_size - (http_start_index - data.data_in);

		nrc_usr_print("-- head content length: %d\n\n", http_start_index - buf);

		if (body_content_length > 0 ) { //In case of HTTP Server
			nrc_usr_print("-- body content length: %d\n\n", body_content_length);
			nrc_fota_write(total_length, (uint8_t*)http_start_index, body_content_length);
			total_length += body_content_length;
		}
	} else {
		//Read until all header data is received.
		while (ret == HTTPC_RET_OK) {
			data.data_in += data.recved_size;
			ret = nrc_httpc_recv_response(&httpc_handle, &data);
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

	uint32_t counter = 0;
	while (ret == HTTPC_RET_OK) {
		++counter;
		ret = nrc_httpc_recv_response(&httpc_handle, &data);

		if (data.recved_size > 0) {
			nrc_fota_write(total_length, (uint8_t*)data.data_in, data.recved_size);
			total_length += data.recved_size;
			data.recved_size = 0;
			nrc_usr_print(".");
			if (strstr(data.data_in, "\r\n\r\n") != NULL) {
				nrc_usr_print("last %d\n", counter);
				break;
			}
		}
	}
	nrc_usr_print("\n");
	nrc_httpc_close(&httpc_handle);
	nrc_usr_print("[total blocks %d] Downloaded bytes: %d\n", counter, total_length);
	nrc_usr_print("=================================================\n");
	nrc_usr_print("STEP 4. Firmware update to new one and reboot.\n");
	nrc_usr_print("=================================================\n\n");
	nrc_fota_update_done();

	return 0;
}

