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

#include "atcmd.h"

#if defined(CPU_CM3)
#define ATCMD_CPU_TYPE		"CM3"
#else
#define ATCMD_CPU_TYPE		"CM0"
#endif

#if defined(NRC7292_STANDALONE_XIP)
#define ATCMD_MEM_TYPE		"XIP"
#else
#define ATCMD_MEM_TYPE		"RAM"
#endif

/*******************************************************************************/

static bool g_nrc_atcmd_enable = false;

static int nrc_atcmd_enable (int type, ...)
{
	_hif_info_t info;
	va_list ap;
	int ret;

	memset(&info, 0, sizeof(info));

	switch (type)
	{
		case _HIF_TYPE_HSPI:
			va_start(ap, type);
			info.hspi.sw_id = va_arg(ap, int);
			info.hspi.bd_id = va_arg(ap, int);
			va_end(ap);
			break;

		case _HIF_TYPE_UART:
		case _HIF_TYPE_UART_HFC:
			va_start(ap, type);
			info.uart.channel = va_arg(ap, int);
			info.uart.baudrate = va_arg(ap, int);
			va_end(ap);

			info.uart.data_bits = UART_DB8;
			info.uart.stop_bits = UART_SB1;
			info.uart.parity = UART_PB_NONE;
			info.uart.hfc = (type == _HIF_TYPE_UART_HFC) ? UART_HFC_ENABLE : UART_HFC_DISABLE;
			break;

		default:
			return -1;
	}

	info.type = type;

	ret	= atcmd_enable(&info);

	if (ret == 0)
		g_nrc_atcmd_enable = true;

	return ret;
}

static void nrc_atcmd_disable (void)
{
	atcmd_disable();

	g_nrc_atcmd_enable = false;
}

bool nrc_atcmd_is_enabled (void)
{
	return g_nrc_atcmd_enable;
}

/*******************************************************************************/

#if defined(CONFIG_ATCMD_HSPI)
static int nrc_atcmd_enable_hspi (bool *console_enable)
{
	uint32_t sw_id = ((VERSION_MAJOR & 0xff) << 16) |
					 ((VERSION_MINOR & 0xff) << 8) |
					 (VERSION_REVISION & 0xff);
	uint32_t bd_id = 0x7292A002;

	*console_enable = true;

	nrc_uart_console_enable();

	nrc_usr_print("AT Command for NRC Halow.[%s/%s/HSPI]\n", ATCMD_CPU_TYPE, ATCMD_MEM_TYPE);

	return nrc_atcmd_enable(_HIF_TYPE_HSPI, sw_id, bd_id);
}

#elif defined(CONFIG_ATCMD_UART) || defined(CONFIG_ATCMD_UART_HFC)

static int nrc_atcmd_enable_uart (bool *console_enable)
{
#ifndef CONFIG_ATCMD_UART_BAUDRATE
#define CONFIG_ATCMD_UART_BAUDRATE 	115200
#endif

	const uint32_t console = NRC_UART_CH3;
	uint32_t channel = 2;
	uint32_t baudrate = CONFIG_ATCMD_UART_BAUDRATE;
	int hfc = false;

#if defined(CONFIG_ATCMD_UART_HFC)
	hfc = true;
#endif

	if (channel == console)
		*console_enable = false;
	else
	{
		*console_enable = true;

		nrc_uart_console_enable();

		nrc_usr_print("AT Command for NRC Halow.[%s/UART_%s_%d_%d]\n",
						ATCMD_MEM_TYPE, hfc ? "HFC" : "",
						channel, baudrate);
	}

	return nrc_atcmd_enable((hfc ? _HIF_TYPE_UART_HFC : _HIF_TYPE_UART), channel, baudrate);
}

#else
static int nrc_atcmd_unknown_hif (bool *console_enable)
{
	*console_enable = true;

	nrc_uart_console_enable();

	nrc_usr_print("AT Command for NRC Halow.[%s]\n", ATCMD_CODE_MEM_TYPE);
	nrc_usr_print(" > A HIF type is not defined.\n");
	nrc_usr_print(" > Define one of below.\n");
	nrc_usr_print("   CONFIG_ATCMD_HSPI\n");
	nrc_usr_print("   CONFIG_ATCMD_UART\n");
	nrc_usr_print("   CONFIG_ATCMD_UART_HFC\n");

	return -1;
}

#endif /* #if defined(CONFIG_ATCMD_HSPI) */

/******************************************************************************
 * FunctionName : user_init
 * Description  : Start Code for User Application, Initialize User function
 * Parameters   : none
 * Returns	  : none
 *******************************************************************************/
void user_init(void)
{
	bool console_enable;
	int ret;

#if defined(CONFIG_ATCMD_HSPI)
	ret = nrc_atcmd_enable_hspi(&console_enable);
#elif defined(CONFIG_ATCMD_UART) || defined(CONFIG_ATCMD_UART_HFC)
	ret = nrc_atcmd_enable_uart(&console_enable);
#else
	ret = nrc_atcmd_unknown_hif(&console_enable);
#endif

	if (ret == 0)
	{
		vTaskSuspend(NULL);

		nrc_atcmd_disable();
	}

	if (console_enable)
		nrc_usr_print("Exit AT Command for NRC Halow\n");
}
