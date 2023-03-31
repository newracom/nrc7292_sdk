// Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include "nrc_sdk.h"
#include "sensor.h"
#include "sccb.h"

#include "nrc_camera.h"
#include "xclk.h"
#if CONFIG_OV2640_SUPPORT
#include "ov2640.h"
#endif
#if CONFIG_OV5640_SUPPORT
#include "ov5640.h"
#endif

#define nrc_camera_log(fmt, ...)    A("%s: " fmt, __func__, ##__VA_ARGS__)

#define GPIO_DATA_READ                (*(volatile uint32_t *) GPIO_BASE_ADDR)
#define CAMERA_PIXEL_DATA             ((GPIO_DATA_READ >> 8) & 0xFF)

typedef struct {
	sensor_t sensor;
} camera_state_t;

static camera_state_t *s_state = NULL;

typedef struct {
	int (*detect)(int slv_addr, sensor_id_t *id);
	int (*init)(sensor_t *sensor);
} sensor_func_t;

static const sensor_func_t g_sensors[] = {
#if CONFIG_OV7725_SUPPORT
	{ov7725_detect, ov7725_init},
#endif
#if CONFIG_OV7670_SUPPORT
	{ov7670_detect, ov7670_init},
#endif
#if CONFIG_OV2640_SUPPORT
	{ov2640_detect, ov2640_init},
#endif
#if CONFIG_OV3660_SUPPORT
	{ov3660_detect, ov3660_init},
#endif
#if CONFIG_OV5640_SUPPORT
	{ov5640_detect, ov5640_init},
#endif
#if CONFIG_NT99141_SUPPORT
	{nt99141_detect, nt99141_init},
#endif
#if CONFIG_GC2145_SUPPORT
	{gc2145_detect, gc2145_init},
#endif
#if CONFIG_GC032A_SUPPORT
	{gc032a_detect, gc032a_init},
#endif
#if CONFIG_GC0308_SUPPORT
	{gc0308_detect, gc0308_init},
#endif
#if CONFIG_BF3005_SUPPORT
	{bf3005_detect, bf3005_init},
#endif
#if CONFIG_BF20A6_SUPPORT
	{bf20a6_detect, bf20a6_init},
#endif
#if CONFIG_SC101IOT_SUPPORT
	{sc101iot_detect, sc101iot_init},
#endif
#if CONFIG_SC030IOT_SUPPORT
	{sc030iot_detect, sc030iot_init},
#endif
#if CONFIG_SC031GS_SUPPORT
	{sc031gs_detect, sc031gs_init},
#endif
};

static int framesize_changed = 0;

static int __attribute__((optimize("O3"))) _nrc_camera_read_data (camera_config_t *config, char *buf, int len)
{
	int cnt = 0;
	unsigned long flags;

	/* Precalculate signal checking constants */
	const uint32_t vsync_check = 1 << config->pin_vsync;
	const uint32_t pclk_check = 1 << config->pin_pclk;
	const uint32_t href_pclk_check = (1 << config->pin_href) | pclk_check;
	uint32_t gpio_value = 0;

	if (!buf || !len)
		return -1;

	memset(buf, 0, len);

	system_modem_api_set_rf_power(false);
	flags = system_irq_save();

	while (GPIO_DATA_READ & vsync_check);
	while (!(GPIO_DATA_READ &vsync_check));

	do {
		gpio_value = GPIO_DATA_READ;
		/* do it only pclk & href are high */
		if ((gpio_value & href_pclk_check) == href_pclk_check) {
			buf[cnt] = (uint8_t) (gpio_value >> config->pin_d0);
			++cnt;
			/* Wait until PCLK goes low */
			while (GPIO_DATA_READ & pclk_check);

			/* Check if buffer will overflow */
			/* Checking will be hopefully done while the pclk is low */
			if (cnt >= len) {
				nrc_camera_log("!!! buffer overflow - %d!!!\n", cnt);
				cnt = 0;
				goto buffer_overflow;
			}
		}
	} while (gpio_value & vsync_check);

buffer_overflow:

	system_irq_restore(flags);
	system_modem_api_set_rf_power(true);

	nrc_camera_log("JPEG image size %d received...\n", cnt);
	print_hex(buf, 16);
	return cnt;
}

int  __attribute__((optimize("O3"))) nrc_camera_capture (camera_config_t *config, char *buf, int len)
{
	const int retry_max = 3;
	int ret = 0;
	int retry_count = 0;

	sensor_t *sensor = &(s_state->sensor);

	if (!buf || !len)
		return -1;

	nrc_camera_log("Taking picture...\n");
	if (framesize_changed) {
		sensor->set_framesize(sensor, config->frame_size);
		framesize_changed = 0;
		_delay_ms(1000);
	}
	for (retry_count = 0 ; retry_count < retry_max ; retry_count++) {
		ret = _nrc_camera_read_data(config, buf, len);

		if (ret >= 20) {
			if (buf[0] != 0xFF || buf[1] != 0xD8) {
				nrc_camera_log("!!! no SOI (%02X%02X) !!!\n", buf[0], buf[1]);
				continue;
			}

			if (!(buf[2] == 0xFF && buf[3] == 0xE0) || strcmp(&buf[6], "JFIF") != 0) {
				nrc_camera_log("!!! no JFIF-APP0 (%02X%02X, %c%c%c%c) !!!\n", buf[2], buf[3], buf[6], buf[7], buf[8], buf[9]);
				continue;
			}

			for (int j = ret ; j >= 0 ; j--) {
				if ((buf[j - 2] == 0xFF && buf[j - 1] == 0xD9)) {
					nrc_camera_log("%d\n", j);
					return j;
				}
			}

			nrc_camera_log("!!! no EOI !!!\n");
			nrc_camera_log("Try lower image size...\n");
			sensor->set_framesize(sensor, FRAMESIZE_FHD);
			framesize_changed = 1;
			_delay_ms(1000);

//            print_hex(buf + (ret - 32), 32);
		}
	}

	return -1;
}

static nrc_err_t camera_probe(camera_config_t *config, camera_model_t *out_camera_model)
{
	nrc_err_t ret = NRC_SUCCESS;
	uint8_t slv_addr = 0;

	*out_camera_model = CAMERA_NONE;
	if (s_state != NULL) {
		return NRC_FAIL;
	}

	s_state = (camera_state_t *) calloc(sizeof(camera_state_t), 1);
	if (!s_state) {
		return NRC_FAIL;
	}

	if (config->pin_xclk >= 0) {
		system_printf("Configure XCLK output\n");
		xclk_timer_conf(config);
		system_printf("Enabling XCLK output\n");
		camera_enable_out_clock();
	}

	if (config->pin_sccb_sda != -1) {
		system_printf("Initializing SCCB\n");
#if defined(CONFIG_OV2640_SUPPORT)
		slv_addr = OV2640_SCCB_ADDR;
		system_printf("OV2640 slv_addr = 0x%x...\n", slv_addr);
#elif defined(CONFIG_OV5640_SUPPORT)
		slv_addr = OV5640_SCCB_ADDR;
		system_printf("OV5640 slv_addr = 0x%x...\n", slv_addr);
#else
		system_printf("slv_addr failed...\n");
		return NRC_FAIL;
#endif

		ret = SCCB_Init(config->pin_sccb_sda, config->pin_sccb_scl);
	}

	if(ret != NRC_SUCCESS) {
		system_printf("sccb init err\n");
		goto err;
	}

	if (config->pin_pwdn >= 0) {
		system_printf("Resetting camera by power down line\n");
		// carefull, logic is inverted compared to reset pin
		nrc_gpio_outb(config->pin_pwdn, 1);
		_delay_ms(1);
		nrc_gpio_outb(config->pin_pwdn, 0);
		_delay_ms(1);
	}

	if (config->pin_reset >= 0) {
		nrc_gpio_outb(config->pin_reset, 0);
		_delay_ms(1);
		nrc_gpio_outb(config->pin_reset, 1);
		_delay_ms(1);
	}

	system_printf("Searching for camera address\n");
	_delay_ms(1);

	system_printf("Detected camera at address=0x%02x\n", slv_addr);
	system_printf("XCLK frequency : %d\n", config->xclk_freq_hz);
	s_state->sensor.slv_addr = slv_addr;
	s_state->sensor.xclk_freq_hz = config->xclk_freq_hz;

	/**
	 * Read sensor ID and then initialize sensor
	 * Attention: Some sensors have the same SCCB address. Therefore, several attempts may be made in the detection process
	 */
	sensor_id_t *id = &s_state->sensor.id;
	for (size_t i = 0; i < sizeof(g_sensors) / sizeof(sensor_func_t); i++) {
		if (g_sensors[i].detect(slv_addr, id)) {
			camera_sensor_info_t *info = nrc_camera_sensor_get_info(id);
			if (NULL != info) {
				*out_camera_model = info->model;
				system_printf("Detected %s camera\n", info->name);
				g_sensors[i].init(&s_state->sensor);
				break;
			}
		}
	}

	if (CAMERA_NONE == *out_camera_model) { //If no supported sensors are detected
		system_printf("Detected camera not supported.\n");
		ret = NRC_FAIL;
		goto err;
	}

	system_printf("Camera PID=0x%02x CAM_VER=0x%02x MIDL=0x%02x MIDH=0x%02x\n",
			 id->PID, id->CAM_VER, id->MIDH, id->MIDL);

	system_printf("Doing SW reset of sensor\n");
	_delay_ms(1);

	return s_state->sensor.reset(&s_state->sensor);
err :
	camera_disable_out_clock();
	return ret;
}

nrc_err_t nrc_camera_init(camera_config_t *config)
{
	nrc_err_t err;

	camera_model_t camera_model = CAMERA_NONE;
	err = camera_probe(config, &camera_model);
	if (err != NRC_SUCCESS) {
		system_printf("Camera probe failed with error\n");
		goto fail;
	}

	framesize_t frame_size = config->frame_size;
	pixformat_t pix_format = config->pixel_format;

	if (frame_size > camera_sensor[camera_model].max_size) {
		system_printf("The frame size exceeds the maximum for this sensor, it will be forced to the maximum possible value\n");
		frame_size = camera_sensor[camera_model].max_size;
	}

	s_state->sensor.status.framesize = frame_size;
	s_state->sensor.pixformat = pix_format;

	system_printf("Setting frame size to %dx%d\n", resolution[frame_size].width, resolution[frame_size].height);
	if (s_state->sensor.set_framesize(&s_state->sensor, frame_size) != 0) {
		system_printf("Failed to set frame size\n");
		err = NRC_FAIL;
		goto fail;
	}
	s_state->sensor.set_pixformat(&s_state->sensor, pix_format);

	if (s_state->sensor.id.PID == OV2640_PID) {
		s_state->sensor.set_gainceiling(&s_state->sensor, GAINCEILING_2X);
		s_state->sensor.set_bpc(&s_state->sensor, false);
		s_state->sensor.set_wpc(&s_state->sensor, true);
		s_state->sensor.set_lenc(&s_state->sensor, true);
	}

	if (pix_format == PIXFORMAT_JPEG) {
		s_state->sensor.set_quality(&s_state->sensor, config->jpeg_quality);
	}
	s_state->sensor.init_status(&s_state->sensor);

	/* Flip the image output horizontally to get correct output */
	s_state->sensor.set_hmirror(&s_state->sensor, true);
	return NRC_SUCCESS;

fail:
	return err;
}

void nrc_camera_deinit()
{
	camera_disable_out_clock();

	if (s_state) {
		SCCB_Deinit();

		free(s_state);
		s_state = NULL;
	}
}

sensor_t *nrc_camera_sensor_get()
{
	if (s_state == NULL) {
		return NULL;
	}
	return &s_state->sensor;
}

nrc_err_t nrc_camera_sleep(camera_config_t *config)
{
	nrc_camera_deinit();
	if (config->pin_pwdn >= 0) {
		system_printf("Power down camera\n");
		// carefull, logic is inverted compared to reset pin
		nrc_gpio_outb(config->pin_pwdn, 1);
		_delay_ms(1);
	} else {
		return NRC_FAIL;
	}

	return NRC_SUCCESS;
}
