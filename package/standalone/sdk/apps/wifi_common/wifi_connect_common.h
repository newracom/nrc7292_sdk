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

#ifndef __WIFI_CONNECT_COMMON_H__
#define __WIFI_CONNECT_COMMON_H__

#include "wifi_config_setup.h"

/*********************************************************************
 * @fn wifi_init
 *
 * @brief initialize the wifi configurations
 *
 * @param wifi configuration ptr
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 **********************************************************************/
tWIFI_STATUS wifi_init(WIFI_CONFIG *param);


/*********************************************************************
 * @fn wifi_connect
 *
 * @brief Connect to AP
 *
 * @param wifi configuration ptr
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 **********************************************************************/
tWIFI_STATUS wifi_connect(WIFI_CONFIG *param);


/*********************************************************************
 * @fn wifi_start_softap
 *
 * @brief Start softAP
 *
 * @param wifi configuration ptr
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 **********************************************************************/
tWIFI_STATUS wifi_start_softap(WIFI_CONFIG *param);


/*********************************************************************
 * @fn wifi_init_with_vif
 *
 * @brief initialize the wifi configurations
 *
 * @param vif
 *
 * @param wifi configuration ptr
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 **********************************************************************/
tWIFI_STATUS wifi_init_with_vif(int vif, WIFI_CONFIG *param);

/*********************************************************************
 * @fn wifi_deinit_with_vif
 *
 * @brief deinitialize the wifi configurations
 *
 * @param vif
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 **********************************************************************/
tWIFI_STATUS wifi_deinit(void);

/*********************************************************************
 * @fn wifi_connect_with_vif
 *
 * @brief Connect to AP
 *
 * @param vif
 *
 * @param wifi configuration ptr
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 **********************************************************************/
tWIFI_STATUS wifi_connect_with_vif(int vif, WIFI_CONFIG *param);


/*********************************************************************
 * @fn wifi_start_softap_with_vif
 *
 * @brief Start softAP with vif
 *
 * @param vif
 *
 * @param wifi configuration ptr
 *
 * @return If success, then WIFI_SUCCESS. Otherwise, error code(tWIFI_STATUS) is returned.
 **********************************************************************/
tWIFI_STATUS wifi_start_softap_with_vif(int vif, WIFI_CONFIG *param);

/*********************************************************************
 * @fn nrc_wait_for_ip
 *
 * @brief Wait for IP to be assigned.
 *
 * @param vif
 *
 * @param timeout : DHCP timeout in sec.
 *
 * @return NRC_SUCCESS if IP is assigned.
 *         NRC_FAIL if timeout occurred without getting IP.
 **********************************************************************/
nrc_err_t nrc_wait_for_ip(int vif, uint32_t timeout);

#endif /* __WIFI_CONNECT_COMMON_H__ */
