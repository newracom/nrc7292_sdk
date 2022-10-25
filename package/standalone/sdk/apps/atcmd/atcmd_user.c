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


#ifdef CONFIG_ATCMD_USER
/*******************************************************************************************/

enum ATCMD_USER_ID
{
	ATCMD_USER_TEST = ATCMD_USER_MIN,

	ATCMD_USER_LAST = ATCMD_USER_MAX
};

static struct
{
	char param1[ATCMD_STR_SIZE(16)];
	int param2;
} g_user_test;

static int _atcmd_user_test_run (int argc, char *argv[])
{
	_atcmd_info("%s\n", __func__);

	return ATCMD_SUCCESS;
}

static int _atcmd_user_test_get (int argc, char *argv[])
{
	switch (argc)
	{
		case 0:
		{
			char param1[ATCMD_STR_PARAM_SIZE(16)];
			int param2;

			if (!atcmd_str_to_param(g_user_test.param1, param1, sizeof(param1)))
				return ATCMD_ERROR_FAIL;

			param2 = g_user_test.param2;

			ATCMD_MSG_INFO("UTEST", "%s,%d", param1, param2);

			break;
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static int _atcmd_user_test_set (int argc, char *argv[])
{
	char *param1 = NULL;
	char *param2 = NULL;

	switch (argc)
	{
		case 0:
			ATCMD_MSG_HELP("AT+UCMD=\"<param1>\",<param2>");
			break;

		case 2:
		{
			param1 = argv[0];
			param2 = argv[1];

			if (!atcmd_param_to_str(param1, g_user_test.param1, sizeof(g_user_test.param1)))
				return ATCMD_ERROR_FAIL;

			g_user_test.param2 = atoi(param2);
			break;
		}

		default:
			return ATCMD_ERROR_INVAL;
	}

	return ATCMD_SUCCESS;
}

static atcmd_info_t g_atcmd_user_test =
{
	.list.next = NULL,
	.list.prev = NULL,

	.group = ATCMD_GROUP_USER,

	.cmd = "TEST",
	.id = ATCMD_USER_TEST,

	.handler[ATCMD_HANDLER_RUN] = _atcmd_user_test_run,
	.handler[ATCMD_HANDLER_GET] = _atcmd_user_test_get,
	.handler[ATCMD_HANDLER_SET] = _atcmd_user_test_set,
};


/*******************************************************************************************/

static atcmd_group_t g_atcmd_group_user =
{
	.list.next = NULL,
	.list.prev = NULL,

	.name = "USER",
	.id = ATCMD_GROUP_USER,

	.cmd_prefix = "U",
	.cmd_prefix_size = 1,

	.cmd_list_head.next = NULL,
	.cmd_list_head.prev = NULL,
};

static atcmd_info_t *g_atcmd_user[] =
{
	&g_atcmd_user_test,

	NULL
};

int atcmd_user_enable (void)
{
	int i;

	if (atcmd_group_register(&g_atcmd_group_user) != 0)
		return -1;

	for (i = 0 ; g_atcmd_user[i] ; i++)
	{
		if (atcmd_info_register(ATCMD_GROUP_USER, g_atcmd_user[i]) != 0)
			return -1;
	}

#ifdef CONFIG_ATCMD_DEBUG
	atcmd_info_print(&g_atcmd_group_user);
#endif

	return 0;
}

void atcmd_user_disable (void)
{
	int i;

	for (i = 0 ; g_atcmd_user[i] ; i++)
		atcmd_info_unregister(ATCMD_GROUP_USER, g_atcmd_user[i]->id);

	atcmd_group_unregister(ATCMD_GROUP_USER);
}

/*******************************************************************************************/
#endif /* #ifdef CONFIG_ATCMD_USER */


