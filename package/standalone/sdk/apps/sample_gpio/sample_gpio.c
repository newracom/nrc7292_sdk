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
#define TEST_INTERVAL 2000 /* msec */

//#define USE_GPIO_INTERRUPT
#define PIN_INPUT	GPIO_10
#define PIN_OUTPUT	GPIO_02

int g_toggle = 0;

#if defined(USE_GPIO_INTERRUPT)
void gpio_intr_handler(int vector)
{
	if (nrc_gpio_inputb(PIN_INPUT)) {
		nrc_usr_print("[%s] LED ON \n", __func__);
		nrc_gpio_outputb(PIN_OUTPUT, 1);
	} else {
		nrc_usr_print("[%s] LED OFF \n", __func__);
		nrc_gpio_outputb(PIN_OUTPUT, 0);
	}
}
#endif

static int gpio_init(void)
{
	NRC_GPIO_CONFIG gpio_conf;

	//GPIO_01 for Output (LED)
	gpio_conf.gpio_pin = PIN_OUTPUT;
	gpio_conf.gpio_dir = GPIO_OUTPUT;
	gpio_conf.gpio_mode = GPIO_PULL_UP;
	gpio_conf.gpio_alt= GPIO_FUNC;
	nrc_gpio_config(&gpio_conf);

#if defined(USE_GPIO_INTERRUPT)
	//GPIO_00 for Input (Switch)
	gpio_conf.gpio_pin = PIN_INPUT;
	gpio_conf.gpio_dir = GPIO_INPUT;
	gpio_conf.gpio_mode = GPIO_PULL_UP;
	gpio_conf.gpio_alt= GPIO_FUNC;
	nrc_gpio_config(&gpio_conf);
	nrc_gpio_register_intr_handler(PIN_INPUT, gpio_intr_handler);
#endif
	return 0;
}

/******************************************************************************
 * FunctionName : run_sample_gpio
 * Description  : sample test for gpio
 * Parameters   : count(test count), interval(test interval)
 * Returns      : 0 or -1 (0: success, -1: fail)
 *******************************************************************************/
int run_sample_gpio(int count, int interval)
{
	int i=0;
	nrc_usr_print("[%s] Sample App for GPIO \n",__func__);

	if(gpio_init() != 0)  {
		nrc_usr_print ("[%s] Fail to init GPIO\n", __func__);
		return RUN_FAIL;
	}

	nrc_usr_print("[%s] Complete GPIO Init\n",__func__);

	for(i=0; i<count; i++) {
		//_wifi_wait_message_async();
#if !defined(USE_GPIO_INTERRUPT)
		_delay_ms(interval);
		//Blink at 1-second interval
		if (g_toggle){
			g_toggle = 0;
		} else {
			g_toggle = 1;
		}
		nrc_usr_print("[%s] LED %s\n", __func__, g_toggle? "ON":"OFF");
		nrc_gpio_outputb(PIN_OUTPUT, g_toggle);
#endif
	}
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

	ret = run_sample_gpio(TEST_COUNT, TEST_INTERVAL);
	nrc_usr_print("[%s] test result!! %s \n",__func__, (ret==0) ?  "Success" : "Fail");
}
