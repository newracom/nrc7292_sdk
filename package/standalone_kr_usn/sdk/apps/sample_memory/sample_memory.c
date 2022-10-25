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

#define TEST_COUNT 10
#define TEST_INTERVAL 1000 /* msec */

/******************************************************************************
 * FunctionName : run_sample_memory
 * Description  : sample test for memory
 * Parameters   : count(test count), interval(test interval)
 * Returns      : 0 or -1 (0: success, -1: fail)
 *******************************************************************************/
nrc_err_t run_sample_memory(int count, int interval)
{
	int i = 0, j=0;

	nrc_usr_print("[%s] Sample App for MEMORY API \n",__func__);

	uint32_t heap_size_ori = xPortGetFreeHeapSize();
	nrc_usr_print("Remaining heap size is %d \n", heap_size_ori);
	_delay_ms(interval);

	for(i=0; i<count; i++) {
		if (j == 300) {
			/* Malloc Range from 100B to 30000B */
			int32_t heap_size_now = xPortGetFreeHeapSize();
			if (heap_size_ori > heap_size_now) {
				nrc_usr_print("MEMORY LEAK!! ori(%d) vs now(%d) \n", heap_size_ori, heap_size_now);
				return NRC_FAIL;
			}
			nrc_usr_print("Start Again!\n");
			j = 1;
		} else {
			++j;
		}

		void *ptr = NULL;
		ptr = nrc_mem_malloc(j * 100);
		if (ptr == NULL) {
			nrc_usr_print("Malloc Fail!! Remaining heap size is %d \n", xPortGetFreeHeapSize());
			return NRC_FAIL;
		} else
			nrc_usr_print("Malloc %d Bytes. Remaining heap size is %d \n", j*100, xPortGetFreeHeapSize());

		nrc_mem_free(ptr);
		nrc_usr_print("Free   %d Bytes. Remaining heap size is %d \n", j*100, xPortGetFreeHeapSize());

		_delay_ms(interval);
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

	//Enable Console for Debugging
	nrc_uart_console_enable(true);

	ret = run_sample_memory(TEST_COUNT, TEST_INTERVAL);
	nrc_usr_print("[%s] test result!! %s \n",__func__, (ret==0) ?  "Success" : "Fail");
}
