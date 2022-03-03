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

#define TIMER_INTERVAL_UNIT	1000000 // us

#define TIMER0_INTERVAL	1*TIMER_INTERVAL_UNIT // 1 sec
#define TIMER1_INTERVAL	2*TIMER_INTERVAL_UNIT // 2 sec

#define TIMER0_COUNT_MAX	30
#define TIMER1_COUNT_MAX	10

int count0, count1;

void timer_callback_func0(int ch)
{
	nrc_usr_print("[%s] count:%d\n",__func__, count0);

	nrc_hw_timer_clear_irq(TIMER0);
	if(count0++ < TIMER0_COUNT_MAX){
		nrc_hw_timer_start(TIMER0, TIMER0_INTERVAL);
	} else {
		nrc_usr_print("[%s] Timer0 is cleared \n",__func__);
		nrc_hw_timer_deinit(TIMER0);
		count0 = 0;
	}
}

void timer_callback_func1(int ch)
{
	nrc_usr_print("[%s] count:%d\n",__func__, count1);

	nrc_hw_timer_clear_irq(TIMER1);
	if(count1++ < TIMER1_COUNT_MAX){
		nrc_hw_timer_start(TIMER1, TIMER1_INTERVAL);
	} else {
		nrc_usr_print("[%s] Timer1 is deinit \n",__func__);
		nrc_hw_timer_deinit(TIMER1);

		/* Forcely stop and clear timer0 */
		nrc_usr_print("[%s] Timer0 is stopped and deinit\n",__func__);
		nrc_hw_timer_stop(TIMER0);
		nrc_hw_timer_clear_irq(TIMER0);
		nrc_hw_timer_deinit(TIMER0);
		count1 = 0;
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
	nrc_hw_timer_init(TIMER0, timer_callback_func0);
	nrc_hw_timer_init(TIMER1, timer_callback_func1);

	nrc_usr_print("[%s] Sample App for timer test\n", __func__);
	nrc_usr_print("============================================\n");
	nrc_usr_print("1. The 1st timer starts with %d second interval, %d times\n",\
		(TIMER0_INTERVAL/TIMER_INTERVAL_UNIT), TIMER0_COUNT_MAX);
	nrc_usr_print("2. The 2nd timer starts with %d second interval, %d times\n",\
		(TIMER1_INTERVAL/TIMER_INTERVAL_UNIT), TIMER1_COUNT_MAX);
	nrc_usr_print("3. The 1nd timer forcely stoped and deinit by timer2 callback after %d second\n",\
		(TIMER1_INTERVAL/TIMER_INTERVAL_UNIT), TIMER1_COUNT_MAX);

	nrc_hw_timer_start(TIMER0, TIMER0_INTERVAL);
	nrc_hw_timer_start(TIMER1, TIMER1_INTERVAL);

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
	nrc_err_t ret;

	//Enable Console for Debugging
	nrc_uart_console_enable(true);

	ret = run_sample_timer();
	nrc_usr_print("[%s] test result!! %s \n",__func__, (ret==0) ?  "Success" : "Fail");
}
