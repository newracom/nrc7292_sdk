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
#include "api_sflash.h"

#define TEST_COUNT 3
#define TEST_INTERVAL 2000 /* msec */

typedef struct {
	char userindex[20];
	uint8_t userkey[12];
	uint8_t uservalue[20];
	uint8_t userdummy[100];
} sf_user_config_t;

/******************************************************************************
* FunctionName : run_sample_sf_userconfig
* Description  : sample test for read/write serial flash userconfig area
* Parameters   : count(test count), interval(test interval)
* Returns	    : 0 or -1 (0: success, -1: fail)
*******************************************************************************/
int run_sample_sf_userconfig(int count, int interval)
{
	int i = 0;

	nrc_usr_print("[%s] Sample App for Serial Flash Read/Write API \n",__func__);

	sf_user_config_t* sf_user_config_info;
	size_t length = sizeof(sf_user_config_t);
	sf_user_config_info = pvPortMalloc(length);

	memset(sf_user_config_info, 0x0, length);

	char index[20] = {0,};
	const uint8_t key[12] = "1234567890";
	const uint8_t value[20] = "user_config_test";
	const uint8_t dummy[100] = "1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZ";

	nrc_usr_print("[%s] Read user config area\n", __func__, i);
	if (nrc_sf_read_user_config(USER_CONFIG_AREA_1, (uint8_t*)sf_user_config_info, length)) {
		print_hex((uint32_t*)sf_user_config_info, length);
	} else {
		vPortFree(sf_user_config_info);
		return false;
	}

	nrc_usr_print("[%s] Erase user config area\n", __func__, i);
	if(nrc_sf_erase_user_config(USER_CONFIG_AREA_1))
	{
		if (nrc_sf_read_user_config(USER_CONFIG_AREA_1, (uint8_t*)sf_user_config_info, length)) {
			print_hex((uint32_t*)sf_user_config_info, length);
		} else {
			vPortFree(sf_user_config_info);
			return false;
		}
	}

	for(i=0; i<count; i++) {
		nrc_usr_print("[%s] Write data to user config area (count:%d)\n", __func__, i);
		sprintf(index, "Count_%04d",i);

		memcpy(sf_user_config_info->userindex, index, sizeof(index));
		memcpy(sf_user_config_info->userkey, key, sizeof(key));
		memcpy(sf_user_config_info->uservalue, value, sizeof(value));
		memcpy(sf_user_config_info->userdummy, dummy, sizeof(dummy));

		if (nrc_sf_write_user_config(USER_CONFIG_AREA_1, (uint8_t*)sf_user_config_info, length)) {
			nrc_usr_print("----------------------------------------------\n");
			nrc_usr_print("index: %s\n", &sf_user_config_info->userindex);
			nrc_usr_print("key  : %s\n", &sf_user_config_info->userkey);
			nrc_usr_print("value: %s\n", &sf_user_config_info->uservalue);
			nrc_usr_print("dummy: %s\n", &sf_user_config_info->userdummy);
			nrc_usr_print("----------------------------------------------\n");
		} else {
			vPortFree(sf_user_config_info);
			return false;
		}

		nrc_usr_print("[%s] Read user config area (count:%d)\n", __func__, i);
		if (nrc_sf_read_user_config(USER_CONFIG_AREA_1, (uint8_t*)sf_user_config_info, length)) {
			print_hex((uint32_t*)sf_user_config_info, length);
		} else {
			vPortFree(sf_user_config_info);
			return false;
		}

		vTaskDelay(pdMS_TO_TICKS(interval));
	}

	vPortFree(sf_user_config_info);
	return true;
}

/******************************************************************************
* FunctionName : user_init
* Description  : Start Code for User Application, Initialize User function
* Parameters   : none
* Returns	    : none
*******************************************************************************/
void user_init(void)
{
	bool ret = false;

	ret = run_sample_sf_userconfig(TEST_COUNT, TEST_INTERVAL);
	nrc_usr_print("[%s] test result!! %s \n",__func__, (ret== true) ?  "Success" : "Fail");

}
