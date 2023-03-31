
#ifndef __HAL_LED_H__
#define __HAL_LED_H__

enum HAL_LED_ID
{
	HAL_LED_NONE = -1,

	HAL_LED_GREEN = 0,
	HAL_LED_RED,

	HAL_LED_ID_MAX
};

enum HAL_LED_STATUS
{
	HAL_LED_OFF = 0,
	HAL_LED_ON
};

extern int hal_led_init (int id, const char *name, int gpio, bool invert);
extern int hal_led_deinit (int id);

extern const char *hal_led_name (int id);
extern int hal_led_gpio (int id);
extern int hal_led_status (int id);

extern int hal_led_on(int id);
extern int hal_led_off(int id);

extern int hal_led_trx_init (int tx_gpio, int rx_gpio, int timer_period, bool invert);
extern int hal_led_trx_deinit (void);

#define hal_led_info(fmt, ...)		I(TT_LED, fmt, ##__VA_ARGS__)
#define hal_led_error(fmt, ...)		E(TT_LED, fmt, ##__VA_ARGS__)
#if 0
#define hal_led_debug(fmt, ...)		A(fmt, ##__VA_ARGS__)
#else
#define hal_led_debug(fmt, ...)
#endif

#endif /* #ifndef __HAL_LED_H__ */

