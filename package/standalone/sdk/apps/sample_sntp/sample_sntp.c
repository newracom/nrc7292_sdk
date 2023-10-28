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
#include "lwip/sys.h"
#include "lwip/sockets.h"
#include "lwip/errno.h"
#include "wifi_config_setup.h"
#include "wifi_connect_common.h"
#include "nrc_lwip.h"

#ifndef MAX_RETRY
#define MAX_RETRY 30
#endif


extern const char *SNTP_FORMAT_TIME (s32_t sec);

static int month(const char *m)
{
    static const char *months[] = {"Jan", "Feb", "Mar", "Apr","May",
		"Jun", "Jul", "Aug","Sep", "Oct", "Nov", "Dec"};

    for (int i = 0; i < 12; i++) {
        if (strcmp(m, months[i]) == 0) {
            return i + 1;
        }
    }
    return 0;
}

static void get_real_time(char *time)
{
    char sntp_time[32];
    char *ptr[5];
    char *time_of_day;

    strcpy(sntp_time, SNTP_FORMAT_TIME(get_utc_time()));
    sntp_time[strlen(sntp_time) - 1] = '\0';

    A("%s: sntp_time %s\n", __func__, sntp_time);

    ptr[0] = strtok(sntp_time, " ");
    int i = 0;

    while (ptr[i] != NULL && i < 4) {
        i++;
        ptr[i] = strtok(NULL, " ");
    }

    int year = atoi(ptr[4]) % 100;
    int mon = month(ptr[1]);
    int day = atoi(ptr[2]);
    time_of_day = ptr[3];

    sprintf(time, "%02d/%02d/%02d,%s", year, mon, day, time_of_day);

    A("%s: %s\n", __func__, time);
}


/****************************************************************************
 * FunctionName : run_sample_sntp
 * Description  : Aging test for tcp client
 * Parameters   : WIFI_CONFIG
 * Returns	  : 0 or -1 (0: success, -1: fail)
 *****************************************************************************/
nrc_err_t run_sample_sntp(WIFI_CONFIG *param)
{
	int count;
	u64_t utc_time = 0;
	char time[20];

	nrc_usr_print("==========================\n");
	nrc_usr_print("SNTP Test - Client\n");
	nrc_usr_print("==========================\n");

	/* set initial wifi configuration */
	if (wifi_init(param)!= WIFI_SUCCESS) {
		nrc_usr_print ("[%s] Error Wifi initialzation failed.\n", __func__);
		return -1;
	}

	/* connect to AP */
	for(count = 0 ; count < MAX_RETRY ; ){
		if (wifi_connect(param)== WIFI_SUCCESS){
			nrc_usr_print ("[%s] Successfully connected to '%s' !! \n", __func__, param->ssid);
			break;
		}

		if (++count == MAX_RETRY){
			nrc_usr_print ("[%s] Failed to connect to '%s'\n", __func__, param->ssid);
			return -1;
		}

		_delay_ms(1000);
	}

	/* check if IP is ready */
	while(1){
		if (nrc_addr_get_state(0) == NET_ADDR_SET) {
			nrc_usr_print("[%s] IP ...\n",__func__);
			break;
		} else {
			nrc_usr_print("[%s] IP Address setting State : %d != NET_ADDR_SET(%d) yet...\n",
						  __func__, nrc_addr_get_state(0), NET_ADDR_SET);
		}
		_delay_ms(1000);
	}

	initialize_sntp();
	count = MAX_RETRY;
	while(count-- > 0){
		utc_time = get_utc_time();
		A("UTC : %lld\n", utc_time);
		get_real_time(time);
		_delay_ms(2000);
	}
	return 0;
}


/****************************************************************************
 * FunctionName : user_init
 * Description  : Start Code for User Application, Initialize User function
 * Parameters   : none
 * Returns      : none
 *****************************************************************************/
WIFI_CONFIG wifi_config;
WIFI_CONFIG* param = &wifi_config;

void user_init(void)
{

	memset(param, 0x0, WIFI_CONFIG_SIZE);

	nrc_wifi_set_config(param);
	run_sample_sntp(param);
}

