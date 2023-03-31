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

/* Wifi pairwise master key calculated */
#define NVS_WIFI_PMK "wifi_pmk"

/* SSID used for calculating PMK */
#define NVS_WIFI_PMK_SSID  "wifi_pmk_ssid"

/* password used for calculating PMK */
#define NVS_WIFI_PMK_PASSWORD "wifi_pmk_pw"

/* Wifi Channel */
/* CLI : nvs set_u32 wifi_channel <channel> */
#define NVS_WIFI_CHANNEL "wifi_channel"

/* Wifi Channel bandwidth to be used in AP mode, set_u8 value */
/* CLI : nvs set_u8 wifi_channel_bw <bandwidth> */
#define NVS_WIFI_CHANNEL_BW "wifi_channel_bw"

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
/* CLI : nvs set wlan0_ip <ip v4 address> */
#define NVS_STATIC_IP "wlan0_ip"

/* IP network mask on wlan0 if static used */
/* CLI : nvs set wlan0_netmask <ip v4 network mask> */
#define NVS_NETMASK "wlan0_netmask"

/* IP gateway on wlan0 if static used */
/* CLI : nvs set wlan0_gateway <ip v4 gateway> */
#define NVS_GATEWAY "wlan0_gateway"

/* IPv6 address on wlan0 if static used */
/* CLI: nvs set wlan0_ip6 <ip address> */
#define NVS_STATIC_IP6 "wlan0_ip6"

/* DHCP server 0 for off, 1 for on, uint8_t */
/* CLI : nvs set_u8 dhcps_on_wlan <0/1> */
#define NVS_DHCP_SERVER_ON_WLAN "dhcps_on_wlan"

/* COUNTRY to be used */
/* CLI : nvs set country <country> */
#define NVS_COUNTRY "country"

/* bss max idle */
#define NVS_BSS_MAX_IDLE "bss_max_idle"

/* bss retry count */
#define NVS_BSS_RETRY_CNT "bss_retry_cnt"

/* Tx Power (dbm) */
#define NVS_WIFI_TX_POWER "tx_power"

/* Tx Power type : Auto(0), Limit(1), Fixed(2)*/
#define NVS_WIFI_TX_POWER_TYPE "tx_power_type"

/* BSSID to be used */
/* CLI : nvs set bssid <bssid> */
#define NVS_BSSID "bssid"

/*WIFI Connection timeout */
#define NVS_WIFI_CONN_TIMEOUT  "conn_timeout"

/*WIFI Disonnection timeout */
#define NVS_WIFI_DISCONN_TIMEOUT  "disconn_timeout"

/*WIFI beacon interval */
#define NVS_WIFI_BCN_INTERVAL "wifi_bcn"

/*WIFI short beacon interval */
#define NVS_WIFI_SHORT_BCN_INTERVAL "wifi_short_bcn"

/* Device mode, the value should be either AP or STA */
/* If not set, default mode will be AP */
#define NVS_DEVICE_MODE "device_mode"

/* Network mode for ethernet and wifi configuration */
/* Possible value bridge(0) or nat(1)" */
/* If not set, default network mode will be nat(1) */
#define NVS_NETWORK_MODE "network_mode"

#endif
