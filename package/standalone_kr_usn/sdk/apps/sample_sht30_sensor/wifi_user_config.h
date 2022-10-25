#ifndef __WIFI_USER_CONFIG_H__
#define __WIFI_USER_CONFIG_H__

/* Add wifi user configurations */

/* Definition here will override the default configuration */
/* found in wifi_common/wifi_config.h. */
/* See wifi_common/wifi_config.h to find the default values. */

/* One can utilize NVS (non-volatile storage) override the configuration as well. */
/* See wifi_common/nvs_config.h to see the key's that can be used to configure the device. */

/* Uncomment below and set AP SSID to connect to */
/* SSID (Service Set Identifier) */
/* #define STR_SSID "halow_demo" */

/* Uncomment below and set country code. */
/* K1:Korea-USN, K2: Korea-MIC, JP: Japan, US:United States, TW: Taiwan, EU: EURO */
/* #define COUNTRY_CODE "US" */

/* Uncomment below and set wifi security used for AP or station */
/* WIFI_SEC_OPEN, WIFI_SEC_WPA2, WIFI_SEC_WPA3_SAE, WIFI_SEC_WPA3_OWE */
/* #define NRC_WIFI_SECURE  WIFI_SEC_OPEN */

/* Uncomment below and set desired password */
/* If WIFI security is not open or WPA3 OWE, set below */
#if defined(NRC_WIFI_SECURE) && (NRC_WIFI_SECURE == WIFI_SEC_WPA2)
/* password */
/* #define NRC_WIFI_PASSWORD  "12345678" */
#endif /* defined(NRC_WIFI_SECURE) && (NRC_WIFI_SECURE == WIFI_SEC_WPA2) */

/* Uncomment below and set whether to use static IP or DHCP */
/* WIFI_STATIC_IP, WIFI_DYNAMIC_IP*/
/* #define NRC_WIFI_IP_MODE	WIFI_DYNAMIC_IP */

/* Uncomment below and set desired IP address settings */
#if (NRC_WIFI_IP_MODE == WIFI_STATIC_IP)
/* Static IP address */
/* #define NRC_STATIC_IP "192.168.200.13" */
/* #define NRC_NETMASK "255.255.255.0" */
/* #define NRC_GATEWAY "192.168.200.1" */
#endif /* (NRC_WIFI_IP_MODE == WIFI_STATIC_IP) */

/* Uncomment below and set remote IP address the sample will use */
/* Some samples that will talk to other end will require this address */
/* Remote IP address to connect */
/* #define NRC_REMOTE_ADDRESS "192.168.200.1" */

/* Uncomment below and set port number to communicate to */
/* Some samples use this port number to talk to other end */
/* Remote Port number to connect */
/* #define NRC_REMOTE_PORT		8099 */

/* Uncomment below to configure maximum TX power to be used */
/* Tx power value (dBm) */
/* #define TX_POWER 24 */

/* Uncomment below and set appropriate value */
/* Some samples use this value to perform number of iteration that test will execute. */
/* Test number */
/* #define NRC_WIFI_TEST_COUNT 20 */

/* Uncomment below and set test interval */
/* Test interval (ms)*/
/* #define NRC_WIFI_TEST_INTERVAL 1000 */

/* Uncomment below and set test duration */
/* Test duration (ms)*/
/* #define NRC_WIFI_TEST_DURATION 10000 */ /* ms */

/* If IPv6 is enabled, and wish to use static IPv6 address,
   Uncomment below and set appropriate IPv6 address */
#ifdef CONFIG_IPV6
/* Static IPv6 address to use. */
/* In order to override this configure it using NVS key "wlan0_ip6" */
/* #define NRC_STATIC_IP6 "fc00::c0a8:c80d" */
#endif /* CONFIG_IPV6 */

/* Scan APs on specfied frequencies */
/* If one wishes to use specific list of frequency, define below to 1 */
/* and set list of sub 1 GHz frequencies in below nrc_scan_freq_list[] */
#define NRC_WIFI_SCAN_LIST 0

/* Uncomment below and set desired frequency list to scan for station */
#if NRC_WIFI_SCAN_LIST
/* Scan channels and lists */
/* uint16_t nrc_scan_freq_list[]={9055,9175,9090,9250,9220};
   #define NRC_SCAN_FREQ_LIST 1 */
#endif /* NRC_WIFI_SCAN_LIST */

/* Access Point settings */

/* Uncomment below and set to 0 (No DHCP server), 1 (start DHCP server) */
/* DHCP server is enable or disable in softAP */
/* #define NRC_WIFI_SOFTAP_DHCP_SERVER 1 */

/* Uncomment below and set designate AP ip address, if device will run as SoftAP. */
/* IP address for softAP */
/* #define NRC_AP_IP NRC_REMOTE_ADDRESS */

/* Uncomment below and set the channel frequency the AP will use */
/* Default CH(MHz) : K1-USN:921.5 K2-MIC:927.5 JP:918.0 US:925.0 TW:839.5 EU:864.0 */
/* #define NRC_AP_SET_CHANNEL 9250 */

/* Uncomment below and set the channel frequency bandwidh the AP will use */
/* Default CH bandwidth : WIFI_1M, WIFI_2M, WIFI_4M */
/* #define NRC_AP_SET_CHANNEL_BW WIFI_2M */

/* Uncomment below and how the device wishes to scan */
/* Scan method */
/* #define WIFI_SCAN_MODE	WIFI_SCAN_NORMAL */

/* Uncomment below and how the device wishes to set connection timeout */
/* Scan method */
/* #define WIFI_CONN_TIMEOUT	5000 */

/* Uncomment below and how the device wishes to set disconnection timeout */
/* Scan method */
/* #define WIFI_DISCONN_TIMEOUT	5000 */

#endif // __WIFI_USER_CONFIG_H__ //
