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

#define LED_GREEN	GPIO_02
#define LED_RED 	GPIO_03

uint32_t g_counter0 = 1;
uint32_t g_toggle1;
uint32_t g_toggle2;

void timer_expire_handler00(const timer_id id) {
	nrc_usr_print("[timer_id:%d] %d\n", id, g_counter0);
}

void timer_expire_handler01(const timer_id id) {
	g_counter0++;
}

void timer_expire_handler02(const timer_id id) {
	nrc_gpio_outputb(LED_GREEN, g_toggle1);
	g_toggle1 = !g_toggle1;
}

void timer_expire_handler03(const timer_id id) {
	nrc_gpio_outputb(LED_RED, g_toggle2);
	g_toggle2 = !g_toggle2;
}

void timer_expire_handler04(const timer_id id) {
	nrc_usr_print("[timer_id:%d] expired.\n", id);
	nrc_timer_stop(id);
}

void timer_expire_handler05(const timer_id id) {
	nrc_usr_print("[timer_id:%d] expired.\n", id);
	nrc_timer_stop(id);
}


/******************************************************************************
 * FunctionName : run_sample_timer
 * Description  : sample test for timer
 * Parameters   : count(test count), interval(test interval)
 * Returns      : 0 or -1 (0: success, -1: fail)
 *******************************************************************************/
int run_sample_timer(void)
{
	nrc_usr_print("[%s] Sample App for timer test\n", __func__);
	nrc_usr_print("============================================\n");
	nrc_usr_print("1. The 1st timer starts with 1 second interval infinitely\n");
	nrc_usr_print("and print out a counter value which will be increased by the 2nd timer.\n");
	nrc_usr_print("2. The 2nd timer starts with 100 milliseconds interval infinitely\n");
	nrc_usr_print("and increases 1 of the counter value whenever expired.\n");
	nrc_usr_print("3. The 3rd timer starts with 100 milliseconds interval infinitely\n");
	nrc_usr_print("and toggles the green LED on the board whenever expired.\n");
	nrc_usr_print("4. The 4th timer starts with 50 milliseconds interval infinitely\n");
	nrc_usr_print("and toggles the red LED on the board whenever expired.\n");
	nrc_usr_print("5. The 5th timer starts with 5 seconds interval infinitely\n");
	nrc_usr_print("but it will be stopped when expired.\n");
	nrc_usr_print("6. The 6th timer starts with 10 seconds interval infinitely\n");
	nrc_usr_print("but it will be stopped when expired.\n\n");

	nrc_timers_init();

	NRC_GPIO_CONFIG gpio_conf;

	gpio_conf.gpio_pin = LED_GREEN;
	gpio_conf.gpio_dir = GPIO_OUTPUT;
	gpio_conf.gpio_mode = GPIO_PULL_UP;
	gpio_conf.gpio_alt= GPIO_FUNC;
	nrc_gpio_config(&gpio_conf);

	gpio_conf.gpio_pin = LED_RED;
	gpio_conf.gpio_dir = GPIO_OUTPUT;
	gpio_conf.gpio_mode = GPIO_PULL_UP;
	gpio_conf.gpio_alt= GPIO_FUNC;
	nrc_gpio_config(&gpio_conf);

	nrc_timer_create(1000000, 1, timer_expire_handler00);
	nrc_timer_create(100000, 1, timer_expire_handler01);
	nrc_timer_create(100000, 1, timer_expire_handler02);
	nrc_timer_create(50000, 1, timer_expire_handler03);
	nrc_timer_create(5000000, 1, timer_expire_handler04);
	nrc_timer_create(10000000, 1, timer_expire_handler05);

	while(1);


	nrc_usr_print("[%s] exit \n",__func__);
	return RUN_SUCCESS;
}


/******************************************************************************
 * FunctionName : user_init
 * Description  : Start Code for User Application, Initialize User function
 * Parameters   : none
 * Returns      : none
 *******************************************************************************/
void user_init(void)
{
	int ret = 0;

	//Enable Console for Debugging
	nrc_uart_console_enable();

	ret = run_sample_timer();
	nrc_usr_print("[%s] test result!! %s \n",__func__, (ret==0) ?  "Success" : "Fail");
}
