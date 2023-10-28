#include "nrc_sdk.h"
#include "led_control.h"

void led_init(void)
{
	NRC_GPIO_CONFIG gpio_conf;

	/* GPIO for On-board LED's */
	gpio_conf.gpio_pin = GREEN_LED;
	gpio_conf.gpio_dir = GPIO_OUTPUT;
	gpio_conf.gpio_mode = GPIO_PULL_DOWN;
	gpio_conf.gpio_alt= GPIO_FUNC;
	nrc_gpio_config(&gpio_conf);

	gpio_conf.gpio_pin = RED_LED;
	gpio_conf.gpio_dir = GPIO_OUTPUT;
	gpio_conf.gpio_mode = GPIO_PULL_DOWN;
	gpio_conf.gpio_alt= GPIO_FUNC;
	nrc_gpio_config(&gpio_conf);
}

int led_status(int led)
{
	int state = 0;

	nrc_gpio_inputb(led, &state);

	return state;
}

void led_onoff(int led, int onoff)
{
	int state = led_status(led);

	if (state == onoff) {
		return;
	}

	nrc_gpio_outputb(led, onoff);
}
