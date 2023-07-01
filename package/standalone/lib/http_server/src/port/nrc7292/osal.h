/*
 * SPDX-FileCopyrightText: 2018-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _OSAL_H_
#define _OSAL_H_

#include <FreeRTOS.h>
#include <task.h>
#include <unistd.h>
#include <stdint.h>
//#include <esp_timer.h>

#ifdef __cplusplus
extern "C" {
#endif

#define OS_SUCCESS ESP_OK
#define OS_FAIL    ESP_FAIL

#define malloc pvPortMalloc
#define free vPortFree
#define calloc pvPortCalloc

typedef TaskHandle_t othread_t;

static inline int httpd_os_thread_create(othread_t *thread,
                                 const char *name, uint16_t stacksize, int prio,
                                 void (*thread_routine)(void *arg), void *arg)
{
    int ret = xTaskCreate(thread_routine, name, stacksize, arg, prio, thread);
    if (ret == pdPASS) {
        return OS_SUCCESS;
    }
    return OS_FAIL;
}

/* Only self delete is supported */
static inline void httpd_os_thread_delete(void)
{
    vTaskDelete(xTaskGetCurrentTaskHandle());
}

static inline void httpd_os_thread_sleep(int msecs)
{
    vTaskDelay(msecs / portTICK_PERIOD_MS);
}

static inline othread_t httpd_os_thread_handle(void)
{
    return xTaskGetCurrentTaskHandle();
}

#ifdef __cplusplus
}
#endif

#endif /* ! _OSAL_H_ */
