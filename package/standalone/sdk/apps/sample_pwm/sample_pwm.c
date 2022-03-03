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
 * FunctionName : run_sample_pwm
 * Description  : sample test for pwm
 * Parameters   : count(test count), interval(test interval)
 * Returns      : 0 or -1 (0: success, -1: fail)
 *******************************************************************************/
nrc_err_t run_sample_pwm(int count, int interval)
{
	int i = 0;

	nrc_usr_print("[%s] Sample App for PWM API \n",__func__);

	nrc_pwm_hw_init(PWM_CH0, GPIO_08, 0);
	nrc_pwm_hw_init(PWM_CH1, GPIO_09, 0);
	nrc_pwm_hw_init(PWM_CH2, GPIO_10, 0);
	nrc_pwm_hw_init(PWM_CH3, GPIO_11, 0);

	nrc_pwm_set_config(PWM_CH0, 0xF0000000, 0x0, 0x0, 0x0);
	nrc_pwm_set_config(PWM_CH1, 0xFFFF0000, 0x0, 0x0, 0x0);
	nrc_pwm_set_config(PWM_CH2, 0xFFFFFFFF, 0xFFFFFFFF, 0x0, 0x0);
	nrc_pwm_set_config(PWM_CH3, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF);

	nrc_pwm_set_enable(PWM_CH0, 1);
	nrc_pwm_set_enable(PWM_CH1, 1);
	nrc_pwm_set_enable(PWM_CH2, 1);
	nrc_pwm_set_enable(PWM_CH3, 1);

	_delay_ms(5000);

	uint32_t a, b, c, d;

	c = 0xFFFFFFFF;
	d = 0x000FFFFF;
	for (a = 0; a < 5; a++) {
		nrc_pwm_set_config(PWM_CH0, c << a, c << a, c << a, c << a);
		nrc_pwm_set_config(PWM_CH1, a, a, a, a);
		nrc_pwm_set_config(PWM_CH2, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF);
		nrc_pwm_set_config(PWM_CH3, d >> a, d >> (a * 2), d >> (a * 3), d >> (a * 4));
		_delay_ms(1000);
		nrc_pwm_set_config(PWM_CH0, 0x0, 0x0, 0x0, 0x0);
		nrc_pwm_set_config(PWM_CH1, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF);
		nrc_pwm_set_config(PWM_CH2, 0x0, 0x0, 0x0, 0x0);
		nrc_pwm_set_config(PWM_CH3, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF);
		_delay_ms(1000);
	}

	for(i=0;i<count; i++)
	{
		nrc_usr_print("[%s] Print GPIO_08\n",__func__);
		nrc_pwm_set_config(PWM_CH0, 0, 0, 0, 0);
		nrc_pwm_set_config(PWM_CH1, 0, 0, 0, 0);
		nrc_pwm_set_config(PWM_CH2, 0, 0, 0, 0);
		nrc_pwm_set_config(PWM_CH3, 0, 0, 0, 0);
		_delay_ms(500);

		a = 1;
		b = c = d = 0;
		while (a != 0xFFFFFFFF)
		{
			nrc_usr_print("[%s] Print GPIO_08\n",__func__);
			nrc_pwm_set_config(PWM_CH0, a, 0, 0, 0);
			a = (a << 1) | 0x1;
			_delay_ms(50);
		}
		b = 1;
		while (b != 0xFFFFFFFF)
		{
			nrc_usr_print("[%s] Print GPIO_09\n",__func__);
			nrc_pwm_set_config(PWM_CH1, 0, b, 0, 0);
			b = (b << 1) | 0x1;
			_delay_ms(50);
		}
		c = 1;
		while (c != 0xFFFFFFFF)
		{
			nrc_usr_print("[%s] Print GPIO_10\n",__func__);
			nrc_pwm_set_config(PWM_CH2, 0, 0, c, 0);
			c = (c << 1) | 0x1;
			_delay_ms(50);
		}
		d = 1;
		while (d != 0xFFFFFFFF)
		{
			nrc_usr_print("[%s] Print GPIO_11\n",__func__);
			nrc_pwm_set_config(PWM_CH3, 0, 0, 0, d);
			d = (d << 1) | 0x1;
			_delay_ms(50);
		}

		_delay_ms(1000);
		nrc_pwm_set_config(PWM_CH0, 0, 0, 0, 0);
		nrc_pwm_set_config(PWM_CH1, 0, 0, 0, 0);
		nrc_pwm_set_config(PWM_CH2, 0, 0, 0, 0);
		nrc_pwm_set_config(PWM_CH3, 0, 0, 0, 0);
		_delay_ms(500);
		nrc_pwm_set_config(PWM_CH0, a, b, c, d);
		nrc_pwm_set_config(PWM_CH1, a, b, c, d);
		nrc_pwm_set_config(PWM_CH2, a, b, c, d);
		nrc_pwm_set_config(PWM_CH3, a, b, c, d);
		_delay_ms(500);

		while (a != 0x0)
		{
			nrc_usr_print("[%s] Print GPIO_08\n",__func__);
			nrc_pwm_set_config(PWM_CH0, a, 0, 0, 0);
			a = a >> 1;
			_delay_ms(50);
		}
		while (b != 0x0)
		{
			nrc_usr_print("[%s] Print GPIO_09\n",__func__);
			nrc_pwm_set_config(PWM_CH1, 0, b, 0, 0);
			b = b >> 1;
			_delay_ms(50);
		}
		while (c != 0x0)
		{
			nrc_usr_print("[%s] Print GPIO_10\n",__func__);
			nrc_pwm_set_config(PWM_CH2, 0, 0, c, 0);
			c = c >> 1;
			_delay_ms(50);
		}
		while (d != 0x0)
		{
			nrc_usr_print("[%s] Print GPIO_11\n",__func__);
			nrc_pwm_set_config(PWM_CH3, 0, 0, 0, d);
			d = d >> 1;
			_delay_ms(50);
		}
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

	ret = run_sample_pwm(TEST_COUNT, TEST_INTERVAL);
	nrc_usr_print("[%s] test result!! %s \n",__func__, (ret==0) ?  "Success" : "Fail");
}
