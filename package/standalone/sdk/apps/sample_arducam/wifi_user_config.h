#ifndef __WIFI_USER_CONFIG_H__
#define __WIFI_USER_CONFIG_H__

/* Add wifi user configurations */

/* Definition here will override the default configuration */
/* found in wifi_common/wifi_config.h. */
/* See wifi_common/wifi_config.h to find the default values. */

/* One can utilize NVS (non-volatile storage) override the configuration as well. */
/* See wifi_common/nvs_config.h to see the key's that can be used to configure the device. */

#define STR_SSID "halow_camera"
#define NRC_WIFI_IP_MODE	WIFI_STATIC_IP
#define NRC_STATIC_IP "192.168.200.21"
#define NRC_NETMASK "255.255.255.0"
#define NRC_GATEWAY "192.168.200.1"

#endif // __WIFI_USER_CONFIG_H__ //
