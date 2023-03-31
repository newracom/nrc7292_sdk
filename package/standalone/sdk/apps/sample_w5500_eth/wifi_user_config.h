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

#define WIFI_DEVICE_MODE 1 // STA(0), AP(1)
#define WIFI_NETWORK_MODE 1 // Bridge(0), NAT(1)
#define NRC_WIFI_IP_MODE	WIFI_STATIC_IP
#define NRC_WIFI_SOFTAP_DHCP_SERVER	0 // 0(off), 1(on)
#define NRC_STATIC_IP "192.168.200.1"
#define NRC_NETMASK "255.255.255.0"
#define NRC_GATEWAY "192.168.200.1"

#define COUNTRY_CODE "US"
#define NRC_WIFI_CHANNEL 9250

#endif // __WIFI_USER_CONFIG_H__ //
