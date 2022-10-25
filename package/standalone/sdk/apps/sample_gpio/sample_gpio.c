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

#define GREEN_LED_PIN	GPIO_02
#define RED_LED_PIN	GPIO_03

#define USE_GPIO_INTERRUPT

#ifdef USE_GPIO_INTERRUPT
#define GPIO_INT0_PIN	GPIO_10
#define GPIO_INT1_PIN	GPIO_11
void gpio_intr_handler0(int vector)
{
	int input_high;

	if (nrc_gpio_inputb(GPIO_INT0_PIN, &input_high))
		return;

   	if (input_high) {
		nrc_usr_print("[%s] GREEN LED ON \n", __func__);
		nrc_gpio_outputb(GREEN_LED_PIN, 1);
	} else {
		nrc_usr_print("[%s] GREEN LED OFF \n", __func__);
		nrc_gpio_outputb(GREEN_LED_PIN, 0);
	}
}

void gpio_intr_handler1(int vector)
{
	int input_high;

	if (nrc_gpio_inputb(GPIO_INT1_PIN, &input_high))
		return;

   	if (input_high) {
		nrc_usr_print("[%s] RED LED ON \n", __func__);
		nrc_gpio_outputb(RED_LED_PIN, 1);
	} else {
		nrc_usr_print("[%s] RED LED OFF \n", __func__);
		nrc_gpio_outputb(RED_LED_PIN, 0);
	}
}
#endif /* USE_GPIO_INTERRUPT */

/******************************************************************************
 * FunctionName : run_sample_gpio
 * Description  : sample test for gpio
 * Parameters   : count(test count), interval(test interval)
 * Returns      : 0 or -1 (0: success, -1: fail)
 *******************************************************************************/
nrc_err_t run_sample_gpio(int count, int interval)
{
	int i=0;
	NRC_GPIO_CONFIG gpio_conf;

	nrc_usr_print("[%s] Sample App for GPIO \n",__func__);
	gpio_conf.gpio_pin = GREEN_LED_PIN;
	gpio_conf.gpio_dir = GPIO_OUTPUT;
	gpio_conf.gpio_mode = GPIO_PULL_UP;
	gpio_conf.gpio_alt= GPIO_FUNC;
	nrc_gpio_config(&gpio_conf);

	gpio_conf.gpio_pin = RED_LED_PIN;
	gpio_conf.gpio_dir = GPIO_OUTPUT;
	gpio_conf.gpio_mode = GPIO_PULL_UP;
	gpio_conf.gpio_alt= GPIO_FUNC;
	nrc_gpio_config(&gpio_conf);

#ifdef USE_GPIO_INTERRUPT
	gpio_conf.gpio_pin = GPIO_INT1_PIN;
	gpio_conf.gpio_dir = GPIO_INPUT;
	gpio_conf.gpio_mode = GPIO_PULL_DOWN;
	gpio_conf.gpio_alt= GPIO_FUNC;
	nrc_gpio_config(&gpio_conf);
	nrc_gpio_register_interrupt_handler(GPIO_INT1_PIN , gpio_intr_handler1);

	gpio_conf.gpio_pin = GPIO_INT0_PIN;
	gpio_conf.gpio_dir = GPIO_INPUT;
	gpio_conf.gpio_mode = GPIO_PULL_DOWN;
	gpio_conf.gpio_alt= GPIO_FUNC;
	nrc_gpio_config(&gpio_conf);
	nrc_gpio_register_interrupt_handler(GPIO_INT0_PIN , gpio_intr_handler0);
#endif /* USE_GPIO_INTERRUPT */

#ifdef USE_GPIO_INTERRUPT
	while(1){
		;
	}
#else
	nrc_usr_print("[%s] Complete GPIO Init\n",__func__);
	bool toggle = 0;
	for(i=0; i<count; i++) {
		_delay_ms(interval);
		toggle = !toggle;
		nrc_usr_print("[%s] LED %s\n", __func__, toggle? "ON":"OFF");
		nrc_gpio_outputb(GREEN_LED_PIN, toggle);
		nrc_gpio_outputb(RED_LED_PIN, toggle);
	}
#endif

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

	ret = run_sample_gpio(TEST_COUNT, TEST_INTERVAL);
	nrc_usr_print("[%s] test result!! %s \n",__func__, (ret==0) ?  "Success" : "Fail");
}
