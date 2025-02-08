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
#include "wifi_config_setup.h"
#include "wifi_connect_common.h"

static int vif_id = 0;

static int run_wifi_init(WIFI_CONFIG* param)
{
	if (wifi_init(param) != WIFI_SUCCESS) {
		nrc_usr_print("[%s] Fail\n", __func__);
		return NRC_FAIL;
	}
	nrc_usr_print("[%s] Success\n", __func__);
	return NRC_SUCCESS;
}

static int run_wifi_connect(WIFI_CONFIG* param)
{
	while (1)
	{
		if (wifi_connect(param) == WIFI_SUCCESS) {
			nrc_usr_print ("[%s] Success (%s) !! \n", __func__, param->ssid);
			return NRC_SUCCESS;
		}
		else {
			nrc_usr_print ("[%s] Fail (%s) - Retrying Connection\n", __func__, param->ssid);
		}
	}
}

void start_wifi(WIFI_CONFIG *wifi_config)
{
	run_wifi_init(wifi_config);
	run_wifi_connect(wifi_config);
}
