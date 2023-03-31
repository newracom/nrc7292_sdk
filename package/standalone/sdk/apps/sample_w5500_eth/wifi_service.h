#ifndef __WIFI_SERVICE_H__
#define __WIFI_SERVICE_H__

#include "nrc_sdk.h"
#include "wifi_config.h"

nrc_err_t start_softap(WIFI_CONFIG *param);
nrc_err_t connect_to_ap(WIFI_CONFIG *param);

#endif
