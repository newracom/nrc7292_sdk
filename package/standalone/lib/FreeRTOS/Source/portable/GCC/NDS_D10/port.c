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


/* Standard includes. */
#include <stdlib.h>
#include <stdio.h>

#ifdef MPU_SUPPORT
/* Defining MPU_WRAPPERS_INCLUDED_FROM_API_FILE prevents task.h from redefining
all the API functions to use the MPU wrappers.  That should only be done when
task.h is included from an application file. */
#define MPU_WRAPPERS_INCLUDED_FROM_API_FILE
#endif

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#ifdef MPU_SUPPORT
#undef MPU_WRAPPERS_INCLUDED_FROM_API_FILE
#endif

/* Processor constants. */
#include "os_cpu.h"

/* Platform definition includes */
#include "bsp_hal.h"

/* Constants required to setup the task context. */
#define portNO_CRITICAL_SECTION_NESTING	( ( portSTACK_TYPE ) 0 )

/*-----------------------------------------------------------*/

extern void vPortISRStartFirstTask( void );
#define OLD_VER 0

#if ( portUSING_MPU_WRAPPERS == 1 )

/* system call number : SWID */
#define SWID_RAISE_PRIVILEGE 5000

/* Set the privilege level to user mode if xRunningPrivileged is false. */
#define portRESET_PRIVILEGE( xRunningPrivileged ) \
if( xRunningPrivileged != pdTRUE ) {__nds32__mtsr(__nds32__mfsr(NDS32_SR_PSW) & ~(1UL << PSW_offPOM), NDS32_SR_PSW); __nds32__dsb();}

/*
 * Configure a number of standard MPU regions that are used by all tasks.
 */
signed portBASE_TYPE prvSetupMPU( void ) PRIVILEGED_FUNCTION;

/*
 * Checks to see if being called from the context of an unprivileged task, and
 * if so raises the privilege level and returns false - otherwise does nothing
 * other than return true.
 */
static portBASE_TYPE prvRaisePrivilege( void ) __attribute__((naked));

/*
 * Restores the MPU TLB setting of task to run.
 */
void vPortRestoreTaskMPU(void) PRIVILEGED_FUNCTION;

/*
 * Misc. functions
 */
static unsigned long setHrange(unsigned long ) PRIVILEGED_FUNCTION;
#ifdef CONFIG_DEBUG
static void _ovly_debug_event(void) PRIVILEGED_FUNCTION;
static int _ovly_get_idx(unsigned int , unsigned int) PRIVILEGED_FUNCTION;

/* _novlys from overlay table in linker script stands for number of overlay regions. */
extern int   _novlys;
extern OVLY_TABLE    _ovly_table[] ;

/* OVLY_TABLE is used to help GDB fetch correct symbol table. */
typedef struct
{
        unsigned long vma;
        unsigned long size;
        unsigned long lma;
        unsigned long mapped;
} OVLY_TABLE ;
#endif

/*-----------------------------------------------------------*/
void *valid_ap = prvRaisePrivilege;
static unsigned long ulStackBottomOffsetVAtoPA;

static const unsigned portLONG xMPU_Region_VAbase[8] =
{
	0x00000000,	/* 0. FIXED 		SUPERUSER 	: Privilege-code/data, global bss */
	0x20000000,	/* 1. RESERVED  	Invalid */
	0x40000000,	/* 2. FIXED		USER		: Common code/data */
	0x60000000,	/* 3. CONFIGURE1	user_config0	: Task code/rodata */
	0x80000000,	/* 4. FIXED		USER		: Peripheral */
	0xA0000000,	/* 5. CONFIGURE0	USER		: Task Stack */
	0xC0000000,	/* 6. CONFIGURE2	user_config1	*/
	0xE0000000,	/* 7. CONFIGURE3	user_config2	*/
};

/*
 * Prototypes for all the MPU wrappers.
 */
BaseType_t MPU_xTaskGenericCreate( TaskFunction_t pvTaskCode, const char * const pcName, uint16_t usStackDepth, void *pvParameters, UBaseType_t uxPriority, TaskHandle_t *pxCreatedTask, StackType_t *puxStackBuffer, const MemoryRegion_t * const xRegions );
void MPU_vTaskAllocateMPURegions( TaskHandle_t xTask, const MemoryRegion_t * const xRegions );
void MPU_vTaskDelete( TaskHandle_t pxTaskToDelete );
void MPU_vTaskDelayUntil( TickType_t * const pxPreviousWakeTime, TickType_t xTimeIncrement );
void MPU_vTaskDelay( TickType_t xTicksToDelay );
UBaseType_t MPU_uxTaskPriorityGet( TaskHandle_t pxTask );
void MPU_vTaskPrioritySet( TaskHandle_t pxTask, UBaseType_t uxNewPriority );
eTaskState MPU_eTaskGetState( TaskHandle_t pxTask );
void MPU_vTaskSuspend( TaskHandle_t pxTaskToSuspend );
void MPU_vTaskResume( TaskHandle_t pxTaskToResume );
void MPU_vTaskSuspendAll( void );
BaseType_t MPU_xTaskResumeAll( void );
TickType_t MPU_xTaskGetTickCount( void );
UBaseType_t MPU_uxTaskGetNumberOfTasks( void );
void MPU_vTaskList( char *pcWriteBuffer );
void MPU_vTaskGetRunTimeStats( char *pcWriteBuffer );
void MPU_vTaskSetApplicationTaskTag( TaskHandle_t xTask, TaskHookFunction_t pxTagValue );
TaskHookFunction_t MPU_xTaskGetApplicationTaskTag( TaskHandle_t xTask );
BaseType_t MPU_xTaskCallApplicationTaskHook( TaskHandle_t xTask, void *pvParameter );
UBaseType_t MPU_uxTaskGetStackHighWaterMark( TaskHandle_t xTask );
TaskHandle_t MPU_xTaskGetCurrentTaskHandle( void );
BaseType_t MPU_xTaskGetSchedulerState( void );
TaskHandle_t MPU_xTaskGetIdleTaskHandle( void );
UBaseType_t MPU_uxTaskGetSystemState( TaskStatus_t *pxTaskStatusArray, UBaseType_t uxArraySize, uint32_t *pulTotalRunTime );
QueueHandle_t MPU_xQueueGenericCreate( UBaseType_t uxQueueLength, UBaseType_t uxItemSize, uint8_t ucQueueType );
BaseType_t MPU_xQueueGenericSend( QueueHandle_t xQueue, const void * const pvItemToQueue, TickType_t xTicksToWait, BaseType_t xCopyPosition );
BaseType_t MPU_xQueueGenericReset( QueueHandle_t pxQueue, BaseType_t xNewQueue );
UBaseType_t MPU_uxQueueMessagesWaiting( const QueueHandle_t pxQueue );
BaseType_t MPU_xQueueGenericReceive( QueueHandle_t pxQueue, void * const pvBuffer, TickType_t xTicksToWait, BaseType_t xJustPeeking );
QueueHandle_t MPU_xQueueCreateMutex( void );
QueueHandle_t MPU_xQueueCreateCountingSemaphore( UBaseType_t uxCountValue, UBaseType_t uxInitialCount );
BaseType_t MPU_xQueueTakeMutexRecursive( QueueHandle_t xMutex, TickType_t xBlockTime );
BaseType_t MPU_xQueueGiveMutexRecursive( QueueHandle_t xMutex );
BaseType_t MPU_xQueueAltGenericSend( QueueHandle_t pxQueue, const void * const pvItemToQueue, TickType_t xTicksToWait, BaseType_t xCopyPosition );
BaseType_t MPU_xQueueAltGenericReceive( QueueHandle_t pxQueue, void * const pvBuffer, TickType_t xTicksToWait, BaseType_t xJustPeeking );
void MPU_vQueueAddToRegistry( QueueHandle_t xQueue, char *pcName );
void MPU_vQueueDelete( QueueHandle_t xQueue );
void *MPU_pvPortMalloc( size_t xSize );
void MPU_vPortFree( void *pv );
void MPU_vPortInitialiseBlocks( void );
size_t MPU_xPortGetFreeHeapSize( void );
QueueSetHandle_t MPU_xQueueCreateSet( UBaseType_t uxEventQueueLength );
QueueSetMemberHandle_t MPU_xQueueSelectFromSet( QueueSetHandle_t xQueueSet, TickType_t xBlockTimeTicks );
BaseType_t MPU_xQueueAddToSet( QueueSetMemberHandle_t xQueueOrSemaphore, QueueSetHandle_t xQueueSet );
BaseType_t MPU_xQueueRemoveFromSet( QueueSetMemberHandle_t xQueueOrSemaphore, QueueSetHandle_t xQueueSet );
BaseType_t MPU_xQueuePeekFromISR( QueueHandle_t xQueue, void * const pvBuffer );

#endif

/*-----------------------------------------------------------*/

/*
 * Initialise the stack of a task to look exactly as if a call to
 * portSAVE_CONTEXT had been called.
 *
 * See header file for description.
 *
 *
 * Stack Layout:
 *		  High  |-----------------|
 *                      |       $R5       |
 *                      |-----------------|
 *                      |        .        |
 *                      |        .        |
 *                      |-----------------|
 *                      |       $R0       |
 *                      |-----------------|
 *                      |       $R30 (LP) |
 *                      |-----------------|
 *                      |       $R29 (GP) |
 *                      |-----------------|
 *                      |       $R28 (FP) |
 *                      |-----------------|
 *                      |   $R15   $R27   |
 *                      |-----------------|
 *                      |   $R10   $R26   |
 *                      |-----------------|
 *                      |        .        |
 *                      |        .        |
 *                      |-----------------|
 *                      |       $R6       |
 *                      |-----------------|
 *                      |       $IFC_LP   | (Option)
 *                      |-----------------|
 *                      |   $LC/$LE/$LB   | (Option)
 *                      |       (ZOL)     |
 *                      |-----------------|
 *                      |       $IPSW     |
 *                      |-----------------|
 *                      |       $IPC      |
 *                      |-----------------|
 *                      |    Dummy word   | (Option, only exist when IFC & ZOL both configured)
 *                      |-----------------|
 *                      |       $FPU      | (Option)
 *                      |-----------------|
 *                Low
 *
 */
/* For relax support, must initial $gp at task init*/
extern INT32U _SDA_BASE_ __attribute__ ((weak));

#if ( portUSING_MPU_WRAPPERS == 1 )
portSTACK_TYPE *pxPortInitialiseStack( portSTACK_TYPE *pxTopOfStack, pdTASK_CODE pxCode, void *pvParameters, portBASE_TYPE xRunPrivileged )
#else
portSTACK_TYPE *pxPortInitialiseStack( portSTACK_TYPE *pxTopOfStack, pdTASK_CODE pxCode, void *pvParameters )
#endif
{
	int i;

	/* Simulate the stack frame as it would be created by a context switch */
	/* R0 ~ R5 registers */
	for (i = 5; i >= 1; i--)                                /* R5, R4, R3, R2 and R1. */
		*--pxTopOfStack = (portSTACK_TYPE) 0x01010101UL * i;
	*--pxTopOfStack = (portSTACK_TYPE) pvParameters;        /* R0 : Argument */

	/* R6 ~ R30 registers */
	*--pxTopOfStack = (portSTACK_TYPE) vPortEndScheduler;   /* R30: $LP */
	*--pxTopOfStack = (portSTACK_TYPE) &_SDA_BASE_;         /* R29: $GP */
	*--pxTopOfStack = (portSTACK_TYPE) 0x2828282828;        /* R28: $FP */
#ifdef __NDS32_REDUCE_REGS__
	*--pxTopOfStack = (portSTACK_TYPE) 0x1515151515;        /* R15 */
	for (i = 10; i >= 6; i--)                               /* R10 ~ R6 */
		*--pxTopOfStack = (portSTACK_TYPE) 0x01010101UL * i;
#else
	for (i = 27; i >= 6; i--)                               /* R27 ~ R6 */
		*--pxTopOfStack = (portSTACK_TYPE) 0x01010101UL * i;
#endif

	/* IFC system register */
#ifdef __TARGET_IFC_EXT
	*--pxTopOfStack = (portSTACK_TYPE) 0x0;                 /* $IFC_LP */
#endif

	/* ZOL system registers */
#ifdef __TARGET_ZOL_EXT
	*--pxTopOfStack = (portSTACK_TYPE) 0x0;                 /* $LC */
	*--pxTopOfStack = (portSTACK_TYPE) 0x0;                 /* $LE */
	*--pxTopOfStack = (portSTACK_TYPE) 0x0;                 /* $LB */
#endif

	/* IPSW and IPC system registers */
#if ( portUSING_MPU_WRAPPERS == 1 )
        /* Default IPSW: enable GIE, set CPL to 7, enable IT/DT, clear IFCON, POM is user or superuser */
	i = (__nds32__mfsr(NDS32_SR_PSW) | PSW_mskGIE | PSW_mskCPL | PSW_mskIT | PSW_mskDT) & ~(PSW_mskIFCON | PSW_mskPOM);
	if ( xRunPrivileged == pdTRUE ) {
		/* Superuser mode */
		i |= (1UL << PSW_offPOM);
	}
#else
        /* Default IPSW: enable GIE, set CPL to 7, clear IFCON */
	i = (__nds32__mfsr(NDS32_SR_PSW) | PSW_mskGIE | PSW_mskCPL) & ~PSW_mskIFCON;
#endif
	*--pxTopOfStack = (portSTACK_TYPE) i;                   /* $IPSW */
        *--pxTopOfStack = (portSTACK_TYPE) pxCode;              /* $IPC */

	/* Dummy word for 8-byte stack alignment */
#if defined(__TARGET_IFC_EXT) && defined(__TARGET_ZOL_EXT)
	*--pxTopOfStack = (portSTACK_TYPE) 0xFFFFFFFF;          /* Dummy */
#endif

	/* FPU registers */
#ifdef __TARGET_FPU_EXT
	for (i = 0; i < FPU_REGS; i++)
		*--pxTopOfStack = (portSTACK_TYPE) 0x0;         /* FPU */
#endif

#if ( portUSING_MPU_WRAPPERS == 1 )
	return (portSTACK_TYPE *)((unsigned int)pxTopOfStack + ulStackBottomOffsetVAtoPA);
#else
	return pxTopOfStack;
#endif
}

/*-----------------------------------------------------------*/

portBASE_TYPE xPortStartScheduler( void )
{
	/* Start the first task. */
	vPortISRStartFirstTask();

	/* Should not get here! */
	return 0;
}

/*-----------------------------------------------------------*/

void vPortEndScheduler( void )
{
	printf("Current Task will be deleted\n");

	/*
 	 * FreeRTOS vTaskDelete() just remove TCB from list.
 	 * vTaskDelete() would call vPortYiled change to Idle task which would do garbage collection.
 	 * So called vTaskDelete() or return from task, make sure GIE is turn-on if context switch by SWI.
	 */
	vTaskDelete(NULL);
}
#include "hal.h"
void vPortYield()
{
#if ( portUSING_MPU_WRAPPERS == 1 )
	BaseType_t xRunningPrivileged = prvRaisePrivilege();
#endif

	/* trigger swi*/
	__nds32__mtsr(0x10000, NDS32_SR_INT_PEND);
	__nds32__isb();

#if ( portUSING_MPU_WRAPPERS == 1 )
	portRESET_PRIVILEGE(xRunningPrivileged);
#endif
}
#define portNO_CRITICAL_NESTING		( ( unsigned long ) 0 )
volatile unsigned long ulCriticalNesting = 0UL;
static OS_CPU_SR psw_0 = 0;
OS_CPU_SR psw_1 = 0;
void vPortEnterCritical()
{
       /*
        *       in order to avoid race condition
        *       1.store psw into stack
        *       2.disable gie
        *       3.store psw into global if ulCriticalNesting==0
        *       4.ulCriticalNesting++ 
        */
#if ( portUSING_MPU_WRAPPERS == 1 )
       BaseType_t xRunningPrivileged = prvRaisePrivilege();
#endif
       volatile unsigned int psw = __nds32__mfsr(NDS32_SR_PSW);
       GIE_DISABLE();
       if (ulCriticalNesting == portNO_CRITICAL_NESTING )
               psw_0 = psw;
       ulCriticalNesting++;

#if ( portUSING_MPU_WRAPPERS == 1 )
       portRESET_PRIVILEGE(xRunningPrivileged);
#endif
}

void vPortExitCritical()
{
#if ( portUSING_MPU_WRAPPERS == 1 )
       BaseType_t xRunningPrivileged = prvRaisePrivilege();
#endif

       --ulCriticalNesting;
       if ( ulCriticalNesting == portNO_CRITICAL_NESTING )
               GIE_RESTORE(psw_0);

#if ( portUSING_MPU_WRAPPERS == 1 )
       portRESET_PRIVILEGE(xRunningPrivileged);
#endif
}


/*-----------------------------------------------------------*/

/* Kernel interrupt mask, it imply the kernel priority comapre with interrupt */
inline unsigned long ulPortSetInterruptMask( void )
{
	/* By default, the kerenl has the highest priority */
	/* It's mean that we don't support kernel preemptive */

#ifdef CONFIG_HW_PRIO_SUPPORT
	/* @Apply HW priority */
	/* GIE on/off control by kernel ISR*/
	/* Mask(disable) all interrupt */
	return hal_intc_irq_mask(-1);
#else
	/* Apply SW priority */
	/* GIE on/off control by user ISR*/
	/* Disable GIE */
	unsigned long psw = __nds32__mfsr(NDS32_SR_PSW);
	GIE_DISABLE();
	return psw;
#endif
}

inline void vPortClearInterruptMask( unsigned long msk )
{
/* see ulPortSetInterruptMask comment */
#ifdef CONFIG_HW_PRIO_SUPPORT
	/* restore interrupt mask */
	hal_timer_irq_unmask(msk)
#else
	/* restore GIE */
	GIE_RESTORE(msk);
#endif
}


/*-----------------------------------------------------------*/

#if configUSE_TICKLESS_IDLE == 1
#define TICK_HZ 100
#define ulTimerCountsForOneTick 		(MB_PCLK / TICK_HZ)
#define xMaximumPossibleSuppressedTicks 	(30*TICK_HZ) /* 30(sec)*TICK_HZ (Ticks!!)*/
#define ulStoppedTimerCompensation		10	/* FIXME add this miss config */	


__attribute__((weak)) void vPortSuppressTicksAndSleep( TickType_t xExpectedIdleTime )
	{
	uint32_t ulReloadValue, ulCompleteTickPeriods, ulCompletedSysTickDecrements;
	TickType_t xModifiableIdleTime;

		/* Make sure the SysTick reload value does not overflow the counter. */
		if( xExpectedIdleTime > xMaximumPossibleSuppressedTicks )
		{
			xExpectedIdleTime = xMaximumPossibleSuppressedTicks;
		}

		/* Stop the SysTick momentarily.  The time the SysTick is stopped for
		is accounted for as best it can be, but using the tickless mode will
		inevitably result in some tiny drift of the time maintained by the
		kernel with respect to calendar time. */
		hal_timer_stop(1);

		/* Calculate the reload value required to wait xExpectedIdleTime
		tick periods.  -1 is used because this code will execute part way
		through one of the tick periods. */
		ulReloadValue = hal_timer_count_read(1) + ( ulTimerCountsForOneTick * ( xExpectedIdleTime - 1UL ) );

		if( ulReloadValue > ulStoppedTimerCompensation )
		{
			ulReloadValue -= ulStoppedTimerCompensation;
		}

		/* Enter a critical section but don't use the taskENTER_CRITICAL()
		method as that will mask interrupts that should exit sleep mode. */
		__nds32__gie_dis();

		/* If a context switch is pending or a task is waiting for the scheduler
		to be unsuspended then abandon the low power entry. */
		if( eTaskConfirmSleepModeStatus() == eAbortSleep )
		{
		
			/* Restart from whatever is left in the count register to complete
			this tick period. */
			/* TODO why need this step? Could skip? */
			hal_timer_set_period( 1, hal_timer_count_read(1) );		

			/* TODO set timer counter as 0 */
			/* Restart SysTick. */
			hal_timer_start(1);

			/* Reset the reload register to the value required for normal tick
			periods. */
			hal_timer_set_period(1, ulTimerCountsForOneTick - 1UL);

			/* Re-enable interrupts - see comments above the cpsid instruction()
			above. */
			__nds32__gie_en();
		}
		else
		{
			/* Set the new reload value. */
			hal_timer_set_period( 1 ,ulReloadValue );

			/* Restart SysTick. */
			hal_timer_start(1);

			/* Sleep until something happens.  configPRE_SLEEP_PROCESSING() can
			set its parameter to 0 to indicate that its implementation contains
			its own wait for interrupt or wait for event instruction, and so wfi
			should not be executed again.  However, the original expected idle
			time variable must remain unmodified, so a copy is taken. */
			xModifiableIdleTime = xExpectedIdleTime;
			configPRE_SLEEP_PROCESSING( xModifiableIdleTime );
			if( xModifiableIdleTime > 0 )
			{
				extern int puts(const char*);
				__nds32__msync_all();
				__nds32__standby_no_wake_grant();
				__nds32__isb();
				
				/* 
 				 * extern int puts(const char*);
				 * puts("          Leave STANDBY\r\n");
				 */

			}
			configPOST_SLEEP_PROCESSING( xExpectedIdleTime );

			/* Stop SysTick.  Again, the time the SysTick is stopped for is
			accounted for as best it can be, but using the tickless mode will
			inevitably result in some tiny drift of the time maintained by the
			kernel with respect to calendar time. */
			hal_timer_stop(1);

			/* Re-enable interrupts - see comments above the cpsid instruction()
			above. */
			__nds32__gie_en();

			if( hal_intc_get_all_pend() & (0x1 << 19) )
			{
				uint32_t ulCalculatedLoadValue;

				/* The tick interrupt has already executed, and the SysTick
				count reloaded with ulReloadValue.  Reset the
				portNVIC_SYSTICK_LOAD_REG with whatever remains of this tick
				period. */
				ulCalculatedLoadValue = ( ulTimerCountsForOneTick - 1UL ) - ( ulReloadValue - hal_timer_count_read(1) );


				/* Don't allow a tiny value, or values that have somehow
				underflowed because the post sleep hook did something
				that took too long. */
				if( ( ulCalculatedLoadValue < ulStoppedTimerCompensation ) || ( ulCalculatedLoadValue > ulTimerCountsForOneTick ) )
				{
					ulCalculatedLoadValue = ( ulTimerCountsForOneTick - 1UL );
				}

				hal_timer_set_period(1, ulCalculatedLoadValue);

				/* The tick interrupt handler will already have pended the tick
				processing in the kernel.  As the pending tick will be
				processed as soon as this function exits, the tick value
				maintained by the tick is stepped forward by one less than the
				time spent waiting. */
				ulCompleteTickPeriods = xExpectedIdleTime - 1UL;
			}
			else
			{
				/* Something other than the tick interrupt ended the sleep.
				Work out how long the sleep lasted rounded to complete tick
				periods (not the ulReload value which accounted for part
				ticks). */
				ulCompletedSysTickDecrements = ( xExpectedIdleTime * ulTimerCountsForOneTick ) - hal_timer_count_read(1);

				/* How many complete tick periods passed while the processor
				was waiting? */
				ulCompleteTickPeriods = ulCompletedSysTickDecrements / ulTimerCountsForOneTick;

				/* The reload value is set to whatever fraction of a single tick
				period remains. */
				hal_timer_set_period( 1, (( ulCompleteTickPeriods + 1 ) * ulTimerCountsForOneTick)-ulCompletedSysTickDecrements);

			}

			/* Restart SysTick so it runs from portNVIC_SYSTICK_LOAD_REG
			again, then set portNVIC_SYSTICK_LOAD_REG back to its standard
			value.  The critical section is used to ensure the tick interrupt
			can only execute once in the case that the reload register is near
			zero. */
			/* TODO reset timer counter as 0 */
			portENTER_CRITICAL();
			{
				hal_timer_start(1);
				vTaskStepTick( ulCompleteTickPeriods );
				hal_timer_set_period( 1, (ulTimerCountsForOneTick - 1UL) );
			}
			portEXIT_CRITICAL();
		}
	}


#endif /* #if configUSE_TICKLESS_IDLE */
/*-----------------------------------------------------------*/

#if ( portUSING_MPU_WRAPPERS == 1 )

#define CXMV(c,x,m,v)   (((c) << 6) | ((x) << 4) | ((m) << 1) | (v))
#define TLB_ENTRY(addr) (addr & ~0x1FFFFFFF)
#define MPU_TLB(entry, hrange, psb, cxmv) \
do { \
        __nds32__mtsr(psb | cxmv, NDS32_SR_TLB_DATA); \
        __nds32__mtsr(hrange, NDS32_SR_TLB_VPN); \
        __nds32__dsb(); \
        __nds32__tlbop_twr(entry); \
        __nds32__isb(); \
} while(0);


#ifdef CONFIG_DEBUG
/* DO NOT MODIFY THE FOLLOWING DEBUGGING HOOKS; KEEP IT AS IS */
static void _ovly_debug_event (void)
{
}
/*-----------------------------------------------------------*/

int _ovly_get_idx(unsigned int lma, unsigned int size)
{
	int i;
	/* checkout bound */
	extern char _ovly_lma_start, _ovly_lma_end;
	/* out of ovly bound, this task doesn't at ovly section */
	if (lma < (unsigned int)&_ovly_lma_start || (lma+size) > (unsigned int)&_ovly_lma_end )
		return -1;

	for ( i=0; i < _novlys; ++i )
	{
		if (lma == _ovly_table[i].lma )
			return i;
	}

	while(1); /* shouldn't come here. */
}
/*-----------------------------------------------------------*/
#endif

static unsigned long setHrange(unsigned long hrange )
{
	/* Hrange value must be 4K alignment and between 0x1000 ~ 0x20000000 range. */
	if (hrange & ~(0x1000 - 1)) {
		if (hrange & ~(0x20000000 - 1))
			return 0x1FFFF000;	// hrange >= 0x20000000
	} else {
		return 0x1000;			// hrange < 0x1000
	}

	return ((hrange + (0x1000 - 1)) & (0x1FFFF000));
}
/*-----------------------------------------------------------*/

static portBASE_TYPE prvRaisePrivilege( void )
{
	/*
	 * Get PSW.POM
	 * PSW.POM : 1, privileged & return true.
	 * PSW.POM : 0, user & switch into privileged, retrun false.
	 *
	 * NOTE:
	 * In user mode, accessing PSW to generate 'Privileged instruction'
	 * exception to get PSW value which return from general exception
	 * handler over '$r0' register. In privilege mode, get PSW value
	 * directly.
	 */
	__asm__ volatile (
		"mfsr	$r0, $PSW\n\t"          // Generate 'Privileged instruction' exception to get PSW if in use mode
		"btst	$r0, $r0, %0\n\t"	// Is the task running privileged? (Check PSW.POM)
		"bnez	$r0, 1f\n\t"            // psw.pom == 1, privileged & return true
		"syscall %1\n\t"                // psw.pom == 0, call syscall to switch into privileged, retrun false
		"1:\n\t"
		"ret\n\t"
		:: "i" (PSW_offPOM), "i" (SWID_RAISE_PRIVILEGE) : "$r0"
	);

	return pdFALSE;		// Dummy return
}
/*-----------------------------------------------------------*/

void prv_Syscall_Handler( unsigned long * arg )
{
	/* The stack contains caller saved registrs:
 	 * r0, r1, r2, r3, r4, r5 as arguments
 	 * r15 ~ r27 depend on REDUCE_REG.
 	 * The first argument (r0) is arg[0]
	 */
	/* swid is the syscall number */
	unsigned int  swid = __nds32__mfsr(NDS32_SR_ITYPE) >> 16;

	switch (swid)
	{
		case SWID_RAISE_PRIVILEGE:
		{
			unsigned int ipsw = __nds32__mfsr(NDS32_SR_IPSW) |  (1UL << PSW_offPOM);
			__nds32__mtsr(ipsw,NDS32_SR_IPSW);
			__nds32__dsb();
			break;
		}
		default:
		{
			/* unknown syscall number */
			while(1);
		}
			break;
	}
}
/*-----------------------------------------------------------*/

signed portBASE_TYPE prvSetupMPU( void )
{
	extern char _stack_end;
	extern char _user_common_end;

	unsigned int mmu_cfg = __nds32__mfsr(NDS32_SR_MMU_CFG);
	unsigned char mmps = (unsigned char)( mmu_cfg & MMU_CFG_mskMMPS);
	signed portBASE_TYPE xReturn = pdFALSE;

	/* Check Memory Management Protection Scheme */
	switch ( mmps )
	{
		case 0x0 :
			printf("Hardware has no MPU Scheme\r\n");
			break;
		case 0x2 :
			printf("Hardware supports MMU Scheme\r\n");
			break;
		case 0x1 :
		{
			/* Set Fixed Fields MPU Entry (Entry, Hrange, PSB, Attr) */
			/* Entry 0 : Privileged	/ non-cache / RW (Privilege-text/data, global bss) */
			MPU_TLB(TLB_ENTRY(0x0), setHrange((unsigned)&_stack_end), 0x0, CXMV(2,2,7,1));
			/* Entry 1 : Privileged		NONE		NONE		  */

			/* Entry 2 : User / non-cache / RW (Common code/data) */
			MPU_TLB(TLB_ENTRY(0x40000000), setHrange((unsigned long)&_user_common_end - 0x40000000), 0x05000000, CXMV(2,3,3,1));
			/* Entry 4 : User / non-cache / RW (Peripheral) */
			MPU_TLB(TLB_ENTRY(0x80000000), setHrange(0x20000000), (0x80000000), CXMV(2,3,3,1));

        		/* Since VA is equal to PA at this stage, we can turn on IT/DT directly */
        		__nds32__mtsr(__nds32__mfsr(NDS32_SR_PSW) | (0x3 << 6), NDS32_SR_PSW);
        		__nds32__isb();

			xReturn = pdTRUE;
			printf("MPU setting seccuss\r\n");
			break;
		}
		default :
		{
			printf("Invalid mmps field value !!!\r\n");
		}
	}
	return xReturn;
}
/*-----------------------------------------------------------*/

#define CXMV(c,x,m,v)      (((c) << 6) | ((x) << 4) | ((m) << 1) | (v))
#define PSB(pbase)         ((unsigned long)pbase & ~(0xFFF))
#define HRANGE(vbase,size) (vbase + size)

// @pxBottomOfStack : the lowest address of stack, would be 4KB-aligned
void vPortStoreTaskMPUSettings( xMPU_SETTINGS *xMPUSettings, const struct xMEMORY_REGION * const xRegions, StackType_t *pxBottomOfStack, unsigned short usStackDepth )
{
	unsigned long ul;

	/* No define gernic MPU regions, use the default setting */
	if( xRegions == NULL ) {
		/* Task Stack Region (PA @pxBottomOfStack) : User / non-cache / RW */
		xMPUSettings->xRegion[ 0 ].ulPA = (unsigned portLONG) pxBottomOfStack;
		xMPUSettings->xRegion[ 0 ].ulSize = usStackDepth*sizeof(StackType_t);
		xMPUSettings->xRegion[ 0 ].ulAttr = CXMV(2,3,3,1);
		xMPUSettings->xRegion[ 0 ].ulTLB_VPN = setHrange(usStackDepth*4);
		xMPUSettings->xRegion[ 0 ].ulTLB_DATA= PSB( (char*)pxBottomOfStack ) | CXMV(2,3,3,1);

		/* ulStackBottomOffsetVAtoPA will be used by stack initialize */
		ulStackBottomOffsetVAtoPA = (unsigned long ) 0xA0000000 - (unsigned long) pxBottomOfStack;
		//printf("pxBottomOfStack is 0x%x\r\n",pxBottomOfStack);
		//printf("ulStackBottomOffsetVAtoPA is 0x%x\r\n",ulStackBottomOffsetVAtoPA);

		/* Should FIXME : set non-restricted task allow all tasks-text read-only */
		/* TASK region (PA @_tasks_start) : User / non-cache / RO */
		extern char _tasks_start;
		xMPUSettings->xRegion[ 1 ].ulPA = (unsigned portLONG)&_tasks_start;
		xMPUSettings->xRegion[ 1 ].ulSize = setHrange(0x20000000);
		xMPUSettings->xRegion[ 1 ].ulAttr = CXMV(2,3,1,1);
		xMPUSettings->xRegion[ 1 ].ulTLB_VPN = setHrange(0x20000000);
		xMPUSettings->xRegion[ 1 ].ulTLB_DATA= PSB(&_tasks_start) | CXMV(2,3,1,1);

		/* Invalidate all other regions. */
		for( ul = 2; ul <= portNUM_CONFIGURABLE_REGIONS; ul++ ) {
			xMPUSettings->xRegion[ul].ulTLB_VPN = 0x0;
			xMPUSettings->xRegion[ul].ulTLB_DATA &= (~0x1);
			xMPUSettings->xRegion[ul].ulPA = 0x0;
			xMPUSettings->xRegion[ul].ulSize = 0x0;
			xMPUSettings->xRegion[ul].ulAttr &= (~0x1);
		}

	} else {
		/*
		 * This function is called automatically when the task is created - in
		 * which case the stack region parameters will be valid.  At all other
		 * times the stack parameters will not be valid and it is assumed that the
		 * stack region has already been configured.
		 */
		if( usStackDepth > 0 ) {
			/*  Define the region that allows access to the task stack */
			xMPUSettings->xRegion[ 0 ].ulPA = (unsigned portLONG) pxBottomOfStack;
			xMPUSettings->xRegion[ 0 ].ulSize = usStackDepth*sizeof(StackType_t);
			xMPUSettings->xRegion[ 0 ].ulAttr = CXMV(2,3,3,1);
			xMPUSettings->xRegion[ 0 ].ulTLB_VPN = setHrange(usStackDepth*sizeof(StackType_t));
			xMPUSettings->xRegion[ 0 ].ulTLB_DATA= PSB((char*)pxBottomOfStack) | CXMV(2,3,3,1);

			ulStackBottomOffsetVAtoPA = (unsigned long ) 0xA0000000 - (unsigned long) pxBottomOfStack;
			//printf("pxBottomOfStack is 0x%x\r\n",pxBottomOfStack);
			//printf("ulStackBottomOffsetVAtoPA is 0x%x\r\n",ulStackBottomOffsetVAtoPA);
		}

		for( ul = 1; ul <= portNUM_CONFIGURABLE_REGIONS; ul++ ) {
			if( ( xRegions[ ul-1] ).ulLengthInBytes > 0UL ) {
				xMPUSettings->xRegion[ul].ulTLB_VPN = setHrange(xRegions[ul-1].ulLengthInBytes);
				xMPUSettings->xRegion[ul].ulTLB_DATA= PSB(( unsigned long ) xRegions[ul-1].pvBaseAddress)| (( unsigned long ) xRegions[ul-1].ulParameters );
				xMPUSettings->xRegion[ul].ulPA = (unsigned portLONG)xRegions[ul-1].pvBaseAddress;
				xMPUSettings->xRegion[ul].ulSize = xRegions[ul-1].ulLengthInBytes;
				xMPUSettings->xRegion[ul].ulAttr = xRegions[ul-1].ulParameters;
			} else {
				/* Invalidate the region. */
				xMPUSettings->xRegion[ul].ulTLB_VPN = 0x0;
				xMPUSettings->xRegion[ul].ulTLB_DATA &= (~0x1);
				xMPUSettings->xRegion[ul].ulPA = 0x0;
				xMPUSettings->xRegion[ul].ulSize = 0x0;
				xMPUSettings->xRegion[ul].ulAttr &= (~0x1);
			}
		}
	}
}
/*-----------------------------------------------------------*/

void vPortRestoreTaskMPU(void)
{
	extern void * pxCurrentTCB;
	xMPU_REGION_ENTRY * p = (xMPU_REGION_ENTRY *)((unsigned int)pxCurrentTCB + 4);
	int i;

#ifdef CONFIG_DEBUG
        /* DO NOT MODIFY THE FOLLOWING DEBUGGING CODE; KEEP THEM AS THEY ARE */
	static int prv_seg;
	static int now_seg = -1;

	/* switch */
	prv_seg = now_seg;
	/* Get now task ovly_table index */
	now_seg = _ovly_get_idx(p[1].ulPA ,p[1].ulSize);
#endif /* CONFIG_DEBUG */

        /* Entry 3 : Task text region, VA @0x60000000 */
        MPU_TLB(TLB_ENTRY(0x60000000),p[1].ulTLB_VPN , (p[1].ulTLB_DATA & ~(0xfff)), (p[1].ulTLB_DATA & (0xfff)));

	/* Entry 5 : Task stack	region VA @0xA0000000 */
	MPU_TLB(TLB_ENTRY(0xA0000000),p[0].ulTLB_VPN , (p[0].ulTLB_DATA & ~(0xfff)), (p[0].ulTLB_DATA & (0xfff)));

	for ( i = 2; i < portTOTAL_NUM_REGIONS; ++i ) {
		if (p[i].ulTLB_DATA & 1) {
			MPU_TLB(TLB_ENTRY(xMPU_Region_VAbase[4+i]),p[i].ulTLB_VPN , (p[i].ulTLB_DATA & ~(0xfff)), (p[i].ulTLB_DATA & (0xfff)));
		} else {
			MPU_TLB(TLB_ENTRY(xMPU_Region_VAbase[4+i]),p[i].ulTLB_VPN , (p[i].ulTLB_DATA & ~(0xfff)), (0x2));
		}
	}

#ifdef CONFIG_DEBUG
        /* DO NOT MODIFY THE FOLLOWING DEBUGGING CODE; KEEP THEM AS THEY ARE */
	if ( now_seg > -1 ) {
		/* _seg > -1 means the task under ovly section */
		/* if ( prv_seg == -1 ) the prv task is outside of ovly section, no need change mapped */
		if ( prv_seg > -1 ) {
        		_ovly_table[prv_seg].mapped = 0;
		}

        	_ovly_table[now_seg].mapped = 1;
        	_ovly_debug_event();
	}
#endif /* CONFIG_DEBUG */
}
/*-----------------------------------------------------------*/

BaseType_t MPU_xTaskGenericCreate( TaskFunction_t pvTaskCode, const char * const pcName, uint16_t usStackDepth, void *pvParameters, UBaseType_t uxPriority, TaskHandle_t *pxCreatedTask, StackType_t *puxStackBuffer, const MemoryRegion_t * const xRegions )
{
BaseType_t xReturn;
BaseType_t xRunningPrivileged = prvRaisePrivilege();

	xReturn = xTaskGenericCreate( pvTaskCode, pcName, usStackDepth, pvParameters, uxPriority, pxCreatedTask, puxStackBuffer, xRegions );
	portRESET_PRIVILEGE( xRunningPrivileged );
	return xReturn;
}
/*-----------------------------------------------------------*/

void MPU_vTaskAllocateMPURegions( TaskHandle_t xTask, const MemoryRegion_t * const xRegions )
{
BaseType_t xRunningPrivileged = prvRaisePrivilege();

	vTaskAllocateMPURegions( xTask, xRegions );
	portRESET_PRIVILEGE( xRunningPrivileged );
}
/*-----------------------------------------------------------*/

#if ( INCLUDE_vTaskDelete == 1 )
	void MPU_vTaskDelete( TaskHandle_t pxTaskToDelete )
	{
    BaseType_t xRunningPrivileged = prvRaisePrivilege();

		vTaskDelete( pxTaskToDelete );
        portRESET_PRIVILEGE( xRunningPrivileged );
	}
#endif
/*-----------------------------------------------------------*/

#if ( INCLUDE_vTaskDelayUntil == 1 )
	void MPU_vTaskDelayUntil( TickType_t * const pxPreviousWakeTime, TickType_t xTimeIncrement )
	{
    BaseType_t xRunningPrivileged = prvRaisePrivilege();

		vTaskDelayUntil( pxPreviousWakeTime, xTimeIncrement );
        portRESET_PRIVILEGE( xRunningPrivileged );
	}
#endif
/*-----------------------------------------------------------*/

#if ( INCLUDE_vTaskDelay == 1 )
	void MPU_vTaskDelay( TickType_t xTicksToDelay )
	{
    BaseType_t xRunningPrivileged = prvRaisePrivilege();

		vTaskDelay( xTicksToDelay );
        portRESET_PRIVILEGE( xRunningPrivileged );
	}
#endif
/*-----------------------------------------------------------*/

#if ( INCLUDE_uxTaskPriorityGet == 1 )
	UBaseType_t MPU_uxTaskPriorityGet( TaskHandle_t pxTask )
	{
	UBaseType_t uxReturn;
    BaseType_t xRunningPrivileged = prvRaisePrivilege();

		uxReturn = uxTaskPriorityGet( pxTask );
        portRESET_PRIVILEGE( xRunningPrivileged );
		return uxReturn;
	}
#endif
/*-----------------------------------------------------------*/

#if ( INCLUDE_vTaskPrioritySet == 1 )
	void MPU_vTaskPrioritySet( TaskHandle_t pxTask, UBaseType_t uxNewPriority )
	{
    BaseType_t xRunningPrivileged = prvRaisePrivilege();

		vTaskPrioritySet( pxTask, uxNewPriority );
        portRESET_PRIVILEGE( xRunningPrivileged );
	}
#endif
/*-----------------------------------------------------------*/

#if ( INCLUDE_eTaskGetState == 1 )
	eTaskState MPU_eTaskGetState( TaskHandle_t pxTask )
	{
    BaseType_t xRunningPrivileged = prvRaisePrivilege();
	eTaskState eReturn;

		eReturn = eTaskGetState( pxTask );
        portRESET_PRIVILEGE( xRunningPrivileged );
		return eReturn;
	}
#endif
/*-----------------------------------------------------------*/

#if ( INCLUDE_xTaskGetIdleTaskHandle == 1 )
	TaskHandle_t MPU_xTaskGetIdleTaskHandle( void )
	{
	TaskHandle_t xReturn;
    BaseType_t xRunningPrivileged = prvRaisePrivilege();

		xReturn = xTaskGetIdleTaskHandle();
        portRESET_PRIVILEGE( xRunningPrivileged );
		return eReturn;
	}
#endif
/*-----------------------------------------------------------*/

#if ( INCLUDE_vTaskSuspend == 1 )
	void MPU_vTaskSuspend( TaskHandle_t pxTaskToSuspend )
	{
    BaseType_t xRunningPrivileged = prvRaisePrivilege();

		vTaskSuspend( pxTaskToSuspend );
        portRESET_PRIVILEGE( xRunningPrivileged );
	}
#endif
/*-----------------------------------------------------------*/

#if ( INCLUDE_vTaskSuspend == 1 )
	void MPU_vTaskResume( TaskHandle_t pxTaskToResume )
	{
    BaseType_t xRunningPrivileged = prvRaisePrivilege();

		vTaskResume( pxTaskToResume );
        portRESET_PRIVILEGE( xRunningPrivileged );
	}
#endif
/*-----------------------------------------------------------*/

void MPU_vTaskSuspendAll( void )
{
BaseType_t xRunningPrivileged = prvRaisePrivilege();

	vTaskSuspendAll();
    portRESET_PRIVILEGE( xRunningPrivileged );
}
/*-----------------------------------------------------------*/

BaseType_t MPU_xTaskResumeAll( void )
{
BaseType_t xReturn;
BaseType_t xRunningPrivileged = prvRaisePrivilege();

	xReturn = xTaskResumeAll();
    portRESET_PRIVILEGE( xRunningPrivileged );
    return xReturn;
}
/*-----------------------------------------------------------*/

TickType_t MPU_xTaskGetTickCount( void )
{
TickType_t xReturn;
BaseType_t xRunningPrivileged = prvRaisePrivilege();

	xReturn = xTaskGetTickCount();
    portRESET_PRIVILEGE( xRunningPrivileged );
	return xReturn;
}
/*-----------------------------------------------------------*/

UBaseType_t MPU_uxTaskGetNumberOfTasks( void )
{
UBaseType_t uxReturn;
BaseType_t xRunningPrivileged = prvRaisePrivilege();

	uxReturn = uxTaskGetNumberOfTasks();
    portRESET_PRIVILEGE( xRunningPrivileged );
	return uxReturn;
}
/*-----------------------------------------------------------*/

#if ( ( configUSE_TRACE_FACILITY == 1 ) && ( configUSE_STATS_FORMATTING_FUNCTIONS == 1 ) )
	void MPU_vTaskList( char *pcWriteBuffer )
	{
	BaseType_t xRunningPrivileged = prvRaisePrivilege();

		vTaskList( pcWriteBuffer );
		portRESET_PRIVILEGE( xRunningPrivileged );
	}
#endif
/*-----------------------------------------------------------*/

#if ( configGENERATE_RUN_TIME_STATS == 1 )
	void MPU_vTaskGetRunTimeStats( char *pcWriteBuffer )
	{
    BaseType_t xRunningPrivileged = prvRaisePrivilege();

		vTaskGetRunTimeStats( pcWriteBuffer );
        portRESET_PRIVILEGE( xRunningPrivileged );
	}
#endif
/*-----------------------------------------------------------*/

#if ( configUSE_APPLICATION_TASK_TAG == 1 )
	void MPU_vTaskSetApplicationTaskTag( TaskHandle_t xTask, TaskHookFunction_t pxTagValue )
	{
    BaseType_t xRunningPrivileged = prvRaisePrivilege();

		vTaskSetApplicationTaskTag( xTask, pxTagValue );
        portRESET_PRIVILEGE( xRunningPrivileged );
	}
#endif
/*-----------------------------------------------------------*/

#if ( configUSE_APPLICATION_TASK_TAG == 1 )
	TaskHookFunction_t MPU_xTaskGetApplicationTaskTag( TaskHandle_t xTask )
	{
	TaskHookFunction_t xReturn;
    BaseType_t xRunningPrivileged = prvRaisePrivilege();

		xReturn = xTaskGetApplicationTaskTag( xTask );
        portRESET_PRIVILEGE( xRunningPrivileged );
		return xReturn;
	}
#endif
/*-----------------------------------------------------------*/

#if ( configUSE_APPLICATION_TASK_TAG == 1 )
	BaseType_t MPU_xTaskCallApplicationTaskHook( TaskHandle_t xTask, void *pvParameter )
	{
	BaseType_t xReturn;
    BaseType_t xRunningPrivileged = prvRaisePrivilege();

		xReturn = xTaskCallApplicationTaskHook( xTask, pvParameter );
        portRESET_PRIVILEGE( xRunningPrivileged );
		return xReturn;
	}
#endif
/*-----------------------------------------------------------*/

#if ( configUSE_TRACE_FACILITY == 1 )
	UBaseType_t MPU_uxTaskGetSystemState( TaskStatus_t *pxTaskStatusArray, UBaseType_t uxArraySize, uint32_t *pulTotalRunTime )
	{
	UBaseType_t uxReturn;
	BaseType_t xRunningPrivileged = prvRaisePrivilege();

		uxReturn = uxTaskGetSystemState( pxTaskStatusArray, uxArraySize, pulTotalRunTime );
		portRESET_PRIVILEGE( xRunningPrivileged );
		return uxReturn;
	}
#endif
/*-----------------------------------------------------------*/

#if ( INCLUDE_uxTaskGetStackHighWaterMark == 1 )
	UBaseType_t MPU_uxTaskGetStackHighWaterMark( TaskHandle_t xTask )
	{
	UBaseType_t uxReturn;
    BaseType_t xRunningPrivileged = prvRaisePrivilege();

		uxReturn = uxTaskGetStackHighWaterMark( xTask );
        portRESET_PRIVILEGE( xRunningPrivileged );
		return uxReturn;
	}
#endif
/*-----------------------------------------------------------*/

#if ( INCLUDE_xTaskGetCurrentTaskHandle == 1 )
	TaskHandle_t MPU_xTaskGetCurrentTaskHandle( void )
	{
	TaskHandle_t xReturn;
    BaseType_t xRunningPrivileged = prvRaisePrivilege();

		xReturn = xTaskGetCurrentTaskHandle();
        portRESET_PRIVILEGE( xRunningPrivileged );
		return xReturn;
	}
#endif
/*-----------------------------------------------------------*/

#if ( INCLUDE_xTaskGetSchedulerState == 1 )
	BaseType_t MPU_xTaskGetSchedulerState( void )
	{
	BaseType_t xReturn;
    BaseType_t xRunningPrivileged = prvRaisePrivilege();

		xReturn = xTaskGetSchedulerState();
        portRESET_PRIVILEGE( xRunningPrivileged );
		return xReturn;
	}
#endif
/*-----------------------------------------------------------*/

QueueHandle_t MPU_xQueueGenericCreate( UBaseType_t uxQueueLength, UBaseType_t uxItemSize, uint8_t ucQueueType )
{
QueueHandle_t xReturn;
BaseType_t xRunningPrivileged = prvRaisePrivilege();

	xReturn = xQueueGenericCreate( uxQueueLength, uxItemSize, ucQueueType );
	portRESET_PRIVILEGE( xRunningPrivileged );
	return xReturn;
}
/*-----------------------------------------------------------*/

BaseType_t MPU_xQueueGenericReset( QueueHandle_t pxQueue, BaseType_t xNewQueue )
{
BaseType_t xReturn;
BaseType_t xRunningPrivileged = prvRaisePrivilege();

	xReturn = xQueueGenericReset( pxQueue, xNewQueue );
	portRESET_PRIVILEGE( xRunningPrivileged );
	return xReturn;
}
/*-----------------------------------------------------------*/

BaseType_t MPU_xQueueGenericSend( QueueHandle_t xQueue, const void * const pvItemToQueue, TickType_t xTicksToWait, BaseType_t xCopyPosition )
{
BaseType_t xReturn;
BaseType_t xRunningPrivileged = prvRaisePrivilege();

	xReturn = xQueueGenericSend( xQueue, pvItemToQueue, xTicksToWait, xCopyPosition );
	portRESET_PRIVILEGE( xRunningPrivileged );
	return xReturn;
}
/*-----------------------------------------------------------*/

UBaseType_t MPU_uxQueueMessagesWaiting( const QueueHandle_t pxQueue )
{
BaseType_t xRunningPrivileged = prvRaisePrivilege();
UBaseType_t uxReturn;

	uxReturn = uxQueueMessagesWaiting( pxQueue );
	portRESET_PRIVILEGE( xRunningPrivileged );
	return uxReturn;
}
/*-----------------------------------------------------------*/

BaseType_t MPU_xQueueGenericReceive( QueueHandle_t pxQueue, void * const pvBuffer, TickType_t xTicksToWait, BaseType_t xJustPeeking )
{
BaseType_t xRunningPrivileged = prvRaisePrivilege();
BaseType_t xReturn;

	xReturn = xQueueGenericReceive( pxQueue, pvBuffer, xTicksToWait, xJustPeeking );
	portRESET_PRIVILEGE( xRunningPrivileged );
	return xReturn;
}
/*-----------------------------------------------------------*/

BaseType_t MPU_xQueuePeekFromISR( QueueHandle_t pxQueue, void * const pvBuffer )
{
BaseType_t xRunningPrivileged = prvRaisePrivilege();
BaseType_t xReturn;

	xReturn = xQueuePeekFromISR( pxQueue, pvBuffer );
	portRESET_PRIVILEGE( xRunningPrivileged );
	return xReturn;
}
/*-----------------------------------------------------------*/

#if ( configUSE_MUTEXES == 1 )
	QueueHandle_t MPU_xQueueCreateMutex( void )
	{
    QueueHandle_t xReturn;
	BaseType_t xRunningPrivileged = prvRaisePrivilege();

		xReturn = xQueueCreateMutex( queueQUEUE_TYPE_MUTEX );
		portRESET_PRIVILEGE( xRunningPrivileged );
		return xReturn;
	}
#endif
/*-----------------------------------------------------------*/

#if configUSE_COUNTING_SEMAPHORES == 1
	QueueHandle_t MPU_xQueueCreateCountingSemaphore( UBaseType_t uxCountValue, UBaseType_t uxInitialCount )
	{
    QueueHandle_t xReturn;
	BaseType_t xRunningPrivileged = prvRaisePrivilege();

		xReturn = xQueueCreateCountingSemaphore( uxCountValue, uxInitialCount );
		portRESET_PRIVILEGE( xRunningPrivileged );
		return xReturn;
	}
#endif
/*-----------------------------------------------------------*/

#if ( configUSE_RECURSIVE_MUTEXES == 1 )
	BaseType_t MPU_xQueueTakeMutexRecursive( QueueHandle_t xMutex, TickType_t xBlockTime )
	{
	BaseType_t xReturn;
	BaseType_t xRunningPrivileged = prvRaisePrivilege();

		xReturn = xQueueTakeMutexRecursive( xMutex, xBlockTime );
		portRESET_PRIVILEGE( xRunningPrivileged );
		return xReturn;
	}
#endif
/*-----------------------------------------------------------*/

#if ( configUSE_RECURSIVE_MUTEXES == 1 )
	BaseType_t MPU_xQueueGiveMutexRecursive( QueueHandle_t xMutex )
	{
	BaseType_t xReturn;
	BaseType_t xRunningPrivileged = prvRaisePrivilege();

		xReturn = xQueueGiveMutexRecursive( xMutex );
		portRESET_PRIVILEGE( xRunningPrivileged );
		return xReturn;
	}
#endif
/*-----------------------------------------------------------*/

#if ( configUSE_QUEUE_SETS == 1 )
	QueueSetHandle_t MPU_xQueueCreateSet( UBaseType_t uxEventQueueLength )
	{
	QueueSetHandle_t xReturn;
	BaseType_t xRunningPrivileged = prvRaisePrivilege();

		xReturn = xQueueCreateSet( uxEventQueueLength );
		portRESET_PRIVILEGE( xRunningPrivileged );
		return xReturn;
	}
#endif
/*-----------------------------------------------------------*/

#if ( configUSE_QUEUE_SETS == 1 )
	QueueSetMemberHandle_t MPU_xQueueSelectFromSet( QueueSetHandle_t xQueueSet, TickType_t xBlockTimeTicks )
	{
	QueueSetMemberHandle_t xReturn;
	BaseType_t xRunningPrivileged = prvRaisePrivilege();

		xReturn = xQueueSelectFromSet( xQueueSet, xBlockTimeTicks );
		portRESET_PRIVILEGE( xRunningPrivileged );
		return xReturn;
	}
#endif
/*-----------------------------------------------------------*/

#if ( configUSE_QUEUE_SETS == 1 )
	BaseType_t MPU_xQueueAddToSet( QueueSetMemberHandle_t xQueueOrSemaphore, QueueSetHandle_t xQueueSet )
	{
	BaseType_t xReturn;
	BaseType_t xRunningPrivileged = prvRaisePrivilege();

		xReturn = xQueueAddToSet( xQueueOrSemaphore, xQueueSet );
		portRESET_PRIVILEGE( xRunningPrivileged );
		return xReturn;
	}
#endif
/*-----------------------------------------------------------*/

#if ( configUSE_QUEUE_SETS == 1 )
	BaseType_t MPU_xQueueRemoveFromSet( QueueSetMemberHandle_t xQueueOrSemaphore, QueueSetHandle_t xQueueSet )
	{
	BaseType_t xReturn;
	BaseType_t xRunningPrivileged = prvRaisePrivilege();

		xReturn = xQueueRemoveFromSet( xQueueOrSemaphore, xQueueSet );
		portRESET_PRIVILEGE( xRunningPrivileged );
		return xReturn;
	}
#endif
/*-----------------------------------------------------------*/

#if configUSE_ALTERNATIVE_API == 1
	BaseType_t MPU_xQueueAltGenericSend( QueueHandle_t pxQueue, const void * const pvItemToQueue, TickType_t xTicksToWait, BaseType_t xCopyPosition )
	{
   	BaseType_t xReturn;
	BaseType_t xRunningPrivileged = prvRaisePrivilege();

		xReturn = 	BaseType_t xQueueAltGenericSend( pxQueue, pvItemToQueue, xTicksToWait, xCopyPosition );
		portRESET_PRIVILEGE( xRunningPrivileged );
		return xReturn;
	}
#endif
/*-----------------------------------------------------------*/

#if configUSE_ALTERNATIVE_API == 1
	BaseType_t MPU_xQueueAltGenericReceive( QueueHandle_t pxQueue, void * const pvBuffer, TickType_t xTicksToWait, BaseType_t xJustPeeking )
	{
    BaseType_t xReturn;
	BaseType_t xRunningPrivileged = prvRaisePrivilege();

		xReturn = xQueueAltGenericReceive( pxQueue, pvBuffer, xTicksToWait, xJustPeeking );
		portRESET_PRIVILEGE( xRunningPrivileged );
		return xReturn;
	}
#endif
/*-----------------------------------------------------------*/

#if configQUEUE_REGISTRY_SIZE > 0
	void MPU_vQueueAddToRegistry( QueueHandle_t xQueue, char *pcName )
	{
	BaseType_t xRunningPrivileged = prvRaisePrivilege();

		vQueueAddToRegistry( xQueue, pcName );

		portRESET_PRIVILEGE( xRunningPrivileged );
	}
#endif
/*-----------------------------------------------------------*/

void MPU_vQueueDelete( QueueHandle_t xQueue )
{
BaseType_t xRunningPrivileged = prvRaisePrivilege();

	vQueueDelete( xQueue );

	portRESET_PRIVILEGE( xRunningPrivileged );
}
/*-----------------------------------------------------------*/

void *MPU_pvPortMalloc( size_t xSize )
{
void *pvReturn;
BaseType_t xRunningPrivileged = prvRaisePrivilege();

	pvReturn = pvPortMalloc( xSize );

	portRESET_PRIVILEGE( xRunningPrivileged );

	return pvReturn;
}
/*-----------------------------------------------------------*/

void MPU_vPortFree( void *pv )
{
BaseType_t xRunningPrivileged = prvRaisePrivilege();

	vPortFree( pv );

	portRESET_PRIVILEGE( xRunningPrivileged );
}
/*-----------------------------------------------------------*/

void MPU_vPortInitialiseBlocks( void )
{
BaseType_t xRunningPrivileged = prvRaisePrivilege();

	vPortInitialiseBlocks();

	portRESET_PRIVILEGE( xRunningPrivileged );
}
/*-----------------------------------------------------------*/

size_t MPU_xPortGetFreeHeapSize( void )
{
size_t xReturn;
BaseType_t xRunningPrivileged = prvRaisePrivilege();

	xReturn = xPortGetFreeHeapSize();

	portRESET_PRIVILEGE( xRunningPrivileged );

	return xReturn;
}

/* Functions that the application writer wants to execute in privileged mode
can be defined in application_defined_privileged_functions.h.  The functions
must take the same format as those above whereby the privilege state on exit
equals the privilege state on entry.  For example:

void MPU_FunctionName( [parameters ] )
{
BaseType_t xRunningPrivileged = prvRaisePrivilege();

	FunctionName( [parameters ] );

	portRESET_PRIVILEGE( xRunningPrivileged );
}
*/

#if configINCLUDE_APPLICATION_DEFINED_PRIVILEGED_FUNCTIONS == 1
	#include "application_defined_privileged_functions.h"
#endif

#endif
