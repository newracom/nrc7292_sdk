/*
 * MIT License
 *
 * Copyright (c) 2022 Newracom, Inc.
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


#if defined(NRC7292)
#define ATCMD_CHIP_NAME		"NRC7292"
# if defined(CPU_CM3)
# define ATCMD_CPU_TYPE		"CM3"
# else
# define ATCMD_CPU_TYPE		"CM0"
# endif
#elif defined(NRC7394)
#define ATCMD_CHIP_NAME		"NRC7394"
#define ATCMD_CPU_TYPE		"CM3"
#else
#error "!!! unknown system !!!"
#endif

#if defined(CONFIG_ATCMD_IPV6)
#define ATCMD_IP_VER		"IPv6"
#else
#define ATCMD_IP_VER		"IPv4"
#endif


static void nrc_atcmd_build_info (void)
{
	_atcmd_info("BUILD: %s,%s,%s (%s, %s)",
					ATCMD_CHIP_NAME,
					ATCMD_CPU_TYPE,
					ATCMD_IP_VER,
					__TIME__,
					__DATE__
					);
}

static void nrc_atcmd_version_info (void)
{
#if defined(SDK_VER_DESCRIPTION)
	_atcmd_info("VERSION: %d.%d.%d (SDK-%d.%d.%d-%s)",
					ATCMD_VER_MAJOR, ATCMD_VER_MINOR, ATCMD_VER_REVISION,
					SDK_VER_MAJOR, SDK_VER_MINOR, SDK_VER_REVISION, SDK_VER_DESCRIPTION);
#else
	_atcmd_info("VERSION: %d.%d.%d (SDK-%d.%d.%d)",
					ATCMD_VER_MAJOR, ATCMD_VER_MINOR, ATCMD_VER_REVISION,
					SDK_VER_MAJOR, SDK_VER_MINOR, SDK_VER_REVISION);
#endif
}

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
#if defined(NRC7292)
	uint32_t bd_id = 0x07020902;
#elif defined(NRC7394)
	uint32_t bd_id = 0x07030904;
#endif

	*console_enable = true;
	nrc_uart_console_enable(true);

	nrc_atcmd_build_info();
	nrc_atcmd_version_info();

	return nrc_atcmd_enable(_HIF_TYPE_HSPI, sw_id, bd_id);
}

#elif defined(CONFIG_ATCMD_UART) || defined(CONFIG_ATCMD_UART_HFC)

static int nrc_atcmd_enable_uart (bool *console_enable)
{
#ifndef CONFIG_ATCMD_UART_BAUDRATE
#define CONFIG_ATCMD_UART_BAUDRATE 	115200
#endif

#if defined(NRC7292)
	const uint32_t console = NRC_UART_CH3;
	uint32_t channel = NRC_UART_CH2;
#elif defined(NRC7394)
	const uint32_t console = NRC_UART_CH0;
	uint32_t channel = NRC_UART_CH1;
#endif
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
		nrc_uart_console_enable(true);

		nrc_atcmd_build_info();
		nrc_atcmd_version_info();
	}

	return nrc_atcmd_enable((hfc ? _HIF_TYPE_UART_HFC : _HIF_TYPE_UART), channel, baudrate);
}
#endif /* #if defined(CONFIG_ATCMD_HSPI) */

/******************************************************************************
 * FunctionName : user_init
 * Description  : Start Code for User Application, Initialize User function
 * Parameters   : none
 * Returns	  : none
 *******************************************************************************/

void user_init (void)
{
	bool console_enable;
	int ret;

#if defined(CONFIG_ATCMD_HSPI)
	ret = nrc_atcmd_enable_hspi(&console_enable);
#elif defined(CONFIG_ATCMD_UART) || defined(CONFIG_ATCMD_UART_HFC)
	ret = nrc_atcmd_enable_uart(&console_enable);
#endif

	if (ret != 0)
	{
		nrc_atcmd_disable();

		if (console_enable)
			_atcmd_info("Exit AT Command");
	}
}

/**********************************************************************************************/

#if defined(CONFIG_ATCMD_CLI)

#if !defined(CONFIG_ATCMD_CLI_MINIMUM)
static int cmd_atcmd_info (cmd_tbl_t *t, int argc, char *argv[])
{
	int ret = CMD_RET_SUCCESS;

	switch (--argc)
	{
		case 0:
			nrc_atcmd_build_info();
			nrc_atcmd_version_info();
			break;

		default:
			ret = CMD_RET_USAGE;
	}

	return ret;
}

SUBCMD_MAND(atcmd,
		info,
		cmd_atcmd_info,
		"build and version info",
		"atcmd info");
#endif /* #if !defined(CONFIG_ATCMD_CLI_MINIMUM) */


CMD_MAND(atcmd,
    NULL,
    "AT Command",
    "atcmd <subcmd> [options]");

#endif /* #if defined(CONFIG_ATCMD_CLI) */
