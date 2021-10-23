#ifndef __HAL_WDT_H__
#define __HAL_WDT_H__

bool hal_wdt_enable(void *wdt_isr, uint32_t load_ms, bool reset);
void hal_wdt_disable();
void hal_wdt_reload(uint32_t load_ms);
void hal_wdt_int_clear();

#endif //__HAL_WDT_H__

