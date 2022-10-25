/*
 * wpa_supplicant/hostapd / Empty OS specific functions
 * Copyright (c) 2005-2006, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 *
 * This file can be used as a starting point when adding a new OS target. The
 * functions here do not really work as-is since they are just empty or only
 * return an error value. os_internal.c can be used as another starting point
 * or reference since it has example implementation of many of these functions.
 */

#include "includes.h"
#include "os.h"
#include <FreeRTOS.h>
#include <timers.h>
#include "system_common.h"
#include "system_modem_api.h"

void os_sleep(os_time_t sec, os_time_t usec)
{
	vTaskDelay(pdMS_TO_TICKS(sec * 1000 + (usec+1) / 1000));
}

int os_get_time(struct os_time *t)
{
    int unit = 1000/configTICK_RATE_HZ;
	TickType_t cur = xTaskGetTickCount()*unit;
	t->sec = (cur + 1) / (unit*configTICK_RATE_HZ);
	t->usec = (cur % unit*configTICK_RATE_HZ)*1000;

	return 0;
}

void * os_memdup(const void *src, size_t len)
{
	void *r = os_malloc(len);

	if (r && src)
		os_memcpy(r, src, len);
	return r;
}


int os_get_reltime(struct os_reltime *t)
{
	// TODO:
	struct os_time cur;
	os_get_time(&cur);
	t->sec = cur.sec;
	t->usec = cur.usec;
	return 0;
}

int os_mktime(int year, int month, int day, int hour, int min, int sec,
	      os_time_t *t)
{
	return -1;
}

int os_gmtime(os_time_t t, struct os_tm *tm)
{
	return -1;
}

int os_daemonize(const char *pid_file)
{
	return -1;
}

void os_daemonize_terminate(const char *pid_file)
{
}

int os_get_random(unsigned char *buf, size_t len)
{
	int i;
	for (i = 0 ;i < len; i++)
		buf[i] = os_random();
	return 0;
}

unsigned long os_random(void)
{
#if defined (NRC7291_SDK_DUAL_CM0) || defined (NRC7291_SDK_DUAL_CM3)
	return 0;
#else
	return lmac_get_tsf_l(0);
#endif
}

char * os_rel2abs_path(const char *rel_path)
{
	return NULL; /* strdup(rel_path) can be used here */
}

int os_program_init(void)
{
	return 0;
}

void os_parogram_deinit(void)
{
}

int os_setenv(const char *name, const char *value, int overwrite)
{
	return -1;
}

int os_unsetenv(const char *name)
{
	return -1;
}

char * os_readfile(const char *name, size_t *len)
{
	return NULL;
}

int os_fdatasync(FILE *stream)
{
	return 0;
}

__attribute__( ( always_inline ) ) static inline uint32_t __get_LR(void)
{
	register uint32_t result;
#if (__arm__)
	__asm volatile ("MOV %0, LR\n" : "=r" (result) );
#else
	// ND 10?
	return 0;
#endif
	return result;
}

void * __attribute__ ((noinline)) os_zalloc(size_t size)
{
	uint32_t lr = __get_LR();
	void *p = pvPortMalloc2(size, lr);
	//system_printf("[wpa] alloc ptr=%p size=%d lr=0x%x \n", p, size, lr);
	if (p)
		os_memset(p, 0, size);
	return p;
}

#ifdef OS_NO_C_LIB_DEFINES
void * __attribute__ ((noinline)) os_malloc(size_t size)
{
	uint32_t lr = __get_LR();
	void* ptr = pvPortMalloc2(size, lr);
	//system_printf("[wpa] alloc ptr=%p size=%d lr=0x%x \n", ptr, size, lr);

	return ptr;
}

void * os_realloc(void *ptr, size_t size)
{
	void *ret = os_malloc(size);
	os_memcpy(ret, ptr, size);
	os_free(ptr);
	return ret;
}

void os_free(void *ptr)
{
//	system_printf("[wpa] free ptr=0x%x \n", ptr);
	vPortFree(ptr);
}

void * os_memcpy(void *dest, const void *src, size_t n)
{
	return memcpy(dest, src, n);
}

void * os_memmove(void *dest, const void *src, size_t n)
{
	return memmove(dest, src, n);
}

void * os_memset(void *s, int c, size_t n)
{
	return memset(s, c, n);
}

int os_memcmp(const void *s1, const void *s2, size_t n)
{
	return memcmp(s1, s2, n);
}

char * os_strdup(const char *s)
{
	size_t len = strlen(s) + 1;
	char *dup = os_malloc(len);

	if (!dup)
		return NULL;

	return (char *) os_memcpy(dup, s, len);
}

size_t os_strlen(const char *s)
{
	return strlen(s);
}

int os_strcasecmp(const char *s1, const char *s2)
{
	/*
	 * Ignoring case is not required for main functionality, so just use
	 * the case sensitive version of the function.
	 */
	return os_strcmp(s1, s2);
}

int os_strncasecmp(const char *s1, const char *s2, size_t n)
{
	/*
	 * Ignoring case is not required for main functionality, so just use
	 * the case sensitive version of the function.
	 */
	return os_strncmp(s1, s2, n);
}

char * os_strchr(const char *s, int c)
{
	return strchr(s, c);
}

char * os_strrchr(const char *s, int c)
{
	return strrchr(s, c);
}

int os_strcmp(const char *s1, const char *s2)
{
	return strcmp(s1, s2);
}

int os_strncmp(const char *s1, const char *s2, size_t n)
{
	return strncmp(s1, s2, n);
}

char * os_strncpy(char *dest, const char *src, size_t n)
{
	return strncpy(dest, src, n);
}

size_t os_strlcpy(char *dest, const char *src, size_t size)
{
	return strlcpy(dest, src, size);
}

int os_memcmp_const(const void *a, const void *b, size_t len)
{
	return memcmp(a, b, len);
}

char * os_strstr(const char *haystack, const char *needle)
{
	return strstr(haystack, needle);
}

#endif /* OS_NO_C_LIB_DEFINES */

int os_exec(const char *program, const char *arg, int wait_completion)
{
	return -1;
}
