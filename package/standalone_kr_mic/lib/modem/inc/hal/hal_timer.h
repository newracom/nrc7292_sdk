#ifndef __HAL_TIMER_H__
#define __HAL_TIMER_H__

#include "system_common.h"

void hal_timer_init(bool (*fn)(void));
void hal_timer_set(uint64_t target);
void hal_timer_stop();
uint64_t hal_timer_get();


#endif //__HAL_TIMER_H__
