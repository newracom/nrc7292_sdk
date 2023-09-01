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
#include "MQTTClient.h"
#include "wifi_config_setup.h"
#include "wifi_connect_common.h"
#include "nvs.h"
#include "nvs_config.h"

//#define TEST_MOSQUITTO_ORG_SERVER
#define USE_MQTTS

#ifdef TEST_MOSQUITTO_ORG_SERVER
#define BROKER_IP   "test.mosquitto.org"
#else
#define BROKER_IP   "10.198.1.214"
#endif

#if defined( USE_MQTTS )
#define BROKER_PORT 8883
#else
#define BROKER_PORT 1883
#endif

#define GET_IP_RETRY_MAX 10
#define CONNECTION_RETRY_MAX 3
#define SET_DEFAULT_SCAN_CHANNEL_THRESHOLD 1

#if defined( SUPPORT_MBEDTLS ) && defined( USE_MQTTS )
const char ca_cert[] =
"-----BEGIN CERTIFICATE-----\n"
"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\n"
"usEtW9TFODpeDRmXEJBC2OD6GE3Sd9KCZA==\n"
"-----END CERTIFICATE-----\n";

const char client_cert[] =
"-----BEGIN CERTIFICATE-----\n"
"MIIIDzCCBfegAwIBAgIUJyoNipX8W6ybhv8TZ89ilexFiz8wDQYJKoZIhvcNAQEN\n"
"BQAwajEXMBUGA1UEAwwOQW4gTVFUVCBicm9rZXIxFjAUBgNVBAoMDU93blRyYWNr\n"
"cy5vcmcxFDASBgNVBAsMC2dlbmVyYXRlLUNBMSEwHwYJKoZIhvcNAQkBFhJub2Jv\n"
"ZHlAZXhhbXBsZS5uZXQwHhcNMjIxMjIwMTk0OTUyWhcNMjUwMzI0MTk0OTUyWjBn\n"
"MRQwEgYDVQQDDAtzYW1wbGVfbXF0dDEWMBQGA1UECgwNT3duVHJhY2tzLm9yZzEU\n"
"MBIGA1UECwwLZ2VuZXJhdGUtQ0ExITAfBgkqhkiG9w0BCQEWEm5vYm9keUBleGFt\n"
"cGxlLm5ldDCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAKsTEfAU7OF5\n"
"pqnyrElYHo0rlrYJ+CbKNBdHZK8xCxwcxssWOccr08p492E6pYRQAg7Vd4RcGVLa\n"
"Jh1zdbTyhg9gDNvRu5FdPbVVNx+a0kk0Ge2gaOxtrjltNDD+SzxxJJmoQwypjL8g\n"
"WciW2maqS+w4Rsd13KVe2mAUofz9ynPHOdZ/SQHf3Y/HY+f/ZzyJhGeD4aQZlidj\n"
"0CMSpSNkzTV/5TXHB6idxk6EFYNIuLZYT1M/mGKFSNU+pZcBKUtnFMOLcvDKB4cO\n"
"VOZTfkLtWhRGkUM1PUEGmEzj5LIK/SojGzZLq3UsBm1GeNsI1AvwIfFZmAGqvd2p\n"
"OlYkbHnZ3chkx0FQt9jh6IyWOLYGich3iWrfgyRGJ4MUIB4Im7mIV2zxET9xOb7n\n"
"3QJMwi+RnaxQay0w5qZexgi0jnFoPeXdR9Ko3TKIUxPSZzbbFEEyIq6JM71hizmT\n"
"1RnTshFFxXzW8iNpOix8fIUkvqLRLxy79Kk5Qtw5tNqV3u/KgcDhBN6aWC/lkdA8\n"
"8UDSAlNl/IE51/E+91XU7NZmdfo8bIVpwNqLA/0hDw+v9NXWOMlcHWOQHT/8ja+a\n"
"scb1IXy05J2woTrR/SRGQD6o26v3U9Lz3WMlcaU+8WiGESjI4mABTG4Z8P/PMT8l\n"
"q2fXpWoV5Js7vGMlzuCogwATm7QpA5i9AgMBAAGjggKuMIICqjAMBgNVHRMBAf8E\n"
"AjAAMBEGCWCGSAGG+EIBAQQEAwIGQDALBgNVHQ8EBAMCBeAwEwYDVR0lBAwwCgYI\n"
"KwYBBQUHAwEwIQYJYIZIAYb4QgENBBQWEkJyb2tlciBDZXJ0aWZpY2F0ZTAdBgNV\n"
"HQ4EFgQUeORJy34xVKv1GbcNMyPzh6bUXj4wgacGA1UdIwSBnzCBnIAUa459h/FA\n"
"r26TE+ATCjW+tz4/zCOhbqRsMGoxFzAVBgNVBAMMDkFuIE1RVFQgYnJva2VyMRYw\n"
"FAYDVQQKDA1Pd25UcmFja3Mub3JnMRQwEgYDVQQLDAtnZW5lcmF0ZS1DQTEhMB8G\n"
"CSqGSIb3DQEJARYSbm9ib2R5QGV4YW1wbGUubmV0ghRYSQQ2ByA0EeL8B1M/H4gb\n"
"x0FtmjCB7wYDVR0RBIHnMIHkhwQKxgHWhxD9tgWcXFYAAFQUwD/rTTGdhxD9tgWc\n"
"XFYAAMt35oW+kTDghxD9tgWcXFYAAKLvGmsYtashhxD+gAAAAAAAAAhwsh1rUhDo\n"
"hxD9tgWcXFYAAEDp2RByc10ThxD9tgWcXFYAADVDzskwAgWRhxD9tgWcXFYAAAAA\n"
"AAAAAAxFhxD9tgWcXFYAADLJkFTTtMS5hwTAqMj+hxD+gAAAAAAAAOTWMoz/tAIK\n"
"hwTAqHoBhwR/AAABhxAAAAAAAAAAAAAAAAAAAAABggtzYW1wbGVfbXF0dIIJbG9j\n"
"YWxob3N0MIGGBgNVHSAEfzB9MHsGAysFCDB0MBwGCCsGAQUFBwIBFhBodHRwOi8v\n"
"bG9jYWxob3N0MFQGCCsGAQUFBwICMEgwEBYJT3duVHJhY2tzMAMCAQEaNFRoaXMg\n"
"Q0EgaXMgZm9yIGEgbG9jYWwgTVFUVCBicm9rZXIgaW5zdGFsbGF0aW9uIG9ubHkw\n"
"DQYJKoZIhvcNAQENBQADggIBAEyhEWnLnDXhYtzHrTkdOyqqb5y6BM7c6PjtTP9b\n"
"SaYjbPwNAhFeyyZT4gqFt4DoY+KK+JtZK7zg7cgv9KdaCH6fyYXcOaVWetL2sN4W\n"
"G/wXXs2+rKEb09+8XQBR2uZQ3sRKSztaspEXNEi3mCg+u1ZyS711WBX9kj0RAG0X\n"
"o/jZWRMXixZrq3mOey9j+VvGEGDg2YELMmoY2ZgBjbJFPkgTD2UT1OdzA//pMN6i\n"
"I1Q2Kq7k4wDXya5881+z+LwVD5WOFZj1iTTxuLLHTfu0UDVBYh4EcWaySZBd4Vg2\n"
"PzEmJVPxyT6MMBO2+4IElp1uu1Ibn0otUZ2NYIggkfsUxM2b8UnWYQb3VUhnK22X\n"
"tp3qJLUdZqlZgVD/4zmtJX8AE2/cUIFtL09wwVbHgJ7othdvdMk3ZJTzSw7+ZMce\n"
"YVNjp0IEogYQMBQsboK4Uh8ek0su3KRide8dAbFz3PhKFOFswclP0mduLOsf0HNe\n"
"1XjtaOXz4DVfubtPsluJHIn3BIYBX6N1f72M/Ntd8ic/chO7UipX/kKiCPzZdFwF\n"
"1tIQU+c2Jac4fXiRHVsRDREhvX1uk90oDrYekK2ctmpJiatOP6Xvenkg+v2vG0wG\n"
"uI857JJ+0NgeBWgWxSw2oz7VnZ9JwvWugdKeUcXa4TtrRUFvAJbXcnhRq6sJfjB1\n"
"psaA\n"
"-----END CERTIFICATE-----\n";

const char client_pk[] =
"-----BEGIN RSA PRIVATE KEY-----\n"
"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\n"
"-----END RSA PRIVATE KEY-----\n";

const char client_pk_pwd[] = "";
#endif

static nvs_handle_t nvs_handle;

void message_arrived(MessageData* data)
{
	nrc_usr_print("Message arrived on topic %.*s: %.*s\n", data->topicName->lenstring.len,
		data->topicName->lenstring.data, data->message->payloadlen, data->message->payload);
}

static nrc_err_t connect_to_ap(WIFI_CONFIG *param)
{
	int retry_count = 0;
	int i = 0;
	uint8_t retry_cnt = 0;

	nrc_usr_print("[%s] Sample App for Wi-Fi  \n",__func__);

	/* set initial wifi configuration */
	wifi_init(param);

	/* connect to AP */
	while(1) {
		if (wifi_connect(param)== WIFI_SUCCESS) {
			nrc_usr_print ("[%s] connect to %s successfully !! \n", __func__, param->ssid);
			break;
		} else {
			nrc_usr_print ("[%s] Fail for connection %s\n", __func__, param->ssid);
			if (retry_cnt > CONNECTION_RETRY_MAX) {
				nrc_usr_print("(connect) Exceeded retry limit (%d). Run sw_reset\n", CONNECTION_RETRY_MAX);
				nrc_sw_reset();
				return NRC_FAIL;
			} else if(retry_cnt == SET_DEFAULT_SCAN_CHANNEL_THRESHOLD){
				if (nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READWRITE, &nvs_handle) == NVS_OK) {
					nrc_usr_print("[%s] NVS_WIFI_CHANNEL:%d...\n", __func__, 0);
					nvs_set_u16(nvs_handle, NVS_WIFI_CHANNEL, 0);
					nrc_set_default_scan_channel(param);
					nvs_close(nvs_handle);
				}
			}
			_delay_ms(1000);
			retry_cnt++;
		}
	}

	/* check if IP is ready */
	retry_cnt = 0;
	while(1){
		if (nrc_addr_get_state(0) == NET_ADDR_SET) {
			nrc_usr_print("[%s] IP ...\n",__func__);
			break;
		}
		if (++retry_cnt > GET_IP_RETRY_MAX) {
			nrc_usr_print("(Get IP) Exceeded retry limit (%d). Run sw_reset\n", GET_IP_RETRY_MAX);
			nrc_sw_reset();
		}
		_delay_ms(1000);
	}


	nrc_usr_print("[%s] Device is online connected to %s\n",__func__, param->ssid);
	return NRC_SUCCESS;
}

/******************************************************************************
 * FunctionName : run_sample_mqtt
 * Description  : sample test for mqtt
 * Parameters   : WIFI_CONFIG
 * Returns      : 0 or -1 (0: success, -1: fail)
 *******************************************************************************/
nrc_err_t run_sample_mqtt(WIFI_CONFIG *param)
{
	int count = 0;
	int interval = 0;
	SCAN_RESULTS results;
	uint32_t channel = 0;

	MQTTClient *mqtt_client = nrc_mem_malloc(sizeof(MQTTClient));
	memset(mqtt_client, 0x0, sizeof(MQTTClient));

	int message_count = 0;

	nrc_usr_print("[%s] Sample App for Mqtt \n",__func__);

	count = 10;
	interval = 1000;

	int i = 0;

	if (nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READONLY, &nvs_handle) == NVS_OK) {
		nvs_get_u16(nvs_handle, NVS_WIFI_CHANNEL, (uint16_t*)&channel);
		nrc_usr_print("[%s] channel:%d...\n", __func__, channel);
		if(channel){
			param->scan_freq_list[0] = channel;
			param->scan_freq_num = 1;
		}
	}

	if (connect_to_ap(param) == NRC_SUCCESS) {
		nrc_usr_print("[%s] Sending data to server...\n", __func__);
		AP_INFO ap_info;
		if (nrc_wifi_get_ap_info(0, &ap_info) == WIFI_SUCCESS) {
			nrc_usr_print("[%s] AP ("MACSTR" %s (len:%d) %c%c bw:%d ch:%d freq:%d security:%d)\n",
				__func__, MAC2STR(ap_info.bssid), ap_info.ssid, ap_info.ssid_len,
				ap_info.cc[0],ap_info.cc[1], ap_info.bw, ap_info.ch, ap_info.freq,
				ap_info.security);
			if (nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READWRITE, &nvs_handle) == NVS_OK) {
				nrc_usr_print("[%s] ap_info.freq:%d\n", __func__, ap_info.freq);
				nvs_set_u16(nvs_handle, NVS_WIFI_CHANNEL, ap_info.freq);
				nvs_close(nvs_handle);
			}
		}
	}

	unsigned char send_buf[80], read_buf[80];
	int rc = 0;
	Network network;
	MQTTPacket_connectData connectData = MQTTPacket_connectData_initializer;

	NetworkInit(&network);
	MQTTClientInit(mqtt_client, &network, 30000, send_buf, sizeof(send_buf), read_buf, sizeof(read_buf));

#if defined( SUPPORT_MBEDTLS ) && defined( USE_MQTTS )
	Certs cert;
	cert.ca_cert = ca_cert;
	cert.ca_cert_length = sizeof(ca_cert);
	cert.client_cert = client_cert;
	cert.client_cert_length = sizeof(client_cert);
	cert.client_pk = client_pk;
	cert.client_pk_length = sizeof(client_pk);
	cert.client_pk_pwd = client_pk_pwd;
	cert.client_pk_pwd_length = sizeof(client_pk_pwd) - 1;
#endif

#if defined( SUPPORT_MBEDTLS ) && defined( USE_MQTTS )
	if ((rc = NetworkConnectTLS(&network, BROKER_IP, BROKER_PORT, &cert)) != 0)
#else
	if ((rc = NetworkConnect(&network, BROKER_IP, BROKER_PORT)) != 0)
#endif
		nrc_usr_print("Return code from network connect is %d\n", rc);
	else
		nrc_usr_print("[%s] Network Connected\n", __func__);

	if ((rc = MQTTStartTask(mqtt_client)) != 1)
		nrc_usr_print("Return code from start tasks is %d\n", rc);

	connectData.MQTTVersion = 3;
	connectData.clientID.cstring = "nrc_11ah_mqtt_test";

	nrc_usr_print("[%s] Try to connect to MQTT Broker......\n", __func__);
	if ((rc = MQTTConnect(mqtt_client, &connectData)) != 0)
		nrc_usr_print("Return code from MQTT connect is %d\n", rc);
	else
		nrc_usr_print("[%s] MQTT Connected\n", __func__);

	if ((rc = MQTTSubscribe(mqtt_client, "halow/11ah/mqtt/sample/mytopic", 2, message_arrived)) != 0)
		nrc_usr_print("Return code from MQTT subscribe is %d\n", rc);

	for(i=0; i<count; i++) {
		MQTTMessage message;
		char payload[35];

		message.qos = 1;
		message.retained = 0;
		message.payload = payload;
		sprintf(payload, "message count %d", ++message_count);
		message.payloadlen = strlen(payload);

		if ((rc = MQTTPublish(mqtt_client, "halow/11ah/mqtt/sample/mytopic", &message)) != 0)
		nrc_usr_print("Return code from MQTT publish is %d\n", rc);

		_delay_ms(interval);
	}

	if ((rc = MQTTUnsubscribe(mqtt_client, "halow/11ah/mqtt/sample/mytopic")) != 0){
		nrc_usr_print("Return code from MQTT unsubscribe is %d\n", rc);
	}

	if ((rc = MQTTDisconnect(mqtt_client)) != 0){
		nrc_usr_print("Return code from MQTT disconnect is %d\n", rc);
	}else{
		nrc_usr_print("[%s] MQTT disconnected\n", __func__);
	}

#if defined( SUPPORT_MBEDTLS ) && defined( USE_MQTTS )
	if (NetworkDisconnectTLS(&network) != 0)
#else
	if (NetworkDisconnect(&network) != 0)
#endif
		nrc_usr_print("Network Disconnect Fail\n");
	else
		nrc_usr_print("[%s] Network Disonnected\n", __func__);

	vTaskDelete((TaskHandle_t)mqtt_client->thread.task);

	if (nrc_wifi_get_state(0) == WIFI_STATE_CONNECTED) {
		nrc_usr_print("[%s] Trying to DISCONNECT... for exit\n",__func__);
		if (nrc_wifi_disconnect(0, 5000) != WIFI_SUCCESS) {
			nrc_usr_print ("[%s] Fail for Wi-Fi disconnection (results:%d)\n", __func__);
			return NRC_FAIL;
		}
	}

	nrc_usr_print("[%s] exit \n",__func__);
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
	nrc_err_t ret;
	nrc_uart_console_enable(true);
	int count=  10;

	if(param == NULL)
		return;
	memset(param, 0x0, WIFI_CONFIG_SIZE);
	nrc_wifi_set_config(param);

	ret = run_sample_mqtt(param);
	nrc_usr_print("[%s] test result!! %s \n",__func__, (ret==0) ?  "Success" : "Fail");
}

