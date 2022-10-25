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
#include "api_ps.h"

/**********************************************************************************************/

static int _atcmd_basic_version_get (int argc, char *argv[])
{
	switch (argc)
	{
		case 0:
		{
			char str_sdk_ver[ATCMD_STR_SIZE(8)];
			char str_atcmd_ver[ATCMD_STR_SIZE(8)];
			char param_sdk_ver[ATCMD_STR_PARAM_SIZE(8)];
			char param_atcmd_ver[ATCMD_STR_PARAM_SIZE(8)];

			snprintf(str_atcmd_ver, sizeof(str_atcmd_ver), "%u.%u.%u",
						ATCMD_VER_MAJOR, ATCMD_VER_MINOR, ATCMD_VER_REVISION);

			snprintf(str_sdk_ver, sizeof(str_sdk_ver), "%u.%u.%u",
						SDK_VER_MAJOR, SDK_VER_MINOR, SDK_VER_REVISION);

			if (!atcmd_str_to_param(str_sdk_ver, param_sdk_ver, sizeof(param_sdk_ver)) ||
				!atcmd_str_to_param(str_atcmd_ver, param_atcmd_ver, sizeof(param_atcmd_ver)))
				return ATCMD_ERROR_FAIL;

			ATCMD_MSG_INFO("VER", "%s,%s", param_sdk_ver, param_atcmd_ver);

			break;
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static atcmd_info_t g_atcmd_basic_version =
{
	.list.next = NULL,
	.list.prev = NULL,

	.group = ATCMD_GROUP_BASIC,

	.cmd = "VER",
	.id = ATCMD_BASIC_VERSION,

	.handler[ATCMD_HANDLER_RUN] = NULL,
	.handler[ATCMD_HANDLER_GET] = _atcmd_basic_version_get,
	.handler[ATCMD_HANDLER_SET] = NULL,
};

/**********************************************************************************************/

#if defined(CONFIG_ATCMD_UART) || defined(CONFIG_ATCMD_UART_HFC) || \
	defined(CONFIG_ATCMD_UART_HOST) || defined(CONFIG_ATCMD_UART_HFC_HOST)

static int _atcmd_basic_uart_get (int argc, char *argv[])
{
	switch (_hif_get_type())
	{
		case _HIF_TYPE_UART:
		case _HIF_TYPE_UART_HFC:
			break;

		default:
			return ATCMD_ERROR_NOTSUPP;
	}

	switch (argc)
	{
		case 0:
		{
			_hif_uart_t uart;

			_hif_uart_get_info(&uart);

			ATCMD_MSG_INFO("UART", "%d,%d,%d,%d,%d",
							uart.baudrate,
							uart.data_bits + 5, uart.stop_bits + 1, uart.parity,
							uart.hfc);
			break;
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static int _atcmd_basic_uart_set (int argc, char *argv[])
{
	char *param_baudrate = NULL;
	char *param_hfc = NULL;

	switch (_hif_get_type())
	{
		case _HIF_TYPE_UART:
		case _HIF_TYPE_UART_HFC:
			break;

		default:
			return ATCMD_ERROR_NOTSUPP;
	}

	switch (argc)
	{
		case 0:
			ATCMD_MSG_HELP("AT+UART=<baudrate>,<HFC>");
			break;

		case 2:
		{
			_hif_uart_t uart =
			{
				.channel = 2,
				/* .baudrate = , */
				/* .hfc = , */

				.data_bits = UART_DB8,
				.stop_bits = UART_SB1,
				.parity = UART_PB_NONE,
			};
			int ret;

			param_baudrate = argv[0];
			param_hfc = argv[1];

			uart.baudrate = atoi(param_baudrate);

			switch (atoi(param_hfc))
			{
				case 0: uart.hfc = false; break;
				case 1: uart.hfc = true; break;

				default:
					return ATCMD_ERROR_INVAL;
			}

			ret = _hif_uart_change(&uart);
			if (ret == 0)
				break;
			else if (ret == -1)
				return ATCMD_ERROR_FAIL;
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static atcmd_info_t g_atcmd_basic_uart =
{
	.list.next = NULL,
	.list.prev = NULL,

	.group = ATCMD_GROUP_BASIC,

	.cmd = "UART",
	.id = ATCMD_BASIC_UART,

	.handler[ATCMD_HANDLER_RUN] = NULL,
	.handler[ATCMD_HANDLER_GET] = _atcmd_basic_uart_get,
	.handler[ATCMD_HANDLER_SET] = _atcmd_basic_uart_set,
};

#endif /* #if defined(CONFIG_ATCMD_UART) || defined(CONFIG_ATCMD_UART_HFC) || \
			  defined(CONFIG_ATCMD_UART_HOST) || defined(CONFIG_ATCMD_UART_HFC_HOST) */

/**********************************************************************************************/

static bool _atcmd_gpio_pin_valid (int pin)
{
	switch (pin)
	{
/*		case GPIO_02:
		case GPIO_03:
		{
			_hif_uart_t uart;

			_hif_uart_get_info(&uart);

			if (uart.hfc == UART_HFC_ENABLE)
				break;
		} */

		case GPIO_08:
		case GPIO_09:
		case GPIO_10:
		case GPIO_11:
		case GPIO_12:
		case GPIO_13:
		case GPIO_14:
		case GPIO_15:
		case GPIO_16:
		case GPIO_17:
			return true;
	}

	return false;
}

static void _atcmd_gpio_get_config (NRC_GPIO_CONFIG *config)
{
	gpio_io_t reg;

	nrc_gpio_get_alt(&reg);

	if (reg.word & (1 << config->gpio_pin))
		config->gpio_alt = GPIO_NOMAL_OP;
	else
		config->gpio_alt = GPIO_FUNC;

	nrc_gpio_get_dir(&reg);

	if (reg.word & (1 << config->gpio_pin))
		config->gpio_dir = GPIO_OUTPUT;
	else
		config->gpio_dir = GPIO_INPUT;

	config->gpio_mode = GPIO_FLOATING;

	nrc_gpio_get_pullup(&reg);

	if (reg.word & (1 << config->gpio_pin))
		config->gpio_mode = GPIO_PULL_UP;

	nrc_gpio_get_pulldown(&reg);

	if (reg.word & (1 << config->gpio_pin))
	{
		if (config->gpio_mode == GPIO_PULL_UP)
			_atcmd_info("Both pullup and pulldown are enabled.");

		config->gpio_mode = GPIO_PULL_DOWN;
	}
}

static void _atcmd_gpio_set_config (NRC_GPIO_CONFIG *config)
{
	nrc_gpio_config(config);
}

static int _atcmd_basic_gpio_config_get (int argc, char *argv[])
{
	int index = -1;

	switch (argc)
	{
		case 1:
			index = atoi(argv[0]);

			if (!_atcmd_gpio_pin_valid(index))
				return ATCMD_ERROR_INVAL;

		case 0:
		{
			NRC_GPIO_CONFIG config;
			int start_index;
			int end_index;

			if (index != -1)
				start_index = end_index = index;
			else
			{
				start_index = GPIO_08;
				end_index = GPIO_17;
			}

			for (index = start_index ; index <= end_index ; index++)
			{
				config.gpio_pin = index;

				_atcmd_gpio_get_config(&config);

				ATCMD_MSG_INFO("GPIOCONF", "%d,%d,%d",
								config.gpio_pin, config.gpio_dir,
								config.gpio_mode == GPIO_PULL_UP ? 1 : 0);
			}

			break;
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static int _atcmd_basic_gpio_config_set (int argc, char *argv[])
{
	char *param_index = NULL;
	char *param_dir = NULL;
	char *param_pullup = NULL;

	switch (argc)
	{
		case 0:
			ATCMD_MSG_HELP("AT+GPIOCONF=<index>,<direction>[,<pull-up]]");
			break;

		case 3:
			param_pullup = argv[2];

		case 2:
		{
			NRC_GPIO_CONFIG config =
			{
				.gpio_alt = GPIO_FUNC,
			};

			param_index = argv[0];
			param_dir = argv[1];

			if (!_atcmd_gpio_pin_valid(atoi(param_index)))
				return ATCMD_ERROR_INVAL;

			config.gpio_pin = atoi(param_index);
			config.gpio_dir = GPIO_INPUT;
			config.gpio_mode = GPIO_FLOATING;

			if (param_dir)
			{
				switch (atoi(param_dir))
				{
					case 0: config.gpio_dir = GPIO_INPUT; break;
					case 1: config.gpio_dir = GPIO_OUTPUT; break;

					default:
						return ATCMD_ERROR_INVAL;
				}
			}

			if (param_pullup)
			{
				switch (atoi(param_pullup))
				{
					case 0: config.gpio_mode = GPIO_FLOATING; break;
					case 1: config.gpio_mode = GPIO_PULL_UP; break;

					default:
						return ATCMD_ERROR_INVAL;
				}
			}

			_atcmd_gpio_set_config(&config);
			break;
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static atcmd_info_t g_atcmd_basic_gpio_config =
{
	.list.next = NULL,
	.list.prev = NULL,

	.group = ATCMD_GROUP_BASIC,

	.cmd = "GPIOCONF",
	.id = ATCMD_BASIC_GPIOCFG,

	.handler[ATCMD_HANDLER_RUN] = NULL,
	.handler[ATCMD_HANDLER_GET] = _atcmd_basic_gpio_config_get,
	.handler[ATCMD_HANDLER_SET] = _atcmd_basic_gpio_config_set,
};

/**********************************************************************************************/

static int _atcmd_basic_gpio_value_get (int argc, char *argv[])
{
	int index = -1;

	switch (argc)
	{
		case 1:
			index = atoi(argv[0]);

			if (!_atcmd_gpio_pin_valid(index))
				return ATCMD_ERROR_INVAL;

		case 0:
		{
			int start_index;
			int end_index;
			int value;

			if (index != -1)
				start_index = end_index = index;
			else
			{
				start_index = GPIO_08;
				end_index = GPIO_17;
			}

			for (index = start_index ; index <= end_index ; index++)
			{
				if (nrc_gpio_inputb(index, &value) != NRC_SUCCESS)
					return ATCMD_ERROR_FAIL;

				ATCMD_MSG_INFO("GPIOVAL", "%d,%d", index, value);
			}

			break;
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static int _atcmd_basic_gpio_value_set (int argc, char *argv[])
{
	switch (argc)
	{
		case 0:
			ATCMD_MSG_HELP("AT+GPIOVAL=<index>,<level>");
			break;

		case 2:
		{
			int index = atoi(argv[0]);
			int level = atoi(argv[1]);

			if (!_atcmd_gpio_pin_valid(index))
				return ATCMD_ERROR_INVAL;

			if (level != 0 && level != 1)
				return ATCMD_ERROR_INVAL;

			nrc_gpio_outputb(index, level);
			break;
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static atcmd_info_t g_atcmd_basic_gpio_value =
{
	.list.next = NULL,
	.list.prev = NULL,

	.group = ATCMD_GROUP_BASIC,

	.cmd = "GPIOVAL",
	.id = ATCMD_BASIC_GPIOVAL,

	.handler[ATCMD_HANDLER_RUN] = NULL,
	.handler[ATCMD_HANDLER_GET] = _atcmd_basic_gpio_value_get,
	.handler[ATCMD_HANDLER_SET] = _atcmd_basic_gpio_value_set,
};

/**********************************************************************************************/

static bool _atcmd_adc_channel_valid (int channel)
{
	switch (channel)
	{
		case ADC1:
		case ADC2:
		case ADC3:
			return true;

		default:
			return false;
	}
}

static int _atcmd_adc_get_value (int channel)
{
	uint16_t value;

	nrc_adc_init();
	_delay_ms(100);

	nrc_adc_get_data(channel, &value);

	nrc_adc_deinit();

	return value;
}

static int _atcmd_basic_adc_get (int argc, char *argv[])
{
	switch (argc)
	{
		case 1:
		{
			int channel = atoi(argv[0]);
			int value;

			if (!_atcmd_adc_channel_valid(channel))
				return ATCMD_ERROR_INVAL;

			value = _atcmd_adc_get_value(channel);

			ATCMD_MSG_INFO("ADC", "%d,%d", channel, value);

			break;
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static int _atcmd_basic_adc_set (int argc, char *argv[])
{
	switch (argc)
	{
		case 0:
			ATCMD_MSG_HELP("AT+ADC=<ADC channel index>");
			break;

		case 1:
		{
			int channel = atoi(argv[0]);
			int value;

			if (!_atcmd_adc_channel_valid(channel))
				return ATCMD_ERROR_INVAL;

			value = _atcmd_adc_get_value(channel);

			ATCMD_MSG_INFO("ADC", "%d,%d", channel, value);

			break;
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}
static atcmd_info_t g_atcmd_basic_adc =
{
	.list.next = NULL,
	.list.prev = NULL,

	.group = ATCMD_GROUP_BASIC,

	.cmd = "ADC",
	.id = ATCMD_BASIC_ADC,

	.handler[ATCMD_HANDLER_RUN] = NULL,
	.handler[ATCMD_HANDLER_GET] = _atcmd_basic_adc_get,
	.handler[ATCMD_HANDLER_SET] = _atcmd_basic_adc_set,
};

/**********************************************************************************************/

#define ATCMD_TIMEOUT_CMD_LEN_MAX		20

typedef struct
{
	char *cmd;
	uint32_t sec;
} atcmd_timeout_t;

static atcmd_timeout_t g_atcmd_timeout_wifi[] =
{
	{ "WSCAN", 0 },
	{ "WCONN", 0 },
	{ "WDISCONN", 0 },
	{ "WDHCP", 60 },

	{ NULL, 0 }
};

#ifndef CONFIG_ATCMD_WITHOUT_LWIP
static atcmd_timeout_t g_atcmd_timeout_socket[] =
{
	{ "SOPEN", 30 },
	{ "SSEND", 1 },

	{ NULL, 0 }
};
#else
static atcmd_timeout_t g_atcmd_timeout_host[] =
{
	{ "HSEND", 1 },

	{ NULL, 0 }
};
#endif

static atcmd_timeout_t *g_atcmd_timeout[2] =
{
	g_atcmd_timeout_wifi,
#ifndef CONFIG_ATCMD_WITHOUT_LWIP
	g_atcmd_timeout_socket,
#else
	g_atcmd_timeout_host,
#endif
};

uint32_t _atcmd_timeout_value (const char *cmd)
{
	atcmd_timeout_t *timeout;
	int i;

	if (cmd[0] == 'W')
		timeout = g_atcmd_timeout_wifi;
#ifndef CONFIG_ATCMD_WITHOUT_LWIP
	else if (cmd[0] == 'S')
		timeout = g_atcmd_timeout_socket;
#else
	else if (cmd[0] == 'H')
		timeout = g_atcmd_timeout_host;
#endif
	else
	{
		_atcmd_error("invalid prefix\n");
		return 0;
	}

	for (i = 0 ; timeout[i].cmd ; i++)
	{
		if (strcmp(cmd, timeout[i].cmd) == 0)
			break;
	}

	if (!timeout[i].cmd)
	{
		_atcmd_error("no command\n");
		return 0;
	}

	return timeout[i].sec * 1000; // msec
}

int _atcmd_basic_timeout_get (int argc, char *argv[])
{
	atcmd_timeout_t *timeout;
	int i = 0;

	switch (argc)
	{
		case 1:
		{
			char *param_group = argv[0];

			i = atoi(argv[0]);

			switch (i)
			{
				case 0:
				case 1:
					break;

				default:
					return ATCMD_ERROR_INVAL;
			}
		}

		case 0:
		{
			char param_cmd[ATCMD_STR_PARAM_SIZE(ATCMD_TIMEOUT_CMD_LEN_MAX)];
			int j;

			for ( ; i < 2 ; i++)
			{
				timeout = g_atcmd_timeout[i];

				for (j = 0 ; timeout[j].cmd ; j++)
				{
					if (!atcmd_str_to_param(timeout[j].cmd, param_cmd, sizeof(param_cmd)))
						return ATCMD_ERROR_FAIL;

					if (timeout[j].cmd[0] == 'W')
						ATCMD_MSG_INFO("WTIMEOUT", "%s,%d", param_cmd, timeout[j].sec);
#ifndef CONFIG_ATCMD_WITHOUT_LWIP
					else if (timeout[j].cmd[0] == 'S')
						ATCMD_MSG_INFO("STIMEOUT", "%s,%d", param_cmd, timeout[j].sec);
#else
					else if (timeout[j].cmd[0] == 'H')
						ATCMD_MSG_INFO("HTIMEOUT", "%s,%d", param_cmd, timeout[j].sec);
#endif
				}

				if (argc == 1)
					break;
			}
			break;
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

int _atcmd_basic_timeout_set (int argc, char *argv[])
{
	char *param_cmd = NULL;
	char *param_time = NULL;

	switch (argc)
	{
		case 0:
			ATCMD_MSG_HELP("AT+TIMEOUT=\"<command>\",<time>");
			break;

		case 2:
		{
			char str_cmd[ATCMD_STR_SIZE(ATCMD_TIMEOUT_CMD_LEN_MAX)];
			atcmd_timeout_t *timeout;
			int time_sec;
			int i;

			param_cmd = argv[0];
			param_time = argv[1];

			if (!atcmd_param_to_str(param_cmd, str_cmd, sizeof(str_cmd)))
				return ATCMD_ERROR_FAIL;

			if (str_cmd[0] == 'W')
				timeout = g_atcmd_timeout_wifi;
#ifndef CONFIG_ATCMD_WITHOUT_LWIP
			else if (str_cmd[0] == 'S')
				timeout = g_atcmd_timeout_socket;
#else
			else if (str_cmd[0] == 'H')
				timeout = g_atcmd_timeout_host;
#endif
			else
				return ATCMD_ERROR_INVAL;

			for (i = 0 ; timeout[i].cmd ; i++)
			{
				if (strcmp(str_cmd, timeout[i].cmd) == 0)
					break;
			}

			if (!timeout[i].cmd)
				return ATCMD_ERROR_INVAL;

			time_sec = atoi(param_time);
			if (time_sec < 0)
				return ATCMD_ERROR_INVAL;

			timeout[i].sec = time_sec;
			break;
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static atcmd_info_t g_atcmd_basic_timeout =
{
	.list.next = NULL,
	.list.prev = NULL,

	.group = ATCMD_GROUP_BASIC,

	.cmd = "TIMEOUT",
	.id = ATCMD_BASIC_TIMEOUT,

	.handler[ATCMD_HANDLER_RUN] = NULL,
	.handler[ATCMD_HANDLER_GET] = _atcmd_basic_timeout_get,
	.handler[ATCMD_HANDLER_SET] = _atcmd_basic_timeout_set,
};

/**********************************************************************************************/

static atcmd_group_t g_atcmd_group_basic =
{
	.list.next = NULL,
	.list.prev = NULL,

	.name = "BASIC",
	.id = ATCMD_GROUP_BASIC,

	.cmd_prefix = "",
	.cmd_prefix_size = 0,

	.cmd_list_head.next = NULL,
	.cmd_list_head.prev = NULL,
};

static atcmd_info_t *g_atcmd_basic[] =
{
	&g_atcmd_basic_version,
#if defined(CONFIG_ATCMD_UART) || defined(CONFIG_ATCMD_UART_HFC) || \
	defined(CONFIG_ATCMD_UART_HOST) || defined(CONFIG_ATCMD_UART_HFC_HOST)
	&g_atcmd_basic_uart,
#endif
	&g_atcmd_basic_gpio_config,
	&g_atcmd_basic_gpio_value,
	&g_atcmd_basic_adc,

/*	&g_atcmd_basic_timeout, */

	NULL
};

int atcmd_basic_enable (void)
{
	int i;

	if (atcmd_group_register(&g_atcmd_group_basic) != 0)
		return -1;

	for (i = 0 ; g_atcmd_basic[i] ; i++)
	{
		if (atcmd_info_register(ATCMD_GROUP_BASIC, g_atcmd_basic[i]) != 0)
			return -1;
	}

#ifdef CONFIG_ATCMD_DEBUG
	atcmd_info_print(&g_atcmd_group_basic);
#endif

	return 0;
}

void atcmd_basic_disable (void)
{
	int i;

	for (i = 0 ; g_atcmd_basic[i] ; i++)
		atcmd_info_unregister(ATCMD_GROUP_BASIC, g_atcmd_basic[i]->id);

	atcmd_group_unregister(ATCMD_GROUP_BASIC);
}
