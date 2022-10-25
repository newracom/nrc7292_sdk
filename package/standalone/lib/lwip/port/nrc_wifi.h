#ifndef __NRC_WIFI_H__
#define __NRC_WIFI_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "system.h"

#include "lwip/ip_addr.h"

typedef enum {
	WIFI_NULL_MODE = 0,      /**< null mode */
	WIFI_STATION_MODE,       /**< WiFi station mode */
	WIFI_SOFTAP_MODE,        /**< WiFi soft-AP mode */
	WIFI_STATIONAP_MODE,     /**< WiFi station + soft-AP mode */
	WIFI_MAX_MODE
} WIFI_MODE;

#define USE_WIFI_MODE		WIFI_STATIONAP_MODE
#define WIFI_AP_NAME		"NRC7292_AP"
#define WIFI_AP_PASSWORD	"00000000"

#define WIFI_PRIMARY_DNS_SERVER		"8.8.8.8"
#define WIFI_SECONDARY_DNS_SERVER	"8.8.4.4"

WIFI_MODE wifi_get_opmode(void);
bool wifi_set_opmode(WIFI_MODE opmode);

typedef enum {
	SOFTAP_IF = 0,
	STATION_IF,
	MAX_IF
} WIFI_INTERFACE;

enum dhcp_status {
	DHCP_STOPPED,	/* disable DHCP */
	DHCP_STARTED	/* enable DHCP */
};

struct ip_info {
	ip_addr_t ip;      /**< IP address */
	ip_addr_t netmask; /**< netmask */
	ip_addr_t gw;      /**< gateway */
};

typedef struct ip_info ip_info_t;

typedef enum {
    AUTH_OPEN = 0,      /**< authenticate mode : open */
    AUTH_WEP,           /**< authenticate mode : WEP */
    AUTH_WPA_PSK,       /**< authenticate mode : WPA_PSK */
    AUTH_WPA2_PSK,      /**< authenticate mode : WPA2_PSK */
    AUTH_WPA_WPA2_PSK,  /**< authenticate mode : WPA_WPA2_PSK */
    AUTH_MAX
} AUTH_MODE;

struct softap_config {
    uint8_t ssid[32];         /**< SSID of ESP8266 soft-AP */
    uint8_t password[64];     /**< Password of ESP8266 soft-AP */
    uint8_t ssid_len;         /**< Length of SSID. If softap_config.ssid_len==0, check the SSID until there is a
						termination character; otherwise, set the SSID length according to softap_config.ssid_len. */
    uint8_t channel;          /**< Channel of ESP8266 soft-AP */
    AUTH_MODE authmode;     /**< Auth mode of ESP8266 soft-AP. Do not support AUTH_WEP in soft-AP mode */
    uint8_t ssid_hidden;      /**< Broadcast SSID or not, default 0, broadcast the SSID */
    uint8_t max_connection;   /**< Max number of stations allowed to connect in, default 4, max 4 */
    uint8_t beacon_interval; /**< Beacon interval, 100 ~ 60000 ms, default 100 */
};

struct station_config {
    uint8_t ssid[32];         /**< SSID of target AP*/
    uint8_t password[64];     /**< password of target AP*/
    uint8_t bssid_set;        /**< whether set MAC address of target AP or not. Generally, station_config.bssid_set
needs to be 0; and it needs to be 1 only when users need to check the MAC address of the AP.*/
    uint8_t bssid[6];         /**< MAC address of target AP*/
};

typedef void (*lwiperf_report_cb_t) (const char *report_type,
									const ip_addr_t* remote_addr, uint16_t remote_port,
									uint32_t bytes_transferred,
									uint32_t ms_duration,
									uint32_t bandwidth_kbitpsec);

bool wifi_get_ip_info(WIFI_INTERFACE if_index, struct ip_info *info);
bool wifi_set_ip_info(WIFI_INTERFACE if_index, struct ip_info *info);
bool wifi_set_dns_server(void);
void wifi_lwip_init( void );
int wifi_station_dhcpc_start(int vif);
int wifi_station_dhcpc_stop(int vif);
int wifi_station_dhcpc_status(int vif);
bool wifi_ifconfig(int num_param, char *params[]);
enum dhcp_status wifi_softap_dhcps_status(void);

u8_t wifi_get_vif_id(ip4_addr_t* addr);
void set_dhcp_status(bool status);
bool get_dhcp_status(void);

#ifdef LWIP_DHCP
int dhcp_run(int vif);
#endif /* LWIP_DHCP */

int setup_wifi_ap_mode(int vif, int updated_lease_time);
int start_dhcps_on_if(struct netif *net_if, int updated_lease_time);

#if defined(SUPPORT_MBEDTLS) && defined(SUPPORT_AES_SAMPLE_TEST)
int aes_sample_test(void);
#endif /* (SUPPORT_MBEDTLS) && (SUPPORT_AES_SAMPLE_TEST) */

#ifdef __cplusplus
}
#endif

#endif // __NRC_WIFI_H__
