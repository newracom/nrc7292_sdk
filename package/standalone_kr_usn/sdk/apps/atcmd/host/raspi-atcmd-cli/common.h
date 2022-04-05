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

#ifndef __COMMON_H__
#define __COMMON_H__
/**********************************************************************************************/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>

#define log_printf(fmt, ...)	printf(" " fmt, ##__VA_ARGS__)

#define log_trace()				log_printf("%s::%d\n", __func__, __LINE__)
#define log_debug(fmt, ...)		log_printf(fmt, ##__VA_ARGS__)
#define log_info(fmt, ...)		log_printf(fmt, ##__VA_ARGS__)
#define log_error(fmt, ...)		log_printf("%s::%d: " fmt, __func__, __LINE__, ##__VA_ARGS__)

typedef enum
{
	false = 0,
	true = 1
} bool;

#define STR_IP4ADDR_LEN_MIN		7
#define STR_IP4ADDR_LEN_MAX		15
/**********************************************************************************************/
#endif /* #ifndef __COMMON_H__ */
