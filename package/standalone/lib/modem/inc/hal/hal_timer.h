#ifndef __HAL_TIMER_H__
#define __HAL_TIMER_H__

#include "system_common.h"

typedef void (*functype)(void);

/* Timer HAL operations : HW dependent*/
struct tmr_ops{
	char		*tmr_name;
	uint8_t		tmr_count;
	uint32_t	(*tmr_get_resolution)(int ch);
	bool		(*tmr_is_ch_valid)(int ch);
	bool 		(*tmr_init)(int ch, int prescale);
	void 		(*tmr_set_interval)(int ch, uint32_t interval);
	uint32_t	(*tmr_get_interval)(int ch);
	void		(*tmr_set_stopmode)(int ch, bool enable);
	void		(*tmr_set_enable)(int ch, bool enable);
	bool		(*tmr_get_enable)(int ch);
	void		(*tmr_get_tick)(int ch, uint32_t *tick);
	uint32_t	(*tmr_get_irq)(int ch);
	void		(*tmr_clr_irq)(int ch);
	bool 		(*tmr_init_64)(int ch, int prescale);
#if defined(NRC_ROMLIB) && defined(NRC7392)
	void 		(*tmr_set_interval_64)(int ch, UINT64 interval);
#else
	void 		(*tmr_set_interval_64)(int ch, uint64_t interval);
#endif /* defined(NRC_ROMLIB) && defined(NRC7392) */
	uint64_t	(*tmr_get_interval_64)(int ch);
	void		(*tmr_set_stopmode_64)(int ch, bool enable);
	void		(*tmr_set_enable_64)(int ch, bool enable);
	bool		(*tmr_get_enable_64)(int ch);
	void		(*tmr_get_tick_64)(int ch, uint64_t *tick);
	uint32_t	(*tmr_get_irq_64)(int ch);
	void		(*tmr_clr_irq_64)(int ch);
} ;

/* Timer HAL APIs */
// Register HAL ops
void hal_timer_register(struct tmr_ops *ops);
//Get Timer Name
char* hal_timer_get_name(void);
//Get Total CH number
uint8_t hal_timer_get_total_ch();
// Get Timer Resolution (32bit or 64bit)
int hal_timer_get_resolution(int ch);
// Check CH is valid
bool hal_timer_is_valid(int ch);
// Clear IRQ
int hal_timer_clear_irq(int ch);
// Initialize timer wih params (ch, callback)
int hal_timer_init(int ch, isr_callback_t isr_cb);
// Start Timer
int hal_timer_start(int ch);
// Stop Timer
int hal_timer_stop(int ch);
// Get Timer Status
bool hal_timer_get_enable(int ch);
// Set Timer Interval
int hal_timer_set_interval(int ch, uint64_t intv);
// Get Timer Interval
uint64_t hal_timer_get_interval(int ch);
// Get Timer Tick Count
uint64_t hal_timer_get_tick(int ch);

#endif //__HAL_TIMER_H__
