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

extern const char *str_log_type[];
extern const char c_log_type[];

static int _atcmd_basic_log_get (int argc, char *argv[])
{
	switch (argc)
	{
		case 0:
		{
			int type;
			char buf[60];
			int len;
			int i;

			for (len = 0, i = 0 ; i < ATCMD_LOG_TYPE_NUM ; i++)
			{
				type = 1 << i;

				if (ATCMD_LOG_VALID(type))
				{
					len += snprintf(&buf[len], sizeof(buf) - len, "%s=%d ",
									str_log_type[i], !!ATCMD_LOG_STATUS(type));
				}
			}

			buf[len - 1] = '\0';

			ATCMD_LOG_INFO("LOG", "%s", "%s", buf);

			return 0;
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static int _atcmd_basic_log_set (int argc, char *argv[])
{
	switch (argc)
	{
		case 0:
			ATCMD_LOG_HELP("AT+LOG={reset}");
			ATCMD_LOG_HELP("AT+LOG={r|i|e|d|h|t}");
			ATCMD_LOG_HELP("AT+LOG={resp|info|event|data|help|trace}");
			break;

		case 1:
		{
			char *param = NULL;
			int type;
			int i;

			param = argv[0];
			strupr(param);

			if (strcmp(param, "RESET") == 0)
			{
				ATCMD_LOG_RESET();
				break;
			}

			for (i = 0 ; i < ATCMD_LOG_TYPE_NUM ; i++)
			{
				switch (strlen(param))
				{
					case 1:
						if (*param != c_log_type[i])
							continue;
						break;

					default:
						if (strcmp(param, str_log_type[i]) != 0)
							continue;
				}

				type = 1 << i;

				if (!!ATCMD_LOG_STATUS(type))
					ATCMD_LOG_DISABLE(type);
				else
					ATCMD_LOG_ENABLE(type);

				_atcmd_info("LOG_%s: %s", str_log_type[i],
							ATCMD_LOG_STATUS(type) ? "enable" : "disable");

				break;
			}

			if (i < ATCMD_LOG_TYPE_NUM)
				break;
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static atcmd_info_t g_atcmd_basic_log =
{
	.list.next = NULL,
	.list.prev = NULL,

	.group = ATCMD_GROUP_BASIC,

	.cmd = "LOG",
	.id = ATCMD_BASIC_LOG,

	.handler[ATCMD_HANDLER_RUN] = NULL,
	.handler[ATCMD_HANDLER_GET] = _atcmd_basic_log_get,
	.handler[ATCMD_HANDLER_SET] = _atcmd_basic_log_set,
};

/**********************************************************************************************/

static int _atcmd_basic_mode_get (int argc, char *argv[])
{
	switch (argc)
	{
		case 0:
			ATCMD_LOG_INFO("MODE", "%d", "%d", atcmd_mode_get());
			break;

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static int _atcmd_basic_mode_set (int argc, char *argv[])
{
	switch (argc)
	{
		case 0:
			ATCMD_LOG_HELP("AT+MODE={0|1|2}");
			ATCMD_LOG_HELP("  0: terminal only");
			ATCMD_LOG_HELP("  1: host only");
			ATCMD_LOG_HELP("  2: terminal & host");
			break;

		case 1:
		{
			int mode = atoi(argv[0]);

			switch (mode)
			{
				case ATCMD_MODE_NORMAL:
				case ATCMD_MODE_TERMINAL:
#ifdef CONFIG_ATCMD_HOST_MODE
				case ATCMD_MODE_HOST:
#endif
					atcmd_mode_set(mode);
					return ATCMD_SUCCESS;
			}
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static atcmd_info_t g_atcmd_basic_mode =
{
	.list.next = NULL,
	.list.prev = NULL,

	.group = ATCMD_GROUP_BASIC,

	.cmd = "MODE",
	.id = ATCMD_BASIC_MODE,

	.handler[ATCMD_HANDLER_RUN] = NULL,
	.handler[ATCMD_HANDLER_GET] = _atcmd_basic_mode_get,
	.handler[ATCMD_HANDLER_SET] = _atcmd_basic_mode_set,
};

/**********************************************************************************************/

static int _atcmd_basic_version_get (int argc, char *argv[])
{
	switch (argc)
	{
		case 0:
		{
			char str_atcmd[ATCMD_STR_SIZE(8)];
			char str_macsw[ATCMD_STR_SIZE(8)];
			char param_atcmd[ATCMD_STR_PARAM_SIZE(8)];
			char param_macsw[ATCMD_STR_PARAM_SIZE(8)];

			snprintf(str_atcmd, sizeof(str_atcmd), "%u.%u.%u",
						ATCMD_VER_MAJOR, ATCMD_VER_MINOR, ATCMD_VER_REVISION);

			snprintf(str_macsw, sizeof(str_macsw), "%u.%u.%u",
						MACSW_VER_MAJOR, MACSW_VER_MINOR, MACSW_VER_REVISION);

			if (!atcmd_str_to_param(str_atcmd, param_atcmd, sizeof(param_atcmd)) ||
				!atcmd_str_to_param(str_macsw, param_macsw, sizeof(param_macsw)))
				return ATCMD_ERROR_FAIL;

			ATCMD_LOG_INFO("VER", "%s,%s", "atcmd=%s macsw=%s", param_atcmd, param_macsw);

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

#ifdef CONFIG_HIF_UART_CH2_ONLY
			ATCMD_LOG_INFO("UART", "%d,%d,%d,%d,%d",
							"baudrate=%d data=%d stop=%d parity=%d hfc=%d",
							uart.baudrate,
							uart.data_bits + 5, uart.stop_bits + 1, uart.parity,
							uart.hfc);
#else
			ATCMD_LOG_INFO("UART", "%d,%d,%d,%d,%d%d",
							"channel=%d baudrate=%d data=%d stop=%d parity=%d hfc=%d",
							uart.channel, uart.baudrate,
							uart.data_bits + 5, uart.stop_bits + 1, uart.parity,
							uart.hfc);
#endif

			break;
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

/* #define CONFIG_ATCMD_UART_SET_BIT_PARAMS	*/

#ifdef CONFIG_ATCMD_UART_SET_BIT_PARAMS
static int _atcmd_basic_uart_set (int argc, char *argv[])
{
	char *param_baudrate = NULL;
	char *param_data_bits = NULL;
	char *param_stop_bits = NULL;
	char *param_parity = NULL;
	char *param_hfc = "0";

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
			ATCMD_LOG_HELP("AT+UART=<baudrate>,<data_bits>,<stop_bits>,<parity>[,<HFC>]");
			break;

		case 5:
			param_hfc = argv[4];

		case 4:
		{
			_hif_uart_t uart;

			param_baudrate = argv[0];
			param_data_bits = argv[1];
			param_stop_bits = argv[2];
			param_parity = argv[3];

			uart.channel = 2;
			uart.baudrate = atoi(param_baudrate);
			uart.data_bits = atoi(param_data_bits);
			uart.stop_bits = atoi(param_stop_bits);
			uart.parity = atoi(param_parity);
			uart.hfc = UART_HFC_DISABLE;

			if (param_hfc)
			{
				switch (atoi(param_hfc))
				{
					case 0: uart.hfc = UART_HFC_DISABLE; break;
					case 1: uart.hfc = UART_HFC_ENABLE;	break;

					default:
						return ATCMD_ERROR_INVAL;
				}
			}

			if (!_hif_uart_baudrate_valid(uart.baudrate, uart.hfc))
				return ATCMD_ERROR_INVAL;

			switch (atoi(param_data_bits))
			{
				case 5: uart.data_bits = UART_DB5; break;
				case 6: uart.data_bits = UART_DB6; break;
				case 7: uart.data_bits = UART_DB7; break;
				case 8: uart.data_bits = UART_DB8; break;

				default:
					return ATCMD_ERROR_INVAL;
			}

			switch (atoi(param_stop_bits))
			{
				case 1: uart.stop_bits = UART_SB1; break;
				case 2: uart.stop_bits = UART_SB2; break;

				default:
					return ATCMD_ERROR_INVAL;
			}

			switch (atoi(param_parity))
			{
				case 0: uart.parity = UART_PB_NONE;	break;
				case 1:	uart.parity = UART_PB_ODD; break;
				case 2: uart.parity = UART_PB_EVEN; break;

				default:
					return ATCMD_ERROR_INVAL;
			}

			if (_hif_uart_change(&uart) != 0)
				return ATCMD_ERROR_FAIL;

			break;
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}
#else
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
			ATCMD_LOG_HELP("AT+UART=<baudrate>,<HFC>");
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

			if (!_hif_uart_baudrate_valid(uart.baudrate))
				return ATCMD_ERROR_INVAL;

			if (_hif_uart_change(&uart) != 0)
				return ATCMD_ERROR_FAIL;

			break;
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}
#endif

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

				ATCMD_LOG_INFO("GPIOCONF", "%d,%d,%d", "pin=%d dir=%d pdpu=%d",
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
			ATCMD_LOG_HELP("AT+GPIOCONF=<index>,<direction>[,<pull-up]]");
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

			if (index != -1)
				start_index = end_index = index;
			else
			{
				start_index = GPIO_08;
				end_index = GPIO_17;
			}

			for (index = start_index ; index <= end_index ; index++)
				ATCMD_LOG_INFO("GPIOVAL", "%d,%d", "index=%d level=%d", index, nrc_gpio_inputb(index));

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
			ATCMD_LOG_HELP("AT+GPIOVAL=<index>,<level>");
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
	int value;

	nrc_nadc_init();
	_delay_ms(100);

	value = nrc_nadc_get_data(channel);

	nrc_nadc_fini();

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

			ATCMD_LOG_INFO("ADC", "%d,%d", "channel=%d value=%d", channel, value);

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
			ATCMD_LOG_HELP("AT+ADC=<ADC channel index>");
			break;

		case 1:
		{
			int channel = atoi(argv[0]);
			int value;

			if (!_atcmd_adc_channel_valid(channel))
				return ATCMD_ERROR_INVAL;

			value = _atcmd_adc_get_value(channel);

			ATCMD_LOG_INFO("ADC", "%d,%d", "channel=%d value=%d", channel, value);

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

static int _atcmd_basic_sleep_set (int argc, char *argv[])
{
	char *param_rtc = NULL;
	char *param_gpio = NULL;

#ifndef INCLUDE_UCODE
	_atcmd_info("deep_sleep: no ucode\n");
	ATCMD_LOG_INFO("%s", "%s", "no ucode");
	return ATCMD_ERROR_NOTSUPP;
#endif

	switch (argc)
	{
		case 0:
			/*
			 * 	rtc: 0=disable, 1=enable
			 * 	gpio: 8 ~ 17
			 */
			ATCMD_LOG_HELP("AT+SLEEP=<rtc>[,<gpio>]");
			break;

		case 2:
			param_gpio = argv[1];

		case 1:
		{
			uint8_t wakeup_source = 0;
			int wakeup_gpio = -1;

			param_rtc = argv[0];

			switch (atoi(param_rtc))
			{
				case 1:
					wakeup_source = WAKEUP_SOURCE_RTC;

				case 0:
					break;

				default:
					_atcmd_info("deep_sleep: invalid param, rtc=%s\n", param_rtc);
					return ATCMD_ERROR_INVAL;
			}

			if (param_gpio)
			{
				wakeup_gpio = atoi(param_gpio);

				if (!(wakeup_gpio >= GPIO_08 && wakeup_gpio <= GPIO_17))
				{
					_atcmd_info("deep_sleep: invalid param, gpio=%d\n", wakeup_gpio);
					return ATCMD_ERROR_INVAL;
				}

				wakeup_source |= WAKEUP_SOURCE_GPIO;
			}

			_atcmd_info("deep_sleep: rtc=%d gpio=%d\n",
						!!(wakeup_source & WAKEUP_SOURCE_RTC), wakeup_gpio);

			nrc_ps_set_wakeup_source(wakeup_source);

			if (wakeup_gpio != -1)
				nrc_ps_set_gpio_wakeup_pin(wakeup_gpio);

			if (!nrc_ps_set_sleep(POWER_SAVE_DEEP_SLEEP_MODE, 0))
			{
				_atcmd_info("deep_sleep: fail\n");
				return ATCMD_ERROR_FAIL;
			}

			break;
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static atcmd_info_t g_atcmd_basic_sleep =
{
	.list.next = NULL,
	.list.prev = NULL,

	.group = ATCMD_GROUP_BASIC,

	.cmd = "SLEEP",
	.id = ATCMD_BASIC_SLEEP,

	.handler[ATCMD_HANDLER_RUN] = NULL,
	.handler[ATCMD_HANDLER_GET] = NULL,
	.handler[ATCMD_HANDLER_SET] = _atcmd_basic_sleep_set,
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
	{ "WSCAN", 0 }, { "WCONN", 0 }, { "WDISCONN", 0 },

	{ NULL, 0 }
};

static atcmd_timeout_t g_atcmd_timeout_socket[] =
{
	{ "SOPEN", 30 }, { "SSEND", 1 },

	{ NULL, 0 }
};

static atcmd_timeout_t *g_atcmd_timeout[2] =
{
	g_atcmd_timeout_wifi,
	g_atcmd_timeout_socket,
};

uint32_t _atcmd_timeout_value (const char *cmd)
{
	const int default_time = 30 * 1000;
	atcmd_timeout_t *timeout;
	int i;

	if (cmd[0] == 'W')
		timeout = g_atcmd_timeout_wifi;
	else if (cmd[0] == 'S')
		timeout = g_atcmd_timeout_socket;
	else
		return default_time;

	for (i = 0 ; timeout[i].cmd ; i++)
	{
		if (strcmp(cmd, timeout[i].cmd) == 0)
			break;
	}

	if (!timeout[i].cmd)
		return default_time;

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
						ATCMD_LOG_INFO("WTIMEOUT", "%s,%d", "cmd=%s time=%d", param_cmd, timeout[j].sec);
					else if (timeout[j].cmd[0] == 'S')
						ATCMD_LOG_INFO("STIMEOUT", "%s,%d", "cmd=%s time=%d", param_cmd, timeout[j].sec);
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
			ATCMD_LOG_HELP("AT+TIMEOUT=\"<command>\",<time>");
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
			else if (str_cmd[0] == 'S')
				timeout = g_atcmd_timeout_socket;
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

#ifdef CONFIG_ATCMD_DEBUG
	&g_atcmd_basic_log,
#endif

/*	&g_atcmd_basic_mode, */

	&g_atcmd_basic_version,
	&g_atcmd_basic_uart,
	&g_atcmd_basic_gpio_config,
	&g_atcmd_basic_gpio_value,
	&g_atcmd_basic_adc,
	&g_atcmd_basic_sleep,

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

	return 0;
}

void atcmd_basic_disable (void)
{
	int err = 0;
	int i;

	for (i = 0 ; g_atcmd_basic[i] ; i++)
		atcmd_info_unregister(ATCMD_GROUP_BASIC, g_atcmd_basic[i]->id);

	atcmd_group_unregister(ATCMD_GROUP_BASIC);
}
