#ifndef __WIFI_USER_CONFIG_H__
#define __WIFI_USER_CONFIG_H__

/**
 * User configurations for Wi-Fi settings can be added here. These definitions will
 * override the default values found in 'wifi_common/wifi_config.h'.
 *
 * The NVS (non-volatile storage) can also be used to override the configuration values
 * dynamically. See 'wifi_common/nvs_config.h' to find the keys that can be used to configure
 * the device using NVS.
 *
 * By defining these user configurations here, specific Wi-Fi settings such as the SSID,
 * password, security type, IP address, and other parameters can be customized for a
 * particular use case or application.
*/

#define NRC_WIFI_BGSCAN_ENABLE 1
#define NRC_WIFI_BGSCAN_SHORT_INTERVAL 30
#define NRC_WIFI_BGSCAN_THRSHOLD -60
#define NRC_WIFI_BGSCAN_LONG_INTERVAL 300

#define COUNTRY_CODE "US"
#define NRC_WIFI_SECURE  WIFI_SEC_WPA2
#define NRC_WIFI_PASSWORD  "12345678"
#define STR_SSID "halow_roaming"

#endif // __WIFI_USER_CONFIG_H__ //
