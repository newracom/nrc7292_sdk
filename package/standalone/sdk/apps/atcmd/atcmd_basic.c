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

/**********************************************************************************************/

#if defined(CONFIG_ATCMD_FWUPDATE)

static struct
{
	uint32_t size;
	uint32_t crc32;
	uint32_t download;
	int verify;
} g_firmware_update =
{
	.size = 0,
	.crc32 = 0,
	.download = 0,
	.verify = 0,
};

static void _atcmd_firmware_error_print (uint8_t *read_buf, uint8_t *buf, int len)
{
	int err_offset = -1;
	int err_len = 0;
	int i;

	for (i = 0 ; i < len ; i++)
	{
		if (read_buf[i] == buf[i])
		{
			if (err_offset >= 0)
			{
				_atcmd_info("  - offset=%d len=%d", err_offset, err_len);

				err_offset = -1;
				err_len = 0;
			}
			continue;
		}

		if (err_offset >= 0)
			err_len++;
		else
		{
			err_offset = i;
			err_len = 1;
		}
	}
}

static int _atcmd_firmware_verify (uint32_t offset, uint8_t *buf, int len)
{
	static uint8_t read_buf[ATCMD_DATA_LEN_MAX];
	int ret;

	ret = util_fota_read(offset, read_buf, len);
   	if (ret != 0)
		_atcmd_info("fw_verify: util_fota_read() failed, offset=%u len=%d ret=%d", offset, len, ret);
	else
	{
		ret = memcmp(read_buf, buf, len);
		if (ret != 0)
		{
			_atcmd_info("fw_verify: flash error, offset=%u len=%d", offset, len);

			if (g_firmware_update.verify == 2)
				_atcmd_firmware_error_print(read_buf, buf, len);
		}
	}

	return ret;
}

static int _atcmd_firmware_download (char *buf, int len)
{
	uint32_t offset = g_firmware_update.download;
	int ret;
	int i;

	for (i = 0 ; i < 2 ; i++)
	{
		ret = util_fota_write(offset, (uint8_t *)buf, len);
		if (ret != 0)
			_atcmd_info("fw_download: util_fota_write() failed, offset=%u len=%d ret=%d", offset, len, ret);
		else if (g_firmware_update.verify > 0)
			ret = _atcmd_firmware_verify(offset, (uint8_t *)buf, len);

		if (ret == 0)
			break;

		if (i == 0)
		{
			_atcmd_debug("fw_download: erase and rewrite"); 

			ret = util_fota_erase(offset, len);
		   	if (ret != 0)
			{
				_atcmd_info("fw_download: util_fota_erase() failed, offset=%u len=%d ret=%d", offset, len, ret);
				break;
			}
		}
	}

	if (ret != 0)
		len = 0;	
		
	g_firmware_update.download += len;		

	return len;
}

#endif /* #if defined(CONFIG_ATCMD_FWUPDATE) */

/**********************************************************************************************/

#if defined(CONFIG_ATCMD_SFUSER)

enum
{
	SF_USER_READ = 0,
	SF_USER_WRITE,
	SF_USER_ERASE
};

static bool _atcmd_sf_user_support (void)
{
	uint32_t flash_size = system_api_get_flash_size() / (1024 * 1024);
	uint32_t addr = nrc_get_user_data_area_address();
	uint32_t size = nrc_get_user_data_area_size();

	switch (flash_size)
	{
		case 2:
			if (addr == 0x1E6000 && size == (100 * 1024)) /* factory default */
				return true;

			if (addr == 0x1F4000 && size == (8 * 1024)) /* extension profile 1/3 */
				return true;

			break;

		case 4:
			if (addr == 0x3DA000 && size == (100 * 1024)) /* factory default */
				return true;
	}

	return false;
}

static bool _atcmd_sf_user_valid_params (int mode, uint32_t offset, uint32_t length)
{
	int sf_user_area_size = nrc_get_user_data_area_size();
	int length_max;

	switch (mode)
	{
		case SF_USER_READ:
		case SF_USER_WRITE:
			length_max = ATCMD_DATA_LEN_MAX;
			break;

		case SF_USER_ERASE:
			length_max = sf_user_area_size;
			break;

		default:
			_atcmd_info("sf_user: invalid param, mode=%u", mode);
			return false;
	}

	if (offset >= sf_user_area_size)
	{
		_atcmd_info("sf_user: invalid param, offset=%u size=%d", 
						offset, sf_user_area_size);
		return false;
	}

	if (length == 0 || length > length_max)
	{
		_atcmd_info("sf_user: invalid param, length=%u/%d size=%d", 
						length, length_max, sf_user_area_size);
		return false;
	}

	if ((offset + length) > sf_user_area_size)
	{
		_atcmd_info("sf_user: invalid param, offset=%u length=%u size=%d", 
						offset, length, sf_user_area_size);
		return false;
	}

	return true;
}

static int _atcmd_sf_user_write (uint32_t offset, uint32_t length, char *data)
{
	_atcmd_debug("sf_user_write: offset=%d length=%d", offset, length);

	if (nrc_write_user_data(offset, (uint8_t *)data, length) != NRC_SUCCESS)
		return 0;

	return length;
}

#endif /* #if defined(CONFIG_ATCMD_SFUSER) */

/**********************************************************************************************/

static int _atcmd_basic_event_handler (int type, ...)
{
	va_list ap;
	int ret = 0;

	va_start(ap, type);

	switch (type)
	{
		case ATCMD_BASIC_EVENT_FWBINDL_IDLE:
		{
			const char *name = "FWBINDL_IDLE";
			uint32_t len = va_arg(ap, uint32_t);
			uint32_t cnt = va_arg(ap, uint32_t);

			_atcmd_info("BEVENT: %s, len=%u cnt=%u", name, len, cnt);
			ATCMD_MSG_BEVENT("\"%s\",%u,%u", name, len, cnt);
			break;
		}

		case ATCMD_BASIC_EVENT_FWBINDL_DROP:
		{
			const char *name = "FWBINDL_DROP";
			uint32_t len = va_arg(ap, uint32_t);

			_atcmd_info("BEVENT: %s, len=%u", name, len);
			ATCMD_MSG_BEVENT("\"%s\",%u", name, len);
			break;
		}

		case ATCMD_BASIC_EVENT_FWBINDL_FAIL:
		{
			const char *name = "FWBINDL_FAIL";
			uint32_t len = va_arg(ap, uint32_t);

			_atcmd_info("BEVENT: %s, len=%u", name, len);
			ATCMD_MSG_BEVENT("\"%s\",%u", name, len);
			break;
		}

		case ATCMD_BASIC_EVENT_FWBINDL_DONE:
		{
			const char *name = "FWBINDL_DONE";
			uint32_t len = va_arg(ap, uint32_t);

			_atcmd_info("BEVENT: %s, len=%u", name, len);
			ATCMD_MSG_BEVENT("\"%s\",%u", name, len);
			break;
		}

#if defined(CONFIG_ATCMD_SFUSER)
		case ATCMD_BASIC_EVENT_SFUSER_IDLE:
		{
			const char *name = "SFUSER_IDLE";
			uint32_t offset = va_arg(ap, uint32_t);
			uint32_t length = va_arg(ap, uint32_t);
			uint32_t count = va_arg(ap, uint32_t);

			_atcmd_info("BEVENT: %s, offset=%u length=%u count=%u", name, offset, length, count);
			ATCMD_MSG_BEVENT("\"%s\",%u,%u,%u", name, offset, length, count);
			break;
		}

		case ATCMD_BASIC_EVENT_SFUSER_DROP:
		{
			const char *name = "SFUSER_DROP";
			uint32_t offset = va_arg(ap, uint32_t);
			uint32_t length = va_arg(ap, uint32_t);

			_atcmd_info("BEVENT: %s, offset=%u length=%u", name, offset, length);
			ATCMD_MSG_BEVENT("\"%s\",%u,%u", name, offset, length);
			break;
		}

		case ATCMD_BASIC_EVENT_SFUSER_FAIL:
		{
			const char *name = "SFUSER_FAIL";
			uint32_t offset = va_arg(ap, uint32_t);
			uint32_t length = va_arg(ap, uint32_t);

			_atcmd_info("BEVENT: %s, offset=%u length=%u", name, offset, length);
			ATCMD_MSG_BEVENT("\"%s\",%u,%u", name, offset, length);
			break;
		}

		case ATCMD_BASIC_EVENT_SFUSER_DONE:
		{
			const char *name = "SFUSER_DONE";
			uint32_t offset = va_arg(ap, uint32_t);
			uint32_t length = va_arg(ap, uint32_t);

			_atcmd_info("BEVENT: %s, offset=%u length=%u", name, offset, length);
			ATCMD_MSG_BEVENT("\"%s\",%u,%u", name, offset, length);
			break;
		}
#endif

		default:
			_atcmd_info("BEVENT: invalid type (%d)", type);
			ret = -1;
	}

	va_end(ap);

	return ret;
}

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

static char g_atcmd_boot_reason[32];

static int _atcmd_basic_boot_get (int argc, char *argv[])
{
	switch (argc)
	{
		case 0:
			_atcmd_info("boot_get: %s", g_atcmd_boot_reason);
			ATCMD_MSG_INFO("BOOT", "\"%s\"", g_atcmd_boot_reason);
			break;

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static atcmd_info_t g_atcmd_basic_boot =
{
	.list.next = NULL,
	.list.prev = NULL,

	.group = ATCMD_GROUP_BASIC,

	.cmd = "BOOT",
	.id = ATCMD_BASIC_BOOT,

	.handler[ATCMD_HANDLER_RUN] = NULL,
	.handler[ATCMD_HANDLER_GET] = _atcmd_basic_boot_get,
	.handler[ATCMD_HANDLER_SET] = NULL,
};

/**********************************************************************************************/

#if defined(NRC7394)

static int _atcmd_basic_xtal_get (int argc, char *argv[])
{
	switch (argc)
	{
		case 0:
		{
			const char *str_status[3] = { "not checked", "working", "not working" };
			int status = nrc_get_xtal_status();

			switch (status)
			{
				case 0:
				case 1:
				case 2:
					_atcmd_info("xtal_status: %d, %s", status, str_status[status]);

					ATCMD_MSG_INFO("XTAL", "%d", status);
			}
			break;
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static atcmd_info_t g_atcmd_basic_xtal =
{
	.list.next = NULL,
	.list.prev = NULL,

	.group = ATCMD_GROUP_BASIC,

	.cmd = "XTAL",
	.id = ATCMD_BASIC_XTAL,

	.handler[ATCMD_HANDLER_RUN] = NULL,
	.handler[ATCMD_HANDLER_GET] = _atcmd_basic_xtal_get,
	.handler[ATCMD_HANDLER_SET] = NULL,
};

#endif

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

#if defined(CONFIG_ATCMD_FWUPDATE)

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
			int verify = g_firmware_update.verify;

			_atcmd_info("fw_update_get: size=%u crc=0x%X verify=%d", size, crc32, verify);

			ATCMD_MSG_INFO("FWUPDATE", "%u,0x%X,%d", size, crc32, verify);
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
	int verify = 0;

	switch (argc)
	{
		case 0:
			ATCMD_MSG_HELP("AT+FWUPDATE=<binary_size>,<crc32>[,<verify>]");
			break;

		case 3:
			verify = atoi(argv[2]);
			if (verify < 0 || verify > 2)
				return ATCMD_ERROR_INVAL;

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
				uint32_t max_fw_size = util_fota_get_max_fw_size();

				if (size <= max_fw_size)
				{
					if (g_firmware_update.size == 0 && g_firmware_update.crc32 == 0)
						break;

					size = g_firmware_update.size;
					crc32 = g_firmware_update.crc32;
					verify = g_firmware_update.verify;

					_atcmd_info("fw_update_busy: size=%u crc=0x%X verify=%d", size, crc32, verify);

					return ATCMD_ERROR_BUSY;
				}
					
				_atcmd_info("fw_update_set: invalid binary size (%u), max_fw_size=%u", size, max_fw_size);
			}

		default:
			return ATCMD_ERROR_INVAL;
	}

	_atcmd_info("fw_update_set: size=%u crc=0x%X verify=%d", size, crc32, verify);

	g_firmware_update.size = size;
	g_firmware_update.crc32 = crc32;
	g_firmware_update.download = 0;
	g_firmware_update.verify = verify;

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

#endif /* #if defined(CONFIG_ATCMD_FWUPDATE) */

/**********************************************************************************************/

#if defined(CONFIG_ATCMD_FWUPDATE)

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
				atcmd_data_mode_params_t data_mode_params;
				uint32_t timeout_msec = _atcmd_timeout_value("FWBINDL");

				if (timeout_msec == 0)
					timeout_msec = 1000;

				_atcmd_info("fw_download_set: offset=%u length=%u timeout=%u", offset, length, timeout_msec);

				atcmd_data_mode_init_params(ATCMD_DATA_FWBINDL, &data_mode_params);

				data_mode_params.len = length;
				data_mode_params.timeout = timeout_msec;
				data_mode_params.done_event = true;

				if (atcmd_data_mode_enable(&data_mode_params) != 0)
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

#endif /*#if defined(CONFIG_ATCMD_FWUPDATE) */

/**********************************************************************************************/

#if defined(CONFIG_ATCMD_SFUSER)

static int _atcmd_basic_sf_user_get (int argc, char *argv[])
{
	switch (argc)
	{
		case 0:
		{
			uint32_t sf_user_area_addr = nrc_get_user_data_area_address();
			uint32_t sf_user_area_size = nrc_get_user_data_area_size();

			_atcmd_info("sf_user_size: addr=0x%X size=%u", sf_user_area_addr, sf_user_area_size); 
			ATCMD_MSG_INFO("SFUSER", "0x%X,%u", sf_user_area_addr, sf_user_area_size / 1024);
			break;
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static int _atcmd_basic_sf_user_set (int argc, char *argv[])
{
	uint32_t offset = ~0;
	uint32_t length = ~0;

	if (!_atcmd_sf_user_support())
		return ATCMD_ERROR_NOTSUPP;

	switch (argc)
	{
		case 0:
			ATCMD_MSG_HELP("AT+SFUSER=<mode>[,<offset>,<length>]");
			break;

		case 3:
			if (atcmd_param_to_uint32(argv[1], &offset) != 0)
				return ATCMD_ERROR_INVAL;

			if (atcmd_param_to_uint32(argv[2], &length) != 0)
				return ATCMD_ERROR_INVAL;

		case 1:
		{
			int mode = atoi(argv[0]);

			if (argc == 1 && mode == SF_USER_ERASE)
			{
				offset = 0;
				length = nrc_get_user_data_area_size();
			}

/*			_atcmd_debug("sf_user_set: mode=%d offset=%d length=%d", mode, offset, length); */

			if (_atcmd_sf_user_valid_params(mode, offset, length))
			{
				char *buffer = NULL;

				switch (mode)
				{
					case SF_USER_READ:
						_atcmd_info("sf_user_read: offset=%d length=%d", offset, length);

						buffer = _atcmd_malloc(ATCMD_DATA_LEN_MAX);
						if (!buffer)
						{
							_atcmd_info("sf_user_read: no memory");
							return ATCMD_ERROR_FAIL;
						}

						memset(buffer, 0, ATCMD_DATA_LEN_MAX);

						if (nrc_read_user_data((uint8_t *)buffer, offset, length) != NRC_SUCCESS)
						{
							_atcmd_info("sf_user_read: failed");
							_atcmd_free(buffer);
							return ATCMD_ERROR_FAIL;
						}
						else
						{
							char msg[ATCMD_MSG_LEN_MAX];
							int len_msg;

							ATCMD_MSG_RETURN("SFUSER", ATCMD_SUCCESS);

							len_msg = ATCMD_MSG_RXD_SFUSER(msg, sizeof(msg), "%d,%d", offset, length);
							atcmd_transmit(msg, len_msg);
							atcmd_transmit(buffer, length);
							_atcmd_free(buffer);

							return ATCMD_NO_RETURN;
						}

					case SF_USER_WRITE:
					{
						atcmd_data_mode_params_t data_mode_params;

						_atcmd_info("sf_user_write: offset=%d length=%d", offset, length);

						atcmd_data_mode_init_params(ATCMD_DATA_SFUSER, &data_mode_params);

						data_mode_params.len = length;
						data_mode_params.timeout = 1000;
						data_mode_params.done_event = true;
						data_mode_params.sf_user.offset = offset;

						if (atcmd_data_mode_enable(&data_mode_params) != 0)
						{
							_atcmd_info("sf_user_write: failed");
							return ATCMD_ERROR_FAIL;
						}

						return ATCMD_SUCCESS;
					}
				
					case SF_USER_ERASE:
						if (argc == 1)
						{
							_atcmd_info("sf_user_erase: all");

							if (nrc_erase_user_data_area() != NRC_SUCCESS)
							{
								_atcmd_info("sf_user_erase: failed");
								return ATCMD_ERROR_FAIL;
							}
						}
						else
						{
							int len_erase;
							int i;

							_atcmd_info("sf_user_erase: offset=%d length=%d", offset, length);

							buffer = _atcmd_malloc(SF_SECTOR_SIZE);
							if (!buffer)
							{
								_atcmd_info("sf_user_erase: no memory");
								return ATCMD_ERROR_FAIL;
							}

							memset(buffer, 0xff, SF_SECTOR_SIZE);

							for (i = 0 ; i < length ; i += len_erase)
							{
								if ((length - i) > SF_SECTOR_SIZE)
									len_erase = SF_SECTOR_SIZE;
								else
									len_erase = length - i;

								if (nrc_write_user_data(offset + i, (uint8_t *)buffer, len_erase) != NRC_SUCCESS)
								{
									_atcmd_info("sf_user_erase: failed, offset=%d length=%d", offset + i, len_erase);
									_atcmd_free(buffer);
									return ATCMD_ERROR_FAIL;
								}

/*								_atcmd_debug("sf_user_erase: success, offset=%d length=%d", offset + i, len_erase); */
							}

							_atcmd_free(buffer);
						}

						return ATCMD_SUCCESS;
				}
			}
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static atcmd_info_t g_atcmd_basic_sf_user =
{
	.list.next = NULL,
	.list.prev = NULL,

	.group = ATCMD_GROUP_BASIC,

	.cmd = "SFUSER",
	.id = ATCMD_BASIC_SF_USER,

	.handler[ATCMD_HANDLER_RUN] = NULL,
	.handler[ATCMD_HANDLER_GET] = _atcmd_basic_sf_user_get,
	.handler[ATCMD_HANDLER_SET] = _atcmd_basic_sf_user_set,
};

#endif /* #if defined(CONFIG_ATCMD_SFUSER) */

/**********************************************************************************************/

#if defined(CONFIG_ATCMD_SFSYSUSER)

static int _atcmd_basic_sf_sys_user_get (int argc, char *argv[])
{
	switch (argc)
	{
		case 0:
		{
			uint32_t sf_sys_user_addr = 0;
			uint32_t sf_sys_user_size = 0;

			if (nrc_get_user_factory_info(&sf_sys_user_addr, &sf_sys_user_size) != NRC_SUCCESS)
				return ATCMD_ERROR_NOTSUPP;

			_atcmd_info("sf_sys_user_get: addr=0x%X size=%u", sf_sys_user_addr, sf_sys_user_size); 
			ATCMD_MSG_INFO("SFSYSUSER", "0x%X,%u", sf_sys_user_addr, sf_sys_user_size);
			break;
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static int _atcmd_basic_sf_sys_user_set (int argc, char *argv[])
{
	uint32_t sf_sys_user_size = 0;
	uint32_t offset = ~0;
	uint32_t length = ~0;

	nrc_get_user_factory_info(NULL, &sf_sys_user_size);
	if (sf_sys_user_size == 0)
		return ATCMD_ERROR_NOTSUPP;

	switch (argc)
	{
		case 0:
			ATCMD_MSG_HELP("AT+SFSYSUSER=<offset>,<length>");
			break;

		case 2:
			if (atcmd_param_to_uint32(argv[1], &length) != 0 || (length == 0 || length > sf_sys_user_size))
				return ATCMD_ERROR_INVAL;

		case 1:
		{
			if (atcmd_param_to_uint32(argv[0], &offset) != 0 || offset >= sf_sys_user_size)
				return ATCMD_ERROR_INVAL;

			if (argc == 1)
				length = sf_sys_user_size - offset;

			if ((offset + length) <= sf_sys_user_size)
			{
				char *buf;
				char msg[ATCMD_MSG_LEN_MAX];
				int len_msg;

				_atcmd_debug("sf_sys_user_set: offset=%u length=%u/%u", offset, length, sf_sys_user_size); 

				buf = _atcmd_malloc(sf_sys_user_size);
				if (!buf || nrc_get_user_factory(buf, sf_sys_user_size) != NRC_SUCCESS)
					return ATCMD_ERROR_FAIL;

				ATCMD_MSG_RETURN("SFSYSUSER", ATCMD_SUCCESS);

				len_msg = ATCMD_MSG_RXD_SFSYSUSER(msg, sizeof(msg), "%u,%u", offset, length);
				atcmd_transmit(msg, len_msg);

				atcmd_transmit(buf + offset, length);
				_atcmd_free(buf);

				return ATCMD_NO_RETURN;
			}
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static atcmd_info_t g_atcmd_basic_sf_sys_user =
{
	.list.next = NULL,
	.list.prev = NULL,

	.group = ATCMD_GROUP_BASIC,

	.cmd = "SFSYSUSER",
	.id = ATCMD_BASIC_SF_SYS_USER,

	.handler[ATCMD_HANDLER_RUN] = NULL,
	.handler[ATCMD_HANDLER_GET] = _atcmd_basic_sf_sys_user_get,
	.handler[ATCMD_HANDLER_SET] = _atcmd_basic_sf_sys_user_set,
};

#endif /* #if defined(CONFIG_ATCMD_SFSYSUSER) */

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
	{ "SSEND", 0 },
	{ "SRECV", 0 },

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
			int j;

			for ( ; i < 2 ; i++)
			{
				timeout = g_atcmd_timeout[i];

				for (j = 0 ; timeout[j].cmd ; j++)
				{
					if (timeout[j].sec == 0)
						continue;

					if (timeout[j].cmd[0] == 'W')
						ATCMD_MSG_INFO("WTIMEOUT", "\"%s\",%d", timeout[j].cmd, timeout[j].sec);
					else if (timeout[j].cmd[0] == 'S')
						ATCMD_MSG_INFO("STIMEOUT", "\"%s\",%d", timeout[j].cmd, timeout[j].sec);
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

static atcmd_info_t *g_atcmd_info_basic[] =
{
	&g_atcmd_basic_version,
	&g_atcmd_basic_boot,
#if defined(NRC7394)
	&g_atcmd_basic_xtal,
#endif
#if defined(CONFIG_ATCMD_UART) || defined(CONFIG_ATCMD_UART_HFC)
	&g_atcmd_basic_uart,
#endif
	&g_atcmd_basic_gpio_config,
	&g_atcmd_basic_gpio_value,
	&g_atcmd_basic_adc,

#if defined(CONFIG_ATCMD_FWUPDATE)
	&g_atcmd_basic_firmware_update,
	&g_atcmd_basic_firmware_donwload,
#endif

#if defined(CONFIG_ATCMD_SFUSER)
	&g_atcmd_basic_sf_user,
#endif
#if defined(CONFIG_ATCMD_SFSYSUSER)
	&g_atcmd_basic_sf_sys_user,
#endif	

/*	&g_atcmd_basic_timeout, */

	NULL
};

int atcmd_basic_enable (void)
{
	int i;

	if (atcmd_group_register(&g_atcmd_group_basic) != 0)
		return -1;

	for (i = 0 ; g_atcmd_info_basic[i] ; i++)
	{
		if (atcmd_info_register(ATCMD_GROUP_BASIC, g_atcmd_info_basic[i]) != 0)
			return -1;
	}

#if defined(CONFIG_ATCMD_INTERNAL)	
	for (i = 0 ; g_atcmd_info_basic_internal[i] ; i++)
	{
		if (atcmd_info_register(ATCMD_GROUP_BASIC, g_atcmd_info_basic_internal[i]) != 0)
			return -1;
	}
#endif

	atcmd_info_print(&g_atcmd_group_basic);

	_atcmd_adc_init();

	return 0;
}

void atcmd_basic_disable (void)
{
	int i;

	_atcmd_adc_deinit();

#if defined(CONFIG_ATCMD_INTERNAL)	
	for (i = 0 ; g_atcmd_info_basic_internal[i] ; i++)
		atcmd_info_unregister(ATCMD_GROUP_BASIC, g_atcmd_info_basic_internal[i]->id);
#endif

	for (i = 0 ; g_atcmd_info_basic[i] ; i++)
		atcmd_info_unregister(ATCMD_GROUP_BASIC, g_atcmd_info_basic[i]->id);

	atcmd_group_unregister(ATCMD_GROUP_BASIC);
}

/**********************************************************************************************/

extern uint8_t drv_get_boot_reason (void);

void atcmd_boot_reason (void)
{
	char *str_boot_reason = g_atcmd_boot_reason;
	uint8_t boot_reason = drv_get_boot_reason();
	int len = 0;

	if (boot_reason & BR_POR)
		len += sprintf(str_boot_reason, "POR");

	if (boot_reason & BR_WDOG)
		len += sprintf(str_boot_reason + len, "%sWDT", len > 0 ? "|" : "");

	if (boot_reason & BR_PMC)
		len += sprintf(str_boot_reason + len, "%sPMC", len > 0 ? "|" : "");

	if (boot_reason & BR_HOSTINF)
		len += sprintf(str_boot_reason + len, "%sHSPI", len > 0 ? "|" : "");

#if defined(NRC7292) && defined(CPU_CM0)		
	if (boot_reason & BR_SYSRST_CM0)
		len += sprintf(str_boot_reason + len, "%sCPU", len > 0 ? "|" : "");
#else	
	if (boot_reason & BR_SYSRST_CM3)
		len += sprintf(str_boot_reason + len, "%sCPU", len > 0 ? "|" : "");
#endif	

	str_boot_reason[len] = '\0';

	_atcmd_info("Boot_Reason: %s (0x%X)", str_boot_reason, boot_reason);

	ATCMD_MSG_EVENT("BOOT", "\"%s\"", str_boot_reason);
}

bool atcmd_gpio_pin_valid (int pin)
{
	return _atcmd_gpio_pin_valid(pin);
}

#if defined(CONFIG_ATCMD_FWUPDATE)
int atcmd_firmware_download (char *buf, int len)
{
	return _atcmd_firmware_download(buf, len);
}

void atcmd_firmware_download_event_idle (uint32_t len, uint32_t cnt)
{
	_atcmd_basic_event_handler(ATCMD_BASIC_EVENT_FWBINDL_IDLE, len, cnt);
}

void atcmd_firmware_download_event_drop (uint32_t len)
{
	_atcmd_basic_event_handler(ATCMD_BASIC_EVENT_FWBINDL_DROP, len);
}

void atcmd_firmware_download_event_fail (uint32_t len)
{
	_atcmd_basic_event_handler(ATCMD_BASIC_EVENT_FWBINDL_FAIL, len);
}

void atcmd_firmware_download_event_done (uint32_t len)
{
	_atcmd_basic_event_handler(ATCMD_BASIC_EVENT_FWBINDL_DONE, len);
}
#endif

#if defined(CONFIG_ATCMD_SFUSER)
int atcmd_sf_user_write (uint32_t offset, uint32_t length, char *data)
{
	return _atcmd_sf_user_write(offset, length, data);
}

void atcmd_sf_user_write_event_idle (uint32_t offset, uint32_t length, uint32_t count)
{
	_atcmd_basic_event_handler(ATCMD_BASIC_EVENT_SFUSER_IDLE, offset, length, count);
}

void atcmd_sf_user_write_event_drop (uint32_t offset, uint32_t length)
{
	_atcmd_basic_event_handler(ATCMD_BASIC_EVENT_SFUSER_DROP, offset, length);
}

void atcmd_sf_user_write_event_fail (uint32_t offset, uint32_t length)
{
		_atcmd_basic_event_handler(ATCMD_BASIC_EVENT_SFUSER_FAIL, offset, length);
}

void atcmd_sf_user_write_event_done (uint32_t offset, uint32_t length)
{
	_atcmd_basic_event_handler(ATCMD_BASIC_EVENT_SFUSER_DONE, offset, length);
}
#endif

