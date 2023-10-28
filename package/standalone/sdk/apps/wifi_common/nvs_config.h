#ifndef __NVS_CONFIG_H__
#define __NVS_CONFIG_H__

/* NVS key used to configure the device */

/* Indicate whether the configuration is written using */
/* nrc_save_wifi_config_with_vif */
/* (type u8) */
#define NVS_CONFIG_WRITTEN "cfg_written"

/* types supported are listed as below */
/* string : char string */
/* i32 : int */
/* u32 : unsigned int */
/* i16 : short */
/* u16 : unsigned short */
/* i8  : char */
/* u8  : unsigned char */

/* SSID to be used */
/* (type string) */
/* CLI : nvs set ssid <ssid> */
#define NVS_SSID "ssid"

/* WIFI security mode, uint8_t value */
/* 0 : OPEN, 1 : WPA2, 2 : WPA3 OWE, 3 : WPA3 SAE */
/* (type u8) */
/* CLI : nvs set_u8 wifi_security <0-3> */
#define NVS_WIFI_SECURITY "wifi_security"

/* Wifi security code for WPA2 and WPA3 SAE */
/* (type string) */
/* CLI : nvs set wifi_password <password> */
#define NVS_WIFI_PASSWORD "wifi_password"

/* Wifi pairwise master key calculated */
/* (type string) */
#define NVS_WIFI_PMK "wifi_pmk"

/* SSID used for calculating PMK */
/* (type string) */
#define NVS_WIFI_PMK_SSID  "wifi_pmk_ssid"

/* password used for calculating PMK */
/* (type string) */
#define NVS_WIFI_PMK_PASSWORD "wifi_pmk_pw"

/* Wifi Channel */
/* (type u16) */
/* CLI : nvs set_u16 wifi_channel <channel> */
#define NVS_WIFI_CHANNEL "wifi_channel"

/* Wifi Channel bandwidth to be used in AP mode, set_u8 value */
/* (type u8) */
/* CLI : nvs set_u8 wifi_channel_bw <bandwidth> */
#define NVS_WIFI_CHANNEL_BW "wifi_channel_bw"

/* IP address mode for wlan0, uint8_t value 0 for static or 1 for dynamic, uint8_t value */
/* (type u8) */
/* CLI : nvs set_u8 ip_mode <0-1> */
#define NVS_IP_MODE "ip_mode"

/* IP address mode for wlan1, uint8_t value 0 for static or 1 for dynamic, uint8_t value */
/* (type u8) */
/* CLI : nvs set_u8 wlan1_ip_mode <0-1> */
#define NVS_REMOTE_ADDRESS "remote_address"

/* remote device port number, uint16_t value */
/* (type u16) */
/* CLI : nvs set_u16 remote_port <port> */
#define NVS_REMOTE_PORT "remote_port"

/* IP address on wlan0 if static used */
/* (type string) */
/* CLI : nvs set wlan0_ip <ip v4 address> */
#define NVS_STATIC_IP "wlan0_ip"

/* IP network mask on wlan0 if static used */
/* (type string) */
/* CLI : nvs set wlan0_netmask <ip v4 network mask> */
#define NVS_NETMASK "wlan0_netmask"

/* IP gateway on wlan0 if static used */
/* (type string) */
/* CLI : nvs set wlan0_gateway <ip v4 gateway> */
#define NVS_GATEWAY "wlan0_gateway"

/* IPv6 address on wlan0 if static used */
/* (type string) */
/* CLI: nvs set wlan0_ip6 <ip address> */
#define NVS_STATIC_IP6 "wlan0_ip6"

/* DHCP server 0 for off, 1 for on, uint8_t */
/* (type u8) */
/* CLI : nvs set_u8 dhcps_on_wlan <0/1> */
#define NVS_DHCP_SERVER_ON_WLAN "dhcps_on_wlan"

/* COUNTRY to be used */
/* (type string) */
/* CLI : nvs set country <country> */
#define NVS_COUNTRY "country"

/* bss max idle */
/* (type i32) */
#define NVS_BSS_MAX_IDLE "bss_max_idle"

/* bss retry count */
/* (type i32) */
#define NVS_BSS_RETRY_CNT "bss_retry_cnt"

/* Tx Power (dbm) */
/* (type u8) */
#define NVS_WIFI_TX_POWER "tx_power"

/* Tx Power type : Auto(0), Limit(1), Fixed(2) */
/* (type u8) */
#define NVS_WIFI_TX_POWER_TYPE "tx_power_type"

/* BSSID to be used */
/* (type string) */
/* CLI : nvs set bssid <bssid> */
#define NVS_BSSID "bssid"

/*WIFI Connection timeout */
/* (type i32) */
#define NVS_WIFI_CONN_TIMEOUT  "conn_timeout"

/*WIFI Disonnection timeout */
/* (type i32) */
#define NVS_WIFI_DISCONN_TIMEOUT  "disconn_timeout"

/*WIFI beacon interval */
/* CLI : nvs set_i32 wifi_bcn <15-65535> */
/* (type u16) */
#define NVS_WIFI_BCN_INTERVAL "wifi_bcn"

/* Device mode, the value should be either AP (1) or STA (0) */
/* If not set, default mode will be AP */
/* CLI : nvs set_u8 device_mode <0-1> */
/* (type u8) */
#define NVS_DEVICE_MODE "device_mode"

/* Network mode for ethernet and wifi configuration (type u8) */
/* Possible value bridge(0) or nat(1)" */
/* If not set, default network mode will be nat(1) */
/* CLI : nvs set_u8 network_mode <0-1> */
/* (type u8) */
#define NVS_NETWORK_MODE "network_mode"

/* Wi-Fi Rate Control: Determines whether rate control is enabled or disabled. */
/* CLI : nvs set_u8 wifi_rc <0-1> */
/* (type u8) */
#define NVS_WIFI_RATE_CONTROL "wifi_rc"

/* Wi-Fi MCS (Modulation and Coding Scheme): Specifies the data rate and error correction capability for Wi-Fi transmissions. */
/* CLI : nvs set_u8 wifi_mcs <0-7,10> */
/* (type u8) */
#define NVS_WIFI_MCS "wifi_mcs"

/* Wi-Fi GI (Guard Interval): Specifies the guard interval type for Wi-Fi transmissions. */
/* CLI : nvs set_u8 wifi_gi <0-1> */
/* (type u8) */
#define NVS_WIFI_GI "wifi_gi"

/* Wi-Fi CCA (Clear Channel Assessment) Threshold: Specifies the signal strength threshold for Wi-Fi channel assessment. */
/* CLI : nvs set_i8 wifi_cca_thres <-100 ~ -35 > */
/* (type i8) */
#define NVS_WIFI_CCA_THRES "wifi_cca_thres"

/* Wi-Fi ignore_broadcast_ssid - Hide SSID in AP mode */
/* This setting controls the behavior of the Access Point (AP) regarding SSID broadcast. */
/* When enabled, the AP will send empty SSID in beacons and ignore probe request frames */
/* that do not specify the full SSID, thus requiring stations to know the SSID. */
/* - 0: Probe requests for broadcast SSID are not ignored. It sends the SSID in beacons. (default) */
/* - 1: Send an empty (length=0) SSID in beacons and ignore probe requests for broadcast SSID. */
/* - 2: Clear SSID (ASCII 0), but keep the original length. (this may be required */
/*		with some clients that do not support empty SSID) and ignore probe */
/*	   requests for broadcast SSID */
/* CLI : nvs set_u8 ssid_type <0-2> */
/* (type u8) */
#define NVS_WIFI_IGNORE_BROADCAST_SSID "ssid_type"

/* Maximum number of stations allowed in softAP (upto 10) */
/* CLI : nvs set_u8 max_num_sta <1-10> */
/* (type u8) */
#define NVS_WIFI_SOFTAP_MAX_NUM_STA "max_num_sta"

/* Wi-Fi beacon count for listen interval */
/* CLI : nvs set_u16 listen_interval */
/* (type u16) */
#define NVS_WIFI_LISTEN_INTERVAL "listen_interval"

#ifdef INCLUDE_SCAN_BACKOFF

#define NVS_SCAN_BACKOFF_START_COUNT "backoff_cnt"
#define NVS_SCAN_MAX_INTERVAL "backoff_max"
#endif
#endif
