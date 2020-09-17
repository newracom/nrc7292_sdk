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


#ifdef CONFIG_ATCMD_TEST
/**********************************************************************************************/

#include "test/atcmd_test.c"
#include "test/atcmd_test_app.c"

static atcmd_group_t g_atcmd_group_test =
{
	.list.next = NULL,
	.list.prev = NULL,

	.name = "TEST",
	.id = ATCMD_GROUP_TEST,

	.cmd_prefix = "T",
	.cmd_prefix_size = 1,

	.cmd_list_head.next = NULL,
	.cmd_list_head.prev = NULL,
};

int atcmd_test_enable (void)
{
	int i;

	if (atcmd_group_register(&g_atcmd_group_test) != 0)
		return -1;

	for (i = 0 ; g_atcmd_test[i] ; i++)
	{
		if (atcmd_info_register(ATCMD_GROUP_TEST, g_atcmd_test[i]) != 0)
			return -1;
	}

	for (i = 0 ; g_atcmd_test_app[i] ; i++)
	{
		if (atcmd_info_register(ATCMD_GROUP_TEST, g_atcmd_test_app[i]) != 0)
			return -1;
	}

#ifdef CONFIG_ATCMD_DEBUG
	atcmd_info_print(&g_atcmd_group_test);
#endif

	return 0;
}

void atcmd_test_disable (void)
{
	int i;

	for (i = 0 ; g_atcmd_test_app[i] ; i++)
		atcmd_info_unregister(ATCMD_GROUP_TEST, g_atcmd_test_app[i]->id);

	for (i = 0 ; g_atcmd_test[i] ; i++)
		atcmd_info_unregister(ATCMD_GROUP_TEST, g_atcmd_test[i]->id);

	atcmd_group_unregister(ATCMD_GROUP_TEST);
}

/**********************************************************************************************/
#endif /* #ifdef CONFIG_ATCMD_TEST */

