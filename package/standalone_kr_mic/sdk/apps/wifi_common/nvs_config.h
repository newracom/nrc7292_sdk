#ifndef __NVS_CONFIG_H__
#define __NVS_CONFIG_H__

/* NVS key used to configure the device */

/* SSID to be used */
/* CLI : nvs set ssid <ssid> */
#define NVS_SSID "ssid"

/* WIFI security mode, uint8_t value */
/* 0 : OPEN, 1 : WPA2, 2 : WPA3 OWE, 3 : WPA3 SAE */
/* CLI : nvs set_u8 wifi_security <0-3> */
#define NVS_WIFI_SECURITY "wifi_security"

/* Wifi security code for WPA2 and WPA3 SAE */
/* CLI : nvs set wifi_password <password> */
#define NVS_WIFI_PASSWORD "wifi_password"

/* Wifi Channel to be used in AP mode, uint32_t value */
/* CLI : nvs set_u32 wifi_channel <channel> */
#define NVS_WIFI_CHANNEL "wifi_channel"

/* IP address mode, uint8_t value 0 for static or 1 for dynamic, uint8_t value */
/* CLI : nvs set_u8 <0-1> */
#define NVS_IP_MODE "ip_mode"

/* remote device ip address */
/* CLI : nvs set remote_address <remote address> */
#define NVS_REMOTE_ADDRESS "remote_address"

/* remote device port number, uint16_t value */
/* CLI : nvs set_u16 remote_port <port> */
#define NVS_REMOTE_PORT "remote_port"

/* IP address on wlan0 if static used */
/* CLI : nvs set wlan0_ip <ip address v4> */
#define NVS_STATIC_IP "wlan0_ip"

/* IPv6 address on wlan0 if static used */
/* CLI: nvs set wlan0_ip6 <ip address> */
#define NVS_STATIC_IP6 "wlan0_ip6"

/* DHCP server 0 for off, 1 for on, uint8_t */
/* CLI : nvs set_u8 dhcps_on_wlan <0/1> */
#define NVS_DHCP_SERVER_ON_WLAN "dhcps_on_wlan"
#endif
