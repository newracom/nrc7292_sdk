#ifndef __UTIL_TRACE_STACK_H__
#define __UTIL_TRACE_STACK_H__

#include "system.h"
#include "system_common.h"

int util_trace_stack_add_task( TaskHandle_t handle );
int util_trace_stack_display(void);

#endif    /* __UTIL_TRACE_STACK_H__ */
