/*
 * MIT License
 *
 * Copyright (c) 2020 Newracom, Inc.
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

typedef void (*func_void)(void);
typedef void (*func_int)(int);

struct pbc_ops {
    uint8_t     GPIO_PushButton;
    void		(*nrc_wifi_wps_pbc_fail)(void);
    void		(*nrc_wifi_wps_pbc_timeout)(void);
    void		(*nrc_wifi_wps_pbc_success)(void);
    void        (*nrc_wifi_wps_pbc_pressed)(int v);
};

/**********************************************
 * @fn  static void wps_pbc_fail_cb(void)
 *
 * @brief This callback is called when wps pbc operation fail
 *
 * @param void
 *
 * @return void
 ***********************************************/
static void wps_pbc_fail_cb(void);

/**********************************************
 * @fn  static void wps_pbc_timeout_cb(void)
 *
 * @brief This callback is called when there is no connection attempt for 120 second and timeout occurs.
 *
 * @param void
 *
 * @return void
 ***********************************************/
static void wps_pbc_timeout_cb(void);

/**********************************************
 * @fn  static void wps_pbc_success_cb(void)
 *
 * @brief This callback is called when wps pbc operation succeeds
 *
 * @param void
 *
 * @return void
 ***********************************************/
static void wps_pbc_success_cb(void);

/**********************************************
 * @fn  static void wps_pbc_button_pressed_event(int vector)
 *
 * @brief This callback is called when user push the button which is connected with GPIO we register for interrupt.
 *
 * @param void
 *
 * @return void
 ***********************************************/
static void wps_pbc_button_pressed_event(int vector);

/**********************************************
 * @fn  void wps_pbc_set_fail_cb(func_void cb)
 *
 * @brief register callback
 *
 * @param cb: callback to register
 *
 * @return void
 ***********************************************/
void wps_pbc_set_fail_cb(func_void cb);

/**********************************************
 * @fn  void wps_pbc_set_timeout_cb(func_void cb)
 *
 * @brief register callback
 *
 * @param cb: callback to register
 *
 * @return void
 ***********************************************/
void wps_pbc_set_timeout_cb(func_void cb);

/**********************************************
 * @fn  void wps_pbc_set_success_cb(func_void cb)
 *
 * @brief register callback
 *
 * @param cb: callback to register
 *
 * @return void
 ***********************************************/
void wps_pbc_set_success_cb(func_void cb);

/**********************************************
 * @fn  void wps_pbc_set_btn_pressed_cb(func_int cb)
 *
 * @brief register callback
 *
 * @param cb: callback to register
 *
 * @return void
 ***********************************************/
void wps_pbc_set_btn_pressed_cb(func_int cb);

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

#endif /* __NRC_API_WPS_PBC_H__ */