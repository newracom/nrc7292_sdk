/*
 * MIT License
 *
 * Copyright (c) 2022 Newracom, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#ifndef __WIFI_CONFIG_H__
#define __WIFI_CONFIG_H__

/*
 * Include user defined options first. Anything not defined in these files
 * will be set to standard values.
 */
#include "wifi_user_config.h"


/**
 * SSID (Service Set Identifier)
 * To set the AP SSID to connect to when the device is in STA mode and
 * to run the AP with that SSID, you will need to configure the wireless
 * network settings of the device.
 */
#ifndef STR_SSID
#define STR_SSID "halow_demo"
#endif /* STR_SSID */

/**
 * BSSID (Basic Service Set Identifier)
 * To set the AP BSSID to connect to when the device is in STA mode
 * it represents the MAC physical address of an access point
 */
#ifndef STR_BSSID
#define STR_BSSID "00:00:00:00:00:00"
#endif /* STR_BSSID */


/**
 * Country code
 * The ISO/IEC alpha2 country code for the country in which this device is
 * currently operating.{US|JP|K1|TW|EU|CN|NZ|AU|K2}
 */
#ifndef COUNTRY_CODE
#define COUNTRY_CODE "US"
#endif /* COUNTRY_CODE */


/**
 * Security settings for Wi-Fi networks can be specified for both the Access Point (AP) and Station (STA) modes.
 * The available security options are:
 *     WIFI_SEC_OPEN: No security
 *     WIFI_SEC_WPA2: Wi-Fi Protected Access II (WPA2)
 *     WIFI_SEC_WPA3_SAE: Wi-Fi Protected Access III (WPA3) Simultaneous Authentication of Equals (SAE)
 *     WIFI_SEC_WPA3_OWE: Wi-Fi Protected Access III (WPA3) Opportunistic Wireless Encryption (OWE)
 *     Note that SoftAP mode may not support WIFI_SEC_WPA3_SAE and WIFI_SEC_WPA3_OWE security options.
 */
#ifndef NRC_WIFI_SECURE
#define NRC_WIFI_SECURE  WIFI_SEC_OPEN
#endif /* NRC_WIFI_SECURE */


/**
 * IP Address settings for Wi-Fi connections can be specified as either static or dynamic (DHCP).
 * The available options are:
 *     WIFI_STATIC_IP: the device will use a manually specified IP address, subnet mask,
 *                     and default gateway for the Wi-Fi connection.
 *     WIFI_DYNAMIC_IP: the device will obtain the IP address, subnet mask, and default gateway
 *                      automatically from the Wi-Fi network's DHCP server.
 */
#ifndef NRC_WIFI_IP_MODE
#define NRC_WIFI_IP_MODE	WIFI_DYNAMIC_IP
#endif /* NRC_WIFI_IP_MODE */


/**
 * When establishing a network connection, a remote IP address needs to be specified.
 * This is the IP address of the remote device or server that the local device will
 * be connecting to.
 */
#ifndef NRC_REMOTE_ADDRESS
#define NRC_REMOTE_ADDRESS "192.168.200.1"
#endif /* NRC_REMOTE_ADDRESS */


/**
 * When establishing a network connection, a remote port number needs to be specified.
 * This is the port number on the remote device or server that the local device will
 * be connecting to.
 */
#ifndef NRC_REMOTE_PORT
#define NRC_REMOTE_PORT		8099
#endif /* NRC_REMOTE_PORT */


/**
 * When scanning for available Wi-Fi Access Points (APs), it's possible to specify
 * the frequency channels to scan on. This can be useful in situations where only certain
 * frequency bands are supported or where interference from other devices is present.
 * By specifying the frequency channels to scan on, the scanning process can be optimized
 * to reduce the amount of time and power required to complete the scan.
 */
#ifndef NRC_WIFI_SCAN_LIST
#define NRC_WIFI_SCAN_LIST 0
#endif /* NRC_WIFI_SCAN_LIST */


/**
 * The SCAN_CHANNEL_LIST is a configuration setting that specifies the list of
 * frequencies to scan on when searching for available Wi-Fi Access Points (APs).
 *
 *  It's used to optimize the scanning process for specific frequency bands or
 *  to avoid interference from other devices operating on nearby frequencies.
 *
 * The SCAN_CHANNEL_LIST can be set to a comma-separated list of frequency values
 * (in MHz) that are supported by the device's Wi-Fi chipset. For example, setting
 * this value to "9255,9275,9270" would cause the scan to only search for APs on
 * the 900MHz frequency band.
 *
 * Note that the specific values that can be used for this setting will depend on
 * the capabilities of the device's Wi-Fi chipset and the regulatory domain in
 * which it is operating.
 */
#ifndef SCAN_CHANNEL_LIST
#define SCAN_CHANNEL_LIST "9025,9035,9045,9055,9065,9075,9085,9095,9105,9115,\
		9125,9135,9145,9155,9165,9175,9185,9195,9205,9215,\
		9225,9235,9245,9255,9265,9275,9030,9050,9070,9090,\
		9110,9130,9150,9170,9190,9210,9230,9250,9270,9060,\
		9100,9140,9180,9220,9260"
#endif /* SCAN_CHANNEL_LIST */


/* Tx power value (dBm) */
#ifndef TX_POWER
#define TX_POWER 24
#endif /* TX_POWER */


/**
 * The Tx power type can be configured for the Wi-Fi radio.
 *
 * There are three possible settings for the Tx power type:
 *     Auto(0): The device will automatically adjust its Tx power based on the
 *              current network conditions and signal strength.
 *     Limit(1): The device will use a specified maximum Tx power limit.
 *
 *     Fixed(2): The device will use a fixed Tx power level, which can be useful
 *               for testing or for applications where a consistent power level is required.
 * The AUTO (0) and LIMIT (1) options operate auto TX gain adjustment using board data file.
 */
#ifndef TX_POWER_TYPE
#define TX_POWER_TYPE 1
#endif /* TX_POWER_TYPE */


/**
 * The NRC_WIFI_PASSWORD configuration setting specifies the Wi-Fi password to use
 * when connecting to a secure network.
 * This setting is used when the NRC_WIFI_SECURE configuration setting is set
 * to WIFI_SEC_WPA2 or WIFI_SEC_WPA3_SAE. If this setting is not overridden,
 * the default password of "12345678" will be used.
 */
#ifndef NRC_WIFI_PASSWORD
#define NRC_WIFI_PASSWORD  "12345678"
#endif /* NRC_WIFI_PASSWORD */


/**
 * The NRC_STATIC_IP, NRC_NETMASK, and NRC_GATEWAY configuration settings are used
 * to specify the static IP address, netmask, and gateway to use when connecting
 * to a network in static IP mode.
 *
 * These settings are used when the NRC_WIFI_IP_MODE configuration setting is
 * set to WIFI_STATIC_IP. If these settings are not overridden, the default
 * IP address of "192.168.200.13", netmask of "255.255.255.0", and gateway
 * of "192.168.200.1" will be used.
 */
#ifndef NRC_STATIC_IP
#define NRC_STATIC_IP "192.168.200.13"
#endif /* NRC_STATIC_IP */

#ifndef NRC_NETMASK
#define NRC_NETMASK "255.255.255.0"
#endif /* NRC_NETMASK */

#ifndef NRC_GATEWAY
#define NRC_GATEWAY "192.168.200.1"
#endif /* NRC_GATEWAY */


/**
 * The NRC_STATIC_IP6 setting is a configuration option that specifies a static IPv6
 * address to use for the Wi-Fi connection. If this setting is not defined, the default
 * value will be used.
 *
 * If a static IPv6 address is specified, it will be used instead of obtaining an
 * address automatically from the Wi-Fi network's DHCPv6 server.
 *
 * This setting can be overridden by configuring the "wlan0_ip6" NVS key for the device.
 * See wifi_common/nvs_config.h for more information on how to configure NVS settings
 * for the device's Wi-Fi connection.
 */
#ifndef NRC_STATIC_IP6
#define NRC_STATIC_IP6 "fc00::c0a8:c80d"
#endif /* NRC_STATIC_IP6 */


/**
 * The DHCP server can be enabled or disabled for the SoftAP Wi-Fi network.
 * Enabling the DHCP server allows devices that connect to the SoftAP network to obtain
 * IP addresses automatically from the device acting as the SoftAP.
 *
 * This can be useful for creating ad-hoc networks or for providing temporary network
 * access to other devices.Disabling the DHCP server will require devices that connect to
 * the SoftAP network to use a static IP address that is manually configured.
 * By default, the DHCP server is enabled for SoftAP networks.
 */
#ifndef NRC_WIFI_SOFTAP_DHCP_SERVER
#define NRC_WIFI_SOFTAP_DHCP_SERVER 1
#endif /* NRC_WIFI_SOFTAP_DHCP_SERVER */


/**
 * The NRC_WIFI_CHANNEL setting is used to specify the default operating channel
 * The value "0" indicates that no dedicated channel has been assigned.
 * This configuration setting is optional and if it is not defined,
 * the default value of 0 will be used.
 */
#ifndef NRC_WIFI_CHANNEL
#define NRC_WIFI_CHANNEL 0
#endif /* NRC_WIFI_CHANNEL */


/**
 * This code sets the default channel bandwidth for an access point (AP).
 * The channel bandwidth determines the amount of frequency spectrum that will be
 * used to transmit data over the wireless network.
 * The available options for bandwidth are 0(Auto), 1(WIFI_1M), 2(WIFI_2M), and 4(WIFI_4M).
 * This parameter applies only to Japan, and bandwidth must be set at 924.5MHz and 925.5MHz.
 */
#ifndef NRC_AP_SET_CHANNEL_BW
#define NRC_AP_SET_CHANNEL_BW 0
#endif /* NRC_AP_SET_CHANNEL_BW */


/**
 * This code snippet defines the default scan mode used for WiFi scanning.
 * WiFi scanning allows a device to discover nearby WiFi access points (APs) and
 * retrieve their information such as SSID, signal strength, security protocols, and more.
 * The scan method is set to WIFI_SCAN_NORMAL by default, which performs a standard WiFi scan.
 * Other available options include WIFI_SCAN_PASSIVE, which performs a passive scan with lower
 * power consumption, and WIFI_SCAN_FAST, which performs a faster scan with less accuracy.
 * By setting a default scan mode, the device can perform WiFi scans automatically
 * without requiring the user to specify the scan mode each time.
 */
#ifndef WIFI_SCAN_MODE
#define WIFI_SCAN_MODE	WIFI_SCAN_NORMAL
#endif /* WIFI_SCAN_MODE */


/**
 * This code block defines the timeout period for WiFi connection attempts. The value of
 * WIFI_CONN_TIMEOUT is set to 0, which means that the connection attempts will continue
 * indefinitely until a successful connection is made or an error occurs.
 * WIFI_CONN_TIMEOUT to the desired value (in milliseconds) in your code.
 */
#ifndef WIFI_CONN_TIMEOUT
#define WIFI_CONN_TIMEOUT	0 /* infinite */
#endif /* WIFI_CONN_TIMEOUT */


/**
 * This code block defines the timeout period for WiFi disconnection attempts. The value of
 * WIFI_DISCONN_TIMEOUT is set to 0, which means that the disconnection attempts will continue
 * indefinitely until a successful disconnection is made or an error occurs.
 * WIFI_DISCONN_TIMEOUT to the desired value (in milliseconds) in your code.
 */
#ifndef WIFI_DISCONN_TIMEOUT
#define WIFI_DISCONN_TIMEOUT	0 /* infinite */
#endif /* WIFI_DISCONN_TIMEOUT */

/**
 * In wireless networking, an AP is considered idle if there are no active connections to it.
 * The WIFI_BSS_MAX_IDLE directive sets the time duration in seconds that an AP will remain active
 * after the last connection has been disconnected.
 * Once the idle time exceeds the maximum value set in this directive, the AP will be automatically
 * disconnected to free up network resources.
 */
#ifndef WIFI_BSS_MAX_IDLE
#define WIFI_BSS_MAX_IDLE	0 /* bss_max_idle unit(second) */
#endif /* WIFI_BSS_MAX_IDLE */

/**
 * This code sets the maximum number of retries for checking the keep alive packet.
 * The default value is 3, which means that if the access point does not receive the keep alive packet
 * after 3 times the maximum idle time, the system will retry up to three times before giving up.
 */
#ifndef WIFI_BSS_RETRY_CNT
#define WIFI_BSS_RETRY_CNT	3 /* retry count */
#endif /* WIFI_BSS_RETRY_CNT */


/**
 * The minimum transmit power for Wi-Fi. The default value is 1.
 */
#ifndef WIFI_MIN_TXPOWER
#define WIFI_MIN_TXPOWER 1
#endif /* WIFI_MIN_TXPOWER */


/**
 * The maximum transmit power for Wi-Fi. The default value is 20.
 */
#ifndef WIFI_MAX_TXPOWER
#define WIFI_MAX_TXPOWER 20
#endif /* WIFI_MAX_TXPOWER */


/**
 * The device mode STA(0) or AP(1) for Wi-Fi. The default value is STA(0).
 */
#ifndef WIFI_DEVICE_MODE
#define WIFI_DEVICE_MODE 0
#endif /* WIFI_DEVICE_MODE */


/**
 * The network mode Bridge(0) or NAT(1) for Wi-Fi. The default value is NAT(1).
 */
#ifndef WIFI_NETWORK_MODE
#define WIFI_NETWORK_MODE 1
#endif /* WIFI_NETWORK_MODE */


/**
 * The maximum number of attempts to connect to Wi-Fi. The default value is 3.
 */
#ifndef MAX_WIFI_CONNECT_TRIES
#define MAX_WIFI_CONNECT_TRIES 3
#endif /* MAX_WIFI_CONNECT_TRIES */


/**
 * The maximum number of attempts to initialize Wi-Fi
 */
#ifndef MAX_WIFI_INIT_TRIES
#define MAX_WIFI_INIT_TRIES 64
#endif /* MAX_WIFI_INIT_TRIES */


/**
 * Beacon interval in TU(Time Unit, 1TU = 1024us).
 * The default value is 100TU. (range 15..65535)
 */
#ifndef NRC_WIFI_BCN_INTERVAL
#define NRC_WIFI_BCN_INTERVAL 100
#endif /* NRC_WIFI_BCN_INTERVAL */


/**
 * The mode for Wi-Fi. The default value is WIFI_MODE_AP.
 */
#ifndef NRC_WIFI_MODE_DEFAULT
#define NRC_WIFI_MODE_DEFAULT WIFI_MODE_AP
#endif /* NRC_WIFI_MODE_DEFAULT */


/**
 * The network mode for Wi-Fi. The default value is WIFI_NETWORK_MODE_NAT.
 */
#ifndef NRC_WIFI_NETWORK_MODE_DEFAULT
#define NRC_WIFI_NETWORK_MODE_DEFAULT WIFI_NETWORK_MODE_NAT
#endif /* NRC_WIFI_NETWORK_MODE_DEFAULT */


/**
 * Wi-Fi Rate Control: Controls the rate at which data is transmitted over Wi-Fi.
 */
#ifndef NRC_WIFI_RATE_CONTROL
#define NRC_WIFI_RATE_CONTROL 1
#endif /* NRC_WIFI_RATE_CONTROL */


/**
 * Wi-Fi MCS (Modulation and Coding Scheme): Determines the data rate and error correction
 * capability for Wi-Fi transmissions. Applied only when rate control is disabled.
 */
#ifndef NRC_WIFI_MCS_DEFAULT
#define NRC_WIFI_MCS_DEFAULT 10
#endif /* NRC_WIFI_MCS_DEFAULT*/


/**
 * Wi-Fi CCA (Clear Channel Assessment) Threshold: Specifies the signal strength threshold
 * below which Wi-Fi considers the channel to be clear and available for transmission.
 */
#ifndef NRC_WIFI_CCA_THRES_DEFAULT
#define NRC_WIFI_CCA_THRES_DEFAULT -75
#endif /* NRC_WIFI_CCA_THRES_DEFAULT */

/**
 * Wi-Fi Guard Interval(GI) Type
 * The network mode Long GI(0) or Short GI(1) for Wi-Fi. The default value is Long GI(0)
 */
#ifndef NRC_WIFI_GUARD_INTERVAL_DEFAULT
#define NRC_WIFI_GUARD_INTERVAL_DEFAULT 0
#endif /* NRC_WIFI_GUARD_INTERVAL_DEFAULT*/

/**
 * Wi-Fi ignore_broadcast_ssid - Hide SSID in AP mode
 *
 * This setting controls the behavior of the Access Point (AP) regarding SSID broadcast.
 * When enabled, the AP will send empty SSID in beacons and ignore probe request frames
 * that do not specify the full SSID, thus requiring stations to know the SSID.
 *
 * - 0: Probe requests for broadcast SSID are not ignored. It sends the SSID in beacons. (default)
 * - 1: Send an empty (length=0) SSID in beacons and ignore probe requests for broadcast SSID.
 * - 2: Clear SSID (ASCII 0), but keep the original length. (this may be required
 *       with some clients that do not support empty SSID) and ignore probe
 *      requests for broadcast SSID
 */
#ifndef NRC_WIFI_IGNORE_BROADCAST_SSID_DEFAULT
#define NRC_WIFI_IGNORE_BROADCAST_SSID_DEFAULT 0
#endif /* NRC_WIFI_IGNORE_BROADCAST_SSID_DEFAULT */

/**
 * Maximum number of stations allowed in softAP (upto 10)
 */
#ifndef NRC_WIFI_SOFTAP_MAX_NUM_STA_DEFAULT
#define NRC_WIFI_SOFTAP_MAX_NUM_STA_DEFAULT 10
#endif /* NRC_WIFI_SOFTAP_MAX_NUM_STA_DEFAULT */

/**
 * Wi-Fi isten Interval
 * Listen Interval Time (us) = listen_interval * beacon_interval * 1TU (1024 us)
 * This value should be configured when enabling the power-saving operation
 * and listen interval time must be larger than sleep duratioon
 */
#ifndef NRC_WIFI_LISTEN_INTERVAL_DEFAULT
#define NRC_WIFI_LISTEN_INTERVAL_DEFAULT 0
#endif /* NRC_WIFI_LISTEN_INTERVAL_DEFAULT*/


/*************************************************************
 * VIF1 configurations
 * These are the interface-specific configurations for VIF1
 *************************************************************/
#ifndef VIF1_STR_SSID
#define VIF1_STR_SSID "halow_demo_1"
#endif /* VIF1_STR_SSID */

#ifndef VIF1_NRC_WIFI_SECURE
#define VIF1_NRC_WIFI_SECURE   WIFI_SEC_OPEN
#endif /* VIF1_NRC_WIFI_SECURE */

#ifndef VIF1_NRC_WIFI_IP_MODE
#define VIF1_NRC_WIFI_IP_MODE	WIFI_DYNAMIC_IP
#endif /* VIF1_NRC_WIFI_IP_MODE */

#ifndef VIF1_NRC_REMOTE_ADDRESS
#define VIF1_NRC_REMOTE_ADDRESS "192.168.200.1"
#endif /* VIF1_NRC_REMOTE_ADDRESS */

#ifndef VIF1_NRC_REMOTE_PORT
#define VIF1_NRC_REMOTE_PORT		8099
#endif /* VIF1_NRC_REMOTE_PORT */

#ifndef VIF1_NRC_WIFI_PASSWORD
#define VIF1_NRC_WIFI_PASSWORD  "12345678"
#endif /* VIF1_NRC_WIFI_PASSWORD */

#ifndef VIF1_NRC_STATIC_IP
#define VIF1_NRC_STATIC_IP "192.168.200.14"
#endif /* VIF1_NRC_STATIC_IP */

#ifndef VIF1_NRC_NETMASK
#define VIF1_NRC_NETMASK "255.255.255.0"
#endif /* VIF1_NRC_NETMASK */

#ifndef VIF1_NRC_GATEWAY
#define VIF1_NRC_GATEWAY "192.168.200.1"
#endif /* VIF1_NRC_GATEWAY */

#ifndef VIF1_NRC_STATIC_IP6
#define VIF1_NRC_STATIC_IP6 "fc00::c0a8:c80e"
#endif /* VIF1_NRC_STATIC_IP6 */

#ifndef VIF1_WIFI_DEVICE_MODE
#define VIF1_WIFI_DEVICE_MODE 0
#endif /* VIF1_WIFI_DEVICE_MODE */

#ifndef VIF1_WIFI_NETWORK_MODE
#define VIF1_WIFI_NETWORK_MODE 1
#endif /* VIF1_WIFI_NETWORK_MODE */

#ifndef VIF1_NRC_WIFI_NETWORK_MODE_DEFAULT
#define VIF1_NRC_WIFI_NETWORK_MODE_DEFAULT WIFI_NETWORK_MODE_NAT
#endif /* VIF1_NRC_WIFI_NETWORK_MODE_DEFAULT */

#ifndef VIF1_NRC_WIFI_RATE_CONTROL
#define VIF1_NRC_WIFI_RATE_CONTROL 1
#endif /* VIF1_NRC_WIFI_RATE_CONTROL */

#endif /* __WIFI_CONFIG_H__ */
