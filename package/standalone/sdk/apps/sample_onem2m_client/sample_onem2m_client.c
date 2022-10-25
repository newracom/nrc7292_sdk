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
#include "cJSON.h"
#include "wifi_config_setup.h"
#include "wifi_connect_common.h"

#define MOBIUS_MQTT_BROKER_PORT 1883
#define MOBIUS_MQTT_BROKER_IP   "192.168.10.199"

#define USE_MQTTS
#if defined( USE_MQTTS )
#undef MOBIUS_MQTT_BROKER_PORT
#define MOBIUS_MQTT_BROKER_PORT 8883
#endif

#if defined( SUPPORT_MBEDTLS ) && defined( USE_MQTTS )
static const char ca_cert[] =
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

static const char client_cert[] =
"-----BEGIN CERTIFICATE-----\r\n"
"MIIDBjCCAe4CCQDOfHUh/FAWyjANBgkqhkiG9w0BAQsFADBFMQswCQYDVQQGEwJB\r\n"
"VTETMBEGA1UECAwKU29tZS1TdGF0ZTEhMB8GA1UECgwYSW50ZXJuZXQgV2lkZ2l0\r\n"
"cyBQdHkgTHRkMB4XDTIxMDQxNTAyNDUzNFoXDTIyMDQxNTAyNDUzNFowRTELMAkG\r\n"
"A1UEBhMCQVUxEzARBgNVBAgMClNvbWUtU3RhdGUxITAfBgNVBAoMGEludGVybmV0\r\n"
"IFdpZGdpdHMgUHR5IEx0ZDCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEB\r\n"
"AKL71f30DySYtPIFOXQ1Zd29itrZ6VZ5WtLXSANkJq2YxNuMJySMvpi9y540970G\r\n"
"xP3ING/rhPr+acsmCQ+IBM9xrUSzU0VsrNUTuom8ZdydrsuZf3Zei5jtAJsWzYXD\r\n"
"P5LKgZbGSTzUmZXmXlX3HGSx0rE0q1Epjh6PGKzpw2rQJzlY3cK7lTHIC8x6YBEV\r\n"
"Yr9Rw/NwXZEKP6lx6HCpRC8DGck2+8MDCDty40Bl7bMjbY712rt3J/1uHz9c5OmN\r\n"
"dof8jwFkO/noOB6vsEXCQXlLW9ZXNL3zsA23S2QHrmGukNcLxCpvYtztaHtOuv8y\r\n"
"OQt8EFHtQyGykTsvP5/0ShkCAwEAATANBgkqhkiG9w0BAQsFAAOCAQEAc1Pd/thJ\r\n"
"4aemeQnQD36ISchvyJs9n8MZW9tMRD2Y0Dq/WpBb4xDOWGQ37Clee2f0k2K3ospJ\r\n"
"zZ4r9q+f6WRaMDc2ZXk0OJGbgPfvqTbaNo4vP058E87bwLsuUj1JYvI3d+d1hWMC\r\n"
"a1o8dUOjQyVYf8IfxkL2zAO42/uij2rQw7u3fcC59EbBOAaPk/SO5VGWwBjsDTwQ\r\n"
"VGQPPoG7iNOU4SvPcLUK6lDWZ68PllFBA+7LIiNHU2xAuHkHc79srySkdbMMJ9pd\r\n"
"Va6JA2K1JKl+lgIeCK1Z6i11DQyL7HdIAKoK4G5V9pId17Uq8zdnvpQkklg3Udpf\r\n"
"oQWOaxyon2nU3w==\r\n"
"-----END CERTIFICATE-----\r\n";


static const char client_pk[] =
"-----BEGIN RSA PRIVATE KEY-----\r\n"
"MIIEogIBAAKCAQEAovvV/fQPJJi08gU5dDVl3b2K2tnpVnla0tdIA2QmrZjE24wn\r\n"
"JIy+mL3LnjT3vQbE/cg0b+uE+v5pyyYJD4gEz3GtRLNTRWys1RO6ibxl3J2uy5l/\r\n"
"dl6LmO0AmxbNhcM/ksqBlsZJPNSZleZeVfccZLHSsTSrUSmOHo8YrOnDatAnOVjd\r\n"
"wruVMcgLzHpgERViv1HD83BdkQo/qXHocKlELwMZyTb7wwMIO3LjQGXtsyNtjvXa\r\n"
"u3cn/W4fP1zk6Y12h/yPAWQ7+eg4Hq+wRcJBeUtb1lc0vfOwDbdLZAeuYa6Q1wvE\r\n"
"Km9i3O1oe066/zI5C3wQUe1DIbKROy8/n/RKGQIDAQABAoIBABzLFOEKjupOOBlR\r\n"
"pvbKwDZOWAuV1805HzyEX+qJdPPSO2T1+6xPWRSu4xwOC35Phdm31tu25gVZkOMc\r\n"
"0xj1VLQ5Rv0OGTX4nwf9tkTDDdPN36WEdqo4xby8khDUFHb/KWoLcJ1sZl/ix0de\r\n"
"LWhOgaugZrJ7tZBfIQZxDVDu82EZIMbqgCQ3NBTIV49wfdj0h/6M/VB1Z9CqNPeq\r\n"
"Wc3lt1cuZfD+NKVonlmzR6mAfMjtJo6caOggdJ1cnDYu4vQ6gKOpyt3oeAPqTq8Y\r\n"
"wa7kwUAJbEDB7k5MS8U8ZsnthtP3PSXo6Hv+dZ4nY7Ua4wrocpiHU/JdLG/psNcA\r\n"
"OVyAzwECgYEAzvcKkJ77vpaohmhfr7xiySb9Y2nJOfwx98/A2EOy1mQ8gM6ugBle\r\n"
"bBT9FvTssaG91LddvZ03MbrHLCMQdISSTKdDM3GQOJFtqz4jarIW4ymrHxbZLB1r\r\n"
"4IwHqgcG8x10Fp2MM1F2Iy/4XeV3F6M8OvZtjOGOK7PhJRtdwcitI6kCgYEAyZk3\r\n"
"uY4//IxuoFOyYqU3mSD/V3CyJOgR3ZaT+hp9DpAOFFOjBiyC9mZeazXvKveSoFIn\r\n"
"2GK4I/INjbyUDZ9dq6pHCFfQWDeP8L4+gzrRcLCW5mxOmrsYMXfdcY1f4U75DHwj\r\n"
"qgi2WYgkcz9U06dqIULfRieDbJN+AN8Chola+PECgYAObCrBT0Lt0iPmUemxHmin\r\n"
"6d6oiduq/cchpMmkiHsy84M/2qdQZ/Qrhf7pFaJU8pd+9lRC/Wy2O3Tbv4nLBN4J\r\n"
"F3LYZ+aL+p5w24CuU8DCjcnN/dKef2JgIIH8OEcks/2+AbaecOPRqesd5/q3m/l7\r\n"
"hma19ZXpt7xN0K1k7q4aGQKBgB8zbnNyd6a/mVOJAJ/R0EQL3lkLIRcjL7iq2GYp\r\n"
"+VbqprMwqpeHBhHakBxpsYVl4bScYnxT8wnlKYHZQNTG6HlsFihNZvpwRv/MgeJP\r\n"
"lSCqxAAPnS7HbBwj4Ar2BXPahCMRh3eGd6ptrq6Di75iN8PEFMhHz0hbn3HFEh/+\r\n"
"XC0RAoGARNLIcnU7U25lEyvWbFK+0m1GSmpTUpinI7wC2XawDI/ZlWbCtTAR/7Yi\r\n"
"RrRaoYGoDTpe4loFecWTHSrc27lxqZHB/WeThz6M9W8BTfyxSJdsgvLDlzRmpJkX\r\n"
"3k9fMpHX+3Awwa66tcjRccMpBwJt9YJXbeIAGTMH/KDAbFCttqk=\r\n"
"-----END RSA PRIVATE KEY-----\r\n";

static const char client_pk_pwd[] = "";
#endif

#define AE_NAME "base0"
#define AE_ID   "S"AE_NAME

#define REQU_TOPIC "/oneM2M/req/"AE_ID"/Mobius/json"
#define RESP_TOPIC "/oneM2M/resp/"AE_ID"/Mobius/json"
#define NOTI_TOPIC "/oneM2M/req/Mobius2/"AE_ID"/json"
#define NOTI_RESP_TOPIC "/oneM2M/resp/Mobius2/"AE_ID"/json"
#define HEARTBEAT_TOPIC "/nCube/heartbeat"

unsigned char send_buf[256];
unsigned char read_buf[256];

#if defined( SUPPORT_MBEDTLS ) && defined( USE_MQTTS )
Certs cert;
#endif
char request_id[16] = {'\0',};
unsigned int request_id_counter;

bool noti_resp;
cJSON* noti_payload;

void resp_msg_arrived(MessageData* data)
{
	nrc_usr_print("[RESPONSE: %.*s]\n%.*s\n", data->topicName->lenstring.len,
		data->topicName->lenstring.data, data->message->payloadlen, data->message->payload);
}

void noti_msg_arrived(MessageData* data)
{
	nrc_usr_print("[NOTIFICATION: %.*s]\n%.*s\n", data->topicName->lenstring.len,
		data->topicName->lenstring.data, data->message->payloadlen, data->message->payload);

	noti_payload = cJSON_Parse(data->message->payload);
	noti_resp = true;
}

char* get_res_for_create_ae(uint8_t ty, char* to, char* rn)
{
	cJSON* root = NULL;
	char* out = NULL;

	root = cJSON_CreateObject();
	cJSON_AddNumberToObject(root, "op", 1);

	return out;
}

char* get_rqi()
{
	sprintf(request_id, "rqi_%05d", request_id_counter++);
	return request_id;
}

void do_publish(char* topic, char* payload, MQTTClient *mqtt_client)
{
	int rc = 0;
	MQTTMessage message;
	message.qos = 1;
	message.retained = 0;
	message.payload = payload;
	message.payloadlen = strlen(payload);

	nrc_usr_print("-----> try to publish...\n");
	if ((rc = MQTTPublish(mqtt_client, topic, &message)) != 0)
		nrc_usr_print("Return code from MQTT publish is %d\n", rc);
	nrc_usr_print("%s\n", payload);
	nrc_usr_print("[ published successfully!! ]\n");

	nrc_mem_free(payload);

}

void publish_create_ae(MQTTClient *mqtt_client)
{
	cJSON* root = NULL;
	cJSON* i1 = NULL;
	cJSON* i2 = NULL;
	char* out = NULL;

	/*
	 * Create AE
	 */
	root = cJSON_CreateObject();
	cJSON_AddNumberToObject(root, "op", 1);
	cJSON_AddStringToObject(root, "to", "/Mobius?rcn=0");
	cJSON_AddStringToObject(root, "fr", AE_ID);
	cJSON_AddStringToObject(root, "rqi", get_rqi());
	cJSON_AddNumberToObject(root, "ty", 2);
	cJSON_AddItemToObject(root, "pc", i1 = cJSON_CreateObject());
	cJSON_AddItemToObject(i1, "m2m:ae", i2 = cJSON_CreateObject());
	cJSON_AddStringToObject(i2, "rn", AE_NAME);
	cJSON_AddStringToObject(i2, "api", "3.14");
	cJSON_AddTrueToObject(i2, "rr");

	out = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);

	do_publish(REQU_TOPIC, out, mqtt_client);
}

void publish_create_cnt(const char* rn, MQTTClient *mqtt_client)
{
	cJSON* root = NULL;
	cJSON* i1 = NULL;
	cJSON* i2 = NULL;
	char* out = NULL;

	/*
	 * Create Container for CO2
	 */
	root = cJSON_CreateObject();
	cJSON_AddNumberToObject(root, "op", 1);
	cJSON_AddStringToObject(root, "to", "/Mobius/base0?rcn=0");
	cJSON_AddStringToObject(root, "fr", AE_ID);
	cJSON_AddStringToObject(root, "rqi", get_rqi());
	cJSON_AddNumberToObject(root, "ty", 3);
	cJSON_AddItemToObject(root, "pc", i1 = cJSON_CreateObject());
	cJSON_AddItemToObject(i1, "m2m:cnt", i2 = cJSON_CreateObject());
	cJSON_AddStringToObject(i2, "rn", rn);

	out = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);

	do_publish(REQU_TOPIC, out, mqtt_client);
}

void publish_delete_sub(MQTTClient *mqtt_client)
{
	cJSON* root = NULL;
	cJSON* i1 = NULL;
	char* out = NULL;

	/*
	 * Delete Subscription
	 */
	root = cJSON_CreateObject();
	cJSON_AddNumberToObject(root, "op", 4);
	cJSON_AddStringToObject(root, "to", "/Mobius/base0/led/sub?rcn=0");
	cJSON_AddStringToObject(root, "fr", AE_ID);
	cJSON_AddStringToObject(root, "rqi", get_rqi());
	cJSON_AddItemToObject(root, "pc", i1 = cJSON_CreateObject());

	out = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);

	do_publish(REQU_TOPIC, out, mqtt_client);
}

void publish_create_sub(MQTTClient *mqtt_client)
{
	cJSON* root = NULL;
	cJSON* i1 = NULL;
	cJSON* i2 = NULL;
	cJSON* i3 = NULL;
	cJSON* i4 = NULL;
	cJSON* i5 = NULL;
	char* out = NULL;

	/*
	 * Create Subscription
	 */
	root = cJSON_CreateObject();
	cJSON_AddNumberToObject(root, "op", 1);
	cJSON_AddStringToObject(root, "to", "/Mobius/base0/led?rcn=0");
	cJSON_AddStringToObject(root, "fr", AE_ID);
	cJSON_AddStringToObject(root, "rqi", get_rqi());
	cJSON_AddNumberToObject(root, "ty", 23);
	cJSON_AddItemToObject(root, "pc", i1 = cJSON_CreateObject());
	cJSON_AddItemToObject(i1, "m2m:sub", i2 = cJSON_CreateObject());
	cJSON_AddStringToObject(i2, "rn", "sub");
	cJSON_AddItemToObject(i2, "enc", i3 = cJSON_CreateObject());
	int arr = 3;
	cJSON_AddItemToObject(i3, "net", i4 = cJSON_CreateIntArray(&arr, 1));
	const char* nu = "mqtt://"MOBIUS_MQTT_BROKER_IP":1883/"AE_ID"?ct=json&rcn=9";
	cJSON_AddItemToObject(i2, "nu", i5 = cJSON_CreateStringArray(&nu, 1));
	cJSON_AddNumberToObject(i2, "nct", 2);

	out = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);

	do_publish(REQU_TOPIC, out, mqtt_client);
}

void publish_noti_resp(MQTTClient *mqtt_client)
{
	cJSON* root = NULL;
	char* out = NULL;

	root = cJSON_CreateObject();
	cJSON_AddNumberToObject(root, "rsc", 2000);
	cJSON_AddStringToObject(root, "to", "");
	cJSON_AddStringToObject(root, "fr", AE_ID);
	cJSON_AddStringToObject(root, "pc", "");
	cJSON_AddItemReferenceToObject(root, "rqi", cJSON_GetObjectItem(noti_payload, "rqi"));

	out = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);
	cJSON_Delete(noti_payload);

	do_publish(NOTI_RESP_TOPIC, out, mqtt_client);
}

void publish_update_co2(MQTTClient *mqtt_client)
{
	cJSON* root = NULL;
	cJSON* i1 = NULL;
	cJSON* i2 = NULL;
	char* out = NULL;

	root = cJSON_CreateObject();
	cJSON_AddNumberToObject(root, "op", 1);
	cJSON_AddStringToObject(root, "to", "/Mobius/base0/co2?rcn=0");
	cJSON_AddStringToObject(root, "fr", AE_ID);
	cJSON_AddStringToObject(root, "rqi", get_rqi());
	cJSON_AddNumberToObject(root, "ty", 4);
	cJSON_AddItemToObject(root, "pc", i1 = cJSON_CreateObject());
	cJSON_AddItemToObject(i1, "m2m:cin", i2 = cJSON_CreateObject());
	cJSON_AddNumberToObject(i2, "con", request_id_counter);

	out = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);

	do_publish(REQU_TOPIC, out, mqtt_client);
}

/******************************************************************************
 * FunctionName : run_sample_onem2m
 * Description  : sample test for onem2m
 * Parameters   : WIFI_CONFIG
 * Returns      : 0 or -1 (0: success, -1: fail)
 *******************************************************************************/
nrc_err_t run_sample_onem2m(WIFI_CONFIG *param)
{
	int count = 0;
	int interval = 0;
	int network_index = 0;
	tWIFI_STATE_ID wifi_state = WIFI_STATE_INIT;
	SCAN_RESULTS results;

	MQTTClient *mqtt_client = nrc_mem_malloc(sizeof(MQTTClient));
	memset(mqtt_client, 0x0, sizeof(MQTTClient));

	Network network;

	nrc_usr_print("[%s] Sample App for Onem2m \n",__func__);
	count = param->count;
	interval = param->interval;

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

	/*
	 * MQTT Initialize
	 */
	NetworkInit(&network);
	MQTTClientInit(mqtt_client, &network, 30000, send_buf, sizeof(send_buf), read_buf, sizeof(read_buf));

#if defined( SUPPORT_MBEDTLS ) && defined( USE_MQTTS )
	cert.ca_cert = ca_cert;
	cert.ca_cert_length = sizeof(ca_cert);
	cert.client_cert = client_cert;
	cert.client_cert_length = sizeof(client_cert);
	cert.client_pk = client_pk;
	cert.client_pk_length = sizeof(client_pk);
	cert.client_pk_pwd = client_pk_pwd;
	cert.client_pk_pwd_length = sizeof(client_pk_pwd) - 1;
#endif

	/*
	 * MQTT Connect
	 */
	int rc = 0;
#if defined( SUPPORT_MBEDTLS ) && defined( USE_MQTTS )
	if ((rc = NetworkConnectTLS(&network, MOBIUS_MQTT_BROKER_IP, MOBIUS_MQTT_BROKER_PORT, &cert)) != 0)
#else
	if ((rc = NetworkConnect(&network, MOBIUS_MQTT_BROKER_IP, MOBIUS_MQTT_BROKER_PORT)) != 0)
#endif
		nrc_usr_print("Return code from network connect is %d\n", rc);
	else
		nrc_usr_print("[%s] Network Connected\n", __func__);

	if ((rc = MQTTStartTask(mqtt_client)) != 1)
		nrc_usr_print("Return code from start tasks is %d\n", rc);

	MQTTPacket_connectData connectData = MQTTPacket_connectData_initializer;
	connectData.MQTTVersion = 3;
	connectData.clientID.cstring = "nrc_11ah_mqtt_test";

	nrc_usr_print("[%s] Try to connect to MQTT Broker......\n", __func__);
	if ((rc = MQTTConnect(mqtt_client, &connectData)) != 0)
		nrc_usr_print("Return code from MQTT connect is %d\n", rc);
	else
		nrc_usr_print("[%s] MQTT Connected\n", __func__);

	/*
	 * MQTT Subscription
	 */
	if ((rc = MQTTSubscribe(mqtt_client, RESP_TOPIC, 0, resp_msg_arrived)) != 0)
		nrc_usr_print("Return code from MQTT subscribe is %d\n", rc);

	if ((rc = MQTTSubscribe(mqtt_client, NOTI_TOPIC, 0, noti_msg_arrived)) != 0)
		nrc_usr_print("Return code from MQTT subscribe is %d\n", rc);

	/*
	 * oneM2M Registration via MQTT
	 */
	publish_create_ae(mqtt_client);
	publish_create_cnt("co2",mqtt_client);
	publish_create_cnt("led",mqtt_client);
	publish_delete_sub(mqtt_client);
	publish_create_sub(mqtt_client);

	for(i=0; i<count; i++) {
		if (noti_resp) {
			noti_resp = false;
			publish_noti_resp(mqtt_client);
		}

		if ((i % 10) == 0) {
			nrc_usr_print("Publish update co2\n");
			publish_update_co2(mqtt_client);
		}

		_delay_ms(interval);
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

	nrc_wifi_get_state(&wifi_state);
	if (wifi_state == WIFI_STATE_GET_IP || wifi_state == WIFI_STATE_CONNECTED) {
		nrc_usr_print("[%s] Trying to DISCONNECT... for exit\n",__func__);
		if (nrc_wifi_disconnect(network_index) != WIFI_SUCCESS) {
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
void user_init(void)
{
	nrc_err_t ret;
	WIFI_CONFIG* param;

	nrc_uart_console_enable(true);

	param = nrc_mem_malloc(WIFI_CONFIG_SIZE);
	memset(param, 0x0, WIFI_CONFIG_SIZE);

	set_wifi_config(param);
	ret = run_sample_onem2m(param);
	nrc_usr_print("[%s] test result!! %s \n",__func__, (ret==0) ?  "Success" : "Fail");
	if(param){
		nrc_mem_free(param);
	}
}
