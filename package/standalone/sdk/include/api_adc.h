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

#ifndef __NRC_API_NADC_H__
#define __NRC_API_NADC_H__

#ifdef __cplusplus
extern "C" {
#endif

/** @brief channel number of ADC */
typedef enum  {
#ifdef NRC7292
	ADC1 = 1,	/**< ADC channel 1 */
	ADC2,		/**< ADC channel 2 */
	ADC3,		/**< ADC channel 3 */
#else
	ADC0 = 2,	/**< ADC channel 2 */
	ADC1,		/**< ADC channel 3 */
#endif
}ADC_CH;


/**********************************************
 * @fn nrc_err_t nrc_adc_init(bool enable)
 *
 * @brief Initialize ADC controller
 *
 * @param enable: Enable or disable ADC controller after initialized
 *
 * @return If success, then NRC_SUCCESS. Otherwise, NRC_FAIL is returned.
 ***********************************************/
nrc_err_t nrc_adc_init(bool enable);


/**********************************************
 * @fn nrc_err_t nrc_adc_deinit(void)
 *
 * @brief Deinitialize ADC controller
 *
 * @return If success, then NRC_SUCCESS. Otherwise, NRC_FAIL is returned.
 ***********************************************/
nrc_err_t nrc_adc_deinit(void);


/**********************************************
 * @fn void nrc_adc_enable(void)
 *
 * @brief Enable ADC controller
 *
 * @return void
 ***********************************************/
void nrc_adc_enable(void);


/**********************************************
 * @fn void nrc_adc_disable(void)
 *
 * @brief Disable ADC controller
 *
 * @return  void
 ***********************************************/
void nrc_adc_disable(void);


/**********************************************
 * @fn uint16_t nrc_adc_get_data(ADC_CH id)
 *
 * @brief Read the data from specific channel ID
 *
 * @param id: Channel ID
 *
 * @return ADC value
 ***********************************************/
uint16_t nrc_adc_get_data(ADC_CH id);

#ifdef __cplusplus
}
#endif

#endif //__NRC_API_NADC_H__
