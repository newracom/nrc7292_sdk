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

#include <stdlib.h>
#include <string.h>

#include "nrc_sdk.h"
#include "lwip/ip_addr.h"
#include "wifi_config_setup.h"
#include "wifi_connect_common.h"

#include "cJSON.h"

#define RUN_HTTPS
#if defined( SUPPORT_MBEDTLS ) && defined( RUN_HTTPS )
#define SERVER_URL "https://192.168.10.199:4443/"
#else
#define SERVER_URL "http://10.198.1.214:8080/"
#endif

#define CURRENT_FW_VER 105
#define CHECK_VER SERVER_URL "version"
#define CHUNK_SIZE 2048

#if defined( SUPPORT_MBEDTLS )
static const char ssl_server_ca_crt[] =
"-----BEGIN CERTIFICATE-----\r\n"
"MIIDBjCCAe4CCQCWOp1NpvxjzDANBgkqhkiG9w0BAQsFADBFMQswCQYDVQQGEwJB\r\n"
"VTETMBEGA1UECAwKU29tZS1TdGF0ZTEhMB8GA1UECgwYSW50ZXJuZXQgV2lkZ2l0\r\n"
"cyBQdHkgTHRkMB4XDTE5MTEwMTA3MDI0MFoXDTIwMTAzMTA3MDI0MFowRTELMAkG\r\n"
"A1UEBhMCQVUxEzARBgNVBAgMClNvbWUtU3RhdGUxITAfBgNVBAoMGEludGVybmV0\r\n"
"IFdpZGdpdHMgUHR5IEx0ZDCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEB\r\n"
"AK3+RdJg0gdmjwXlGU8mpmjZFqs6etKPoqQifP+U6jbJ75Po7qwazdUsaW2aSyBo\r\n"
"zzA3kHYGwPTKL8m4ptcGmkcEDN2oLolcdtxpyBTA3Zg2qgaArdfwdWamN2NZp+cD\r\n"
"zBk0Hh6ygSxq8h16oq84eqN642ox+Kzh+WjhBFI58qGp+blffdZ89aMDwaFUBI9y\r\n"
"zW9/c5RoJQ4CjtFDvP8N4NXzRGdf5ydXPjbS0sFVPe/scP8u/KgynCTix8CRBGc5\r\n"
"hMow8Mxti2Yp4wl5fCKD3eEMuivEA16wexHrrOp5ZyUsT92flSjC96GfqY8XZw4H\r\n"
"KiVEi/O6ThaJ2GdpDe2adB8CAwEAATANBgkqhkiG9w0BAQsFAAOCAQEAKMHf6/Xb\r\n"
"XDKCippqiDI83xz3rb0EEt/VUkgEiVRHKBwztGT94djOPn5Yh5+Gdfhfp3WUItZ2\r\n"
"Djpii9GKHXXzIbW9qtjJHdhh9FDMbXxkjFBdEiVRkfTVKNy84N3DlVCUGcFC//lF\r\n"
"p7UC3vD6xrQtKyTQ8H4cqSrSkev2xmplfy82nZ/5O+6dew4FQ5xjr0FkyvVQqRrZ\r\n"
"NTr9ApyxCAxFkjM+dHs7Ei0aIOzHA3qXItlRtd8L9JD2jr2sGRi5KreyUyx9eZ/t\r\n"
"Ne0MdQ9sfCEfBQ07em5i4RBIfRuUjWj9qOJ0qbLVc01+aFe+Nj6OIV0lFblspRYw\r\n"
"Mjqoi7TUHGGfzA==\r\n"
"-----END CERTIFICATE-----\r\n";



static const char ssl_client_ca_crt[] =
"-----BEGIN CERTIFICATE-----\r\n"
"MIIDBjCCAe4CCQCSl2yS9eWWkzANBgkqhkiG9w0BAQsFADBFMQswCQYDVQQGEwJB\r\n"
"VTETMBEGA1UECAwKU29tZS1TdGF0ZTEhMB8GA1UECgwYSW50ZXJuZXQgV2lkZ2l0\r\n"
"cyBQdHkgTHRkMB4XDTE5MTEwMTExMDM1MloXDTIwMTAzMTExMDM1MlowRTELMAkG\r\n"
"A1UEBhMCQVUxEzARBgNVBAgMClNvbWUtU3RhdGUxITAfBgNVBAoMGEludGVybmV0\r\n"
"IFdpZGdpdHMgUHR5IEx0ZDCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEB\r\n"
"AOCI9MV2dlppCfC135BqEpJzd1UPxbrE10pbGOHgALfbs0afd23sd/b0b04G7iGU\r\n"
"O2VZ5uSSCq5iNxS+97JeqUbIJP9ELIz9xEO3H5cdz8TRSNFoKeWlULA6ir53MGb+\r\n"
"8EnEGVCFF0AlvfZ/jkJ0XS/fL6ozW2wkBi05kJ84rXnIwwBBLNCLZ6NPtSZ9DltB\r\n"
"o/3eXpEpiNh26+aIldodKpekA5dUm7xThrh7YqE7kb7ZLeifuYtHQs+BUgF3hPtJ\r\n"
"Hg1ClftVBHg6N70lvvX5Z/bMmaOwFLJJvMEaGxt26xJ3YckqYioah1PBmuhhozXS\r\n"
"cWA51f8BatFbqquBeFP8StMCAwEAATANBgkqhkiG9w0BAQsFAAOCAQEAdwZ25Jfh\r\n"
"Al6fODpb61/ZnnpWdrsx9K14AgnmQ+zG4u9+Blj1KwSUZ82tsSL5+eZw1dqPd0D+\r\n"
"piP3mdnqXoxC6iJo0KhNtBk7TWAZ7NfwsjkUyZyY1UfZygEe2zPdQsNCz1leoxw0\r\n"
"buPoUPHmbhhB4/lXSVadP6oqOM1L0OEOOCakooTAVxW8y/0/ro95ijGGVoTKNTwA\r\n"
"3P6flw/+DhoDjTFcZXoIliucbWnZV7LwPdk8BzJPs3ClWf2UV+4prgdRFlCENqlO\r\n"
"0VLOlVyjuGtqKS+j/ETamGwZGVP/lJ9cI5omHmn4BHypd7xQSHrpBsWjInmxIfsG\r\n"
"vepVtWNFpNrEBg==\r\n"
"-----END CERTIFICATE-----\r\n";


static const char ssl_client_key[] =
"-----BEGIN RSA PRIVATE KEY-----\r\n"
"MIIEowIBAAKCAQEA4Ij0xXZ2WmkJ8LXfkGoSknN3VQ/FusTXSlsY4eAAt9uzRp93\r\n"
"bex39vRvTgbuIZQ7ZVnm5JIKrmI3FL73sl6pRsgk/0QsjP3EQ7cflx3PxNFI0Wgp\r\n"
"5aVQsDqKvncwZv7wScQZUIUXQCW99n+OQnRdL98vqjNbbCQGLTmQnzitecjDAEEs\r\n"
"0Itno0+1Jn0OW0Gj/d5ekSmI2Hbr5oiV2h0ql6QDl1SbvFOGuHtioTuRvtkt6J+5\r\n"
"i0dCz4FSAXeE+0keDUKV+1UEeDo3vSW+9fln9syZo7AUskm8wRobG3brEndhySpi\r\n"
"KhqHU8Ga6GGjNdJxYDnV/wFq0Vuqq4F4U/xK0wIDAQABAoIBAGseq7fw9jHX3tgp\r\n"
"zIi3MjkQQSQhrDGYayWcJFjOZ0lP1U2iEnYs1GbK4rcU81Ktx1Bo/ZCaY+IiFSke\r\n"
"mklMg/Gy1oO54I87GgE8QiP0IwVA2z6cNTDMF5ybsUmAz2Szx6tJlNInTJpb5y7M\r\n"
"V/A4V6TZE4JdkgYbgZ7d0bNEdO6eBg/z9BhqJ4Zl6VtEUXx4gMobOkdxrneqE3QY\r\n"
"wAMt9QkkTadsdQ1nPhouKbs3yqIhKWb7cd38D+dqGeYHBCDxoHxebUWkJNPE5h5Q\r\n"
"/OaVYlxhr1NMxP9a1p6A9xevpFYVJD7RK/GQluIdUrJqf6C6yClw2yP0x7LCGjq4\r\n"
"LPpYyYECgYEA+KbR5jGacSd3TZe/nIov+q/bwUaJaZEY5K4YyMZxJFJ1+ILUANY/\r\n"
"KLAPztDYq6wucriQBMYWzJ6iS3MRZFwfI6FAbebeX3TT8i9KPgHVhBr7QwOzVFtx\r\n"
"yTnUe5XrmRNl1QzZCEyriMZ7uCdbvkzBS16xajpsGetaYp1gm3C+rrMCgYEA5yuu\r\n"
"WrXya7ZX+PrSXdDKascTMyOm3qIZom1YPFVWo+jHFVEsUN2gmIPTHwMGDNnckOeb\r\n"
"00hf/BDbWMLCuvIkJzWRgvr83yW9zN9M+g4LSUImCRQOkMiOo0sUH3my2dFmt3W+\r\n"
"X6fp5/Ket4IU7Tgb1Z3DqcaU7QLPtFLzpvmQA2ECgYEAqzMYxBiVEKGut+Lqj9pp\r\n"
"TH42nS12wROhAxqHf/15uxt3lEJnu6fH1rjaOXh8Jj8nv98pcc/9tKbocXBpoiL3\r\n"
"Ya3N0Z2qsCidIVvED0tt+kYlh6+NkmBfyL+jd+/yRfQgIf91kwxO8p5OYq3esfjh\r\n"
"AYbSOqS891+fXNSkxoFrGJcCgYA/6zMNf+uk3slaXbgXGqktdxgW9s+oFXgzEjro\r\n"
"i8wmDDIn8cboIS/Lm/+fPo3IteCn7HKIrCVmJB8SXt/LIzLd6JDwf4e2B9CAOmol\r\n"
"Zga23eR4dCRG4j2WZycMQPE0CxN0vMjD2EDz0oESSpSQtwfzO+kjI3aARlu6B4m5\r\n"
"bJ3mYQKBgGxksP3SErSRNbnjCDk0ywio3BR2latOOcRoRJDx9tufMEx3ntQgdGKI\r\n"
"iBuGyHuy7FeJBK755T4xY3bDq59Q8gTcfrJjHldQPsU5J2jUHTB5vNWf+E3+nm6T\r\n"
"YjruhiS//W+Cn/bNrzTB0wEHck4OEFqXbBtqQZaWTUs3nCgAJRK4\r\n"
"-----END RSA PRIVATE KEY-----\r\n";

#endif

typedef struct {
	int version;
	uint32_t crc;
	char fw_url[128];
} update_info_t;

update_info_t update_info = {0, 0, {'\0'}};

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
			update_info.version  = atoi(version);
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

		nrc_usr_print("[%s] version: %d,  crc: %x  fw_url: %s\n",
			__func__,  update_info.version, update_info.crc, update_info.fw_url);
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
 * FunctionName : run_sample_fota
 * Description  : sample test for fota (firmware over the air)
 * Parameters   : WIFI_CONFIG
 * Returns      : 0 or -1 (0: success, -1: fail)
 *******************************************************************************/
nrc_err_t run_sample_fota(WIFI_CONFIG *param)
{
	nrc_usr_print("[%s] Sample App for fota (firmware over the air) \n",__func__);
	tWIFI_STATE_ID wifi_state = WIFI_STATE_INIT;
  	int network_index = 0;
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
		if (nrc_wifi_scan() == WIFI_SUCCESS){
			if (nrc_wifi_scan_results(&results)== WIFI_SUCCESS) {
				/* Find the ssid in scan results */
				for(i=0; i<results.n_result ; i++){
					if(strcmp((char*)param->ssid, (char*)results.result[i].ssid)== 0 ){
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

	nrc_wifi_get_network_index(&network_index );

	/* check the IP is ready */
	while(1){
		nrc_wifi_get_state(&wifi_state);
		if (wifi_state == WIFI_STATE_GET_IP) {
			nrc_usr_print("[%s] IP ...\n",__func__);
			break;
		} else{
			nrc_usr_print("[%s] Current State : %d...\n",__func__, wifi_state);
		}
		_delay_ms(1000);
	}

#if defined( SUPPORT_MBEDTLS ) && defined( RUN_HTTPS )
	ssl_certs_t certs;
	certs.client_cert = ssl_client_ca_crt;
	certs.client_cert_length = sizeof(ssl_client_ca_crt);
	certs.client_pk = ssl_client_key;
	certs.client_pk_length = sizeof(ssl_client_key);
	certs.server_cert = ssl_server_ca_crt;
	certs.server_cert_length = sizeof(ssl_server_ca_crt);
#endif

	if (!nrc_fota_is_support()) {
		nrc_usr_print("=========================================\n");
		nrc_usr_print("   Serial Flash does not support FOTA.\n");
		nrc_usr_print("=========================================\n");
		return NRC_FAIL;
	}

	con_handle_t handle0, handle1, handle2;
	httpc_data_t data;
	httpc_ret_e ret;

	char* buf = (char*)nrc_mem_malloc(CHUNK_SIZE);
	memset(buf, 0x0, CHUNK_SIZE);

	data.data_in_length = CHUNK_SIZE;

	nrc_usr_print("=========================================\n");
	nrc_usr_print("STEP 1. Check firmware version in server.\n");
	nrc_usr_print("=========================================\n");
	while (1) {
		uint32_t data_size = 0;
		data.data_in = buf;
		memset(buf, 0, CHUNK_SIZE);

#if defined( SUPPORT_MBEDTLS ) && defined( RUN_HTTPS )
		ret = nrc_httpc_get(&handle0, CHECK_VER, NULL, &data, &certs);
#else
		ret = nrc_httpc_get(&handle0, CHECK_VER, NULL, &data, NULL);
#endif

		while (ret == HTTPC_RET_OK) {
			data.data_in += data.recved_size;
			data_size += data.recved_size;
			ret = nrc_httpc_recv_response(&handle0, &data);

			if (data.recved_size == 0) {
				break;
			}
		}
		nrc_httpc_close(&handle0);

		nrc_usr_print("[HTTP Response Length] %d\n", data_size);
		nrc_usr_print("-----Recvd Data-----\n");
		nrc_usr_print("%s\n", buf);
		parse_response_version(buf, data_size);

		if (update_info.version > CURRENT_FW_VER)
			break;

		_delay_ms(10000);
	}

	nrc_usr_print("======================================================\n");
	nrc_usr_print("STEP 2. Erase flash area for OTA firmware to be downloaded.\n");
	nrc_usr_print("======================================================\n");
	nrc_usr_print("Erasing......");
	nrc_fota_erase(0, 1024*1024);
	nrc_usr_print("Done.\n\n");

	nrc_usr_print("===========================================\n");
	nrc_usr_print("STEP 3. Download new firmware and update.\n");
	nrc_usr_print("===========================================\n\n");
	data.data_in = buf;
	memset(buf, 0, CHUNK_SIZE);

	uint32_t total_length = 0;
	uint32_t body_content_length = 0;

#if defined( SUPPORT_MBEDTLS ) && defined( RUN_HTTPS )
	ret = nrc_httpc_get(&handle2, update_info.fw_url, NULL, &data, &certs);
#else //HTTP
	ret = nrc_httpc_get(&handle2, update_info.fw_url, NULL, &data, NULL);
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
			ret = nrc_httpc_recv_response(&handle2, &data);

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
		ret = nrc_httpc_recv_response(&handle2, &data);

		if (data.recved_size > 0) {
			nrc_fota_write(total_length, (uint8_t*)data.data_in, data.recved_size);
			total_length += data.recved_size;
			data.recved_size = 0;
			nrc_usr_print("[%d] %d\n", counter, total_length);
			if (strstr(data.data_in, "\r\n\r\n") != NULL) {
				nrc_usr_print("last %d\n", counter);
				break;
			}
		}
	}
	nrc_httpc_close(&handle2);

	nrc_usr_print("=================================================\n");
	nrc_usr_print("STEP 4. Firmware update to new one and reboot.\n");
	nrc_usr_print("=================================================\n\n");
	nrc_fota_update_done(&fota_info);

	while(1);

	return NRC_SUCCESS;
}


/******************************************************************************
 * FunctionName : user_init
 * Description  : Start Code for User Application, Initialize User function
 * Parameters   : none
 * Returns      : none
 *******************************************************************************/
void user_init(void)
{
	nrc_err_t ret;
	WIFI_CONFIG* param;

	nrc_uart_console_enable(true);

	param = nrc_mem_malloc(WIFI_CONFIG_SIZE);
	memset(param, 0x0, WIFI_CONFIG_SIZE);

	set_wifi_config(param);
	ret = run_sample_fota(param);
	nrc_usr_print("[%s] test result!! %s \n",__func__, (ret==0) ?  "Success" : "Fail");
	if(param){
		nrc_mem_free(param);
	}
}

