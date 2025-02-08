/*
 * MIT License
 *
 * Copyright (c) 2024 Newracom, Inc.
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
#include "lwip/sys.h"
#include "lwip/sockets.h"
#include "lwip/errno.h"
#include "lwip/netdb.h"
#include "wifi_config_setup.h"
#include "wifi_connect_common.h"

#include "wifi_task.h"

#include "nrc_dev_ov2640.h"
#include "network_service.h"

static WIFI_CONFIG wifi_config;

static int send_image_data (char *remote_addr, uint32_t port, char *buf, int len)
{
	int ret;

	nrc_usr_print("[%s]: image size : %u\n", __func__, len);

	if (!upload_data_packet(remote_addr, port, (char *) buf, len)) {
		nrc_usr_print("Failed to send(), errno=%d\n", errno);
		return -1;
	}

	return 0;
}

static int send_photo()
{
	char *buf = NULL;

	nrc_usr_print("[SEND PHOTO]\n");

	int image_size = ov2640_capture_and_wait();
	nrc_usr_print("IMAGE SIZE = %d\n", image_size);

	buf = nrc_mem_malloc(image_size);
	if (!buf) {
		nrc_usr_print("[%s] Error allocating buffer for image\n", __func__);
		return NRC_FAIL;
	}

	ov2640_read_captured_bytes((uint8_t *) buf, image_size);
	send_image_data(wifi_config.remote_addr, wifi_config.remote_port, buf, image_size);

	nrc_mem_free(buf);

	return NRC_SUCCESS;
}

/* delay in second between sending picture */
static void stream_photo(int delay)
{
	while (send_photo() == NRC_SUCCESS) {
		if (delay > 0) {
			_delay_ms(delay * 1000);
		}
	}
}

void user_init(void)
{
	int retry_delay = 10;

	nrc_wifi_set_config(&wifi_config);
	nrc_uart_console_enable(true);
	ov2640_init();
	start_wifi(&wifi_config);
	while (!open_connection(wifi_config.remote_addr, wifi_config.remote_port)) {
		nrc_usr_print("** Error connecting to %s:%d, retry in %d sec.\n",
				wifi_config.remote_addr, wifi_config.remote_port, retry_delay);
		_delay_ms(retry_delay * 1000);
	}

	/* send picture to the client */
	stream_photo(3);
}
