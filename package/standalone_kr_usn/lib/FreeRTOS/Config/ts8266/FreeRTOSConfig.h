/*
    FreeRTOS V8.0.0 - Copyright (C) 2014 Real Time Engineers Ltd.
    All rights reserved

    VISIT http://www.FreeRTOS.org TO ENSURE YOU ARE USING THE LATEST VERSION.

    ***************************************************************************
     *                                                                       *
     *    FreeRTOS provides completely free yet professionally developed,    *
     *    robust, strictly quality controlled, supported, and cross          *
     *    platform software that has become a de facto standard.             *
     *                                                                       *
     *    Help yourself get started quickly and support the FreeRTOS         *
     *    project by purchasing a FreeRTOS tutorial book, reference          *
     *    manual, or both from: http://www.FreeRTOS.org/Documentation        *
     *                                                                       *
     *    Thank you!                                                         *
     *                                                                       *
    ***************************************************************************

    This file is part of the FreeRTOS distribution.

    FreeRTOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License (version 2) as published by the
    Free Software Foundation >>!AND MODIFIED BY!<< the FreeRTOS exception.

    >>! NOTE: The modification to the GPL is included to allow you to distribute
    >>! a combined work that includes FreeRTOS without being obliged to provide
    >>! the source code for proprietary components outside of the FreeRTOS
    >>! kernel.

    FreeRTOS is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE.  Full license text is available from the following
    link: http://www.freertos.org/a00114.html

    1 tab == 4 spaces!

    ***************************************************************************
     *                                                                       *
     *    Having a problem?  Start by reading the FAQ "My application does   *
     *    not run, what could be wrong?"                                     *
     *                                                                       *
     *    http://www.FreeRTOS.org/FAQHelp.html                               *
     *                                                                       *
    ***************************************************************************

    http://www.FreeRTOS.org - Documentation, books, training, latest versions,
    license and Real Time Engineers Ltd. contact details.

    http://www.FreeRTOS.org/plus - A selection of FreeRTOS ecosystem products,
    including FreeRTOS+Trace - an indispensable productivity tool, a DOS
    compatible FAT file system, and our tiny thread aware UDP/IP stack.

    http://www.OpenRTOS.com - Real Time Engineers ltd license FreeRTOS to High
    Integrity Systems to sell under the OpenRTOS brand.  Low cost OpenRTOS
    licenses offer ticketed support, indemnification and middleware.

    http://www.SafeRTOS.com - High Integrity Systems also provide a safety
    engineered and independently SIL3 certified version for use in safety and
    mission critical applications that require provable dependability.

    1 tab == 4 spaces!
*/

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/*-----------------------------------------------------------
 * Application specific definitions.
 *
 * These definitions should be adjusted for your particular hardware and
 * application requirements.
 *
 * THESE PARAMETERS ARE DESCRIBED WITHIN THE 'CONFIGURATION' SECTION OF THE
 * FreeRTOS API DOCUMENTATION AVAILABLE ON THE FreeRTOS.org WEB SITE.
 *
 * See http://www.freertos.org/a00110.html.
 *----------------------------------------------------------*/

#if defined(CONFIG_PLAT_AE210P) || defined(CONFIG_PLAT_AE100)
	#define configUSE_PREEMPTION				1
	#define configUSE_IDLE_HOOK				1
	#define configUSE_TICK_HOOK				0
	#define configCPU_CLOCK_HZ				( ( unsigned long ) 47923200 )
	#define configTICK_RATE_HZ				( ( portTickType ) 100 )
	#define configMAX_PRIORITIES				20
	//#define configMAX_PRIORITIES				32

#define INCLUDE_xTaskAbortDelay			1
#define INCLUDE_xTaskGetCurrentTaskHandle		1

#ifdef CONFIG_PLAT_AE210P
	// CONFIG_PLAT_AE210P
#ifdef CONFIG_OSC_SUPPORT
	#define configMINIMAL_STACK_SIZE			( ( unsigned short ) 256 )
#else
	//#define configMINIMAL_STACK_SIZE			( ( unsigned short ) 4*1024 )
	#define configMINIMAL_STACK_SIZE			( ( unsigned short ) 256 )
#endif

		#ifdef CONFIG_SMALL_HEAP
		/* This small heap is for WAVBMP demo in 16MB */
			#define configTOTAL_HEAP_SIZE				( ( size_t ) 512*1024 )
		#else
			#ifdef CONFIG_OSC_SUPPORT
				#define configTOTAL_HEAP_SIZE				( ( size_t ) 8*1024 )
			#else
				//#define configTOTAL_HEAP_SIZE				( ( size_t ) 32*1024 )
                #ifdef TS8266_SDIO_FREERTOS
    				#define configTOTAL_HEAP_SIZE				( ( size_t ) 2*1024 )
                #else
    				#define configTOTAL_HEAP_SIZE				( ( size_t ) 55*1024 )
                #endif
			#endif
		#endif
#else
	// CONFIG_PLAT_AE100
	#define configMINIMAL_STACK_SIZE			( ( unsigned short ) 4*1024 )
	#define configTOTAL_HEAP_SIZE				( ( size_t ) 32*1024 )
#endif

	#define configMAX_TASK_NAME_LEN				( 32 )
	#define configUSE_TRACE_FACILITY			1
	#define configUSE_16_BIT_TICKS				0
	#define configIDLE_SHOULD_YIELD				1
    #define configUSE_TASK_NOTIFICATIONS        1
    #define configTIMER_QUEUE_LENGTH		5

	/* Co-routine definitions. */
	#define configUSE_CO_ROUTINES 				0
	#define configMAX_CO_ROUTINE_PRIORITIES 		( 2 )

	/* Set the following definitions to 1 to include the API function, or zero
	to exclude the API function. */

	#define INCLUDE_vTaskPrioritySet			1
	#define INCLUDE_uxTaskPriorityGet			1
	#define INCLUDE_vTaskDelete				1
	#define INCLUDE_vTaskCleanUpResources			1
	#define INCLUDE_vTaskSuspend				1
	#define INCLUDE_vTaskDelayUntil				1
	#define INCLUDE_vTaskDelay				1
	#define INCLUDE_xTaskGetCurrentTaskHandle		1
#define INCLUDE_xTaskAbortDelay			1
#define	INCLUDE_xTaskGetHandle			1
#define configUSE_MUTEXES				1

#define configSUPPORT_STATIC_ALLOCATION		1
#define configSUPPORT_DYNAMIC_ALLOCATION 	1
#define configAPPLICATION_ALLOCATED_HEAP    0

#define configUSE_TIMERS				1
#define configGENERATE_RUN_TIME_STATS	0



	/* IPC Semaphore */
	#define configUSE_COUNTING_SEMAPHORES			1

	/* For debug */
	#define configCHECK_FOR_STACK_OVERFLOW			0
	#define configUSE_MALLOC_FAILED_HOOK			0

	/* For OS aware debugging */
	#define configQUEUE_REGISTRY_SIZE			10
    #define configUSE_RECURSIVE_MUTEXES		1


#else

#define configUSE_PREEMPTION				1
#define configUSE_IDLE_HOOK				0
#ifdef MPU_SUPPORT
#define configUSE_TICK_HOOK				1
#else
#define configUSE_TICK_HOOK				0
#endif
#define configCPU_CLOCK_HZ				( ( unsigned long ) 47923200 )
//#define configTICK_RATE_HZ				( ( portTickType ) 100 )
#define configTICK_RATE_HZ				( ( portTickType ) 1000 )
//#define configMAX_PRIORITIES				( ( unsigned portBASE_TYPE ) 20 )
#define configMAX_PRIORITIES				20

#ifdef CONFIG_OSC_SUPPORT
#define configMINIMAL_STACK_SIZE                        ( ( unsigned short ) 256 )
#else
#define configMINIMAL_STACK_SIZE                        ( ( unsigned short ) 4*1024 )
#endif  // #ifdef CONFIG_OSC_SUPPORT
#ifdef CONFIG_LATENCY_HEAP
#define configTOTAL_HEAP_SIZE                           ( ( size_t ) 28*1024 )
#else
#ifdef CONFIG_PLAT_AG101P_16MB
#ifdef CONFIG_SMALL_HEAP
/* This small heap is for WAVBMP demo in 16MB */
#define configTOTAL_HEAP_SIZE				( ( size_t ) 512*1024 )
#else
#ifdef CONFIG_OSC_SUPPORT
#define configTOTAL_HEAP_SIZE                           ( ( size_t ) 8*1024 )
#else
#define configTOTAL_HEAP_SIZE                           ( ( size_t ) 4*1024*1024 )
#endif  // #ifdef CONFIG_OSC_SUPPORT
#endif
#else
#ifdef CONFIG_SMALL_HEAP
/* This small heap is for NDS_AUDIO demo */
#define configTOTAL_HEAP_SIZE				( ( size_t ) 512*1024 )
#else
#ifdef MPU_SUPPORT
#define configTOTAL_HEAP_SIZE				( ( size_t ) 10*1024*1024 )
#else
#define configTOTAL_HEAP_SIZE				( ( size_t ) 40*1024*1024 )
#endif
#endif
#endif
#endif
#define configMAX_TASK_NAME_LEN				( 16 )
#define configUSE_TRACE_FACILITY			1
#define configUSE_16_BIT_TICKS				0
#define configIDLE_SHOULD_YIELD				1

/* Co-routine definitions. */
#define configUSE_CO_ROUTINES 				0
#define configMAX_CO_ROUTINE_PRIORITIES 		( 2 )


/* Set the following definitions to 1 to include the API function, or zero
to exclude the API function. */

#define INCLUDE_vTaskPrioritySet			1
#define INCLUDE_uxTaskPriorityGet			1
#define INCLUDE_vTaskDelete				1
#define INCLUDE_vTaskCleanUpResources			0
#define INCLUDE_vTaskSuspend				1
#define INCLUDE_vTaskDelayUntil				1
#define INCLUDE_vTaskDelay				1
#define	INCLUDE_xTaskGetHandle			1
#define INCLUDE_xTaskAbortDelay			1
#define INCLUDE_xTaskGetCurrentTaskHandle		1
#define INCLUDE_xSemaphoreGetMutexHolder	1

/* IPC Semaphore */
#define configUSE_COUNTING_SEMAPHORES			1

/* IPC Mutex */
#define configUSE_MUTEXES				1

/* For debug */
#define configCHECK_FOR_STACK_OVERFLOW			0
#define configUSE_MALLOC_FAILED_HOOK			0

/* For OS aware debugging */
#define configQUEUE_REGISTRY_SIZE			10
#endif /* AE210P or AG101P */
#if 0
/* For EventGroups */
#ifdef MPU_SUPPORT
#define configUSE_TIMERS				0
#else
#define configUSE_TIMERS				0	/* EventGroup need SW Timer daemon Task */
#endif
#endif

#define configTIMER_TASK_PRIORITY			2	/* Daemon task priority */
#define configTIMER_QUEUE_LENGTH			5	/* Command Queue of daemon task */
#define configTIMER_TASK_STACK_DEPTH			1024	/* Daemon stack size */
#define INCLUDE_eTaskGetState                           1	/* EventGroup demo program need */
#ifdef MPU_SUPPORT
#define INCLUDE_xTimerPendFunctionCall			0
#else
#define INCLUDE_xTimerPendFunctionCall                  0	/* EventGroup "FromISR" API need */
#endif

/* Normal assert() semantics without relying on the provision of an assert.h
header file. */
#define configASSERT( x ) if( ( x ) == 0 ) { taskDISABLE_INTERRUPTS(); for( ;; ); }

#define configSUPPORT_STATIC_ALLOCATION		1
#define configSUPPORT_DYNAMIC_ALLOCATION 	1

/* Arch Optimization Option */
//#define configUSE_PORT_OPTIMISED_TASK_SELECTION		1

/* Tickless idle/low power functionality support. */
//#define configUSE_TICKLESS_IDLE				1

/* The priority for the task that unblocked by the MAC interrupt to process
received packets. */
#define NRC_TASK_PRIORITY	( configMAX_PRIORITIES - 3 )
#define NRC_TASK_STACK_SIZE 	(2048 / sizeof(StackType_t))

/* The priority of the task that runs the lwIP stack. */
#define LWIP_TASK_PRIORITY	( configMAX_PRIORITIES - 2 )
#define LWIP_TASK_STACK_SIZE	( 1024 )

#define LWIP_IPERF_TASK_PRIORITY	( configMAX_PRIORITIES - 5 )
#define LWIP_IPERF_TASK_STACK_SIZE	( 512 )

#define LWIP_PING_TASK_PRIORITY	( configMAX_PRIORITIES - 6 )
#define LWIP_PING_TASK_STACK_SIZE	( 1024 )

/* The priority of the task that runs the test task */
#define TEST_TASK_PRIORITY	( 2 )
#define TEST_TASK_STACK_SIZE 	(1024 / sizeof(StackType_t))

/* The priority of the wlan_manager task */
#if defined (INCLUDE_NEW_TASK_ARCH)
#define WLAN_MANAGER_TASK_PRIORITY	NRC_TASK_PRIORITY
#else
#define WLAN_MANAGER_TASK_PRIORITY	NRC_TASK_PRIORITY-1
#endif //#if defined (INCLUDE_NEW_TASK_ARCH)
#define WLAN_MANAGER_STACK_SIZE	( 4096 )

#endif /* FREERTOS_CONFIG_H */
