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


#ifndef __HIF_DMA_H__
#define __HIF_DMA_H__
/**********************************************************************************************/

#define HIF_DMA_CHANNEL_MAX			8
#define HIF_DMA_XFER_SIZE_MAX		4096

enum DMA_ERROR
{
	HIF_DMA_OK = 0,
	HIF_DMA_EPERM = -1,
	HIF_DMA_EINVAL = -2,
	HIF_DMA_EBUSY = -3
};

enum DMA_BSIZE
{
	HIF_DMA_BSIZE_1 = 0,
	HIF_DMA_BSIZE_4,
	HIF_DMA_BSIZE_8,
	HIF_DMA_BSIZE_16,
	HIF_DMA_BSIZE_32,
	HIF_DMA_BSIZE_64,
	HIF_DMA_BSIZE_128,
	HIF_DMA_BSIZE_256,

	HIF_DMA_BSIZE_MAX
};

enum DMA_WIDTH
{
	HIF_DMA_WIDTH_8 = 0,
	HIF_DMA_WIDTH_16,
	HIF_DMA_WIDTH_32,

	HIF_DMA_WIDTH_MAX
};

enum DMA_AHBM
{
	HIF_DMA_AHB_M1 = 0,
	HIF_DMA_AHB_M2,

	HIF_DMA_AHB_MAX
};

enum DMA_PERI_ID
{
	HIF_DMA_PERI_SSP0_RX = 0,
	HIF_DMA_PERI_SSP0_TX,

	HIF_DMA_PERI_SSP1_RX,
	HIF_DMA_PERI_SSP1_TX,

	HIF_DMA_PERI_HSUART0_RX,
	HIF_DMA_PERI_HSUART0_TX,

	HIF_DMA_PERI_HSUART1_RX,
	HIF_DMA_PERI_HSUART1_TX,

	HIF_DMA_PERI_HSUART2_RX,
	HIF_DMA_PERI_HSUART2_TX,

	HIF_DMA_PERI_HSUART3_RX,
	HIF_DMA_PERI_HSUART3_TX,

	HIF_DMA_PERI_SSP2_RX,
	HIF_DMA_PERI_SSP2_TX,

	HIF_DMA_PERI_SSP3_RX,
	HIF_DMA_PERI_SSP3_TX,

	HIF_DMA_PERI_ID_MAX,
};

typedef struct
{
	uint32_t Addr;

	uint32_t ID:5; // request peripheral
	uint32_t Reserved:27;
	uint32_t AddrInc:1; // address increment
	uint32_t FlowCtrl:1; // flow controller
} dma_peri_t;

typedef struct _dma_desc
{
	uint32_t SrcAddr;
	uint32_t DestAddr;

	struct _dma_desc *Next; // word-aligned

	/* Channel Control Information */
	uint32_t XferSize:12; // Trasfer size
	uint32_t SBSize:3; // Source burst size
	uint32_t DBSize:3; // Destination burst size
	uint32_t SWidth:3; // Source transfer widith
	uint32_t DWidth:3; // Destination transfer widith
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

extern void _hif_dma_enable (void);
extern void _hif_dma_disable (void);
extern bool _hif_dma_is_enabled (void);

extern int _hif_dma_get_channel (int highest);
extern bool _hif_dma_valid_channel (int channel);

extern int _hif_dma_config_m2m (int channel, dma_isr_t inttc_isr, dma_isr_t interr_isr);
extern int _hif_dma_config_m2p (int channel, dma_peri_t *dest_peri, dma_isr_t inttc_isr, dma_isr_t interr_isr);
extern int _hif_dma_config_p2m (int channel, dma_peri_t *src_peri, dma_isr_t inttc_isr, dma_isr_t interr_isr);
extern int _hif_dma_config_p2p (int channel, dma_peri_t *src_peri, dma_peri_t *dest_peri,
											dma_isr_t inttc_isr, dma_isr_t interr_isr);

extern int _hif_dma_start (int channel, dma_desc_t *desc);
extern int _hif_dma_stop (int channel);
extern bool _hif_dma_busy (int channel);

extern uint32_t _hif_dma_src_addr (int channel);
extern uint32_t _hif_dma_dest_addr (int channel);

extern int _hif_dma_peri_init (dma_peri_t *peri, int id, uint32_t addr, bool addr_inc, bool flow_ctrl);

extern void _hif_dma_desc_print (dma_desc_t *desc);
extern int _hif_dma_desc_init (dma_desc_t *desc, uint32_t src_addr, uint32_t dest_addr, uint16_t size);
extern int _hif_dma_desc_link (dma_desc_t *desc, dma_desc_t *next);

extern int _hif_dma_desc_set_addr (dma_desc_t *desc, uint32_t src_addr, uint32_t dest_addr);
extern int _hif_dma_desc_set_addr_inc (dma_desc_t *desc, bool src_inc, bool dest_inc);
extern int _hif_dma_desc_set_size (dma_desc_t *desc, uint16_t size);
extern int _hif_dma_desc_set_width (dma_desc_t *desc, uint8_t src_width, uint8_t dest_width);
extern int _hif_dma_desc_set_bsize (dma_desc_t *desc, uint8_t src_bsize, uint8_t dest_bsize);
extern int _hif_dma_desc_set_inttc (dma_desc_t *desc, bool inttc);
extern int _hif_dma_desc_set_ahb_master (dma_desc_t *desc, int src_ahbm, int dest_ahbm);
extern int _hif_dma_desc_set_protection (dma_desc_t *desc, bool privileged, bool bufferable, bool cacheable);

/**********************************************************************************************/
#endif /* #ifndef __HIF_DMA_H__ */

