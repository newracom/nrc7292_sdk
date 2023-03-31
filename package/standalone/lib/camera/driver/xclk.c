#include "nrc_sdk.h"
#include "xclk.h"
#include "nrc_camera.h"

nrc_err_t xclk_timer_conf(camera_config_t *config)
{
	system_printf("[%s] Setting PWM...\n", __func__);
	nrc_err_t err = nrc_pwm_hw_init(PWM_CH0, config->pin_xclk, 1);
	uint32_t reg = 0;
	if (err != NRC_SUCCESS) {
		system_printf("nrc_pwm_hw_init failed.\n");
		goto err;
	}

	nrc_pwm_config(PWM_CH0, PWM_DIV2, PWM_MODE_REG_OUT, 0);

	err = nrc_pwm_set_config(PWM_CH0, 0xAAAAAAAA, 0xAAAAAAAA, 0xAAAAAAAA, 0xAAAAAAAA);
	if (err != NRC_SUCCESS) {
		system_printf("nrc_pwm_set_config failed.\n");
		goto err;
	}

	system_printf("[%s] Setting PWM clock control register...\n", __func__);
	/* PCLKCFG08 (PWM clock control register) clock divider to 1*/
	if (config->xclk_freq_hz == 1000000) {
		*(volatile uint32_t *) (0x40001040) = 0x81000001;
	} else if (config->xclk_freq_hz == 2000000) {
		*(volatile uint32_t *) (0x40001040) = 0x81000000;
	} else if (config->xclk_freq_hz == 4000000) {
		*(volatile uint32_t *) (0x40001040) = 0x84000001;
	} else if (config->xclk_freq_hz == 6000000) {
		*(volatile uint32_t *) (0x40001040) = 0x83000001;
	} else if (config->xclk_freq_hz == 8000000) {
		*(volatile uint32_t *) (0x40001040) = 0x84000000;
	} else {
		err = NRC_FAIL;
	}

err:
	return err;
}

nrc_err_t camera_enable_out_clock()
{
	return nrc_pwm_set_enable(PWM_CH0, true);
}

void camera_disable_out_clock()
{
	nrc_pwm_set_enable(PWM_CH0, false);
}
