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

#ifndef __NRC_CTRL_FUNC_H__
#define __NRC_CTRL_FUNC_H__

int nrc_ctrl_show_config(char *config, size_t *length);
int nrc_ctrl_show_sysconfig(char *config, size_t *length);
int nrc_ctrl_show_apinfo(char *info, size_t *length);
int nrc_ctrl_get_version(char *version, size_t *length);
int nrc_ctrl_get_ssid(char *ssid, size_t *length);
int nrc_ctrl_set_ssid(char *ssid);
int nrc_ctrl_get_security(uint8_t *security);
int nrc_ctrl_set_security(uint8_t security);
int nrc_ctrl_get_password(char *password, size_t *length);
int nrc_ctrl_set_password(char *password);
int nrc_ctrl_get_country(char *country, size_t *length);
int nrc_ctrl_set_country(char *country);
int nrc_ctrl_get_ipmode(uint8_t *ipmode);
int nrc_ctrl_set_ipmode(uint8_t ipmode);
int nrc_ctrl_get_conn_timeout(int32_t *timeout);
int nrc_ctrl_set_conn_timeout(int32_t timeout);
int nrc_ctrl_get_disconn_timeout(int32_t *timeout);
int nrc_ctrl_set_disconn_timeout(int32_t timeout);
int nrc_ctrl_set_mcs(uint8_t mcs);
int nrc_ctrl_get_mcs(uint8_t* mcs);
int nrc_ctrl_set_rc(uint8_t rc);
int nrc_ctrl_get_rc(uint8_t* rc);
int nrc_ctrl_set_cca_thres(int8_t cca_thres);
int nrc_ctrl_get_cca_thres(int8_t* cca_thres);
int nrc_ctrl_get_static_ip(char *ip, size_t *length);
int nrc_ctrl_set_static_ip(char *ip);
int nrc_ctrl_get_netmask(char *netmask, size_t *length);
int nrc_ctrl_set_netmask(char *netmask);
int nrc_ctrl_get_gateway(char *gateway, size_t *length);
int nrc_ctrl_set_gateway(char *gateway);
int nrc_ctrl_get_txpower(uint8_t *txpower, uint8_t *txpower_type);
int nrc_ctrl_set_txpower(uint8_t txpower, uint8_t txpower_type);
int nrc_ctrl_get_txpower_type(uint8_t *txpower_type);
int nrc_ctrl_set_txpower_type(uint8_t txpower_type);
#ifdef INCLUDE_SCAN_BACKOFF
uint32_t nrc_ctrl_get_scan_max_interval();
int nrc_ctrl_set_scan_max_interval(uint32_t interval);
uint32_t nrc_ctrl_get_backoff_start_count();
int nrc_ctrl_set_backoff_start_count(uint32_t count);
#endif
int nrc_ctrl_nvs_reset();
int nrc_ctrl_sys_reboot();

#endif /* __NRC_CTRL_FUNC_H__ */
