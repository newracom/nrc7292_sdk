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

#include "nrc_sdk.h"
#include "nrc_lwip.h"
#include "lwip/sys.h"
#include "lwip/sockets.h"
#include "lwip/errno.h"
#include "wifi_config_setup.h"
#include "wifi_connect_common.h"
#include "wic.h"
#include "transport.h"
#include "sample_websocket_client_version.h"
#include "sample_websocket_client_cert.h"

#include <stdlib.h>
#include <string.h>

WIFI_CONFIG wifi_config;
struct wic_inst inst;

#define SEND_DATA_NUM 10

#define USE_SSL_WS 1
#if USE_SSL_WS
const char* ws_server_url = "wss://172.16.200.1:9000";
#else
const char* ws_server_url = "ws://172.16.200.1:9000";
#endif

static void on_open_handler(struct wic_inst *inst);
static bool on_message_handler(struct wic_inst *inst, enum wic_encoding encoding, bool fin, const char *data, uint16_t size);
static void on_close_handler(struct wic_inst *inst, uint16_t code, const char *reason, uint16_t size);
static void on_close_transport_handler(struct wic_inst *inst);
static void on_send_handler(struct wic_inst *inst, const void *data, size_t size, enum wic_buffer type);
static void on_handshake_failure_handler(struct wic_inst *inst, enum wic_handshake_failure reason);
static void *on_buffer_handler(struct wic_inst *inst, size_t min_size, enum wic_buffer type, size_t *max_size);

static bool on_message_handler(struct wic_inst *inst, enum wic_encoding encoding, bool fin, const char *data, uint16_t size)
{
    if(encoding == WIC_ENCODING_UTF8){

        WIC_LOG("received text: %.*s", size, data);
    }

    return true;
}

static void on_handshake_failure_handler(struct wic_inst *inst, enum wic_handshake_failure reason)
{
    WIC_LOG("websocket handshake failed for reason %d", reason);
}

static void on_open_handler(struct wic_inst *inst)
{
    const char *name, *value;

    WIC_LOG("websocket is open");

    WIC_LOG("received handshake:");

    for(value = wic_get_next_header(inst, &name); value; value = wic_get_next_header(inst, &name)){

        WIC_LOG("%s: %s", name, value);
    }

   const char msg[] = "hello world";
   wic_send_text(inst, true, msg, strlen(msg));
}

static void on_close_handler(struct wic_inst *inst, uint16_t code, const char *reason, uint16_t size)
{
    WIC_LOG("websocket closed for reason %u", code);
}

static void on_close_transport_handler(struct wic_inst *inst)
{
    transport_close((int *)wic_get_app(inst), inst);
}

static void on_send_handler(struct wic_inst *inst, const void *data, size_t size, enum wic_buffer type)
{
    WIC_LOG("sending buffer type %d", type);

    transport_write(*(int *)wic_get_app(inst), data, size, inst);
}

static void *on_buffer_handler(struct wic_inst *inst, size_t min_size, enum wic_buffer type, size_t *max_size)
{
    static uint8_t tx[1000U];

    *max_size = sizeof(tx);

    return (min_size <= sizeof(tx)) ? tx : NULL;
}

static void run_ws_client(void)
{
	int s = -1;
	static uint8_t rx[1000];
	struct wic_init_arg param = {0};

	param.rx = rx;
	param.rx_max = sizeof(rx);    
	param.on_send = on_send_handler;
	param.on_buffer = on_buffer_handler;
	param.on_message = on_message_handler;
	param.on_open = on_open_handler;
	param.on_close = on_close_handler;
	param.on_close_transport = on_close_transport_handler;        
	param.on_handshake_failure = on_handshake_failure_handler;
	param.app = &s;
	param.url = ws_server_url;
	param.role = WIC_ROLE_CLIENT;

#ifdef ROOT_CA
	param.root_ca = ROOT_CA;
#endif
#ifdef CLIENT_CERT
	param.client_cert = CLIENT_CERT;
#endif
#ifdef PRIVATE_KEY
	param.private_key = PRIVATE_KEY;
#endif

	if(!wic_init(&inst, &param)){
		return;
	};

	struct wic_header user_agent = {
		.name = "User-Agent",
		.value = "wic"
	};

	(void)wic_set_header(&inst, &user_agent);

	if(transport_open_client(
			wic_get_url_schema(&inst),
			wic_get_url_hostname(&inst),
			wic_get_url_port(&inst),
			&s,
			&inst
		)
	){
		if(wic_start(&inst) == WIC_STATUS_SUCCESS){
			transport_recv(s, &inst);
			const char msg[] = "hello world";
			for(int k=0; k<SEND_DATA_NUM; k++){
				wic_send_text(&inst, true, msg, strlen(msg));
				transport_recv(s, &inst);
				_delay_ms(1000);
			}
		}
		wic_close(&inst);
		transport_close(&s, &inst);
	}
}


static nrc_err_t connect_to_ap(WIFI_CONFIG *wifi_config)
{
	/* set initial wifi configuration */
	if (wifi_init(wifi_config) != WIFI_SUCCESS) {
		nrc_usr_print ("[%s] ASSERT! Fail for init\n", __func__);
		return NRC_FAIL;
	}

	/* connect to AP */
	while(1) {
		if (wifi_connect(wifi_config)== WIFI_SUCCESS) {
			nrc_usr_print ("[%s] connect to %s successfully !! \n", __func__, wifi_config->ssid);
			break;
		} else{
			nrc_usr_print ("[%s] Fail for connection %s\n", __func__, wifi_config->ssid);
			_delay_ms(1000);
		}
	}

	/* check if IP is ready */
	if (nrc_wait_for_ip(0, wifi_config->dhcp_timeout) == NRC_FAIL) {
		return NRC_FAIL;
	}

	return NRC_SUCCESS;
}

void user_init(void)
{
	memset(&wifi_config, 0, WIFI_CONFIG_SIZE);

	nrc_wifi_set_config(&wifi_config);

	if (connect_to_ap(&wifi_config) != NRC_SUCCESS) {
		nrc_usr_print("Error: Network connection failed!\n");
		return;
	}

	run_ws_client();
}
