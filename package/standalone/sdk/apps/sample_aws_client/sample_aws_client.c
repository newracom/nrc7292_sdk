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
#include "lwip/errno.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>

#include "aws_iot_config.h"
#include "aws_iot_log.h"
#include "aws_iot_version.h"
#include "aws_iot_mqtt_client_interface.h"
#include "wifi_config_setup.h"
#include "wifi_connect_common.h"

char rootCA[] =
"-----BEGIN CERTIFICATE-----\r\n"
"MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF\r\n"
"ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6\r\n"
"b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL\r\n"
"MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv\r\n"
"b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj\r\n"
"ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM\r\n"
"9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw\r\n"
"IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6\r\n"
"VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L\r\n"
"93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm\r\n"
"jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC\r\n"
"AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA\r\n"
"A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI\r\n"
"U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs\r\n"
"N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv\r\n"
"o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU\r\n"
"5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy\r\n"
"rqXRfboQnoZsG4q5WTP468SQvvG5\r\n"
"-----END CERTIFICATE-----\r\n";

char clientCRT[]=
"-----BEGIN CERTIFICATE-----\r\n"
"MIIDWjCCAkKgAwIBAgIVAMirgRSSKZfIio4qN0hFgOhOUf7zMA0GCSqGSIb3DQEB\r\n"
"CwUAME0xSzBJBgNVBAsMQkFtYXpvbiBXZWIgU2VydmljZXMgTz1BbWF6b24uY29t\r\n"
"IEluYy4gTD1TZWF0dGxlIFNUPVdhc2hpbmd0b24gQz1VUzAeFw0yMDAxMDYwMTQz\r\n"
"NDJaFw00OTEyMzEyMzU5NTlaMB4xHDAaBgNVBAMME0FXUyBJb1QgQ2VydGlmaWNh\r\n"
"dGUwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQC1uW5ufdZF8RfA+xGl\r\n"
"/fefTuzn9N8lMNd7EnfZkyXQwIlVatZRy9nWWsb+7LDDiD0ZFrCDZonuqmb+2YAr\r\n"
"OkYSnufFCAv809Shytll8o2CCTi/vXQWeajUY4IYhTL5eBNW2DIGDz045T7pVyvX\r\n"
"xwCABUtGOAew161cCZRRURiyKAbBI6X7Gcd7A/DizCbgXVK0yCZis13G2Squ/3TT\r\n"
"vPSQSWHkzWudvDWzO+pzLMMkN3Q/PYcrOELuz+GvOI32vCycKPkttnix1++MiIbh\r\n"
"uX8BOhJhXd57b0o8qFXVewz5RgFTjtfQoL2eQmQoox+TBDWcylWSDDKLWrwQ2SII\r\n"
"YQBxAgMBAAGjYDBeMB8GA1UdIwQYMBaAFAUYJVYcySBsFAhPAyrU/ou8tS5lMB0G\r\n"
"A1UdDgQWBBQf9eyFmqfMiBHM9B/CKDalsQijwzAMBgNVHRMBAf8EAjAAMA4GA1Ud\r\n"
"DwEB/wQEAwIHgDANBgkqhkiG9w0BAQsFAAOCAQEAj/2mqgBfGiQESlLkrTickcpb\r\n"
"9vOGxncYB+PhQjfH8ZOUtRAwfHefGbjxSHT48eLQjFofvmJRb8wLq1YC2iviC5jD\r\n"
"MgnJEO7n0hWQoBZc5jPymat7xNvtUy8oV6KqOwj6nlTQSlSjGa+tfEwZXRuBp/3r\r\n"
"uFgvCHEBuJB5c8xNIChWp0pm6llRLMVDPpRi83sALj0FsOPGy/6di7oqBRHpKEPY\r\n"
"yBVLjJfFluoDbFTNiRG8E+oCJ/I3QYyOLykE5tabDy/5f5SFxlkBaIg1xJtLMSWe\r\n"
"FkKTnL5700j+sgEKU3byLPRQ+A1vPfQb5tcge6eOX/HHEmorePDfNS0UecxM/g==\r\n"
"-----END CERTIFICATE-----\r\n";


char clientKey[] =
"-----BEGIN RSA PRIVATE KEY-----\r\n"
"MIIEogIBAAKCAQEAtblubn3WRfEXwPsRpf33n07s5/TfJTDXexJ32ZMl0MCJVWrW\r\n"
"UcvZ1lrG/uyww4g9GRawg2aJ7qpm/tmAKzpGEp7nxQgL/NPUocrZZfKNggk4v710\r\n"
"Fnmo1GOCGIUy+XgTVtgyBg89OOU+6Vcr18cAgAVLRjgHsNetXAmUUVEYsigGwSOl\r\n"
"+xnHewPw4swm4F1StMgmYrNdxtkqrv9007z0kElh5M1rnbw1szvqcyzDJDd0Pz2H\r\n"
"KzhC7s/hrziN9rwsnCj5LbZ4sdfvjIiG4bl/AToSYV3ee29KPKhV1XsM+UYBU47X\r\n"
"0KC9nkJkKKMfkwQ1nMpVkgwyi1q8ENkiCGEAcQIDAQABAoIBAFL2tVP8XzbPOuj/\r\n"
"tq7xo3s3rTC4uqdgBQBLDixm4XzMaeZ1QCNXzbvu9aqspIagTESH904GIjKz2RHA\r\n"
"eqNBl3woz+dEJTToAMtcsKO6eKBFVYM0Gyunn0xXoa1QIlWCXFHpoeziSDEReAdP\r\n"
"pNR+JdxMjnEgY2J5FMxQE0aMn5m1pzwml681lhswhfKJ9zQkCM723kSo0glsobr+\r\n"
"FJfuBcXEP00Fn6av5bGD32Ftx2nBFsit33U1BKWh2s3OKNuAGMSm0jwVME52t1ff\r\n"
"s3afHUw/24yY5GGqr4GZa/It0r8AwDwwVQBaOxU3w5EkwqRlUafuRqT5hoRUlf3D\r\n"
"o0eyxYECgYEA4Y8y6Cq/Gv8dcFUlxrSk/GA1aiQsbhCSFrEYVsplQTCPYeaGu2Wj\r\n"
"3KC2koiHgCrqX79GmTh2j5546s7qL/wncYyOhrmvsj3VdHJiDYTGWSPsnhVhDZNA\r\n"
"JUezIpasaL5d/ZpGHb3NtRZ2Tbk7dletq/qVXjGC6IUhitJy1TszE2MCgYEAzj/J\r\n"
"JR9eMAlliL2ztVZ5HcQC4Pil9Hv4ZykqEG6gALBpL8KL2bscWVWE7NER4h9JPgiM\r\n"
"oG/6+f/JrnW49DXrE7sA7NDHsikz/Qfh7YklWyhY4xP1RzvWSYpf3XKbrFWSUpp6\r\n"
"IxGZTR8Lq5QFvw6kG3Q1qbpXFgVxpKEFXfQkxxsCgYBfJWWm86krfSPw56oIuNjD\r\n"
"sN23SU0InKrTMZ7/tV6i8hX5iSGaWRSXE6AQDdGCqhe1jdz88wloKcP94Ix+81G8\r\n"
"Hztb72YCvPyWo01jQHfe0D2WrZQEAvJTB3y0AXT2th61xGcBUrg8RB5hSNqhX/jd\r\n"
"i9WIJ0B+TvJIOd/AvLkd2wKBgBx1erjPmjyAn2z92BU2iGHOESOfy65viLgbWv2H\r\n"
"djaCIRGBA7EYe3HsNfpDYvdTn9Sac2UT7oqee+LBxbKU2goP7LHAoT15J/5LHyaa\r\n"
"nPf2GkXDFD4vCIN/P3kb/lUKPbV+MUozfHbCyOZ091IEoCpVn63601driuHPgiET\r\n"
"thtvAoGAVkJwQnWcJ4D0KMaS2w8FD+Ch+B4W35TFHuJXsyRcay3fvIiL5t7r9ees\r\n"
"qvo8ibydgwC5dirdccY8TSBvX+pbkn7TkvsPZm1MEkrgGMfbZcsDZF44Tq+TaMB2\r\n"
"Vct5EDooE9Has4zRuYRxO1k1RT1qwfO+58RLNhi6p3b54ZpBTGU=\r\n"
"-----END RSA PRIVATE KEY-----\r\n";


#define HOST_ADDRESS_SIZE 255
/**
 * @brief Default cert location
 */
char certDirectory[PATH_MAX + 1] = "../../../certs";

/**
 * @brief Default MQTT HOST URL is pulled from the aws_iot_config.h
 */
char HostAddress[HOST_ADDRESS_SIZE] = AWS_IOT_MQTT_HOST;

/**
 * @brief Default MQTT port is pulled from the aws_iot_config.h
 */
uint32_t port = AWS_IOT_MQTT_PORT;

/**
 * @brief This parameter will avoid infinite loop of publish and exit the program after certain number of publishes
 */
uint32_t publishCount = 0;

void iot_subscribe_callback_handler(AWS_IoT_Client *pClient, char *topicName, uint16_t topicNameLen,
									IoT_Publish_Message_Params *params, void *pData) {
	IOT_UNUSED(pData);
	IOT_UNUSED(pClient);
	IOT_INFO("Subscribe callback");
	IOT_INFO("%.*s\t%.*s", topicNameLen, topicName, (int) params->payloadLen, (char *) params->payload);
}

void disconnectCallbackHandler(AWS_IoT_Client *pClient, void *data) {
	IOT_WARN("MQTT Disconnect");
	IoT_Error_t rc = FAILURE;

	if(NULL == pClient) {
		return;
	}

	IOT_UNUSED(data);

	if(aws_iot_is_autoreconnect_enabled(pClient)) {
		IOT_INFO("Auto Reconnect is enabled, Reconnecting attempt will start now");
	} else {
		IOT_WARN("Auto Reconnect not enabled. Starting manual reconnect...");
		rc = aws_iot_mqtt_attempt_reconnect(pClient);
		if(NETWORK_RECONNECTED == rc) {
			IOT_WARN("Manual Reconnect Successful");
		} else {
			IOT_WARN("Manual Reconnect Failed - %d", rc);
		}
	}
}

void parseInputArgsForConnectParams(int argc, char **argv) {
	int opt;

	while(-1 != (opt = getopt(argc, argv, "h:p:c:x:"))) {
		switch(opt) {
			case 'h':
				strncpy(HostAddress, optarg, HOST_ADDRESS_SIZE);
				IOT_DEBUG("Host %s", optarg);
				break;
			case 'p':
				port = atoi(optarg);
				IOT_DEBUG("arg %s", optarg);
				break;
			case 'c':
				strncpy(certDirectory, optarg, PATH_MAX + 1);
				IOT_DEBUG("cert root directory %s", optarg);
				break;
			case 'x':
				publishCount = atoi(optarg);
				IOT_DEBUG("publish %s times\n", optarg);
				break;
			case '?':
				if(optopt == 'c') {
					IOT_ERROR("Option -%c requires an argument.", optopt);
				} else if(isprint(optopt)) {
					IOT_WARN("Unknown option `-%c'.", optopt);
				} else {
					IOT_WARN("Unknown option character `\\x%x'.", optopt);
				}
				break;
			default:
				IOT_ERROR("Error in command line argument parsing");
				break;
		}
	}

}

#define LOCAL_PORT 8080

int subscribe_publish_sample(int cnt, int intval){
	bool infinitePublishFlag = true;
	char cPayload[100];

	int32_t i = 0;

	int count = cnt;
	int interval = intval;

	IoT_Error_t rc = FAILURE;

	AWS_IoT_Client client;
	IoT_Client_Init_Params mqttInitParams = iotClientInitParamsDefault;
	IoT_Client_Connect_Params connectParams = iotClientConnectParamsDefault;

	IoT_Publish_Message_Params paramsQOS0;
	IoT_Publish_Message_Params paramsQOS1;

	IOT_INFO("\nAWS IoT SDK Version %d.%d.%d-%s\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, VERSION_TAG);

	mqttInitParams.enableAutoReconnect = false; // We enable this later below
	mqttInitParams.pHostURL = HostAddress;
	mqttInitParams.port = port;
	mqttInitParams.pRootCALocation = rootCA;
	mqttInitParams.pDeviceCertLocation = clientCRT;
	mqttInitParams.pDevicePrivateKeyLocation = clientKey;
	mqttInitParams.mqttCommandTimeout_ms = 20000;
	mqttInitParams.tlsHandshakeTimeout_ms = 5000;
	mqttInitParams.isSSLHostnameVerify = true;
	mqttInitParams.disconnectHandler = disconnectCallbackHandler;
	mqttInitParams.disconnectHandlerData = NULL;

	rc = aws_iot_mqtt_init(&client, &mqttInitParams);
	if(SUCCESS != rc) {
		IOT_ERROR("aws_iot_mqtt_init returned error : %d ", rc);
		return rc;
	}

	connectParams.keepAliveIntervalInSec = 600;
	connectParams.isCleanSession = true;
	connectParams.MQTTVersion = MQTT_3_1_1;
	connectParams.pClientID = AWS_IOT_MQTT_CLIENT_ID;
	connectParams.clientIDLen = (uint16_t) strlen(AWS_IOT_MQTT_CLIENT_ID);
	connectParams.isWillMsgPresent = false;

	IOT_INFO("Connecting...");
	rc = aws_iot_mqtt_connect(&client, &connectParams);
	if(SUCCESS != rc) {
		IOT_ERROR("Error(%d) connecting to %s:%d", rc, mqttInitParams.pHostURL, mqttInitParams.port);
		return rc;
	}
	/*
	 * Enable Auto Reconnect functionality. Minimum and Maximum time of Exponential backoff are set in aws_iot_config.h
	 *  #AWS_IOT_MQTT_MIN_RECONNECT_WAIT_INTERVAL
	 *  #AWS_IOT_MQTT_MAX_RECONNECT_WAIT_INTERVAL
	 */
	rc = aws_iot_mqtt_autoreconnect_set_status(&client, true);
	if(SUCCESS != rc) {
		IOT_ERROR("Unable to set Auto Reconnect to true - %d", rc);
		return rc;
	}

	IOT_INFO("Subscribing...");
	rc = aws_iot_mqtt_subscribe(&client, "sdkTest/sub", 11, QOS0, iot_subscribe_callback_handler, NULL);
	if(SUCCESS != rc) {
		IOT_ERROR("Error subscribing : %d ", rc);
		return rc;
	}

	sprintf(cPayload, "%s : %d ", "hello from SDK", (int)i);

	paramsQOS0.qos = QOS0;
	paramsQOS0.payload = (void *) cPayload;
	paramsQOS0.isRetained = 0;

	paramsQOS1.qos = QOS1;
	paramsQOS1.payload = (void *) cPayload;
	paramsQOS1.isRetained = 0;

	if(count != 0) {
		infinitePublishFlag = false;
	}

	while((NETWORK_ATTEMPTING_RECONNECT == rc || NETWORK_RECONNECTED == rc || SUCCESS == rc)
		  && (count > 0 || infinitePublishFlag)) {

		//Max time the yield function will wait for read messages
		rc = aws_iot_mqtt_yield(&client, 100);
		if(NETWORK_ATTEMPTING_RECONNECT == rc) {
			// If the client is attempting to reconnect we will skip the rest of the loop.
			continue;
		}

		IOT_INFO("-->sleep");
		_delay_ms(1000);
		sprintf(cPayload, "%s : %d ", "hello from SDK QOS0", (int)i++);
		paramsQOS0.payloadLen = strlen(cPayload);
		rc = aws_iot_mqtt_publish(&client, "sdkTest/sub", 11, &paramsQOS0);
		if(count > 0) {
			count--;
		}

		if(count == 0 && !infinitePublishFlag) {
			break;
		}

		sprintf(cPayload, "%s : %d ", "hello from SDK QOS1", (int)i++);
		paramsQOS1.payloadLen = strlen(cPayload);
		rc = aws_iot_mqtt_publish(&client, "sdkTest/sub", 11, &paramsQOS1);
		if (rc == MQTT_REQUEST_TIMEOUT_ERROR) {
			IOT_WARN("QOS1 publish ack not received.\n");
			rc = SUCCESS;
		}
		if(count > 0) {
			count--;
		}

		_delay_ms(interval);
	}

	// Wait for all the messages to be received
	aws_iot_mqtt_yield(&client, 100);

	if(SUCCESS != rc) {
		IOT_ERROR("An error occurred in the loop.\n");
	} else {
		IOT_INFO("Publish done\n");
	}
	IOT_INFO("Disonnecting...");
	rc = aws_iot_mqtt_disconnect(&client);
	if(SUCCESS != rc) {
		IOT_ERROR("Error(%d) Disconnecting to %s:%d", rc, mqttInitParams.pHostURL, mqttInitParams.port);
		return rc;
	}

	return rc;
}

/******************************************************************************
 * FunctionName : run_sample_aws_client
 * Description  : sample test for aws client
 * Parameters   : WIFI_CONFIG
 * Returns      : 0 or -1 (0: success, -1: fail)
 *******************************************************************************/
int  run_sample_aws_client(WIFI_CONFIG *param)
{
	int i = 0;
	int count = 0;
	int interval =0;
	int network_index = 0;
	int wifi_state = WLAN_STATE_INIT;
	int result;

	nrc_usr_print("[%s] Sample App for aws client \n",__func__);

	count = param->count;
	interval = param->interval;

	if (wifi_init(param)!= WIFI_SUCCESS) {
		nrc_usr_print ("[%s] ASSERT! Fail for init\n", __func__);
		return RUN_FAIL;
	}

	/* 1st trial to connect */
	if (wifi_connect(param)!= WIFI_SUCCESS) {
		nrc_usr_print ("[%s] Fail for Wi-Fi connection (results:%d)\n", __func__);
		return RUN_FAIL;
	}

	if (nrc_wifi_get_state() != WLAN_STATE_GET_IP) {
		nrc_usr_print("[%s] Fail to connect or get IP !\n",__func__);
		return RUN_FAIL;
	}

	result = subscribe_publish_sample(count, interval);

 	wifi_state = nrc_wifi_get_state();
	if (wifi_state == WLAN_STATE_GET_IP || wifi_state == WLAN_STATE_CONNECTED) {
		nrc_usr_print("[%s] Trying to DISCONNECT... for exit\n",__func__);
		if (nrc_wifi_disconnect(network_index) != WIFI_SUCCESS) {
			nrc_usr_print ("[%s] Fail for Wi-Fi disconnection (results:%d)\n", __func__);
			return RUN_FAIL;
		}
	}

	if (SUCCESS != result)
		return RUN_FAIL;

	nrc_usr_print("[%s] End of run_sample_aws_client!! \n",__func__);
	return RUN_SUCCESS;
}


/******************************************************************************
 * FunctionName : user_init
 * Description  : Start Code for User Application, Initialize User function
 * Parameters   : none
 * Returns      : none
 *******************************************************************************/
void user_init(void)
{
	int ret = 0;
	WIFI_CONFIG* param;

	nrc_uart_console_enable();

	param = nrc_mem_malloc(WIFI_CONFIG_SIZE);
	memset(param, 0x0, WIFI_CONFIG_SIZE);

	set_wifi_config(param);
	ret = run_sample_aws_client(param);
	nrc_usr_print("[%s] test result!! %s \n",__func__, (ret==0) ?  "Success" : "Fail");
	if(param){
		nrc_mem_free(param);
	}
}
