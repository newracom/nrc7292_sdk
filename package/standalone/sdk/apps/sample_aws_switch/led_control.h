#ifndef __LED_CONTROL_H__
#define __LED_CONTROL_H__

#ifdef NRC7292
/* NRC7292 EVK : Green on GPIO 8 and Red on GPIO 9 */
#define GREEN_LED	GPIO_08
#define RED_LED		GPIO_09
#else
/* NRC7394 EVK : Geen on GPIO 10 and RED on GPIO 11 */
#define GREEN_LED	GPIO_10
#define RED_LED		GPIO_11
#endif

void led_init();
int led_status(int led);
void led_onoff(int led, int onoff);
#endif /* __LED_CONTROL_H__ */
