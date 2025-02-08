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

#define NRC_WIFI_LISTEN_INTERVAL_DEFAULT 1000

#define NRC_WIFI_PS_MODE_DEFAULT 0
#define NRC_WIFI_PS_IDLE_TIMEOUT_DEFAULT 100
#define NRC_WIFI_PS_SLEEP_TIME_DEFAULT 60000

#endif // __WIFI_USER_CONFIG_H__ //
