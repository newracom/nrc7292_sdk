#ifndef __NRC_LWIP_H__
#define __NRC_LWIP_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "system.h"

#include "lwip/netif.h"
#include "lwip/ip_addr.h"
#include "lwip/dhcp.h"

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

#define MAC_STR			"%02X:%02X:%02X:%02X:%02X:%02X"
#define MAC_VALUE(a)		(a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]
#define IP4_ADDR_STR		"%" U16_F ".%" U16_F ".%" U16_F ".%" U16_F
#define IP4_ADDR_VALUE(addr)	ip4_addr1_16(addr), ip4_addr2_16(addr),ip4_addr3_16(addr), ip4_addr4_16(addr)

WIFI_MODE wifi_get_opmode(void);
bool wifi_set_opmode(WIFI_MODE opmode);

#define MAX_IF 2

typedef enum {
	WLAN0_INTERFACE = 0,
	WLAN1_INTERFACE,
#ifdef SUPPORT_ETHERNET_ACCESSPOINT
	ETHERNET_INTERFACE,
#endif /* SUPPORT_ETHERNET_ACCESSPOINT */
#if LWIP_BRIDGE
	BRIDGE_INTERFACE,
#endif /* LWIP_BRIDGE */
	END_INTERFACE,
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

struct netif * nrc_netif_get_by_idx(uint8_t idx);
int nrc_idx_get_by_name(char *argv);
int nrc_is_local_mac(uint8_t *addr);

bool wifi_get_ip_info(int vif_id, struct ip_info *info);
bool wifi_set_ip_info(int vif_id, struct ip_info *info);
bool wifi_set_dns_server(ip_addr_t *pri_dns, ip_addr_t *sec_dns);
void wifi_lwip_init( void );
bool wifi_setup_interface(WIFI_INTERFACE i);

int static_run(int vif);

#if LWIP_IPV4 && LWIP_DHCP
int dhcp_run(int vif);
int wifi_dhcpc_start(int vif);
int wifi_dhcpc_start_with_event (int vif, dhcp_event_handler_t event_handler);
int wifi_dhcpc_stop(int vif);
int wifi_dhcpc_get_lease_time(int vif);
int wifi_dhcpc_status(int vif);
int wifi_station_dhcpc_start(int vif);
int wifi_station_dhcpc_stop(int vif);
int wifi_station_dhcpc_status(int vif);
int wifi_bridge_dhcpc_start(void);
int wifi_bridge_dhcpc_stop(void);
int wifi_bridge_dhcpc_status(void);
#endif

bool wifi_ifconfig(int argc, char *argv[]);
enum dhcp_status wifi_softap_dhcps_status(void);

bool wifi_get_ip_address(int vif_id, char **ip_addr);
uint8_t wifi_get_vif_id(ip_addr_t* dest);
bool get_dhcp_status(int vif_id);
bool wifi_dhcps(int argc, char *argv[]);
bool wifi_bridge(int argc, char *argv[]);
bool setup_wifi_bridge_interface(void);
bool delete_wifi_bridge_interface(void);

int setup_wifi_ap_mode(struct netif *net_if, int updated_lease_time);
int start_dhcps_on_if(struct netif *net_if, int updated_lease_time);

void reset_ip_address(int vif);
int reset_wifi_ap_mode(int vif);

u32_t get_utc_time(void);
char *get_utc_time_str (char *buf, int len);
int initialize_sntp(const char *server, u32_t timeout);

#if LWIP_IPV6
void wifi_nd6_restart_netif( int vif );
#endif

extern struct netif *nrc_netif[];

#ifdef __cplusplus
}
#endif

#endif // __NRC_LWIP_H__
