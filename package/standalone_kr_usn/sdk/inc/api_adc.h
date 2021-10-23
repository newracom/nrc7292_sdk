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

#ifndef __NRC_API_NADC_H__
#define __NRC_API_NADC_H__


/** @brief channel number of ADC */
typedef enum  {
	ADC1 = 1,	/**< ADC channel 1 */
	ADC2,		/**< ADC channel 2 */
	ADC3,		/**< ADC channel 3 */
}ADC_CH;

/**********************************************
 * @fn nrc_err_t nrc_adc_init(void)
 *
 * @brief Initialize ADC controller
 *
 * @return If success, then NRC_SUCCESS. Otherwise, NRC_FAIL is returned.
 ***********************************************/
nrc_err_t nrc_adc_init(void);


/**********************************************
 * @fn nrc_err_t nrc_adc_deinit(void)
 *
 * @brief Deinitialize ADC controller
 *
 * @return If success, then NRC_SUCCESS. Otherwise, NRC_FAIL is returned.
 ***********************************************/
nrc_err_t nrc_adc_deinit(void);


/**********************************************
 * @fn nrc_err_t nrc_adc_get_data(uint32_t id, uint16_t *data)
 *
 * @brief Read the data from specific channel ID
 *
 * @param id: Channel ID
 *
 * @param *data: A pointer for of data(Max value : 0x1FF)
 *
 * @return If success, then NRC_SUCCESS. Otherwise, NRC_FAIL is returned.
 ***********************************************/
nrc_err_t nrc_adc_get_data(uint32_t id, uint16_t *data);

#endif //__NRC_API_NADC_H__
