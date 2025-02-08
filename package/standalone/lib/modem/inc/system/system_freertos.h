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

#define SYS_TASK_QUEUE_LENGTH		256
#if defined(LMAC_TEST)
#define SYS_TASK_STACK_SIZE         (2048 / sizeof(StackType_t))
#define SYSTEM_LMAC_TASK_STACK_SIZE (2048 / sizeof(StackType_t))
#define SYSTEM_LMAC_TASK_PRIORITY   (31)
#elif defined(INCLUDE_STANDALONE)
#define SYS_TASK_STACK_SIZE         ((2 * 4096) / sizeof(StackType_t))
#else
#define SYS_TASK_STACK_SIZE         (2*4096 / sizeof(StackType_t))
#endif /* defined(LMAC_TEST) */

#if defined(NRC7292) || defined(NRC7394)
#define BACKGROUND_TASK_STACK_SIZE		(4096 / sizeof(StackType_t))
#else
#define BACKGROUND_TASK_STACK_SIZE		(1024 / sizeof(StackType_t))
#endif
#define BACKGROUND_TASK_PRIORITY	 	(0)

#if defined(LMAC_CONFIG_FREERTOS)
#if !defined(SYSTEM_LMAC_TASK_STACK_SIZE)

#if defined(INCLUDE_STANDALONE)
#define LMAC_TASK_STACK_SIZE	    ((2 * 4096) / sizeof(StackType_t))
#else //#if defined(INCLUDE_STANDALONE)
#define LMAC_TASK_STACK_SIZE	    (4096 / sizeof(StackType_t))
#endif //#if defined(INCLUDE_STANDALONE)

#if defined (INCLUDE_NEW_TASK_ARCH)
//#define LMAC_TASK_PRIORITY		    (30)
#define LMAC_TASK_PRIORITY		    (configMAX_PRIORITIES - 3) // PRI_29
#else // #if defined (INCLUDE_NEW_TASK_ARCH)
#define LMAC_TASK_PRIORITY		    (31) //PRI_31
#endif //#if defined (INCLUDE_NEW_TASK_ARCH)

#else //#if !defined(SYSTEM_LMAC_TASK_STACK_SIZE)

#define LMAC_TASK_STACK_SIZE	    SYSTEM_LMAC_TASK_STACK_SIZE
#define LMAC_TASK_PRIORITY		    SYSTEM_LMAC_TASK_PRIORITY
#endif /* !defined(SYSTEM_LMAC_TASK_STACK_SIZE) */
#endif /* defined(LMAC_CONFIG_FREERTOS) */

#if defined (INCLUDE_NEW_TASK_ARCH)
#define SYS_TASK_PRIORITY			(configMAX_PRIORITIES - 3) //PRI_29 
#else
#define SYS_TASK_PRIORITY           (configMAX_PRIORITIES - 5) //PRI_27
#endif //#if defined (INCLUDE_NEW_TASK_ARCH)

#if defined (INCLUDE_NEW_TASK_ARCH)
#define FAST_TASK_QUEUE_LENGTH			10
#if defined(INCLUDE_STANDALONE)
#define FAST_TASK_STACK_SIZE			((2 * 4096) / sizeof(StackType_t))
#else
#define FAST_TASK_STACK_SIZE			(2048 / sizeof(StackType_t))
#endif
#define FAST_TASK_PRIORITY				(configMAX_PRIORITIES - 2) //PRI_30
#endif //#if defined (INCLUDE_NEW_TASK_ARCH)

#define USER_APP_TASK_STACK_SIZE	(1024*13)/ sizeof(StackType_t)
#define USER_APP_TASK_PRIORITY		2

__attribute__((always_inline)) static inline void system_irq_enable(void){	__asm volatile  ( " cpsie i " ::: "memory" );}
__attribute__((always_inline)) static inline void system_irq_disable(void){	__asm volatile  ( " cpsid i " ::: "memory" );}
#endif /* defined(CONFIG_OS_FREERTOS) */
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
#endif /* __SYSTEM_FREERTOS_H__ */
