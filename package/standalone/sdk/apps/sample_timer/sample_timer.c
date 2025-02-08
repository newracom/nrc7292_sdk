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

#define TIMER_INTERVAL_UNIT	 1000000 // us

#define TIMER0_INTERVAL	5*TIMER_INTERVAL_UNIT // 5 sec

#define TIMER0_COUNT_MAX	30

int count0;

void timer_callback_func0(int vector)
{
	nrc_usr_print("[%s] count:%d vector:%d\n",__func__, count0, vector);

	int ch = nrc_hw_timer_get_channel(vector);

	nrc_hw_timer_clear_irq(ch);
	if(count0++ < TIMER0_COUNT_MAX){
		nrc_hw_timer_start(ch, TIMER0_INTERVAL);
	} else {
		nrc_usr_print("[%s] Timer0 is cleared \n",__func__);
		nrc_hw_timer_deinit(ch);
		count0 = 0;
	}
}


/******************************************************************************
 * FunctionName : run_sample_timer
 * Description  : sample test for timer
 * Parameters   : count(test count), interval(test interval)
 * Returns      : 0 or -1 (0: success, -1: fail)
 *******************************************************************************/
nrc_err_t run_sample_timer(void)
{
	nrc_hw_timer_init(TIMER_CH_3, timer_callback_func0);

	nrc_usr_print("[%s] Sample App for timer test\n", __func__);
	nrc_usr_print("============================================\n");
	nrc_usr_print("The timer starts with %d second interval, %d times\n",\
		(TIMER0_INTERVAL/TIMER_INTERVAL_UNIT), TIMER0_COUNT_MAX);

	nrc_hw_timer_start(TIMER_CH_3, TIMER0_INTERVAL);

	while(1);

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
	//Enable Console for Debugging
	nrc_uart_console_enable(true);

	run_sample_timer();
}
