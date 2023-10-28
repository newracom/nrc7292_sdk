/*
 * MIT License
 *
 * Copyright (c) 2023 Newracom, Inc.
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

#include <nrc_sdk.h>
#include <nvs.h>
#include <nvs_config.h>
#include "build_ver.h"
#include "lmac_config.h"
#include "umac_s1g_config.h"
#include "lmac_rate_control.h"
#include "driver_nrc.h"
#ifndef NRC7292
#include "nrc_sflash.h"
#endif

int nrc_ctrl_show_config(char *config, size_t *length)
{
	if ((!config) || (!length)) {
		return -1;
	}

	memset(config, 0, *length);

	int vif_id = 0;
	int tx_pwr, tx_gain, rx_gain, frequency, mac_frequency, mcs=0, rts_threshold, color, cc_id;
	char device_mode[16], mac_addr[30], country[10], bandwidth[10], rate_control[5];
	char short_gi[6], security[5], akm[15], type[5], rts[5],  bw[24], format[20], preamble[20], center_lo[5];
	char promiscuous_mode[5], auto_cal[20], bssid[30], aid[30];
	char *str_cyper[6] ={"WEP40", "WEP104", "TKIP", "CCMP", "CCMP256", "WAPI"};

	tx_pwr = hal_rf_get_txpwr();
	tx_gain = get_phy_txgain();
	rx_gain =  get_phy_rxgain();
	frequency = get_frequency();
	mac_frequency = get_mac80211_frequency();
	rts_threshold = get_rts_threshold(vif_id);
	color =  umac_s1g_config_get_color();
	cc_id = lmac_get_country_code_index();

	sprintf(device_mode, "%s", get_device_mode(vif_id)? "AP" : get_device_mode(vif_id) == 2? "MESH-POINT": "STA");
	sprintf(mac_addr, "%02x:%02x:%02x:%02x:%02x:%02x",
			MAC2STR(get_macaddr(vif_id)));
	sprintf(bandwidth, "%s", get_ch_bw()== BW_1M? "1M" : get_ch_bw()==BW_2M? "2M" : "4M");
	sprintf(center_lo, "%s", (get_1m_center_lo(cc_id))?"ON":"OFF");
	sprintf(rate_control, "%s",get_rate_ctrl_en(vif_id)? "ON" : "OFF");
	sprintf(short_gi, "%s", get_short_gi(vif_id)? "SHORT":get_auto_gi_flag()?"AUTO":"LONG");
	sprintf(security, "%s", get_cipher_en(vif_id)? "ON":"OFF");
	sprintf(akm, "%s", get_akm(vif_id)==1? "802.1X(128)":get_akm(vif_id)==2?"WPA2-PSK":
			get_akm(vif_id)==5?"802.1X(256)":get_akm(vif_id)==8?"WPA3-SAE":
			get_akm(vif_id)==18?"WPA3-OWE":"Unknown");
	sprintf(rts, "%s", get_rts(vif_id) ? "ON" : "OFF");
	sprintf(format, "%s", !get_format(vif_id)? "S1G":(get_format(vif_id) == 1)?"S1G_DUP_1M":"S1G_DUP_2M");
	sprintf(preamble, "%s", get_ch_bw()==BW_1M? "1M":(!get_preamble_type(vif_id)? "S1G_SHORT":"S1G_LONG"));
	sprintf(promiscuous_mode, "%s", get_promiscuous_mode(vif_id)? "ON":"OFF");
	sprintf(auto_cal, "%s", get_common_cfo_cal_en()? "ON" : "OFF");
	sprintf(country, "%s", lmac_get_country());

	if(strlen(country) == 0) {
		sprintf(country, "%s"," ");
	}

	if (!get_rate_ctrl_en(vif_id)) {
		mcs = get_mcs();
		sprintf(bw, "%s","N/A");
	} else {
		const RC_RATE *rate;
#if !defined (INCLUDE_RC_W_RSSI)
	uint8_t rate_index	= lmac_rc_get_rate_index( vif_id , 0 , 0) ;
#else
	uint8_t rate_index	= lmac_rc_get_rate_index( vif_id , 0 , 0, 0, 0) ;
#endif
		rate = lmac_rc_get_rate(rate_index);
		mcs = rate->mcs;
		sprintf(bw, "%d Mhz (NRC Auto)", 1 << rate->bw );
	}

	if(get_cipher_en(vif_id)){
		sprintf(type, "%s", str_cyper[get_cipher_type(vif_id, KEY_GTK)]);
	}else{
		sprintf(type, "%s", "N/A");
	}

	if (get_device_mode(vif_id) == MODE_STA && get_aid(vif_id)) {
		sprintf(bssid, MACSTR, MAC2STR(lmac_get_bssid(vif_id)));
		sprintf(aid, "%d", get_aid(vif_id) );
	}else{
		sprintf(bssid, "%s", "N/A");
		sprintf(aid, "%s", "N/A");
	}

	sprintf(config + strlen(config), "==================================\n");
	sprintf(config + strlen(config), "    MAC Configuration\n");
	sprintf(config + strlen(config), "----------------------------------\n");
	sprintf(config + strlen(config), " Device Mode   : %s\n", device_mode);
	sprintf(config + strlen(config), " MAC Address   : %s\n", mac_addr);
	sprintf(config + strlen(config), " Country       : %s\n", country);
	sprintf(config + strlen(config), " Bandwidth     : %s\n", bandwidth);
	if (get_ch_bw()==BW_1M) {
		sprintf(config + strlen(config), "  - Center LO  : %s\n", center_lo);
	}
	sprintf(config + strlen(config), " Frequency     : %d\n", frequency);
	sprintf(config + strlen(config), " MAC80211_freq : %d\n", mac_frequency);
	sprintf(config + strlen(config), " Rate Control  : %s\n", rate_control);
	if (!get_rate_ctrl_en(vif_id)) {
		sprintf(config + strlen(config), "  - MCS : %d\n", mcs);
	}else {
		sprintf(config + strlen(config), "  - MCS : %d , bw = %s\n" , mcs , bw);
	}
	sprintf(config + strlen(config), " Guard Interval: %s\n", short_gi);
	sprintf(config + strlen(config), " Security      : %s\n", security);
	if(get_cipher_en(vif_id)) {
		sprintf(config + strlen(config), "  - AKM  : %s\n", akm);
		sprintf(config + strlen(config), "  - Type : %s\n", type);
	}
	sprintf(config + strlen(config), " RTS           : %s\n", rts);
	sprintf(config + strlen(config), " RTS threshold : %d\n", rts_threshold);
	sprintf(config + strlen(config), " Format        : %s\n", format);
	sprintf(config + strlen(config), " preamble type : %s\n", preamble);
	sprintf(config + strlen(config), " Promiscuous Mode : %s\n", promiscuous_mode);
	sprintf(config + strlen(config), " Color:0x%x\n", color);
	sprintf(config + strlen(config), " Auto CFO Cal  : %s\n", auto_cal);

	if (get_device_mode(vif_id) == MODE_STA && get_aid(vif_id)) {
		sprintf(config + strlen(config), "----------------------------------\n");
		sprintf(config + strlen(config), " BSSID : %s\n", bssid);
		sprintf(config + strlen(config), " AID   : %s\n", aid);
	}
	sprintf(config + strlen(config), "----------------------------------\n");
	sprintf(config + strlen(config), "    PHY Configuration\n");
	sprintf(config + strlen(config), "----------------------------------\n");
	sprintf(config + strlen(config), " Tx_Gain:%d, Rx_Gain:%d\n", tx_gain, rx_gain);
    sprintf(config + strlen(config), " Tx Power:%d\n", tx_pwr);
	sprintf(config + strlen(config), "----------------------------------\n");

	if (strlen(config) > *length) {
		return -1;
	}
	*length = strlen(config);
	return 0;
}

int nrc_ctrl_show_sysconfig(char *config, size_t *length)
{
	sf_slot_t slot_info;
	sf_sys_config_t sys_config;
	size_t slot_length;
#ifdef NRC7292
	char* mac_addr0 = (char* )sys_config.mac_addr;
	char* mac_addr1 = (char* )sys_config.mac_addr_1;
#else
	char* mac_addr0 = (char* )sys_config.mac_addr0;
	char* mac_addr1 = (char* )sys_config.mac_addr1;
#endif
	char tmp_buf[sizeof(sys_config.user_factory)+1] = {0,};
	if(nrc_sf_read_slot_info(SF_SYSTEM_CONFIG, &slot_info)) {
		slot_length = MIN(sizeof(sf_sys_config_t), slot_info.length);
		nrc_sf_read_slot_data(SF_SYSTEM_CONFIG, slot_length, (uint8_t*)&sys_config);
		sprintf(config + strlen(config), "----------------------------------------\n");
		sprintf(config + strlen(config), "[sys_config information]\n");
		sprintf(config + strlen(config), "----------------------------------------\n");
		sprintf(config + strlen(config), "version    : %08x\n", (unsigned int) sys_config.version);
		sprintf(config + strlen(config), "mac_addr[0]: " MACSTR "\n", MAC2STR(mac_addr0));
		sprintf(config + strlen(config), "mac_addr[1]: " MACSTR "\n", MAC2STR(mac_addr1));
		sprintf(config + strlen(config), "cal_use    : %s\n", (sys_config.cal_use==1)?"on":"off");
		sprintf(config + strlen(config), "hw_version : %d\n", sys_config.hw_version);
#ifdef NRC7292
		if ((sys_config.version > 0) && (sys_config.version != 0xFFFFFFFF)) {
			sprintf(config + strlen(config), "rf_pllldo12_tr: 0x%08X (%s)\n", (unsigned int) sys_config.rf_pllldo12_tr.value,
			  (sys_config.rf_pllldo12_tr.control==1)?"Use":"Disabled");
		}
#endif
		sprintf(config + strlen(config), "----------------------------------------\n");
	} else {
		sprintf(config + strlen(config), "sysconfig is invalid.\n");
	}

	if (strlen(config) > *length) {
		return -1;
	}

	*length = strlen(config);
	return 0;
}

int nrc_ctrl_show_apinfo(char *info, size_t *length)
{
	AP_INFO ap_info;

	memset(info, 0, *length);

	if (nrc_wifi_get_ap_info(0, &ap_info) != WIFI_SUCCESS) {
		return -1;
	}

	if(ap_info.ssid_len != 0) {
		sprintf(info + strlen(info), "BSSID     : "MACSTR"\n", MAC2STR(ap_info.bssid));
		sprintf(info + strlen(info), "SSID      : %s\n", ap_info.ssid);
		sprintf(info + strlen(info), "country   : '%c%c'\n", ap_info.cc[0], ap_info.cc[1]);
		sprintf(info + strlen(info), "channel   : %d\n", ap_info.ch);
		sprintf(info + strlen(info), "frequency : %d\n", ap_info.freq);
		sprintf(info + strlen(info), "bandwidth : %d\n", ap_info.bw);
		sprintf(info + strlen(info), "security  : %d\n", ap_info.security);
	} else {
		return -1;
	}

	if (strlen(info) > *length) {
		return -1;
	}
	*length = strlen(info);
	return 0;
}

int nrc_ctrl_get_version(char *version, size_t *length)
{
	size_t size;

	if ((!version) || (!length)) {
		return -1;
	}

	memset(version, 0, *length);

	if ((size = snprintf(version, *length,
						 "Newracom Firmware Version : %02u.%02u.%02u\n"
						 "%s:%s\n"
						 "Compiled on %s at %s\n"
						 , (VERSION_MAJOR), (VERSION_MINOR), (VERSION_REVISION)
						 , (VERSION_BRANCH), (VERSION_BUILD)
						 , __DATE__, __TIME__)) > *length) {
		return -1;
	}

	*length = size;
	return 0;
}

int nrc_ctrl_get_ssid(char *ssid, size_t *length)
{
	nvs_handle_t nvs_handle = 0;
	int retval = 0;

	memset(ssid, 0, *length);

	if (nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READWRITE, &nvs_handle) != NVS_OK) {
		return -1;
	}

	if (nvs_get_str(nvs_handle, NVS_SSID, ssid, length) != NVS_OK) {
		retval = -1;
	}

	if (nvs_handle)
		nvs_close(nvs_handle);

	return retval;
}

int nrc_ctrl_set_ssid(char *ssid)
{
	nvs_handle_t nvs_handle = 0;
	int retval = 0;

	if (nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READWRITE, &nvs_handle) != NVS_OK) {
		return -1;
	}

	if (nvs_set_str(nvs_handle, NVS_SSID, ssid) != NVS_OK) {
		retval = -1;
	}

	if (nvs_handle)
		nvs_close(nvs_handle);

	return retval;
}

int nrc_ctrl_get_security(uint8_t *security)
{
	nvs_handle_t nvs_handle = 0;
	int retval = 0;

	if (nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READWRITE, &nvs_handle) != NVS_OK) {
		return -1;
	}

	if (nvs_get_u8(nvs_handle, NVS_WIFI_SECURITY, security) != NVS_OK) {
		retval = -1;
	}

	if (nvs_handle)
		nvs_close(nvs_handle);

	return retval;
}

int nrc_ctrl_set_security(uint8_t security)
{
	nvs_handle_t nvs_handle = 0;
	int retval = 0;

	if (security > 3) {
		return -1;
	}
	if (nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READWRITE, &nvs_handle) != NVS_OK) {
		return -1;
	}

	if (nvs_set_u8(nvs_handle, NVS_WIFI_SECURITY, security) != NVS_OK) {
		retval = -1;
	}

	if (nvs_handle)
		nvs_close(nvs_handle);

	return retval;
}

int nrc_ctrl_get_password(char *password, size_t *length)
{
	nvs_handle_t nvs_handle = 0;
	int retval = 0;

	if (nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READWRITE, &nvs_handle) != NVS_OK) {
		return -1;
	}

	if (nvs_get_str(nvs_handle, NVS_WIFI_PASSWORD, password, length) != NVS_OK) {
		retval = -1;
	}

	if (nvs_handle)
		nvs_close(nvs_handle);

	return retval;
}

int nrc_ctrl_set_password(char *password)
{
	nvs_handle_t nvs_handle = 0;
	int retval = 0;

	if (nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READWRITE, &nvs_handle) != NVS_OK) {
		return -1;
	}

	if (nvs_set_str(nvs_handle, NVS_WIFI_PASSWORD, password) != NVS_OK) {
		retval = -1;
	}

	if (nvs_handle)
		nvs_close(nvs_handle);

	return retval;
}

int nrc_ctrl_get_country(char *country, size_t *length)
{
	nvs_handle_t nvs_handle = 0;
	int retval = 0;

	if (nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READWRITE, &nvs_handle) != NVS_OK) {
		return -1;
	}

	if (nvs_get_str(nvs_handle, NVS_COUNTRY, country, length) != NVS_OK) {
		retval = -1;
	}

	if (nvs_handle)
		nvs_close(nvs_handle);

	return retval;
}

int nrc_ctrl_set_country(char *country)
{
	nvs_handle_t nvs_handle = 0;
	int retval = 0;

	if (nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READWRITE, &nvs_handle) != NVS_OK) {
		return -1;
	}

	if (nvs_set_str(nvs_handle, NVS_COUNTRY, country) != NVS_OK) {
		retval = -1;
	}

	if (nvs_handle)
		nvs_close(nvs_handle);

	return retval;
}

int nrc_ctrl_get_ipmode(uint8_t *ipmode)
{
	nvs_handle_t nvs_handle = 0;
	int retval = 0;

	if (nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READWRITE, &nvs_handle) != NVS_OK) {
		return -1;
	}

	if (nvs_get_u8(nvs_handle, NVS_IP_MODE, ipmode) != NVS_OK) {
		retval = -1;
	}

	if (nvs_handle)
		nvs_close(nvs_handle);

	return retval;
}

int nrc_ctrl_set_ipmode(uint8_t ipmode)
{
	nvs_handle_t nvs_handle = 0;
	int retval = 0;

	if (ipmode > 1) {
		return -1;
	}
	if (nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READWRITE, &nvs_handle) != NVS_OK) {
		return -1;
	}

	if (nvs_set_u8(nvs_handle, NVS_IP_MODE, ipmode) != NVS_OK) {
		retval = -1;
	}

	if (nvs_handle)
		nvs_close(nvs_handle);

	return retval;
}

int nrc_ctrl_get_conn_timeout(int32_t *timeout)
{
	nvs_handle_t nvs_handle = 0;
	int retval = 0;

	if (nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READWRITE, &nvs_handle) != NVS_OK) {
		return -1;
	}

	if (nvs_get_i32(nvs_handle, NVS_WIFI_CONN_TIMEOUT, timeout) != NVS_OK) {
		retval = -1;
	}

	if (nvs_handle)
		nvs_close(nvs_handle);

	return retval;
}

int nrc_ctrl_set_conn_timeout(int32_t timeout)
{
	nvs_handle_t nvs_handle = 0;
	int retval = 0;

	if (nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READWRITE, &nvs_handle) != NVS_OK) {
		return -1;
	}

	if (nvs_set_u8(nvs_handle, NVS_WIFI_CONN_TIMEOUT, timeout) != NVS_OK) {
		retval = -1;
	}

	if (nvs_handle)
		nvs_close(nvs_handle);

	return retval;
}

int nrc_ctrl_get_disconn_timeout(int32_t *timeout)
{
	nvs_handle_t nvs_handle = 0;
	int retval = 0;

	if (nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READWRITE, &nvs_handle) != NVS_OK) {
		return -1;
	}

	if (nvs_get_i32(nvs_handle, NVS_WIFI_DISCONN_TIMEOUT, timeout) != NVS_OK) {
		retval = -1;
	}

	if (nvs_handle)
		nvs_close(nvs_handle);

	return retval;
}

int nrc_ctrl_set_disconn_timeout(int32_t timeout)
{
	nvs_handle_t nvs_handle = 0;
	int retval = 0;

	if (nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READWRITE, &nvs_handle) != NVS_OK) {
		return -1;
	}

	if (nvs_set_u8(nvs_handle, NVS_WIFI_DISCONN_TIMEOUT, timeout) != NVS_OK) {
		retval = -1;
	}

	if (nvs_handle)
		nvs_close(nvs_handle);

	return retval;
}

int nrc_ctrl_set_mcs(uint8_t mcs)
{
	nvs_handle_t nvs_handle = 0;
	int retval = 0;

	nrc_usr_print("[%s] mcs = %s\n", __func__, mcs);
	if (nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READWRITE, &nvs_handle) != NVS_OK) {
		return -1;
	}

	if (nvs_set_u8(nvs_handle, NVS_WIFI_MCS, mcs) != NVS_OK) {
		retval = -1;
	}

	if (nvs_handle)
		nvs_close(nvs_handle);

	nrc_wifi_set_mcs( mcs);

	return retval;
}


int nrc_ctrl_get_mcs(uint8_t* mcs)
{
	nvs_handle_t nvs_handle = 0;
	int retval = 0;

	if (nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READWRITE, &nvs_handle) != NVS_OK) {
		return -1;
	}

	if (nvs_get_u8(nvs_handle, NVS_WIFI_MCS, mcs) != NVS_OK) {
		retval = -1;
	}

	if (nvs_handle)
		nvs_close(nvs_handle);

	return retval;
}

int nrc_ctrl_set_rc(uint8_t rc)
{
	nvs_handle_t nvs_handle = 0;
	int retval = 0;

	nrc_usr_print("[%s] rc = %s\n", __func__, rc);
	if (nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READWRITE, &nvs_handle) != NVS_OK) {
		return -1;
	}

	if (nvs_set_u8(nvs_handle, NVS_WIFI_RATE_CONTROL, rc) != NVS_OK) {
		retval = -1;
	}

	if (nvs_handle)
		nvs_close(nvs_handle);

	nrc_wifi_set_rate_control(0, (bool)rc);

	return retval;
}


int nrc_ctrl_get_rc(uint8_t* rc)
{
	nvs_handle_t nvs_handle = 0;
	int retval = 0;

	if (nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READWRITE, &nvs_handle) != NVS_OK) {
		return -1;
	}

	if (nvs_get_u8(nvs_handle, NVS_WIFI_RATE_CONTROL, rc) != NVS_OK) {
		retval = -1;
	}

	if (nvs_handle)
		nvs_close(nvs_handle);

	return retval;
}

int nrc_ctrl_set_cca_thres(int8_t cca_thres)
{
	nvs_handle_t nvs_handle = 0;
	int retval = 0;

	nrc_usr_print("[%s] cca_thres = %s\n", __func__, cca_thres);
	if (nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READWRITE, &nvs_handle) != NVS_OK) {
		return -1;
	}

	if (nvs_set_i8(nvs_handle, NVS_WIFI_CCA_THRES, cca_thres) != NVS_OK) {
		retval = -1;
	}

	if (nvs_handle)
		nvs_close(nvs_handle);

	nrc_wifi_set_cca_threshold(0, cca_thres);

	return retval;
}


int nrc_ctrl_get_cca_thres(int8_t* cca_thres)
{
	nvs_handle_t nvs_handle = 0;
	int retval = 0;

	if (nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READWRITE, &nvs_handle) != NVS_OK) {
		return -1;
	}

	if (nvs_get_i8(nvs_handle, NVS_WIFI_CCA_THRES, cca_thres) != NVS_OK) {
		retval = -1;
	}

	if (nvs_handle)
		nvs_close(nvs_handle);

	return retval;
}

int nrc_ctrl_get_static_ip(char *ip, size_t *length)
{
	nvs_handle_t nvs_handle = 0;
	int retval = 0;

	if (nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READWRITE, &nvs_handle) != NVS_OK) {
		return -1;
	}

	if (nvs_get_str(nvs_handle, NVS_STATIC_IP, ip, length) != NVS_OK) {
		retval = -1;
	}

	if (nvs_handle)
		nvs_close(nvs_handle);

	return retval;
}

int nrc_ctrl_set_static_ip(char *ip)
{
	nvs_handle_t nvs_handle = 0;
	int retval = 0;

	nrc_usr_print("[%s] ip = %s\n", __func__, ip);
	if (nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READWRITE, &nvs_handle) != NVS_OK) {
		return -1;
	}

	if (nvs_set_str(nvs_handle, NVS_STATIC_IP, ip) != NVS_OK) {
		retval = -1;
	}

	if (nvs_handle)
		nvs_close(nvs_handle);

	return retval;
}

int nrc_ctrl_get_netmask(char *netmask, size_t *length)
{
	nvs_handle_t nvs_handle = 0;
	int retval = 0;

	if (nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READWRITE, &nvs_handle) != NVS_OK) {
		return -1;
	}

	if (nvs_get_str(nvs_handle, NVS_NETMASK, netmask, length) != NVS_OK) {
		retval = -1;
	}

	if (nvs_handle)
		nvs_close(nvs_handle);

	return retval;
}

int nrc_ctrl_set_netmask(char *netmask)
{
	nvs_handle_t nvs_handle = 0;
	int retval = 0;

	nrc_usr_print("[%s] ip = %s\n", __func__, netmask);

	if (nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READWRITE, &nvs_handle) != NVS_OK) {
		return -1;
	}

	if (nvs_set_str(nvs_handle, NVS_NETMASK, netmask) != NVS_OK) {
		retval = -1;
	}

	if (nvs_handle)
		nvs_close(nvs_handle);

	return retval;
}

int nrc_ctrl_get_gateway(char *gateway, size_t *length)
{
	nvs_handle_t nvs_handle = 0;
	int retval = 0;

	if (nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READWRITE, &nvs_handle) != NVS_OK) {
		return -1;
	}

	if (nvs_get_str(nvs_handle, NVS_GATEWAY, gateway, length) != NVS_OK) {
		retval = -1;
	}

	if (nvs_handle)
		nvs_close(nvs_handle);

	return retval;
}

int nrc_ctrl_set_gateway(char *gateway)
{
	nvs_handle_t nvs_handle = 0;
	int retval = 0;

	if (nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READWRITE, &nvs_handle) != NVS_OK) {
		return -1;
	}

	if (nvs_set_str(nvs_handle, NVS_GATEWAY, gateway) != NVS_OK) {
		retval = -1;
	}

	if (nvs_handle)
		nvs_close(nvs_handle);

	return retval;
}

int nrc_ctrl_get_txpower(uint8_t *txpower, uint8_t *txpower_type)
{
	nvs_handle_t nvs_handle = 0;
	int retval = 0;

	if (nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READWRITE, &nvs_handle) != NVS_OK) {
		return -1;
	}

	if (nvs_get_u8(nvs_handle, NVS_WIFI_TX_POWER, txpower) != NVS_OK) {
		retval = -1;
	}

	if (nvs_get_u8(nvs_handle, NVS_WIFI_TX_POWER_TYPE, txpower_type) != NVS_OK) {
		retval = -1;
	}

	if (nvs_handle)
		nvs_close(nvs_handle);

	return retval;
}

int nrc_ctrl_set_txpower(uint8_t txpower, uint8_t txpower_type)
{
	nvs_handle_t nvs_handle = 0;
	int retval = 0;

	if (txpower > 24) {
		return -1;
	}
	if (nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READWRITE, &nvs_handle) != NVS_OK) {
		return -1;
	}

	if (nvs_set_u8(nvs_handle, NVS_WIFI_TX_POWER, txpower) != NVS_OK) {
		retval = -1;
	}

	if (nvs_set_u8(nvs_handle, NVS_WIFI_TX_POWER_TYPE, txpower_type) != NVS_OK) {
		retval = -1;
	}

	if (nvs_handle)
		nvs_close(nvs_handle);

	if (retval == 0) {
		nrc_wifi_set_tx_power(txpower, txpower_type);
	}

	return retval;
}

#ifdef INCLUDE_SCAN_BACKOFF
uint32_t nrc_ctrl_get_scan_max_interval()
{
	return nrc_get_scan_max_interval();
}

int nrc_ctrl_set_scan_max_interval(uint32_t interval)
{
	nvs_handle_t nvs_handle = 0;
	int retval = 0;

	if (nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READWRITE, &nvs_handle) != NVS_OK) {
		return -1;
	}

	if (nvs_set_u32(nvs_handle, NVS_SCAN_MAX_INTERVAL, interval) != NVS_OK) {
		retval = -1;
	}

	if (nvs_handle)
		nvs_close(nvs_handle);

	if (retval == 0) {
		nrc_set_scan_max_interval(interval);
	}
	return retval;
}

uint32_t nrc_ctrl_get_backoff_start_count()
{
	return nrc_get_backoff_start_count();
}

int nrc_ctrl_set_backoff_start_count(uint32_t count)
{
	nvs_handle_t nvs_handle = 0;
	int retval = 0;

	if (nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READWRITE, &nvs_handle) != NVS_OK) {
		return -1;
	}

	if (nvs_set_u32(nvs_handle, NVS_SCAN_BACKOFF_START_COUNT, count) != NVS_OK) {
		retval = -1;
	}

	if (nvs_handle)
		nvs_close(nvs_handle);

	if (retval == 0) {
		nrc_set_backoff_start_count(count);
	}
	return retval;
}
#endif

int nrc_ctrl_nvs_reset()
{
	nvs_handle_t nvs_handle;
	if (nvs_open(NVS_DEFAULT_NAMESPACE, NVS_READWRITE, &nvs_handle) == NVS_OK) {
		if (nvs_erase_all(nvs_handle) != NVS_OK) {
			system_printf("nvs erase failed.\n");
			return -1;
		} else {
			system_printf("nvs erased all.\n");
			return 0;
		}
		nvs_close(nvs_handle);
	} else {
		system_printf("nvs open failed.\n");
		return -1;
	}
}

static void sys_reboot_task(void *pvParameters)
{
	for (int i = 0; i < 5; i++) {
		nrc_usr_print("System reboot in %d sec\n", 5 - i);
		_delay_ms(1000);
	}
	nrc_sw_reset();
}

int nrc_ctrl_sys_reboot()
{
	xTaskCreate(sys_reboot_task, "sys reboot task", 512,
				NULL, uxTaskPriorityGet(NULL), NULL);
	return 0;
}
