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
#include "nvs.h"
#include "nvs_flash.h"

nvs_handle_t nvs_handle;

/******************************************************************************
 * FunctionName : run_nvs_flash_test
 * Description  : sample test for nvs flash
 * Parameters   : void
 * Returns	    : 0 or -1 (0: success, -1: fail)
 *******************************************************************************/
nrc_err_t run_nvs_flash_test()
{
	nvs_err_t err = NVS_OK;
	int32_t value;

	err = nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READWRITE, &nvs_handle);
	if (err != NVS_OK) {
		nrc_usr_print("[%s] nvs_open failed (0x%x).\n", __func__, err);
		return NRC_FAIL;
	}

	nrc_usr_print("[%s] Call nvs_set_i32 for foo = 0x12345678...\n", __func__);
	err = nvs_set_i32(nvs_handle, "foo", 0x12345678);
	if (err != NVS_OK) {
		nrc_usr_print("[%s] nvs_set_i32 failed (0x%x).\n", __func__, err);
		return NRC_FAIL;
	}

	nrc_usr_print("[%s] Call nvs_get_i32 for foo\n", __func__);
	err = nvs_get_i32(nvs_handle, "foo", &value);
	if (err != NVS_OK) {
		nrc_usr_print("[%s] nvs_get_i32 failed (0x%x).\n", __func__, err);
		return NRC_FAIL;
	} else {
		nrc_usr_print("[%s] nvs_get_i32 : foo = 0x%x\n", __func__, value);
		if (value != 0x12345678) {
			nrc_usr_print("[%s] written value 0x12345678 != 0x%x\n", value);
			return NRC_FAIL;
		}
	}

	nrc_usr_print("[%s] Call nvs_set_i32 for foo = 0x23456789...\n", __func__);
	err = nvs_set_i32(nvs_handle, "foo", 0x23456789);
	if (err != NVS_OK) {
		nrc_usr_print("[%s] nvs_set_i32 failed (0x%x).\n", __func__, err);
		return NRC_FAIL;
	}

	nrc_usr_print("[%s] Call nvs_get_i32 for foo\n", __func__);
	err = nvs_get_i32(nvs_handle, "foo", &value);
	if (err != NVS_OK) {
		nrc_usr_print("[%s] nvs_get_i32 failed (0x%x).\n", __func__, err);
		return NRC_FAIL;
	} else {
		nrc_usr_print("[%s] nvs_get_i32 : foo = 0x%x\n", __func__, value);
		if (value != 0x23456789) {
			nrc_usr_print("[%s] written value 0x23456789 != 0x%x\n", value);
			return NRC_FAIL;
		}
	}

	return NRC_SUCCESS;
}


/******************************************************************************
 * FunctionName : user_init
 * Description  : Start Code for User Application, Initialize User function
 * Parameters   : none
 * Returns	    : none
 *******************************************************************************/
void user_init(void)
{
	nrc_err_t ret = 0;
	nrc_usr_print("[%s] Starting run_nvs_flash_test...\n",__func__);

	ret = run_nvs_flash_test();
	nrc_usr_print("[%s] test result!! %s \n",__func__, (ret==0) ?  "Success" : "Fail");

	/* Will not call nvs_close, so that command line can be used. */
	/* nvs_close(nvs_handle); */
}
