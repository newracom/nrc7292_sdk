/*
 * MIT License
 *
 * Copyright (c) 2024 Newracom, Inc.
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


#ifndef __API_DMA_H__
#define __API_DMA_H__
/**********************************************************************************************/

#define NRC_DMA_CHANNEL_MAX			8
#define NRC_DMA_XFER_SIZE_MAX		4096

enum DMA_ERROR {
	NRC_DMA_OK = 0,
	NRC_DMA_EPERM = -1,
	NRC_DMA_EINVAL = -2,
	NRC_DMA_EBUSY = -3
};

enum DMA_BSIZE {
	NRC_DMA_BSIZE_1 = 0,
	NRC_DMA_BSIZE_4,
	NRC_DMA_BSIZE_8,
	NRC_DMA_BSIZE_16,
	NRC_DMA_BSIZE_32,
	NRC_DMA_BSIZE_64,
	NRC_DMA_BSIZE_128,
	NRC_DMA_BSIZE_256,

	NRC_DMA_BSIZE_MAX
};

enum DMA_WIDTH {
	NRC_DMA_WIDTH_8 = 0,
	NRC_DMA_WIDTH_16,
	NRC_DMA_WIDTH_32,

	NRC_DMA_WIDTH_MAX
};

enum DMA_AHBM {
	NRC_DMA_AHB_M1 = 0,
	NRC_DMA_AHB_M2,

	NRC_DMA_AHB_MAX
};

enum DMA_PERI_ID {
	NRC_DMA_PERI_SSP0_RX = 0,
	NRC_DMA_PERI_SSP0_TX,

	NRC_DMA_PERI_SSP1_RX,
	NRC_DMA_PERI_SSP1_TX,

	NRC_DMA_PERI_HSUART0_RX,
	NRC_DMA_PERI_HSUART0_TX,

	NRC_DMA_PERI_HSUART1_RX,
	NRC_DMA_PERI_HSUART1_TX,

	NRC_DMA_PERI_HSUART2_RX,
	NRC_DMA_PERI_HSUART2_TX,

	NRC_DMA_PERI_HSUART3_RX,
	NRC_DMA_PERI_HSUART3_TX,

	NRC_DMA_PERI_SSP2_RX,
	NRC_DMA_PERI_SSP2_TX,

	NRC_DMA_PERI_SSP3_RX,
	NRC_DMA_PERI_SSP3_TX,

	NRC_DMA_PERI_ID_MAX,
};

typedef struct {
	uint32_t Addr;

	uint32_t ID:5; // request peripheral
	uint32_t Reserved:27;
	uint32_t AddrInc:1; // address increment
	uint32_t FlowCtrl:1; // flow controller
} dma_peri_t;

typedef struct _dma_desc {
	uint32_t SrcAddr;
	uint32_t DestAddr;

	struct _dma_desc *Next; // word-aligned

	/* Channel Control Information */
	uint32_t XferSize:12; // Trasfer size
	uint32_t SBSize:3; // Source burst size
	uint32_t DBSize:3; // Destination burst size
	uint32_t SWidth:3; // Source transfer widith
	uint32_t DWidth:3; // Destination transfer width
	uint32_t SAHBM:1; // Source AHB master
	uint32_t DAHBM:1; // Destination AHB master
	uint32_t SAInc:1; // Source address increment
	uint32_t DAInc:1; // Destination address increment
	uint32_t Privileged:1; // Protection: Privileged mode
	uint32_t Bufferable:1; // Protection: Buffered access
	uint32_t Cacheable:1; // Protection: Cached access
	uint32_t IntTC:1; // Terminal count interrupt enable
} dma_desc_t __attribute__((aligned(4)));

typedef void (*dma_isr_t) (int channel);

/**********************************************************************************************/

/**********************************************
 * @fn nrc_dma_enable (void)
 *
 * @brief This function enables DMA subsystem on NRC SoC.
 *
 * @param NONE
 *
 * @return NONE
 ***********************************************/
void nrc_dma_enable (void);

/**********************************************
 * @fn nrc_dma_disable (void)
 *
 * @brief This function disables DMA subsystem on NRC SoC.
 *
 * @param NONE
 *
 * @return NONE
 ***********************************************/
void nrc_dma_disable (void);

/**********************************************
 * @fn nrc_dma_is_enabled (void)
 *
 * @brief Check if DMA subsystem is enabled on NRC SoC.
 *        Sample code used is shown below.
 *    if (nrc_dma_is_enabled()) {
 *       nrc_dma_disable();
 *   }
 *   nrc_dma_enable();
 *
 * @param NONE
 *
 * @return return true if DMA is enabled, false otherwise.
 ***********************************************/
bool nrc_dma_is_enabled (void);

/**********************************************
 * @fn nrc_dma_get_channel (int highest)
 *
 * @brief This function returns the DMA channel that is available to be used
 *        on the system.
 *        If the argument highest set to non-zero, then the highest channel number will be
 *        returned from 8 channels available on the system. If highest is set to zero,
 *        then low channel number will be returned.
 *        i.e. If firmware is already using DMA channel 0 and 7,
 *             non-zero highest will result in returning channel number 6 and
 *             zero highest will return channel number 1.
 *
 * @param highest : whether to find the higher channel number or not. (0 or 1)
 *
 * @return Free DMA channel that can be used.
 ***********************************************/
int nrc_dma_get_channel (int highest);

/**********************************************
 * @fn nrc_dma_valid_channel (int channel)
 *
 * @brief Validate if channel number provided are within 0..7.
 *
 * @param channel : channel number to be verified.
 *
 * @return true if channel number if valid, otherwise false.
 ***********************************************/
bool nrc_dma_valid_channel (int channel);

/**********************************************
 * @fn nrc_dma_peri_init (dma_peri_t *peri, int id, uint32_t addr, bool addr_inc, bool flow_ctrl)
 *
 * @brief Initialize dma_peri_t data structure using supplied data.
 *
 * @param peri : dma_peri_t data structure that will be initialized.
 * @param id : Identification number that will be used (see DMA_PERI_ID definition for available identifications).
 * @param addr : Address to be used for the DMA.
 * @param addr_inc : boolean value whether to automatically increase address.
 * @param flow_ctrl : boolean value whether the flow control should be used.
 *
 * @return 0 if successful, NRC_DMA_EINVAL if error occurs.
 ***********************************************/
int nrc_dma_peri_init (dma_peri_t *peri, int id, uint32_t addr, bool addr_inc, bool flow_ctrl);

/**********************************************
 * @fn nrc_dma_config_m2m (int channel, dma_isr_t inttc_isr, dma_isr_t interr_isr)
 *
 * @brief Configure DMA to be used within the system.
 *
 * @param channel : DMA channel to be used.
 * @param inttc_isr : interrupt handler that will be called upon completion of DMA operation.
 * @param interr_isr : interrupt handler that will be call if there is any error occurred during DMA operation.
 *
 * @return One of below values will be returned.
 *         NRC_DMA_OK = 0,
 *         NRC_DMA_EPERM = -1,
 *         NRC_DMA_EINVAL = -2,
 *         NRC_DMA_EBUSY = -3
 ***********************************************/
int nrc_dma_config_m2m (int channel, dma_isr_t inttc_isr, dma_isr_t interr_isr);

/**********************************************
 * @fn nrc_dma_config_m2p (int channel, dma_peri_t *dest_peri, dma_isr_t inttc_isr, dma_isr_t interr_isr)
 *
 * @brief Configure DMA to be used for the data direction from SoC to peripheral.
 *
 * @param channel : DMA channel to be used.
 * @param dest_peri : destination peripheral address.
 *        i.e. SSP0_BASE_ADDR + 8 for data section of SPI channel 0. (See SoC specification for details.)
 * @param inttc_isr : interrupt handler that will be called upon DMA operation.
 * @param interr_isr : interrupt handler that will be call if there is any error occurred during DMA operation.
 *
 * @return One of below values will be returned.
 *         NRC_DMA_OK = 0,
 *         NRC_DMA_EPERM = -1,
 *         NRC_DMA_EINVAL = -2,
 *         NRC_DMA_EBUSY = -3
 ***********************************************/
int nrc_dma_config_m2p (int channel, dma_peri_t *dest_peri, dma_isr_t inttc_isr, dma_isr_t interr_isr);

/**********************************************
 * @fn nrc_dma_config_p2m (int channel, dma_peri_t *src_peri, dma_isr_t inttc_isr, dma_isr_t interr_isr)
 *
 * @brief Configure DMA to be used for the data incoming from peripheral to SoC.
 *
 * @param channel : DMA channel to be used.
 * @param src_peri : source peripheral address.
 *        i.e. SSP0_BASE_ADDR + 8 for data section of SPI channel 0. (See SoC specification for details.)
 * @param inttc_isr : interrupt handler that will be called upon DMA operation.
 * @param interr_isr : interrupt handler that will be call if there is any error occurred during DMA operation.
 *
 * @return One of below values will be returned.
 *         NRC_DMA_OK = 0,
 *         NRC_DMA_EPERM = -1,
 *         NRC_DMA_EINVAL = -2,
 *         NRC_DMA_EBUSY = -3
 ***********************************************/
int nrc_dma_config_p2m (int channel, dma_peri_t *src_peri, dma_isr_t inttc_isr, dma_isr_t interr_isr);

/**********************************************
 * @fn nrc_dma_config_p2p (int channel, dma_peri_t *src_peri, dma_peri_t *dest_peri,
                           dma_isr_t inttc_isr, dma_isr_t interr_isr)
 *
 * @brief Configure DMA to be used between the peripherals.
 *
 * @param channel : DMA channel to be used.
 * @param src_peri : source peripheral address.
 *        i.e. SSP0_BASE_ADDR + 8 for data section of SPI channel 0. (See SoC specification for details.)
 * @param dest_peri : destination peripheral address.
 *        i.e. SSP0_BASE_ADDR + 8 for data section of SPI channel 0. (See SoC specification for details.)
 * @param inttc_isr : interrupt handler that will be called upon DMA operation.
 * @param interr_isr : interrupt handler that will be call if there is any error occurred during DMA operation.
 *
 * @return One of below values will be returned.
 *         NRC_DMA_OK = 0,
 *         NRC_DMA_EPERM = -1,
 *         NRC_DMA_EINVAL = -2,
 *         NRC_DMA_EBUSY = -3
 ***********************************************/
int nrc_dma_config_p2p (int channel, dma_peri_t *src_peri, dma_peri_t *dest_peri,
                        dma_isr_t inttc_isr, dma_isr_t interr_isr);

/**********************************************
 * @fn nrc_dma_start (int channel, dma_desc_t *desc)
 *
 * @brief Starts the DMA operation for the channel and the data given as dma_desc_t.
 *
 * @param channel : DMA channel to be used.
 * @param desc : Linked list of DMA descriptors. (See above for dma_desc_t definition.)
 *
 * @return One of below values will be returned.
 *         NRC_DMA_OK = 0,
 *         NRC_DMA_EPERM = -1,
 *         NRC_DMA_EINVAL = -2,
 *         NRC_DMA_EBUSY = -3
 ***********************************************/
int nrc_dma_start (int channel, dma_desc_t *desc);

/**********************************************
 * @fn nrc_dma_stop (int channel)
 *
 * @brief Stops the DMA operation.
 *
 * @param channel : DMA channel to be used.
 *
 * @return One of below values will be returned.
 *         NRC_DMA_OK = 0,
 *         NRC_DMA_EPERM = -1,
 *         NRC_DMA_EINVAL = -2,
 *         NRC_DMA_EBUSY = -3
 ***********************************************/
int nrc_dma_stop (int channel);

/**********************************************
 * @fn nrc_dma_busy (int channel)
 *
 * @brief Check whether the given channel is busy or not.
 *
 * @param channel : DMA channel to be used.
 *
 * @return true if the channel is busy, false otherwise.
 ***********************************************/
bool nrc_dma_busy (int channel);

/**********************************************
 * @fn nrc_dma_src_addr (int channel)
 *
 * @brief Retrieves the source memory address of DMA channel.
 *
 * @param channel : DMA channel to be used.
 *
 * @return source address.
 ***********************************************/
uint32_t nrc_dma_src_addr (int channel);

/**********************************************
 * @fn nrc_dma_dest_addr (int channel)
 *
 * @brief Retrieves the destination memory address of DMA channel.
 *
 * @param channel : DMA channel to be used.
 *
 * @return destination address.
 ***********************************************/
uint32_t nrc_dma_dest_addr (int channel);

/**********************************************
 * @fn nrc_dma_desc_print (dma_desc_t *desc)
 *
 * @brief Prints the dma_desc_t contents for given desc.
 *
 * @param desc : dma_desc_t to be printed.
 *
 * @return NONE
 ***********************************************/
void nrc_dma_desc_print (dma_desc_t *desc);

/**********************************************
 * @fn nrc_dma_desc_init (dma_desc_t *desc, uint32_t src_addr, uint32_t dest_addr, uint16_t size)
 *
 * @brief Initialize DMA descriptor using given arguments.
 *
 * @param desc : dma_desc_t to be initialize.
 * @param src_addr : source address to be used.
 * @param dest_addr : destination address to be used.
 * @param size : size of data to be transferred.
 *
 * @return 0 if successful, NRC_DMA_EINVAL otherwise.
 ***********************************************/
int nrc_dma_desc_init (dma_desc_t *desc, uint32_t src_addr, uint32_t dest_addr, uint16_t size);

/**********************************************
 * @fn nrc_dma_desc_link (dma_desc_t *desc, dma_desc_t *next)
 *
 * @brief Set the next field in dma_desc_t for given desc to complete dma_desc_t linked list.
 *
 * @param desc : dma_desc_t next field to be updated.
 * @param next : dma_desc_t to be linked to desc provided.
 *
 * @return 0 if successful, NRC_DMA_EINVAL otherwise.
 ***********************************************/
int nrc_dma_desc_link (dma_desc_t *desc, dma_desc_t *next);

/**********************************************
 * @fn nrc_dma_desc_set_addr (dma_desc_t *desc, uint32_t src_addr, uint32_t dest_addr)
 *
 * @brief Update source and destination for given desc.
 *
 * @param desc : dma_desc_t to be updated.
 * @param src_addr : source address to update to.
 * @param deset_addr : destination address to update to.
 *
 * @return 0 if successful, NRC_DMA_EINVAL otherwise.
 ***********************************************/
int nrc_dma_desc_set_addr (dma_desc_t *desc, uint32_t src_addr, uint32_t dest_addr);

/**********************************************
 * @fn nrc_dma_desc_set_addr_inc (dma_desc_t *desc, bool src_inc, bool dest_inc)
 *
 * @brief Update whether to automatically increment source and destination address for given dma_desc_t.
 *
 * @param desc : dma_desc_t to be updated.
 * @param src_inc : boolean value to enable/disable automatic source address incrementation.
 * @param dest_inc : boolean value to enable/disable automatic destination address incrementation.
 *
 * @return 0 if successful, NRC_DMA_EINVAL otherwise.
 ***********************************************/
int nrc_dma_desc_set_addr_inc (dma_desc_t *desc, bool src_inc, bool dest_inc);

/**********************************************
 * @fn nrc_dma_desc_set_size (dma_desc_t *desc, uint16_t size)
 *
 * @brief Update transfer size for given dma_desc_t.
 *
 * @param desc : dma_desc_t to be updated.
 * @param size : transfer size to update to.
 *
 * @return 0 if successful, NRC_DMA_EINVAL otherwise.
 ***********************************************/
int nrc_dma_desc_set_size (dma_desc_t *desc, uint16_t size);

/**********************************************
 * @fn nrc_dma_desc_set_width (dma_desc_t *desc, uint8_t src_width, uint8_t dest_width)
 *
 * @brief Update data width in bits for given dma_desc_t.
 *
 * @param desc : dma_desc_t to be updated.
 * @param src_width : source data width in bits.
 * @param dest_width : destination data width in bits.
 *
 * @return 0 if successful, NRC_DMA_EINVAL otherwise.
 ***********************************************/
int nrc_dma_desc_set_width (dma_desc_t *desc, uint8_t src_width, uint8_t dest_width);

/**********************************************
 * @fn nrc_dma_desc_set_bsize (dma_desc_t *desc, uint8_t src_bsize, uint8_t dest_bsize)
 *
 * @brief Update data burst size for given dma_desc_t.
 *
 * @param desc : dma_desc_t to be updated.
 * @param src_bsize : source burst data size.
 * @param dest_bsize : destination burst data size.
 *
 * @return 0 if successful, NRC_DMA_EINVAL otherwise.
 ***********************************************/
int nrc_dma_desc_set_bsize (dma_desc_t *desc, uint8_t src_bsize, uint8_t dest_bsize);

/**********************************************
 * @fn nrc_dma_desc_set_inttc (dma_desc_t *desc, bool inttc)
 *
 * @brief Enable/disable interrupt for given dma_desc_t.
 *
 * @param desc : dma_desc_t to be updated.
 * @param inttc : true to enable, false to disable interrupt.
 *
 * @return 0 if successful, NRC_DMA_EINVAL otherwise.
 ***********************************************/
int nrc_dma_desc_set_inttc (dma_desc_t *desc, bool inttc);

/**********************************************
 * @fn nrc_dma_desc_set_ahb_master (dma_desc_t *desc, int src_ahbm, int dest_ahbm)
 *
 * @brief Set source and destination AHB interface. AHB master can be 0 or 1.
 *
 * @param desc : dma_desc_t to be updated.
 * @param src_ahbm : source AHB master interface.
 * @param dest_ahbm : destination AHB master interface.
 *
 * @return 0 if successful, NRC_DMA_EINVAL otherwise.
 ***********************************************/
int nrc_dma_desc_set_ahb_master (dma_desc_t *desc, int src_ahbm, int dest_ahbm);

/**********************************************
 * @fn nrc_dma_desc_set_protection (dma_desc_t *desc, bool privileged, bool bufferable, bool cacheable)
 *
 * @brief Update protection scheme to be used for given dma_desc_t.
 *
 * @param desc : dma_desc_t to be updated.
 * @param privileged : Set whether privileged protection be enabled or disabled.
 * @param bufferable : Set whether bufferable protection be enabled or disabled.
 * @param cacheable : Set whether cacheable protection be enabled or disabled.
 *
 * @return 0 if successful, NRC_DMA_EINVAL otherwise.
 ***********************************************/
int nrc_dma_desc_set_protection (dma_desc_t *desc, bool privileged, bool bufferable, bool cacheable);

/**********************************************************************************************/
#endif /* #ifndef __API_DMA_H__ */
