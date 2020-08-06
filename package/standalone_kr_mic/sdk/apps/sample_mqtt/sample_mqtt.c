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

#include <string.h>
#include "nrc_sdk.h"
#include "MQTTClient.h"
#include "wifi_config_setup.h"
#include "wifi_connect_common.h"

#define BROKER_PORT 1883
#define BROKER_IP   "192.168.10.199"

#define USE_MQTTS
#if defined( USE_MQTTS )
#undef BROKER_PORT
#define BROKER_PORT 8883
#endif

#if defined( SUPPORT_MBEDTLS ) && defined( USE_MQTTS )
const char ca_cert[] =
"-----BEGIN CERTIFICATE-----\r\n"
"MIIDqjCCApKgAwIBAgIJAMS7T21cIG3NMA0GCSqGSIb3DQEBCwUAMGoxFzAVBgNV\r\n"
"BAMMDkFuIE1RVFQgYnJva2VyMRYwFAYDVQQKDA1Pd25UcmFja3Mub3JnMRQwEgYD\r\n"
"VQQLDAtnZW5lcmF0ZS1DQTEhMB8GCSqGSIb3DQEJARYSbm9ib2R5QGV4YW1wbGUu\r\n"
"bmV0MB4XDTE5MTEwMTA0NDA0N1oXDTMyMTAyODA0NDA0N1owajEXMBUGA1UEAwwO\r\n"
"QW4gTVFUVCBicm9rZXIxFjAUBgNVBAoMDU93blRyYWNrcy5vcmcxFDASBgNVBAsM\r\n"
"C2dlbmVyYXRlLUNBMSEwHwYJKoZIhvcNAQkBFhJub2JvZHlAZXhhbXBsZS5uZXQw\r\n"
"ggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDxLadbdZJi635QcgZIzODn\r\n"
"l2smAm5Yp56++DB7c0RLEYwAtWNffby64duLT1g3B97sDzPZtYmLipKFxgc78NU3\r\n"
"k9o1atV4F8aRIFrd9EDlQUMtpehG7cE3zemfDyaPlVHgMcY+XhZOamvQrZxikX38\r\n"
"usNWaPMkhlPLMciKZhCzGcq1JD1Li0FSyZ1uXTNhMulIw0KkQEiex3oV94QU85lV\r\n"
"hiG/rNT4kB5LVU1Xx3xmmZc2QcglxQ2dGXCvMyrzzqW4bQy2t9ZJYNia15u1ryye\r\n"
"ldImK1e3LukzEGf55JccC6YLExKzDt18EtB1Hvk8H2gNNmoEDGrhzEQo7bq6Ygap\r\n"
"AgMBAAGjUzBRMB0GA1UdDgQWBBRBZeQV8SL9moz8ZO9GBQ7K3HpRQDAfBgNVHSME\r\n"
"GDAWgBRBZeQV8SL9moz8ZO9GBQ7K3HpRQDAPBgNVHRMBAf8EBTADAQH/MA0GCSqG\r\n"
"SIb3DQEBCwUAA4IBAQBrLpykFoOIcal7UMO8A270d7pdXWtPLbDKEWNHL/zKaaqI\r\n"
"2HQQaOGMZS231d/Oxfnv6C5u/ESJJIAi7E1XYMwNq1eQkNKVDpP5UFLSFk4sI5D6\r\n"
"YsOyu85IdodPD+28XnriWvaYUgtnxXsIT4HwIXVzgkNG3/AnWprLp4HIt1ukkxFu\r\n"
"Em24L3lDlYYMJYTr4Rnu7AiodYkkloJyhzyJ8UBBhCuy0mUO59j6rIXDaZGL71ja\r\n"
"yKExVSv/h3i7WUiNKnI8sIneh9JvWXHYEA0lUULsQnEGx5N0AC2hmyWdh1xvCMHV\r\n"
"7EpoLTDRlWv50yFDARsLVMVdnVw5+Ef3G37J3rLj\r\n"
"-----END CERTIFICATE-----\r\n";

const char client_cert[] =
"-----BEGIN CERTIFICATE-----\r\n"
"MIIDBjCCAe4CCQCmB35rT9UYsjANBgkqhkiG9w0BAQsFADBFMQswCQYDVQQGEwJB\r\n"
"VTETMBEGA1UECAwKU29tZS1TdGF0ZTEhMB8GA1UECgwYSW50ZXJuZXQgV2lkZ2l0\r\n"
"cyBQdHkgTHRkMB4XDTE5MTAzMTEwNTIxNFoXDTIwMTAzMDEwNTIxNFowRTELMAkG\r\n"
"A1UEBhMCQVUxEzARBgNVBAgMClNvbWUtU3RhdGUxITAfBgNVBAoMGEludGVybmV0\r\n"
"IFdpZGdpdHMgUHR5IEx0ZDCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEB\r\n"
"ALAKfY9bE3PM6EvlSZRcAsBdYrwrXRadAqLfo7SWlTXoGC9RrI0IvK2lo2sWjBw1\r\n"
"isQEpZD0lxRpUPHIxMYcuOxsztphWYc3nxQZvcngsgREI7jJd/+VC5EyHn7k08bh\r\n"
"OydVOxgCe7Sl8Mhh6CGpn0DMjq35mSEElUmnGkuWwUXh5GREC0PUJyt0fGYDM1qd\r\n"
"eqy9dnvDnpnTUy3tRoWFE5LwhZt3VEH6anvSGGCFpqK0PAsTM9R12b0rqE8CAqM0\r\n"
"hIbHxOzjHennvZAomGQL+20Q7DCFM+K1q0wH6WPvkMIkiSNQ/TrmIfsI/S+u7wAz\r\n"
"V4abljVUqVvMHbOpWUWMUBMCAwEAATANBgkqhkiG9w0BAQsFAAOCAQEAgyfu7nsV\r\n"
"olez07yqCLvN0cwFjKApKw13yZ65CylmUDSM0JRF2yZ/AkG1M7LLCza0rqBrO4F6\r\n"
"S/YvKrbw3dCALPD0MZjHEJfjb5nlS303dAezlixEftugTANgadafXJJl1PLDALQC\r\n"
"5N0Bt2xKtzdbo/cTR1aAUOIJQDqjn/qMdylzG+Xp4bfAeo3Mco7f6lEV8AZdSmxa\r\n"
"EKXyEtfZI4H9MiMx9t76FUfyxGLGYNZRWBG+/IkHnEPbo8+ndyDalS90ymqIQsof\r\n"
"fnzLY+kHZZMQRGm780X/RkocCK4QJRYOdb45eZ+Imh6GhNXPect+BbL54yNph1Sx\r\n"
"eCSxYSAwvukuFw==\r\n"
"-----END CERTIFICATE-----\r\n";


const char client_pk[] =
"-----BEGIN RSA PRIVATE KEY-----\r\n"
"MIIEpQIBAAKCAQEAsAp9j1sTc8zoS+VJlFwCwF1ivCtdFp0Cot+jtJaVNegYL1Gs\r\n"
"jQi8raWjaxaMHDWKxASlkPSXFGlQ8cjExhy47GzO2mFZhzefFBm9yeCyBEQjuMl3\r\n"
"/5ULkTIefuTTxuE7J1U7GAJ7tKXwyGHoIamfQMyOrfmZIQSVSacaS5bBReHkZEQL\r\n"
"Q9QnK3R8ZgMzWp16rL12e8OemdNTLe1GhYUTkvCFm3dUQfpqe9IYYIWmorQ8CxMz\r\n"
"1HXZvSuoTwICozSEhsfE7OMd6ee9kCiYZAv7bRDsMIUz4rWrTAfpY++QwiSJI1D9\r\n"
"OuYh+wj9L67vADNXhpuWNVSpW8wds6lZRYxQEwIDAQABAoIBAF75OeZGb4cxDD4t\r\n"
"9HVa8o0PlL4J8w3JJWvzlaFPAC72CV42BQ6NoSVZ0IFsx6hMZpH8I8rBemSjsOzQ\r\n"
"sQBk7It45FxC2wctReCVRqQMWl4c2NzPrLKxmWz7CiLRl0Obrcs5m0kHxe+e+vlQ\r\n"
"gOjwVx+hfHR1zVxX2abDxVb0fU8Se44W5iTkCMSgI6Yx0b8kvPirVtyDfs8LlaTb\r\n"
"pDcImh1rJ5DHJ2t9vTo++PlMx5kz16ekB1oya86W0aXsbcNooo+EV1aW0zd/VJKh\r\n"
"zV8RUlxmoKAAUSzGJL0Xte8bsdTWneNXdl99Xp5SB3n08eb1LXIkQUaXTzPScvH9\r\n"
"6M+v+UECgYEA1vTiNzvHjfmlAkQ3ICtuZVJj0Nq7dO2ikYtE4DH0mme/GqbqJ2CO\r\n"
"XJy/H5bNxH/SJF4KO6gJTppIHEwRbzmToxYWNPN3zXNETmj2x2VTCXIGif57wwfY\r\n"
"8QCenDKCRH1K13//nWixavbI+Xw6jsJzqmbxZzGo0dHhm2O6nJ+l07MCgYEA0adn\r\n"
"w4KivQ8UKkKoeWIfxOE63w3RGqx8Bd+OFmMcYulW32dWCbk8jT7E+8iVVS38n1nU\r\n"
"jHpKDM/3hYTIKXjFxVbH4r19K64AhiCU1EwuzUnCNfOs6gBOs/Fo1KmzUhJpBwoH\r\n"
"MriGpZkuW0RTS6DepANOYRGN3jh/+Zhd7f6Y4iECgYEAxsesjkFU1+Edi8wDYldn\r\n"
"foFDVvd/VLEQniE5L05jEYqROhdS/9kVrWUyhQbTADzn2sOwGNzaxnp75tcdZErN\r\n"
"UYpyPMNNbYuDhjMgyuHTeS+eBmx0jLWsE6psAqvetXFcDY8LlcgkEXNoBPaxvCTs\r\n"
"C4+o+7H7mRTK2gTOpFoqYmsCgYEApkZ35DaSbREtdArj5UkuWw4qi8dAhUAxKNNR\r\n"
"tG2skMHxVUN2mEWiQX976Rj/XswXJCaxjSxb2GeELw/NVB7l84nNBAY25NKetcHX\r\n"
"Z7x4DfamofV9uSS/RrV659cjUj8prxyD8vUoTOP9QQMicMPIcBnzKscqVglHbiQq\r\n"
"stuOUyECgYEAoHU9uthRFRVYjAYUjcEv5+szfm7BDVzFjxfvmxsQG92eTv+LMXMn\r\n"
"d9cv57Uy15NyL6EO/3Zy2W2P+nx1ydqay/h04KSXW5ynGwNsRZfu+0qTZdtl56ic\r\n"
"ro3rBoq6RzhS6GxFw2MCuA81v8KUps4RubM3eht5bAfHUdZbiOCThFE=\r\n"
"-----END RSA PRIVATE KEY-----\r\n";


const char client_pk_pwd[] = "newratek2019";
#endif


void message_arrived(MessageData* data)
{
	nrc_usr_print("Message arrived on topic %.*s: %.*s\n", data->topicName->lenstring.len,
		data->topicName->lenstring.data, data->message->payloadlen, data->message->payload);
}

/******************************************************************************
 * FunctionName : run_sample_mqtt
 * Description  : sample test for mqtt
 * Parameters   : WIFI_CONFIG
 * Returns      : 0 or -1 (0: success, -1: fail)
 *******************************************************************************/
int  run_sample_mqtt(WIFI_CONFIG *param)
{
	int i = 0;
	int count = 0;
	int interval = 0;
	int network_index = 0;
	int wifi_state = WLAN_STATE_INIT;

	MQTTClient *mqtt_client = nrc_mem_malloc(sizeof(MQTTClient));
	memset(mqtt_client, 0x0, sizeof(MQTTClient));

	int message_count = 0;

	nrc_usr_print("[%s] Sample App for Mqtt \n",__func__);

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
		char payload[30];

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

	wifi_state = nrc_wifi_get_state();
	if (wifi_state == WLAN_STATE_GET_IP || wifi_state == WLAN_STATE_CONNECTED) {
		nrc_usr_print("[%s] Trying to DISCONNECT... for exit\n",__func__);
		if (nrc_wifi_disconnect(network_index) != WIFI_SUCCESS) {
			nrc_usr_print ("[%s] Fail for Wi-Fi disconnection (results:%d)\n", __func__);
			return RUN_FAIL;
		}
	}

	nrc_usr_print("[%s] exit \n",__func__);
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
	ret = run_sample_mqtt(param);
	nrc_usr_print("[%s] test result!! %s \n",__func__, (ret==0) ?  "Success" : "Fail");
	if(param){
		nrc_mem_free(param);
	}
}
