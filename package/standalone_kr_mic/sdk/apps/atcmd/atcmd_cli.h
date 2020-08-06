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

#ifndef __NRC_ATCMD_CLI_H__
#define __NRC_ATCMD_CLI_H__
/**********************************************************************************************/
#if CONFIG_ATCMD_CLI

enum
{
	ATCMD_TRACE_TASK_HIF_RX = 0,
	ATCMD_TRACE_TASK_DATA_MODE,

	ATCMD_TRACE_TASK_NUM,
};

enum
{
	ATCMD_TRACE_MUTEX_LOG = 0,
	ATCMD_TRACE_MUTEX_TX,
	ATCMD_TRACE_MUTEX_FDS,

	ATCMD_TRACE_MUTEX_NUM,
};

enum
{
	ATCMD_TRACE_FUNC_SELECT = 0,
	ATCMD_TRACE_FUNC_SELECT_DONE,
	ATCMD_TRACE_FUNC_SEND,
	ATCMD_TRACE_FUNC_SENDTO,
	ATCMD_TRACE_FUNC_RECV,
	ATCMD_TRACE_FUNC_RECVFROM,
	ATCMD_TRACE_FUNC_EVTSET,
	ATCMD_TRACE_FUNC_EVTWAIT,

	ATCMD_TRACE_FUNC_NUM,
};

typedef struct
{
	struct
	{
		uint32_t loop;
		uint32_t suspend;
		uint32_t resume;
	} task[ATCMD_TRACE_TASK_NUM];

	struct
	{
		uint32_t take;
		uint32_t give;
	} mutex[ATCMD_TRACE_MUTEX_NUM];

	struct
	{
		bool call;
		uint32_t cnt;
	} func[ATCMD_TRACE_FUNC_NUM];
} atcmd_trace_t;

extern int atcmd_trace_init (void);
extern void atcmd_trace_exit (void);

extern void atcmd_trace_task_loop (int id);
extern void atcmd_trace_task_suspend (int id);
extern void atcmd_trace_task_resume (int id);

extern void atcmd_trace_mutex_take (int id);
extern void atcmd_trace_mutex_give (int id);

extern void atcmd_trace_func_call (int id);
extern void atcmd_trace_func_return (int id);

#else /****************************************************************************************/

#define atcmd_trace_init()	0
#define atcmd_trace_exit()

#define atcmd_trace_task_loop(id)
#define atcmd_trace_task_suspend(id)
#define atcmd_trace_task_resume(id)

#define atcmd_trace_mutex_take(id)
#define atcmd_trace_mutex_give(id)

#define atcmd_trace_func_call(id)
#define atcmd_trace_func_return(id)


#endif /* #if CONFIG_ATCMD_CLI */
/**********************************************************************************************/
#endif /* #ifndef __NRC_ATCMD_CLI_H__ */

