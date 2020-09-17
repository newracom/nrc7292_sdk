#ifndef	__SYSTEM_FREERTOS_H__
#define __SYSTEM_FREERTOS_H__

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#if defined(CONFIG_OS_FREERTOS)
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "queue.h"
#include "timers.h"

#define SYS_TASK_QUEUE_LENGTH		128
#if defined(LMAC_TEST)
    #define SYS_TASK_STACK_SIZE         (2048 / sizeof(StackType_t))
	#define SYSTEM_LMAC_TASK_STACK_SIZE (2048 / sizeof(StackType_t))
	#define SYSTEM_LMAC_TASK_PRIORITY   (31)
#elif defined(NRC7292_STANDALONE_XIP)
    #define SYS_TASK_STACK_SIZE         ((2 * 4096) / sizeof(StackType_t))
#else
    #define SYS_TASK_STACK_SIZE         (4096 / sizeof(StackType_t))
#endif /* defined(LMAC_TEST) */

#define BACKGROUND_TASK_STACK_SIZE		(1024 / sizeof(StackType_t))
#define BACKGROUND_TASK_PRIORITY	 	(1)

#if defined(LMAC_CONFIG_FREERTOS)
#if !defined(SYSTEM_LMAC_TASK_STACK_SIZE)
#define LMAC_TASK_STACK_SIZE	    (4096 / sizeof(StackType_t))
#define LMAC_TASK_PRIORITY		    (31)
#else
#define LMAC_TASK_STACK_SIZE	    SYSTEM_LMAC_TASK_STACK_SIZE
#define LMAC_TASK_PRIORITY		    SYSTEM_LMAC_TASK_PRIORITY
#endif /* !defined(SYSTEM_LMAC_TASK_STACK_SIZE) */
#endif /* defined(LMAC_CONFIG_FREERTOS) */

#define SYS_TASK_PRIORITY           (configMAX_PRIORITIES - 5)

extern uint32_t ulSetInterruptMaskFromISR( void ) __attribute__((naked));
extern void vClearInterruptMaskFromISR( uint32_t ulMask )  __attribute__((naked));

__attribute__((always_inline)) static inline unsigned long system_irq_save(void) {
	return portSET_INTERRUPT_MASK_FROM_ISR();
	//vPortEnterCritical();
	//return 0;
}
__attribute__((always_inline)) static inline void system_irq_restore(unsigned long flags){
	portCLEAR_INTERRUPT_MASK_FROM_ISR(flags);
	//vPortExitCritical();
}
__attribute__((always_inline)) static inline void system_irq_enable(void){	portENABLE_INTERRUPTS();	}
__attribute__((always_inline)) static inline void system_irq_disable(void){	portDISABLE_INTERRUPTS();}
#else
__attribute__((always_inline)) static inline unsigned long system_irq_save(void) {
	unsigned long flags;
	asm volatile(
	    "	mrs	%0, primask	@ local_irq_save\n"
	    "	cpsid	i"
	    : "=r"(flags) : : "memory", "cc");
	return flags;
}
__attribute__((always_inline)) static inline void system_irq_restore(unsigned long flags){
	asm volatile(
	    "	msr	primask, %0	@ local_irq_restore"
	    :
	    : "r"(flags)
	    : "memory", "cc");
}
#endif /* defined(CONFIG_OS_FREERTOS) */
#endif /* __SYSTEM_FREERTOS_H__ */
