#ifndef __BASE64_HTML_H__
#define __BASE64_HTML_H__

#define WIFI_MODE_SUBST 0
#define WIFI_SSID_SUBST 1
#define WIFI_SECURITY_SUBST 2
#define WIFI_PASSWORD_SUBST 3
#define WIFI_COUNTRY_SUBST 4
#define WIFI_CHANNEL_SUBST 5
#define WIFI_IPMODE_SUBST 6
#define WIFI_STATICIP_SUBST 7
#define WIFI_DHCPSERVER_SUBST 8
#define WIFI_SHORT_BCN_SUBST 9
#define WIFI_TXPOWER_SUBST 10
#define WIFI_BSS_MAX_IDLE_SUBST 11
#define WIFI_BSS_RETRY_CNT_SUBST 12
#define WIFI_CONN_TIMEOUT_SUBST 13
#define WIFI_DISCONN_TIMEOUT_SUBST 14
#define WIFI_TXPOWER_TYPE_SUBST 15
#define WIFI_NETMASK_SUBST 16
#define WIFI_GATEWAY_SUBST 17

const char* html_subst[] = {
  "%%WIFI_MODE%%", "%%WIFI_SSID%%", "%%WIFI_SECURITY%%",
  "%%WIFI_PASSWORD%%", "%%WIFI_COUNTRY%%",   "%%WIFI_CHANNEL%%",
  "%%WIFI_IP_MODE%%",  "%%WIFI_STATIC_IP%%", "%%WIFI_DHCP_SERVER%%",
  "%%WIFI_SHORT_BCN%%", "%%WIFI_TXPOWER%%", "%%WIFI_BSS_MAX_IDLE%%",
  "%%WIFI_BSS_RETRY_CNT%%",    "%%WIFI_CONN_TIMEOUT%%" ,    "%%WIFI_DISCONN_TIMEOUT%%",
  "%%WIFI_TXPOWER_TYPE%%", "%%WIFI_NETMASK%%", "%%WIFI_GATEWAY%%",
};

#define WIFI_SUBST_SIZE sizeof(html_subst) / sizeof(html_subst[0])

const char html_base64[] = {
#include "setting_html_lzo.h"
};

#endif /* __BASE64_HTML_H__ */
