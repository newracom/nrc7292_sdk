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
//#include <stdlib.h>
//#include <stdio.h>


/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "nds32_intrinsic.h"


/* Processor constants. */
//#include "os_cpu.h"


/* Platform definition includes */
//#include "bsp_hal.h"

/* Constants required to setup the task context. */
#define portNO_CRITICAL_SECTION_NESTING	( ( portSTACK_TYPE ) 0 )

/*-----------------------------------------------------------*/

extern void vPortISRStartFirstTask( void );


#ifdef MPW

unsigned long ulPortSetInterruptMask( void )
{
	/* By default, the kerenl has the highest priority */
	/* It's mean that we don't support kernel preemptive */
	unsigned long psw = __nds32__mfsr(NDS32_SR_PSW);
	__nds32__gie_dis();
	return psw;
}

void vPortClearInterruptMask( unsigned long msk )
{
	if (msk & 1)
		__nds32__gie_en();
}


void vPortYield()
{
	/* trigger swi*/
	__nds32__mtsr(0x10000, NDS32_SR_INT_PEND);
	__nds32__isb();
}


#define PSW_offGIE		0	/* Global Interrupt Enable */
#define PSW_offCPL		16	/* Current Priority Level */
#define PSW_offIFCON		15	/* Hardware Single Stepping */


#define PSW_mskGIE		( 0x1 << PSW_offGIE )
#define PSW_mskCPL		( 0x7 << PSW_offCPL )
#define PSW_mskIFCON		( 0x1 << PSW_offIFCON )


portSTACK_TYPE *pxPortInitialiseStack_rom( portSTACK_TYPE *pxTopOfStack, pdTASK_CODE pxCode, void *pvParameters,  unsigned int *_SDA_BASE_)
{
	int i;

	/* Simulate the stack frame as it would be created by a context switch */
	/* R0 ~ R5 registers */
	for (i = 5; i >= 1; i--)                                /* R5, R4, R3, R2 and R1. */
		*--pxTopOfStack = (portSTACK_TYPE) 0x01010101UL * i;
	*--pxTopOfStack = (portSTACK_TYPE) pvParameters;        /* R0 : Argument */

	/* R6 ~ R30 registers */
	*--pxTopOfStack = (portSTACK_TYPE) vPortEndScheduler;   /* R30: $LP */
	*--pxTopOfStack = (portSTACK_TYPE) _SDA_BASE_;         /* R29: $GP */
	*--pxTopOfStack = (portSTACK_TYPE) 0x2828282828;        /* R28: $FP */

	for (i = 27; i >= 6; i--)                               /* R27 ~ R6 */
		*--pxTopOfStack = (portSTACK_TYPE) 0x01010101UL * i;

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
	i = (__nds32__mfsr(NDS32_SR_PSW) | PSW_mskGIE | PSW_mskCPL) & ~PSW_mskIFCON;	/* Default IPSW: enable GIE, set CPL to 7, clear IFCON */
	*--pxTopOfStack = (portSTACK_TYPE) i;                   /* $IPSW */
	*--pxTopOfStack = (portSTACK_TYPE) pxCode;              /* $IPC */

	/* Dummy word for 8-byte stack alignment */
#if defined(__TARGET_IFC_EXT) && defined(__TARGET_ZOL_EXT)
	*--pxTopOfStack = (portSTACK_TYPE) 0xFFFFFFFF;          /* Dummy */
#endif

	return pxTopOfStack;
}


#else

//extern unsigned long ulPortSetInterruptMask( void );
//extern void vPortClearInterruptMask( unsigned long msk );
portSTACK_TYPE *pxPortInitialiseStack_rom( portSTACK_TYPE *pxTopOfStack, pdTASK_CODE pxCode, void *pvParameters,  unsigned int *_SDA_BASE_);


#define PSW_offGIE		0	/* Global Interrupt Enable */
#define PSW_offCPL		16	/* Current Priority Level */
#define PSW_offIFCON		15	/* Hardware Single Stepping */


#define PSW_mskGIE		( 0x1 << PSW_offGIE )
#define PSW_mskCPL		( 0x7 << PSW_offCPL )
#define PSW_mskIFCON		( 0x1 << PSW_offIFCON )


portSTACK_TYPE *pxPortInitialiseStack_rom( portSTACK_TYPE *pxTopOfStack, pdTASK_CODE pxCode, void *pvParameters,  unsigned int *_SDA_BASE_)
{
	int i;

	/* Simulate the stack frame as it would be created by a context switch */
	/* R0 ~ R5 registers */
	for (i = 5; i >= 1; i--)                                /* R5, R4, R3, R2 and R1. */
		*--pxTopOfStack = (portSTACK_TYPE) 0x01010101UL * i;
	*--pxTopOfStack = (portSTACK_TYPE) pvParameters;        /* R0 : Argument */

	/* R6 ~ R30 registers */
	*--pxTopOfStack = (portSTACK_TYPE) vPortEndScheduler;   /* R30: $LP */
	*--pxTopOfStack = (portSTACK_TYPE) _SDA_BASE_;         /* R29: $GP */
	*--pxTopOfStack = (portSTACK_TYPE) 0x2828282828;        /* R28: $FP */

	for (i = 27; i >= 6; i--)                               /* R27 ~ R6 */
		*--pxTopOfStack = (portSTACK_TYPE) 0x01010101UL * i;

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
	i = (__nds32__mfsr(NDS32_SR_PSW) | PSW_mskGIE | PSW_mskCPL) & ~PSW_mskIFCON;	/* Default IPSW: enable GIE, set CPL to 7, clear IFCON */
	*--pxTopOfStack = (portSTACK_TYPE) i;                   /* $IPSW */
	*--pxTopOfStack = (portSTACK_TYPE) pxCode;              /* $IPC */

	/* Dummy word for 8-byte stack alignment */
#if defined(__TARGET_IFC_EXT) && defined(__TARGET_ZOL_EXT)
11
	*--pxTopOfStack = (portSTACK_TYPE) 0xFFFFFFFF;          /* Dummy */
#endif

	return pxTopOfStack;
}


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
extern unsigned int _SDA_BASE_ __attribute__ ((weak));
portSTACK_TYPE *pxPortInitialiseStack( portSTACK_TYPE *pxTopOfStack, pdTASK_CODE pxCode, void *pvParameters )
{
	return pxPortInitialiseStack_rom(pxTopOfStack, pxCode, pvParameters,  &_SDA_BASE_);
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
	//printf("Current Task will be deleted\n");

	/*
 	 * FreeRTOS vTaskDelete() just remove TCB from list.
 	 * vTaskDelete() would call vPortYiled change to Idle task which would do garbage collection.
 	 * So called vTaskDelete() or return from task, make sure GIE is turn-on if context switch by SWI.
	 */
	vTaskDelete(NULL);
}
//#include "hal.h"
#define portNO_CRITICAL_NESTING		( ( unsigned long ) 0 )
volatile unsigned long ulCriticalNesting = 0UL;
static unsigned long psw_0 = 0;
unsigned long psw_1 = 0;
void vPortEnterCritical()
{
       /*
        *       in order to avoid race condition
        *       1.store psw into stack
        *       2.disable gie
        *       3.store psw into global if ulCriticalNesting==0
        *       4.ulCriticalNesting++ 
        */
       volatile unsigned int psw = __nds32__mfsr(NDS32_SR_PSW);
	__nds32__gie_dis();
       if (ulCriticalNesting == portNO_CRITICAL_NESTING )
               psw_0 = psw;
       ulCriticalNesting++;
}

void vPortExitCritical()
{
       --ulCriticalNesting;
       if ( ulCriticalNesting == portNO_CRITICAL_NESTING )
       {
	   	if (psw_0 & 1)
			__nds32__gie_en();
       }
}


/*-----------------------------------------------------------*/

#if configUSE_TICKLESS_IDLE == 1
#define TICK_HZ 100
#define ulTimerCountsForOneTick 		(MB_PCLK / TICK_HZ)
#define xMaximumPossibleSuppressedTicks 	(10*TICK_HZ) /* 30(sec)*TICK_HZ (Ticks!!)*/
#define ulStoppedTimerCompensation		10	/* FIXME add this miss config */	


__attribute__((weak)) void vPortSuppressTicksAndSleep( TickType_t xExpectedIdleTime )
{
}


#endif /* #if configUSE_TICKLESS_IDLE */
/*-----------------------------------------------------------*/
