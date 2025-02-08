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

#ifndef __NRC_API_WPS_PBC_H__
#define __NRC_API_WPS_PBC_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*func_void)(void);
typedef void (*func_int)(int);

#define PBC_IF_NUM  2
struct pbc_ops {
    int8_t     GPIO_PushButton[PBC_IF_NUM];
    void		(*nrc_wifi_wps_pbc_fail)(void *priv);
    void		(*nrc_wifi_wps_pbc_timeout)(void *priv);
    void		(*nrc_wifi_wps_pbc_success)(void *priv, int, uint8_t *, uint8_t, uint8_t, char *);
    void        (*nrc_wifi_wps_pbc_pressed)(int v);
};

/**********************************************
 * @fn  static void wps_pbc_fail_cb(void *priv)
 *
 * @brief This callback is called when wps pbc operation fail
 *
 * @param[in]  priv          : wpa interface
 *
 * @return void
 ***********************************************/
static void wps_pbc_fail_cb(void *priv);

/**********************************************
 * @fn  static void wps_pbc_timeout_cb(void *priv)
 *
 * @brief This callback is called when there is no connection
 *          attempt for 120 second and timeout occurs.
 *
 * @param[in]  priv          : wpa interface
 *
 * @return void
 ***********************************************/
static void wps_pbc_timeout_cb(void *priv);

/**********************************************
 * @fn  static void wps_pbc_success_cb(void *priv, uint8_t *ssid,
 *                     uint8_t ssid_len, uint8_t security_mode, char *passphrase)
 *
 * @brief This callback is called when wps pbc operation succeeds
 *
 * @param[in]  priv          : wpa interface
 * @param[in]  net_id        : network id
 * @param[in]  ssid          : Service set identifier (network name)
 * @param[in]  ssid_len      : Length of the SSID
 * @param[in]  security_mode : WIFI_SEC_OPEN=0, WIFI_SEC_WPA2=1,
 *                                  WIFI_SEC_WPA3_OWE=2, WIFI_SEC_WPA3_SAE=3
 * @param[in]  passphrase    : WPA ASCII passphrase
 *                           (ASCII passphrase must be between 8 and 63 characters)
 *
 * @return void
 ***********************************************/
static void wps_pbc_success_cb(void *priv, int net_id, uint8_t *ssid,
	uint8_t ssid_len, uint8_t security_mode, char *passphrase);


/**********************************************
 * @fn  static void wps_pbc_button_pressed_event(int vector)
 *
 * @brief This callback is called when user push the button which is
 *          connected with GPIO we register for interrupt.
 *
 * @param void
 *
 * @return void
 ***********************************************/
static void wps_pbc_button_pressed_event(int vector);

/**********************************************
 * @fn  void init_wps_pbc(struct pbc_ops *ops)
 *
 * @brief Initialize functino
 *
 * @param ops: structure contains GPIO and callbacks
 *
 * @return void
 ***********************************************/
void init_wps_pbc(struct pbc_ops *ops);

extern struct pbc_ops *wps_pbc_ops;

#ifdef __cplusplus
}
#endif

#endif /* __NRC_API_WPS_PBC_H__ */
