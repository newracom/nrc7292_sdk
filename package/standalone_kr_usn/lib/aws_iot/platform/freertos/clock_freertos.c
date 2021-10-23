/**
 * @file clock_freertos.c
 * @brief Implementation of the functions in clock.h for POSIX systems.
 */

/* Platform clock include. */
#include "FreeRTOS.h"
#include "task.h"

#include "clock.h"

/*-----------------------------------------------------------*/

uint32_t Clock_GetTimeMs( void )
{
    return ( uint32_t ) xTaskGetTickCount() / portTICK_PERIOD_MS;
}

/*-----------------------------------------------------------*/

void Clock_SleepMs( uint32_t sleepTimeMs )
{
    ( void ) vTaskDelay(pdMS_TO_TICKS(sleepTimeMs));
}
