#pragma once

#include "system_common.h"
#include "nrc_camera.h"

nrc_err_t xclk_timer_conf(camera_config_t *config);

nrc_err_t camera_enable_out_clock();

void camera_disable_out_clock();
