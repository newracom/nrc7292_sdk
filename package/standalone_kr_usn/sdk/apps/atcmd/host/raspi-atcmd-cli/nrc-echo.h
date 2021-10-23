
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

#ifndef __NRC_ECHO_H__
#define __NRC_ECHO_H__
/**********************************************************************************************/

#include "common.h"

typedef struct echo_data
{
	int id;
	int len;
	char *buf;
	struct
	{
		char addr[15 + 1]; /* xxx.xxx.xxx.xxx */
		uint16_t port;
	} remote;

	struct echo_data *next;
} echo_data_t;

typedef struct
{
	bool log;
	bool enable;

	pthread_t thread;
	pthread_cond_t cond;
	pthread_mutex_t mutex;

	struct
	{
		uint32_t cnt;
		struct echo_data *head;
		struct echo_data *tail;
	} queue;
} echo_info_t;

extern int nrc_echo_enable (void);
extern void nrc_echo_disable (void);

extern void nrc_echo_log_on (void);
extern void nrc_echo_log_off (void);
extern bool nrc_echo_log_is_on (void);

/**********************************************************************************************/
#endif /* #ifndef __NRC_ECHO_H__ */


