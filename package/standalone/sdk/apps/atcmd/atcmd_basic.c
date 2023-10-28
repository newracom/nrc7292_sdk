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

/**********************************************************************************************/

static bool g_atcmd_version_verbose = false;

static int _atcmd_basic_version_get (int argc, char *argv[])
{
	switch (argc)
	{
		case 0:
		{
			char str_sdk_ver[ATCMD_STR_SIZE(8)];
			char str_atcmd_ver[ATCMD_STR_SIZE(8)];

			snprintf(str_sdk_ver, sizeof(str_sdk_ver), "%u.%u.%u",
						SDK_VER_MAJOR, SDK_VER_MINOR, SDK_VER_REVISION);

			snprintf(str_atcmd_ver, sizeof(str_atcmd_ver), "%u.%u.%u",
						ATCMD_VER_MAJOR, ATCMD_VER_MINOR, ATCMD_VER_REVISION);

			if (!g_atcmd_version_verbose)
				ATCMD_MSG_INFO("VER", "\"%s\",\"%s\"", str_sdk_ver, str_atcmd_ver);
			else
			{
#if defined(SDK_VER_DESCRIPTION)
				ATCMD_MSG_INFO("VER", "\"%s%s\",\"%s\",\"%s, %s\"",
						str_sdk_ver, SDK_VER_DESCRIPTION, str_atcmd_ver, __TIME__, __DATE__);
#else
				ATCMD_MSG_INFO("VER", "\"%s\",\"%s\",\"%s, %s\"",
						str_sdk_ver, str_atcmd_ver, __TIME__, __DATE__);
#endif
			}

			break;
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static int _atcmd_basic_version_set (int argc, char *argv[])
{
	switch (argc)
	{
		case 0:
			ATCMD_MSG_HELP("AT+VER=<verbose>");
			break;

		case 1:
		{
			int verbose = atoi(argv[0]);

			if (verbose == 0 || verbose == 1)
			{
				g_atcmd_version_verbose = !!verbose;
				break;
			}
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
	.handler[ATCMD_HANDLER_SET] = _atcmd_basic_version_set,
};

/**********************************************************************************************/

static int _atcmd_basic_heap_get (int argc, char *argv[])
{
	switch (argc)
	{
		case 0:
		{
			size_t cur_size = xPortGetFreeHeapSize();
			size_t min_size = xPortGetMinimumEverFreeHeapSize();

			_atcmd_info("heap_size: %d, %d", cur_size, min_size);

			ATCMD_MSG_INFO("HEAP", "%d,%d", cur_size, min_size);
			break;
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}
static atcmd_info_t g_atcmd_basic_heap =
{
	.list.next = NULL,
	.list.prev = NULL,

	.group = ATCMD_GROUP_BASIC,

	.cmd = "HEAP",
	.id = ATCMD_BASIC_HEAP,

	.handler[ATCMD_HANDLER_RUN] = NULL,
	.handler[ATCMD_HANDLER_GET] = _atcmd_basic_heap_get,
	.handler[ATCMD_HANDLER_SET] = NULL,
};

/**********************************************************************************************/

#if defined(CONFIG_ATCMD_UART) || defined(CONFIG_ATCMD_UART_HFC)

bool g_atcmd_uart_passthrough_support = false;

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

		case 1:
		{
			if (_hif_get_type() != _HIF_TYPE_UART)
				return ATCMD_ERROR_NOTSUPP;

			switch (atoi(argv[0]))
			{
				case 0: g_atcmd_uart_passthrough_support = false; break;
				case 1: g_atcmd_uart_passthrough_support = true; break;
				default: return ATCMD_ERROR_INVAL;
			}

			_atcmd_info("UART-PT %s", g_atcmd_uart_passthrough_support ? "ON" : "OFF");
			break;
		}

		case 2:
		{
			_hif_uart_t uart;
			int ret;

			param_baudrate = argv[0];
			param_hfc = argv[1];

			_hif_uart_get_info(&uart);

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

#endif /* #if defined(CONFIG_ATCMD_UART) || defined(CONFIG_ATCMD_UART_HFC) */

/**********************************************************************************************/

static bool _atcmd_gpio_pin_valid (int pin)
{
	/*
	 * index 0 : ATCMD_HSPI
	 * index 1 : ATCMD_UART & ATCMD_UART_HFC
	 */
	const uint32_t valid_gpios[2] =
	{
#if defined(NRC7292)
 		[0] = (1 << GPIO_00) | (1 << GPIO_01) | (1 << GPIO_02) | (1 << GPIO_03) | \
			  (1 << GPIO_08) | (1 << GPIO_09) | (1 << GPIO_10) | (1 << GPIO_11) | \
			  (1 << GPIO_12) | (1 << GPIO_13) | (1 << GPIO_14) | (1 << GPIO_15) | \
			  (1 << GPIO_16) | (1 << GPIO_17),

		[1] = (1 << GPIO_08) | (1 << GPIO_09) | (1 << GPIO_10) | (1 << GPIO_11) | \
			  (1 << GPIO_12) | (1 << GPIO_13) | (1 << GPIO_14) | (1 << GPIO_15) | \
			  (1 << GPIO_16) | (1 << GPIO_17)
#elif defined(NRC7394)
		[0] = (1 << GPIO_10) | (1 << GPIO_11) | (1 << GPIO_12) | (1 << GPIO_13) | \
			  (1 << GPIO_14) | (1 << GPIO_20) | (1 << GPIO_25),

		[1] = (1 << GPIO_06) | (1 << GPIO_07) | (1 << GPIO_10) | (1 << GPIO_11) | \
			  (1 << GPIO_25) | (1 << GPIO_28) | (1 << GPIO_29) | (1 << GPIO_30)
#endif
	};

	enum _HIF_TYPE hif_type = _hif_get_type();
	const uint32_t *gpios;

	switch (hif_type)
	{
		case _HIF_TYPE_HSPI:
			gpios = &valid_gpios[0];
			break;

		case _HIF_TYPE_UART:
		case _HIF_TYPE_UART_HFC:
			gpios = &valid_gpios[1];
			break;

		default:
			_atcmd_error("invalid hif type (%d)", hif_type);
			return false;
	}

	return !!(*gpios & (1 << pin));
}

#if defined(NRC7292)
static void _atcmd_gpio_get_config (NRC_GPIO_CONFIG *config)
{
	gpio_io_t reg;

	/* ALT function */
	config->gpio_alt = GPIO_FUNC;

	nrc_gpio_get_alt(&reg);

	if (reg.word & (1 << config->gpio_pin))
	{
		uio_sel_t uio;
		int i;

		_atcmd_info("GPIO_GET: ALT bit%d is 1", config->gpio_pin);

		config->gpio_alt = GPIO_NORMAL_OP;

		for (i = 0 ; i <= 15 ; i++)
		{
			nrc_gpio_get_uio_sel(i, &uio);

			if (uio.word == 0)
				continue;

			_atcmd_info(" - UIO_SEL%02d : %03u, %03u, %03u, %03u",
				i, uio.bit.sel7_0, uio.bit.sel15_8, uio.bit.sel23_16, uio.bit.sel31_24);
		}
	}

	/* Direction */
	nrc_gpio_get_dir(&reg);

	if (reg.word & (1 << config->gpio_pin))
		config->gpio_dir = GPIO_OUTPUT;
	else
		config->gpio_dir = GPIO_INPUT;

	/* Pullup & Pulldown */
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
#elif defined(NRC7394)
static void _atcmd_gpio_get_config (NRC_GPIO_CONFIG *config)
{
	gpio_io_t reg;

	/* ALT function */
	config->gpio_alt = GPIO_FUNC;

	nrc_gpio_get_alt(&reg);

	if (reg.word & (1 << config->gpio_pin))
	{
		uio_sel_t uio;
		int i;

		_atcmd_info("GPIO_GET: ALT0 bit%d is 1", config->gpio_pin);
		config->gpio_alt = GPIO_NORMAL_OP;

		for (i = 0 ; i <= 15 ; i++)
		{
			nrc_gpio_get_uio_sel(i, &uio);

			if (uio.word == 0)
				continue;

			_atcmd_info(" - UIO_SEL%02d : %03u, %03u, %03u, %03u",
				i, uio.bit.sel7_0, uio.bit.sel15_8, uio.bit.sel23_16, uio.bit.sel31_24);
		}
	}

	nrc_gpio_get_alt1(&reg);

	if (reg.word & (1 << config->gpio_pin))
	{
		_atcmd_info("GPIO_GET: ALT1 bit%d is 1", config->gpio_pin);
		config->gpio_alt = GPIO_NORMAL_OP;
	}

	nrc_gpio_get_alt2(&reg);

	if (reg.word & (1 << config->gpio_pin))
	{
		_atcmd_info("GPIO_GET: ALT2 bit%d is 1", config->gpio_pin);
		config->gpio_alt = GPIO_NORMAL_OP;
	}

	nrc_gpio_get_alt3(&reg);

	if (reg.word & (1 << config->gpio_pin))
	{
		_atcmd_info("GPIO_GET: ALT3 bit%d is 1", config->gpio_pin);
		config->gpio_alt = GPIO_NORMAL_OP;
	}

	nrc_gpio_get_alt4(&reg);

	if (reg.word & (1 << config->gpio_pin))
	{
		_atcmd_info("GPIO_GET: ALT4 bit%d is 1", config->gpio_pin);
		config->gpio_alt = GPIO_NORMAL_OP;
	}

	/* Direction */
	nrc_gpio_get_dir(&reg);

	if (reg.word & (1 << config->gpio_pin))
		config->gpio_dir = GPIO_OUTPUT;
	else
		config->gpio_dir = GPIO_INPUT;

	/* Pullup & Pulldown */
	nrc_gpio_get_pullup(&reg);

	if (reg.word & (1 << config->gpio_pin))
		config->gpio_mode = GPIO_PULL_UP;
	else
		config->gpio_mode = GPIO_PULL_DOWN;
}
#endif

static void _atcmd_gpio_set_config (NRC_GPIO_CONFIG *config)
{
	if (nrc_gpio_config(config) != NRC_SUCCESS)
		_atcmd_error("nrc_gpio_config()");
}

static void _atcmd_gpio_init (NRC_GPIO_DIR dir, NRC_GPIO_MODE mode)
{
	NRC_GPIO_CONFIG config;
	int i;

	config.gpio_alt = GPIO_FUNC,
	config.gpio_dir = dir;
	config.gpio_mode = mode;

	for (i = 0 ; i < 32 ; i++)
	{
		if (!_atcmd_gpio_pin_valid(i))
			continue;

		config.gpio_pin = i;

		_atcmd_info("gpio_init: %02d %s %s",
					config.gpio_pin,
					(config.gpio_dir == GPIO_INPUT) ? "input" : "output",
					(config.gpio_mode == GPIO_PULL_UP) ? "pullup" : "pulldown");

		_atcmd_gpio_set_config(&config);
	}
}

static int _atcmd_basic_gpio_config_get (int argc, char *argv[])
{
	bool verbose = false;
	int pin = -1;

	switch (argc)
	{
		case 1:
			pin = atoi(argv[0]);

			if (pin == -1)
				verbose = true;
			else if (!_atcmd_gpio_pin_valid(pin))
				return ATCMD_ERROR_INVAL;

		case 0:
		{
			NRC_GPIO_CONFIG config;
			int start = 0;
			int end = 31;

			if (pin >= 0)
				start = end = pin;

			for (pin = start ; pin <= end ; pin++)
			{
				if (!_atcmd_gpio_pin_valid(pin))
					continue;

				config.gpio_pin = pin;

				_atcmd_gpio_get_config(&config);

				if (verbose)
				{
					ATCMD_MSG_INFO("GPIOCONF", "%d,%d,%d,%d",
									config.gpio_pin, config.gpio_dir,
									(config.gpio_mode == GPIO_PULL_UP) ? 1 : 0,
									config.gpio_alt);
				}
				else
				{
					ATCMD_MSG_INFO("GPIOCONF", "%d,%d,%d",
									config.gpio_pin, config.gpio_dir,
									(config.gpio_mode == GPIO_PULL_UP) ? 1 : 0);
				}
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
	char *param_dir = NULL;
	char *param_pullup = NULL;

	switch (argc)
	{
		case 0:
			ATCMD_MSG_HELP("AT+GPIOCONF=<number>,<direction>[,<pull-up>]");
			break;

		case 3:
			param_pullup = argv[2];

		case 2:
		{
			NRC_GPIO_CONFIG config =
			{
				.gpio_alt = GPIO_FUNC,
			};
			int pin = atoi(argv[0]);

			param_dir = argv[1];

			if (pin != -1 && !_atcmd_gpio_pin_valid(pin))
				return ATCMD_ERROR_INVAL;

			config.gpio_pin = pin;

			switch (atoi(param_dir))
			{
				case 0: config.gpio_dir = GPIO_INPUT; break;
				case 1: config.gpio_dir = GPIO_OUTPUT; break;
				default: return ATCMD_ERROR_INVAL;
			}

			if (!param_pullup)
				config.gpio_mode = GPIO_FLOATING;
			else
			{
				switch (atoi(param_pullup))
				{
					case 0: config.gpio_mode = GPIO_PULL_DOWN; break;
					case 1: config.gpio_mode = GPIO_PULL_UP; break;
					default: return ATCMD_ERROR_INVAL;
				}
			}

			if (pin == -1)
				_atcmd_gpio_init(config.gpio_dir, config.gpio_mode);
			else
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
	int pin = -1;

	switch (argc)
	{
		case 1:
			pin = atoi(argv[0]);

			if (!_atcmd_gpio_pin_valid(pin))
				return ATCMD_ERROR_INVAL;

		case 0:
		{
			int start = 0;
			int end = 31;
			int value;

			if (pin != -1)
				start = end = pin;

			for (pin = start ; pin <= end ; pin++)
			{
				if (!_atcmd_gpio_pin_valid(pin))
					continue;

				if (nrc_gpio_inputb(pin, &value) != NRC_SUCCESS)
					return ATCMD_ERROR_FAIL;

				ATCMD_MSG_INFO("GPIOVAL", "%d,%d", pin, value);
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
			ATCMD_MSG_HELP("AT+GPIOVAL=<number>,<level>");
			break;

		case 2:
		{
			int pin = atoi(argv[0]);
			int level = atoi(argv[1]);

			if (_atcmd_gpio_pin_valid(pin))
			{
				if (level == 0 || level == 1)
				{
					nrc_gpio_outputb(pin, level);
					break;
				}
			}
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

static bool g_atcmd_adc_enable = false;

static void _atcmd_adc_init (void)
{
	 nrc_adc_init(false);

	 g_atcmd_adc_enable = false;
}

static void _atcmd_adc_deinit (void)
{
	 nrc_adc_deinit();

	 g_atcmd_adc_enable = false;
}

static void _atcmd_adc_enable (void)
{
	if (!g_atcmd_adc_enable)
	{
		_atcmd_info("ADC: enable");

		g_atcmd_adc_enable = true;
		nrc_adc_enable();
	}
}

static void _atcmd_adc_disable (void)
{
	if (g_atcmd_adc_enable)
	{
		_atcmd_info("ADC: disable");

		g_atcmd_adc_enable = false;
		nrc_adc_disable();
	}
}

static bool _atcmd_adc_channel_valid (int channel)
{
	switch (channel)
	{
#if defined(NRC7292)
		case ADC1:
		case ADC2:
		case ADC3:
#else
		case ADC0:
		case ADC1:
#endif
			return true;

		default:
			return false;
	}
}

static uint16_t _atcmd_adc_get_value (int channel)
{
	return nrc_adc_get_data(channel);
}

static int _atcmd_basic_adc_get (int argc, char *argv[])
{
	int channel = -1;

	if (!g_atcmd_adc_enable)
		return ATCMD_ERROR_NOTSUPP;

	switch (argc)
	{
		case 1:
			channel = atoi(argv[0]);
#if !defined(NRC7292)
			channel += 2;
#endif
			if (!_atcmd_adc_channel_valid(channel))
				return ATCMD_ERROR_INVAL;

		case 0:
		{
			int start = 0;
			int end = 3;

			if (channel != -1)
				start = end = channel;

			for (channel = start ; channel <= end ; channel++)
			{
				if (!_atcmd_adc_channel_valid(channel))
					continue;

#if defined(NRC7292)
				ATCMD_MSG_INFO("ADC", "%d,%u", channel, _atcmd_adc_get_value(channel));
#else
				ATCMD_MSG_INFO("ADC", "%d,%u", channel - 2, _atcmd_adc_get_value(channel));
#endif
			}

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
			ATCMD_MSG_HELP("AT+ADC={0|1}");
			break;

		case 1:
		{
			int param = atoi(argv[0]);

			if (param == 1)
			{
				_atcmd_adc_enable();
				break;
			}
			else if (param == 0)
			{
				_atcmd_adc_disable();
				break;
			}
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

static struct
{
	uint32_t size;
	uint32_t crc32;
	uint32_t download;
} g_firmware_update =
{
	.size = 0,
	.crc32 = 0,
	.download = 0,
};

void atcmd_firmware_write (char *buf, int len)
{
	util_fota_write(g_firmware_update.download, (uint8_t *)buf, len);

	g_firmware_update.download += len;
}

void atcmd_firmware_download_event_idle (uint32_t len, uint32_t cnt)
{
	ATCMD_MSG_EVENT("BEVENT", "\"FWBINDL_IDLE\",%u,%u,%u", g_firmware_update.download, len, cnt);
}

void atcmd_firmware_download_event_drop (uint32_t len)
{
	ATCMD_MSG_EVENT("BEVENT", "\"FWBINDL_DROP\",%u,%u", g_firmware_update.download, len);
}

void atcmd_firmware_download_event_done (uint32_t len)
{
	ATCMD_MSG_EVENT("BEVENT", "\"FWBINDL_DONE\",%u,%u", g_firmware_update.download - len, len);
}

/**********************************************************************************************/

static int _atcmd_basic_firmware_update_run (int argc, char *argv[])
{
	uint32_t max_size = util_fota_get_max_fw_size();
	uint32_t size = g_firmware_update.size;
	uint32_t crc32 = g_firmware_update.crc32;
	uint32_t _crc32;

	if (size == 0 || size > max_size || crc32 == 0)
		return ATCMD_ERROR_INVAL;

	_atcmd_info("fw_update_run: size=%u crc=0x%X", size, crc32);

	_crc32 = util_fota_cal_crc((uint8_t *)util_fota_fw_addr(), size);

	if (_crc32 != crc32)
	{
		_atcmd_info("fw_update_run: crc mismatch, crc=0x%X", _crc32);
		return ATCMD_ERROR_FAIL;
	}

	util_fota_set_info(size, crc32);
	util_fota_set_ready(true);

	return ATCMD_SUCCESS;
}

static int _atcmd_basic_firmware_update_get (int argc, char *argv[])
{
	switch (argc)
	{
		case 0:
		{
			uint32_t size = g_firmware_update.size;
			uint32_t crc32 = g_firmware_update.crc32;

			_atcmd_info("fw_update_get: size=%u crc=0x%X", size, crc32);

			ATCMD_MSG_INFO("FWUPDATE", "%u,0x%X", size, crc32);
			break;
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static int _atcmd_basic_firmware_update_set (int argc, char *argv[])
{
	uint32_t size = 0;
	uint32_t crc32 = 0;

	switch (argc)
	{
		case 0:
			ATCMD_MSG_HELP("AT+FWUPDATE=<binary_size>[,<crc32>]");
			break;

		case 2:
			if (atcmd_param_to_hex(argv[1], &crc32) != 0 || crc32 == 0)
				return ATCMD_ERROR_INVAL;

		case 1:
			if (atcmd_param_to_uint32(argv[0], &size) != 0)
				return ATCMD_ERROR_INVAL;

			if (size == 0 && crc32 == 0)
				break;
			else if (size > 0 && crc32 > 0)
			{
				if (size <= util_fota_get_max_fw_size())
				{
					if (g_firmware_update.size == 0 && g_firmware_update.crc32 == 0)
						break;

					size = g_firmware_update.size;
					crc32 = g_firmware_update.crc32;

					_atcmd_info("fw_update_busy: size=%u crc=0x%X", size, crc32);

					return ATCMD_ERROR_BUSY;
				}
			}

		default:
			return ATCMD_ERROR_INVAL;
	}

	_atcmd_info("fw_update_set: size=%u crc=0x%X", size, crc32);

	g_firmware_update.size = size;
	g_firmware_update.crc32 = crc32;
	g_firmware_update.download = 0;

	if (size > 0)
		util_fota_erase(0, size);

	util_fota_erase_info();

	return ATCMD_SUCCESS;
}

static atcmd_info_t g_atcmd_basic_firmware_update =
{
	.list.next = NULL,
	.list.prev = NULL,

	.group = ATCMD_GROUP_BASIC,

	.cmd = "FWUPDATE",
	.id = ATCMD_BASIC_FW_UPDATE,

	.handler[ATCMD_HANDLER_RUN] = _atcmd_basic_firmware_update_run,
	.handler[ATCMD_HANDLER_GET] = _atcmd_basic_firmware_update_get,
	.handler[ATCMD_HANDLER_SET] = _atcmd_basic_firmware_update_set,
};

/**********************************************************************************************/

uint32_t _atcmd_timeout_value (const char *cmd);

static int _atcmd_basic_firmware_download_get (int argc, char *argv[])
{
	switch (argc)
	{
		case 0:
		{
			uint32_t size = g_firmware_update.size;
			uint32_t download = g_firmware_update.download;

			_atcmd_info("fw_download_get: size=%u download=%u", size, download);

			ATCMD_MSG_INFO("FWBINDL", "%u,%u", size, download);
			break;
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static int _atcmd_basic_firmware_download_set (int argc, char *argv[])
{
	switch (argc)
	{
		case 0:
			ATCMD_MSG_HELP("AT+FWBINDL=<offset>,<length>");
			break;

		case 2:
		{
			uint32_t offset;
			uint32_t length;

			if (atcmd_param_to_uint32(argv[0], &offset) != 0)
				return ATCMD_ERROR_INVAL;

			if (atcmd_param_to_uint32(argv[1], &length) != 0)
				return ATCMD_ERROR_INVAL;

			if (length == 0 || length > ATCMD_DATA_LEN_MAX)
				return ATCMD_ERROR_INVAL;

			if (offset == g_firmware_update.download)
			{
				uint32_t timeout_msec = _atcmd_timeout_value("FWBINDL");

				if (timeout_msec == 0)
					timeout_msec = 1000;

				_atcmd_info("fw_download_set: offset=%u length=%u timeout=%u", offset, length, timeout_msec);

				if (atcmd_firmware_download_enable(length, timeout_msec) != 0)
					return ATCMD_ERROR_FAIL;

				break;
			}

			_atcmd_info("fw_download_set: offset mismatch (%u -> %u)", g_firmware_update.download, offset);
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static atcmd_info_t g_atcmd_basic_firmware_donwload =
{
	.list.next = NULL,
	.list.prev = NULL,

	.group = ATCMD_GROUP_BASIC,

	.cmd = "FWBINDL",
	.id = ATCMD_BASIC_FW_DOWNLOAD,

	.handler[ATCMD_HANDLER_RUN] = NULL,
	.handler[ATCMD_HANDLER_GET] = _atcmd_basic_firmware_download_get,
	.handler[ATCMD_HANDLER_SET] = _atcmd_basic_firmware_download_set,
};

/**********************************************************************************************/

#define ATCMD_TIMEOUT_CMD_LEN_MAX		20

typedef struct
{
	char *cmd;
	uint32_t sec;
} atcmd_timeout_t;

static atcmd_timeout_t g_atcmd_timeout_basic[] =
{
	{ "FWBINDL", 1 },

	{ NULL, 0 }
};

static atcmd_timeout_t g_atcmd_timeout_wifi[] =
{
	{ "WSCAN", 0 },
	{ "WCONN", 60 },
	{ "WDISCONN", 60 },
	{ "WDHCP", 60 },
	{ "WSOFTAP", 60 },

	{ NULL, 0 }
};

static atcmd_timeout_t g_atcmd_timeout_socket[] =
{
	{ "SOPEN", 30 },
#ifdef CONFIG_ATCMD_IPV6
	{ "SOPEN6", 30 },
#endif
	{ "SSEND", 1 },

	{ NULL, 0 }
};

static atcmd_timeout_t *g_atcmd_timeout[2] =
{
	g_atcmd_timeout_wifi,
	g_atcmd_timeout_socket,
};

uint32_t _atcmd_timeout_value (const char *cmd)
{
	atcmd_timeout_t *timeout;
	int i;

	if (cmd[0] == 'W')
		timeout = g_atcmd_timeout_wifi;
	else if (cmd[0] == 'S')
		timeout = g_atcmd_timeout_socket;
	else
		timeout = g_atcmd_timeout_basic;

	for (i = 0 ; timeout[i].cmd ; i++)
	{
		if (strcmp(cmd, timeout[i].cmd) == 0)
			break;
	}

	if (!timeout[i].cmd)
	{
		_atcmd_error("no cmd");
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

					if (timeout[j].sec == 0)
						continue;

					if (timeout[j].cmd[0] == 'W')
						ATCMD_MSG_INFO("WTIMEOUT", "%s,%d", param_cmd, timeout[j].sec);
					else if (timeout[j].cmd[0] == 'S')
						ATCMD_MSG_INFO("STIMEOUT", "%s,%d", param_cmd, timeout[j].sec);
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
	&g_atcmd_basic_version,
	&g_atcmd_basic_heap,
#if defined(CONFIG_ATCMD_UART) || defined(CONFIG_ATCMD_UART_HFC)
	&g_atcmd_basic_uart,
#endif
	&g_atcmd_basic_gpio_config,
	&g_atcmd_basic_gpio_value,
	&g_atcmd_basic_adc,

	&g_atcmd_basic_firmware_update,
	&g_atcmd_basic_firmware_donwload,

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

	atcmd_info_print(&g_atcmd_group_basic);

	_atcmd_adc_init();

	return 0;
}

void atcmd_basic_disable (void)
{
	int i;

	_atcmd_adc_deinit();

	for (i = 0 ; g_atcmd_basic[i] ; i++)
		atcmd_info_unregister(ATCMD_GROUP_BASIC, g_atcmd_basic[i]->id);

	atcmd_group_unregister(ATCMD_GROUP_BASIC);
}

bool atcmd_gpio_pin_valid (int pin)
{
	return _atcmd_gpio_pin_valid(pin);
}
