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

#include "nrc_sdk.h"
#include "wifi_config_setup.h"
#include "wifi_connect_common.h"


static struct
{
	bool run;

	tWIFI_COUNTRY_CODE country;
	int tx_power;

	tWIFI_SECURITY security;
	char password[MAX_PW_LENGTH+1];

	char ssid[MAX_SSID_LENGTH+1];
} g_wifi_user =
{
	.run = false,
};

const char *str_country[WIFI_CC_MAX] =
{
	[WIFI_CC_US] = "US", [WIFI_CC_JP] = "JP", [WIFI_CC_K1] = "K1",
	[WIFI_CC_TW] = "TW", [WIFI_CC_EU] = "EU", [WIFI_CC_CN] = "CN",
	[WIFI_CC_NZ] = "NZ", [WIFI_CC_AU] = "AU", [WIFI_CC_K2] = "K2",
};

const char *str_security[WIFI_SEC_MAX] =
{
	[WIFI_SEC_OPEN] = "open", [WIFI_SEC_WPA2] = "wpa2-psk",
	[WIFI_SEC_WPA3_OWE] = "wpa3-owe", [WIFI_SEC_WPA3_SAE] ="wpa3-sae"
};

static void cmd_user_default (void)
{
	g_wifi_user.country = WIFI_CC_US;
	g_wifi_user.tx_power = 17;

	g_wifi_user.security = WIFI_SEC_WPA3_SAE;
	strcpy(g_wifi_user.password, "12345678");

	strcpy(g_wifi_user.ssid, "halow_test");
}

static int cmd_user_help (void)
{
	nrc_usr_print("Usage: user [options]\n");
	nrc_usr_print("Options:\n");
	nrc_usr_print(" help\n");
	nrc_usr_print(" show\n");
	nrc_usr_print(" reset\n");
	nrc_usr_print(" run [<ssid> [<security> <password>]]\n");
	nrc_usr_print(" set {default|txpower|country|ssid|security} [<value1> [value2]]\n");

	return CMD_RET_SUCCESS;
}

static int cmd_user_show (void)
{
	nrc_usr_print("[ Wi-Fi User Configuration ]\n");
	nrc_usr_print(" - country: %s\n", str_country[g_wifi_user.country]);
	nrc_usr_print(" - txpower: %d\n", g_wifi_user.tx_power);
	nrc_usr_print(" - security: %s %s\n", str_security[g_wifi_user.security], g_wifi_user.password);
	nrc_usr_print(" - ssid: %s\n", g_wifi_user.ssid);

	return CMD_RET_SUCCESS;
}

static int cmd_user_reset (void)
{
	util_fota_reboot_firmware();

	return CMD_RET_SUCCESS;
}

static int cmd_user_run (void)
{
	g_wifi_user.run = true;

	return CMD_RET_SUCCESS;
}

static int cmd_user_set (int argc, char *argv[])
{
	switch (argc)
	{
		case 1:
			if (strcmp(argv[0], "default") == 0)
			{
				cmd_user_default();

				goto cmd_user_set_success;
			}
			break;

		case 2:
			if (strcmp(argv[0], "country") == 0)
			{
				const char *country_code = argv[1];
				int i;

				for (i = 0 ; i < WIFI_CC_MAX ; i++)
				{
					if (strcmp(country_code, str_country[i]) == 0)
					{
						g_wifi_user.country = i;

						goto cmd_user_set_success;
					}
				}
			}
			else if (strcmp(argv[0], "txpower") == 0)
			{
				int txpower = atoi(argv[1]);

				if (txpower < 1 || txpower > 30)
					return CMD_RET_USAGE;

				g_wifi_user.tx_power = txpower;

				goto cmd_user_set_success;
			}
			else if (strcmp(argv[0], "ssid") == 0)
			{
				char *ssid = argv[1];

				if (strlen(ssid) > MAX_SSID_LENGTH)
					return CMD_RET_USAGE;

				strcpy(g_wifi_user.ssid, ssid);

				goto cmd_user_set_success;
			}
			else if (strcmp(argv[0], "security") != 0)
				break;

		case 3:
			if (strcmp(argv[0], "security") == 0)
			{
				const char *security = argv[1];
				const char *password = argc == 3 ? argv[2] : NULL;
				int i;

				for (i = 0 ; i < WIFI_SEC_MAX ; i++)
				{
					if (strcmp(security, str_security[i]) == 0)
					{
						if (password && strlen(password) > MAX_PW_LENGTH)
							break;

						g_wifi_user.security = i;
						strcpy(g_wifi_user.password, password);

						goto cmd_user_set_success;
					}
				}
			}
	}

	return CMD_RET_USAGE;

cmd_user_set_success:

	cmd_user_show();

	return CMD_RET_SUCCESS;
}

static int cmd_user_handler (cmd_tbl_t *t, int argc, char *argv[])
{
	if (argc < 2 || strcmp(argv[0], "user") != 0)
		return CMD_RET_USAGE;

	if (strcmp(argv[1], "help") == 0)
		return cmd_user_help();
	else if (strcmp(argv[1], "show") == 0)
		return cmd_user_show();
	else if (strcmp(argv[1], "reset") == 0)
		return cmd_user_reset();
	else if (strcmp(argv[1], "run") == 0)
		return cmd_user_run();
	else if (strcmp(argv[1], "set") == 0)
		return cmd_user_set(argc - 2, argv + 2);

	return CMD_RET_USAGE;
}

CMD_MAND(user,
	cmd_user_handler,
	"user configuration",
	"user help");

/******************************************************************************/

static void set_wifi_user_config (WIFI_CONFIG *param)
{
	/*
	 * User configuration
	 */
	cmd_user_default();
	cmd_user_show();

	nrc_usr_print("\nusr: Press ENTER key\n");

	while (!g_wifi_user.run)
		_delay_ms(1000);

	/*
	 * Wi-Fi configuration
	 */
	strcpy((char *)param->country, str_country[g_wifi_user.country]);
	param->tx_power = g_wifi_user.tx_power;

	param->security_mode = g_wifi_user.security;
	strcpy((char *)param->password, g_wifi_user.password);

	strcpy((char *)param->ssid, g_wifi_user.ssid);
}


/******************************************************************************
 * FunctionName : user_init
 * Description  : Start Code for User Application, Initialize User function
 * Parameters   : none
 * Returns	    : none
 *******************************************************************************/
WIFI_CONFIG param;

void user_init(void)
{
	tWIFI_STATE_ID wifi_state;

	memset(&param, 0x0, sizeof(WIFI_CONFIG));

	nrc_wifi_set_config(&param);
	set_wifi_user_config(&param);

	if (wifi_init(&param) != WIFI_SUCCESS || wifi_connect(&param) != WIFI_SUCCESS)
	{
		nrc_usr_print("\nusr: Wi-Fi connection failed.\n");
		return;
	}

	nrc_usr_print("\nusr: Wi-Fi connection success.\n");

	/* check the IP is ready */
	while (nrc_addr_get_state(0) != NET_ADDR_SET)
		_delay_ms(1000);

	nrc_usr_print("\nusr: You can use iperf command. (iperf -h)\n");
	nrc_usr_print("\nusr: Press ENTER key\n");
}
