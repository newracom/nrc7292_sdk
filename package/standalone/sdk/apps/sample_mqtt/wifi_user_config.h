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
#define STR_SSID "halow_demo"
#define COUNTRY_CODE "US"
#define NRC_WIFI_SECURE  WIFI_SEC_WPA2
#define NRC_WIFI_PASSWORD  "12345678"

#define NRC_WIFI_SCAN_LIST 1
#define SCAN_CHANNEL_LIST "9025,9035,9045,9055,9065,9075,9085,9095,9105,9115,\
		9125,9135,9145,9155,9165,9175,9185,9195,9205,9215,\
		9225,9235,9245,9255,9265,9275,9030,9050,9070,9090,\
		9110,9130,9150,9170,9190,9210,9230,9250,9270,9060,\
		9100,9140,9180,9220,9260"

#define WIFI_CONN_TIMEOUT	10000 // ms
#define NRC_WIFI_TEST_COUNT 5

#define TX_POWER 17  // dBm
#define TX_POWER_TYPE 2 // Auto(0), Limit(1), Fixed(2)

#endif // __WIFI_USER_CONFIG_H__ //
