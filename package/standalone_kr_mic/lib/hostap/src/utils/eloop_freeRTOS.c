/*
 * Event loop based on select() loop
 * Copyright (c) 2002-2009, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#include "includes.h"
#include "utils/list.h"
#include <assert.h>
#include <FreeRTOS.h>
#include <task.h>
#include <timers.h>
#include <stdbool.h>
#include <semphr.h>

#include "common.h"
#include "trace.h"
#include "eloop.h"
#include "wpa_debug.h"

#define US_TIME_UNIT			(1000000)
#define SEC_TO_US(x)			(x * US_TIME_UNIT)

struct eloop_sock {
	int sock;
	void *eloop_data;
	void *user_data;
	eloop_sock_handler handler;
};

struct eloop_timeout {
	struct dl_list list;
	struct os_reltime time;
	void *eloop_data;
	void *user_data;
	eloop_timeout_handler handler;
};

struct eloop_global {
	int max_sock;
	int count;
	struct dl_list timeout;
	struct dl_list execute;
	int signal_count;
	struct eloop_signal *signals;
	int signaled;
	int pending_terminate;
	int terminate;
	struct eloop_timeout *next_timeout;
	TaskHandle_t task;
	SemaphoreHandle_t run_signal;
	SemaphoreHandle_t timeout_list_lock;
};

static struct eloop_global	eloop;
//static StaticTimer_t		eloop_timer;
static TickType_t eloop_rearrange_timeout(void);

#if 0
#ifdef CONFIG_NO_STDOUT_DEBUG
#include "system_common.h"
#include "hal_uart.h"
#ifdef wpa_printf
#undef wpa_printf
static void wpa_printf(int level, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	system_vprintf(fmt, ap);
	system_printf("\n");
}
#endif
#endif
#endif

int eloop_init(void)
{
	os_memset(&eloop, 0, sizeof(eloop));

	dl_list_init(&eloop.timeout);
	dl_list_init(&eloop.execute);

	eloop.run_signal = xSemaphoreCreateBinary();
	eloop.timeout_list_lock = xSemaphoreCreateMutex();

	eloop.task = xTaskGetHandle("wpa_supplicant");

	if (!eloop.task) {
		wpa_printf(MSG_ERROR, "eloop: Unable to find wpa_supplicant task.\n");
		return -1;
	}

	return 0;
}

static bool lock_timeout_list()
{
	const int MAX_TIMEOUT = pdMS_TO_TICKS(100);
	return (xSemaphoreTake(eloop.timeout_list_lock, MAX_TIMEOUT) == pdTRUE);
}

static bool try_lock_timeout_list()
{
	return (xSemaphoreTake(eloop.timeout_list_lock, 0) == pdTRUE);
}

static void unlock_timeout_list()
{
	xSemaphoreGive(eloop.timeout_list_lock);
}

static void eloop_run_signal()
{
	xSemaphoreGive(eloop.run_signal);
}

static void eloop_wait_for_signal(TickType_t max)
{
	xSemaphoreTake(eloop.run_signal, max);
}

static int os_reltime_before_or_same(struct os_reltime *a,
				    struct os_reltime *b)
{
	return (a->sec < b->sec) ||
		(a->sec == b->sec && a->usec <= b->usec);
}


int eloop_sock_requeue(void)
{
	wpa_printf(MSG_INFO, "eloop: %s", __func__);
	return 0;
}

int eloop_register_read_sock(int sock, eloop_sock_handler handler,
		void *eloop_data, void *user_data)
{
	wpa_printf(MSG_INFO, "eloop: %s", __func__);
	return 0;
}

void eloop_unregister_read_sock(int sock)
{
	wpa_printf(MSG_INFO, "eloop: %s", __func__);
}

int eloop_register_sock(int sock, eloop_event_type type,
		eloop_sock_handler handler,
		void *eloop_data, void *user_data)
{
	wpa_printf(MSG_INFO, "eloop: %s", __func__);
	return 0;
}

void eloop_unregister_sock(int sock, eloop_event_type type)
{
	wpa_printf(MSG_INFO, "eloop: %s", __func__);
}

static void eloop_remove_timeout(struct eloop_timeout *timeout)
{
	dl_list_del(&timeout->list);
	wpa_trace_remove_ref(timeout, eloop, timeout->eloop_data);
	wpa_trace_remove_ref(timeout, user, timeout->user_data);
	os_free(timeout);
}

TickType_t eloop_rearrange_timeout(void)
{
	struct eloop_timeout *eto = NULL, *prev = NULL, *first = NULL;
	struct os_reltime now;

	os_get_reltime(&now);

	if (!lock_timeout_list())
		return 1; // return min value for the delay to come this again

	if (dl_list_empty(&eloop.timeout)) {
		unlock_timeout_list();
		return portMAX_DELAY;
	}

	dl_list_for_each_safe(eto, prev, &eloop.timeout, struct eloop_timeout, list) {
		if (os_reltime_before_or_same(&eto->time, &now)) {
			dl_list_del(&eto->list);
			dl_list_add_tail(&eloop.execute, &eto->list);
		}
	}

	first = dl_list_first(&eloop.timeout, struct eloop_timeout, list);

	unlock_timeout_list();

	if (!first)
		return portMAX_DELAY;

	return pdMS_TO_TICKS((first->time.sec - now.sec) * 1000 +
				(first->time.usec - now.usec + 1) / 1000);
}

void eloop_clear_timeout_list(struct dl_list *list) {
	struct eloop_timeout *eto = NULL;

	if (!lock_timeout_list())
		return;

	if (!dl_list_empty(list)) {
		dl_list_for_each(eto, list, struct eloop_timeout, list) {
			if (eto != NULL)
				eloop_remove_timeout(eto);
		}
	}
	dl_list_init(&eloop.execute);
	unlock_timeout_list();
}

static int eloop_trigger_timeout(void)
{
	struct eloop_timeout *timeout = NULL, *prev = NULL;
	int executed = 0;

	dl_list_for_each_safe(timeout, prev, &eloop.execute,
		struct eloop_timeout, list) {
		timeout->handler(timeout->eloop_data, timeout->user_data);
		eloop_remove_timeout(timeout);
		executed++;
	}

	return 0;
}



int eloop_register_timeout(unsigned int secs, unsigned int usecs,
		eloop_timeout_handler handler,
		void *eloop_data, void *user_data)
{
	struct eloop_timeout *timeout = NULL, *tmp = NULL;
	int cnt = 0;

	timeout = (struct eloop_timeout *) os_zalloc(sizeof(*timeout));

	if (!timeout) {
		wpa_printf(MSG_ERROR, "eloop: Failed to allocate new timeout \n");
		return -1;
	}

	if (os_get_reltime(&timeout->time) < 0) {
		os_free(timeout);
		return -1;
	}

	timeout->eloop_data	= eloop_data;
	timeout->user_data	= user_data;
	timeout->handler	= handler;

	if (!lock_timeout_list())
		return -1;

	if (secs == 0 && usecs == 0) {
		dl_list_add_tail(&eloop.execute, &timeout->list);
		eloop_run_signal();
		unlock_timeout_list();
		return 0;
	}

	timeout->time.sec	+= secs;
	timeout->time.usec	+= usecs;

	while (timeout->time.usec >= SEC_TO_US(1)) {
		timeout->time.sec++;
		timeout->time.usec -= US_TIME_UNIT;
	}

	if (dl_list_empty(&eloop.timeout)) {
		dl_list_add_tail(&eloop.timeout, &timeout->list);
		eloop_run_signal();
	} else {
		cnt = 0;
		dl_list_for_each(tmp, &eloop.timeout, struct eloop_timeout, list) {
			if (os_reltime_before(&timeout->time, &tmp->time)) {
				dl_list_add(tmp->list.prev, &timeout->list);

				if (cnt == 0) // Re-calcuate how much in sleep
					eloop_run_signal();
			unlock_timeout_list();
			return 0;
			}
			cnt++;
		}
		dl_list_add_tail(&eloop.timeout, &timeout->list);
	}
	unlock_timeout_list();

	return 0;
}

int eloop_cancel_timeout(eloop_timeout_handler handler,
		void *eloop_data, void *user_data)
{
	struct eloop_timeout *timeout, *prev;
	int removed = 0;

	if (!lock_timeout_list())
		return -1;

	dl_list_for_each_safe(timeout, prev, &eloop.timeout,
		struct eloop_timeout, list) {
		if (timeout->handler == handler &&
			(timeout->eloop_data == eloop_data ||
				eloop_data == ELOOP_ALL_CTX) &&
			(timeout->user_data == user_data ||
				user_data == ELOOP_ALL_CTX)) {
			eloop_remove_timeout(timeout);
			removed++;
		}
	}
	unlock_timeout_list();

	return removed;
}

int eloop_cancel_timeout_one(eloop_timeout_handler handler,
		void *eloop_data, void *user_data,
		struct os_reltime *remaining)
{
	struct eloop_timeout *timeout, *prev;
	int removed = 0;
	struct os_reltime now;

	if (!lock_timeout_list())
		return -1;

	os_get_reltime(&now);
	remaining->sec = remaining->usec = 0;

	dl_list_for_each_safe(timeout, prev, &eloop.timeout,
		struct eloop_timeout, list) {
		if (timeout->handler == handler &&
			(timeout->eloop_data == eloop_data) &&
			(timeout->user_data == user_data)) {
			removed = 1;
			if (os_reltime_before(&now, &timeout->time))
				os_reltime_sub(&timeout->time, &now, remaining);
			eloop_remove_timeout(timeout);
			break;
		}
	}
	unlock_timeout_list();
	return removed;
}

int eloop_is_timeout_registered(eloop_timeout_handler handler,
		void *eloop_data, void *user_data)
{
	struct eloop_timeout *tmp;

	if (!lock_timeout_list())
		return -1;

	dl_list_for_each(tmp, &eloop.timeout, struct eloop_timeout, list) {
		if (tmp->handler == handler &&
			tmp->eloop_data == eloop_data &&
			tmp->user_data == user_data) {
			unlock_timeout_list();
			return 1;
		}
	}
	unlock_timeout_list();
	return 0;
}

int eloop_deplete_timeout(unsigned int req_secs, unsigned int req_usecs,
		eloop_timeout_handler handler, void *eloop_data,
		void *user_data)
{
	struct os_reltime now, requested, remaining;
	struct eloop_timeout *tmp;

	if (!lock_timeout_list())
		return -1;

	//wpa_printf(MSG_INFO, "eloop: %s", __func__);

	dl_list_for_each(tmp, &eloop.timeout, struct eloop_timeout, list) {
		if (tmp->handler == handler &&
		    tmp->eloop_data == eloop_data &&
		    tmp->user_data == user_data) {
			requested.sec = req_secs;
			requested.usec = req_usecs;
			os_get_reltime(&now);
			os_reltime_sub(&tmp->time, &now, &remaining);
			if (os_reltime_before(&requested, &remaining)) {
				eloop_cancel_timeout(handler, eloop_data,
						     user_data);
				eloop_register_timeout(requested.sec,
						       requested.usec,
						       handler, eloop_data,
						       user_data);
				unlock_timeout_list();
				return 1;
			}
			unlock_timeout_list();
			return 0;
		}
	}
	unlock_timeout_list();
	return -1;
}

int eloop_replenish_timeout(unsigned int req_secs, unsigned int req_usecs,
			    eloop_timeout_handler handler, void *eloop_data,
			    void *user_data)
{
	struct os_reltime now, requested, remaining;
	struct eloop_timeout *tmp;
	//int ret = -1;

	if (!lock_timeout_list())
		return -1;

	dl_list_for_each(tmp, &eloop.timeout, struct eloop_timeout, list) {
		if (tmp->handler == handler &&
		    tmp->eloop_data == eloop_data &&
		    tmp->user_data == user_data) {
			requested.sec = req_secs;
			requested.usec = req_usecs;
			os_get_reltime(&now);
			os_reltime_sub(&tmp->time, &now, &remaining);
			if (os_reltime_before(&remaining, &requested)) {
				eloop_cancel_timeout(handler, eloop_data,
						     user_data);
				eloop_register_timeout(requested.sec,
						       requested.usec,
						       handler, eloop_data,
						       user_data);
				unlock_timeout_list();
				return 1;
			}
			unlock_timeout_list();
			return 0;
		}
	}
	unlock_timeout_list();
	return -1;
}

int eloop_register_signal(int sig, eloop_signal_handler handler,
		void *user_data)
{
	wpa_printf(MSG_INFO, "eloop: %s", __func__);
	return 0;
}

int eloop_register_signal_terminate(eloop_signal_handler handler,
		void *user_data)
{
	wpa_printf(MSG_INFO, "eloop: %s", __func__);
	return 0;
}

int eloop_register_signal_reconfig(eloop_signal_handler handler,
		void *user_data)
{
	wpa_printf(MSG_INFO, "eloop: %s", __func__);
	return 0;
}

void eloop_run(void)
{
	TickType_t next;

	while (!eloop.terminate && !eloop.pending_terminate) {
		eloop_trigger_timeout();
		next = eloop_rearrange_timeout();

		if (!dl_list_empty(&eloop.execute))
			continue;

		eloop_wait_for_signal(next);
	}
	eloop.terminate = 0;
}

void eloop_terminate(void)
{
	wpa_printf(MSG_INFO, "eloop: %s", __func__);
	eloop.terminate = 1;
	eloop_run_signal();
}

void eloop_destroy(void)
{
	struct eloop_timeout *timeout, *prev;
	struct os_reltime now;

	os_get_reltime(&now);
	dl_list_for_each_safe(timeout, prev, &eloop.timeout,
					struct eloop_timeout, list) {
		int sec, usec;
		sec = timeout->time.sec - now.sec;
		usec = timeout->time.usec - now.usec;
		if (timeout->time.usec < now.usec) {
			sec--;
			usec += 1000000;
		}
		wpa_printf(MSG_INFO, "ELOOP: remaining timeout: %d.%06d "
			"eloop_data=%p user_data=%p handler=%p",
			sec, usec, timeout->eloop_data, timeout->user_data,
			timeout->handler);
		wpa_trace_dump_funcname("eloop unregistered timeout handler",
					timeout->handler);
		wpa_trace_dump("eloop timeout", timeout);
		eloop_remove_timeout(timeout);
	}
	vSemaphoreDelete(eloop.run_signal);
	vSemaphoreDelete(eloop.timeout_list_lock);
}

int eloop_terminated(void)
{
	return 0;
}

void eloop_wait_for_read_sock(int sock)
{
}
