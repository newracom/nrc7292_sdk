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


#include "hif.h"


#define _hif_dma_printf(fmt, ...)	_hif_log(fmt, ##__VA_ARGS__)

#define _hif_dma_trace()			_hif_dma_printf("%s::%d\n", __func__, __LINE__)
#define _hif_dma_debug(fmt, ...)	_hif_dma_printf(fmt, ##__VA_ARGS__)
#define _hif_dma_info(fmt, ...)		_hif_dma_printf(fmt, ##__VA_ARGS__)
#define _hif_dma_error(fmt, ...)	_hif_dma_printf("%s: " fmt, __func__, ##__VA_ARGS__)

/**********************************************************************************************/

#define HIF_DMAC_REG_BASE		0x40091000

typedef uint32_t rDMACC_Addr_t;

typedef struct
{
	uint32_t AHBM:1; // 0:AHB_Master_1, 1:AHB_Master_2
	uint32_t Undefined:1;
	uint32_t Next:30; // Bits [31:2] of the address for the next LLI.
} rDMACC_LLI_t;

typedef struct
{
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
} rDMACC_Control_t;

typedef struct
{
	uint32_t Enable:1; // Channel enable
	uint32_t SrcPeri:4; // Source peripheral
	uint32_t Undefined1:1;
	uint32_t DestPeri:4; // Destination peripheral
	uint32_t Undefined2:1;
	uint32_t FlowCntrl:3; // Flow control and transfer type
	uint32_t IntErr:1; // Interrupt error mask
	uint32_t IntTC:1; // Terminal count interrupt mask
	uint32_t Lock:1; // Locked transfer
	uint32_t Active:1; // RO, Actived channel
	uint32_t Halt:1; // Channel halt
	uint32_t Undefined:13;
} rDMACC_Config_t;

typedef struct
{
	rDMACC_Addr_t SrcAddr;
	rDMACC_Addr_t DestAddr;
	rDMACC_LLI_t LLI;
	rDMACC_Control_t Control;
	rDMACC_Config_t Config;
	uint8_t Reserved[0x10 - 0x04];
} rDMACC_t;

typedef struct
{
	uint32_t IntStatus:8; // RO, 1:active
	uint32_t Undefined0:24;

	uint32_t IntTCStatus:8; // RO, 1:active
	uint32_t Undefined1:24;

	uint32_t IntTCClear:8; // WO, 1:clear
	uint32_t Undefined2:24;

	uint32_t IntErrorStatus:8; // RO, 1:error
	uint32_t Undefined3:24;

	uint32_t IntErrClr:8; // WO, 1:clear
	uint32_t Undefined4:24;

	uint32_t RawIntTCStatus:8; // RO, 1:active
	uint32_t Undefined5:24;

	uint32_t RawIntErrorStatus:8; // RO, 1:active
	uint32_t Undefined6:24;

	uint32_t EnabledChannels:8; // RO, 1:active, A bit is cleared on completion of the DMA transfer.
	uint32_t Undefined7:24;

	uint32_t SoftBReq:16; // RW
	uint32_t Undefined8:16;

	uint32_t SoftSReq:16; // RW
	uint32_t Undefined9:16;

	uint32_t SoftLBReq:16; // RW
	uint32_t Undefined10:16;

	uint32_t SoftLSReq:16; // RW
	uint32_t Undefined11:16;

	struct
	{
		uint32_t Enable:1; // 0:disable, 1:enable, reset:0
		uint32_t AHBM1_Endian:1; // 0:little_endian, 1:big_endian, reset:0
		uint32_t AHBM2_Endian:1; // 0:little_endian, 1:big_endian, reset:0
		uint32_t Undefined:29;
	} Config; // RW

	uint32_t Sync:16; // RW, 0:enable, 1:disable
	uint32_t Undefined12:16;

	uint8_t Reserved[0x100 - 0x38];

	rDMACC_t Channel[HIF_DMA_CHANNEL_MAX];
} rDMAC_t;

static volatile rDMAC_t *g_dma_regs = (volatile rDMAC_t *)HIF_DMAC_REG_BASE;

/**********************************************************************************************/

static void _hif_dma_common_regs_print (void)
{
	volatile rDMAC_t *dmac = g_dma_regs;

	_hif_dma_info("[ DMAC Common Registers ]\n");
	_hif_dma_info(" - IntStatus: 0x%02X\n", dmac->IntStatus);
	_hif_dma_info(" - IntTCStatus: 0x%02X\n", dmac->IntTCStatus);
	_hif_dma_info(" - IntTCClear: 0x%02X\n", dmac->IntTCClear);
	_hif_dma_info(" - IntErrorStatus: 0x%02X\n", dmac->IntErrorStatus);
	_hif_dma_info(" - IntErrClr: 0x%02X\n", dmac->IntErrClr);
	_hif_dma_info(" - RawIntTCStatus: 0x%02X\n", dmac->RawIntTCStatus);
	_hif_dma_info(" - RawIntErrorStatus: 0x%02X\n", dmac->RawIntErrorStatus);
	_hif_dma_info(" - Enabledchannels: 0x%02X\n", dmac->EnabledChannels);
	_hif_dma_info(" - SoftBReq: 0x%04X\n", dmac->SoftBReq);
	_hif_dma_info(" - SoftSReq: 0x%04X\n", dmac->SoftSReq);
	_hif_dma_info(" - SoftLBReq: 0x%04X\n", dmac->SoftLBReq);
	_hif_dma_info(" - SoftLSReq: 0x%04X\n", dmac->SoftLSReq);
	_hif_dma_info(" - Configuration\n");
	_hif_dma_info(" -- DMAC_Enable: %u\n", dmac->Config.Enable);
	_hif_dma_info(" -- AHB_M1_Endian: %u\n", dmac->Config.AHBM1_Endian);
	_hif_dma_info(" -- AHB_M2_Endian: %u\n", dmac->Config.AHBM2_Endian);
	_hif_dma_info(" - SyncLogic: 0x%04X\n", dmac->Sync);
}

static void _hif_dma_channel_regs_print (int channel)
{
	volatile rDMACC_t *dmacc = &g_dma_regs->Channel[channel];

	_hif_dma_info("[ DMAC Channel Registers : ch%d ]\n", channel);
	_hif_dma_info(" - SrcAddr: 0x%X\n", dmacc->SrcAddr);
	_hif_dma_info(" - DestAddr: 0x%X\n", dmacc->DestAddr);
	_hif_dma_info(" - LLI\n");
	_hif_dma_info(" -- AHBM: %u\n", dmacc->LLI.AHBM);
	_hif_dma_info(" -- Next: 0x%X(0x%X)\n", dmacc->LLI.Next, dmacc->LLI.Next << 2);
	_hif_dma_info(" - Control\n");
	_hif_dma_info(" -- XferSize: %u\n", dmacc->Control.XferSize);
	_hif_dma_info(" -- SBSize: %u\n", dmacc->Control.SBSize);
	_hif_dma_info(" -- DBSize: %u\n", dmacc->Control.DBSize);
	_hif_dma_info(" -- SWidth: %u\n", dmacc->Control.SWidth);
	_hif_dma_info(" -- DWidth: %u\n", dmacc->Control.DWidth);
	_hif_dma_info(" -- SAHBM: %u\n", dmacc->Control.SAHBM);
	_hif_dma_info(" -- DAHBM: %u\n", dmacc->Control.DAHBM);
	_hif_dma_info(" -- SAInc: %u\n", dmacc->Control.SAInc);
	_hif_dma_info(" -- DAInc: %u\n", dmacc->Control.DAInc);
	_hif_dma_info(" -- Privileged: %u\n", dmacc->Control.Privileged);
	_hif_dma_info(" -- Bufferable: %u\n", dmacc->Control.Bufferable);
	_hif_dma_info(" -- Cacheable: %u\n", dmacc->Control.Cacheable);
	_hif_dma_info(" -- IntTC: %u\n", dmacc->Control.IntTC);
	_hif_dma_info(" - Configuration\n");
	_hif_dma_info(" -- Enable: %u\n", dmacc->Config.Enable);
	_hif_dma_info(" -- SrcPeri: %u\n", dmacc->Config.SrcPeri);
	_hif_dma_info(" -- DestPeri: %u\n", dmacc->Config.DestPeri);
	_hif_dma_info(" -- FlowCntrl: %u\n", dmacc->Config.FlowCntrl);
	_hif_dma_info(" -- IntErr: %u\n", dmacc->Config.IntErr);
	_hif_dma_info(" -- IntTC: %u\n", dmacc->Config.IntTC);
	_hif_dma_info(" -- Lock: %u\n", dmacc->Config.Lock);
	_hif_dma_info(" -- Active: %u\n", dmacc->Config.Active);
	_hif_dma_info(" -- Halt: %u\n", dmacc->Config.Halt);
}

static void _hif_dma_regs_print (void)
{
	int channel;

	_hif_dma_common_regs_print();

	for (channel = 0 ; channel < HIF_DMA_CHANNEL_MAX ; channel++)
		_hif_dma_channel_regs_print(channel);
}

/**********************************************************************************************/

static dma_isr_t g_dmac_inttc_isr[HIF_DMA_CHANNEL_MAX];
static dma_isr_t g_dmac_interr_isr[HIF_DMA_CHANNEL_MAX];

static void _hif_dma_inttc_isr (int vector)
{
	static uint8_t status;
	static int channel;

	while (1)
	{
		status = g_dma_regs->IntTCStatus;
		g_dma_regs->IntTCClear |= status;

		if (!status)
			return;

/*		_hif_dma_debug("inttc_isr: 0x%X\n", status); */

		/* 0: highest priority */
		for (channel = 0 ; channel < HIF_DMA_CHANNEL_MAX ; channel++)
		{
			if (status & (1 << channel))
			{
/*				_hif_dma_channel_regs_print(channel); */

				if (g_dmac_inttc_isr[channel])
					g_dmac_inttc_isr[channel](channel);
			}
		}
	}
}

static void _hif_dma_interr_isr (int vector)
{
	static uint8_t status;
	static int channel;

	while (1)
	{
		status = g_dma_regs->IntErrorStatus;
		g_dma_regs->IntErrClr |= status;

		if (!status)
			return;

/*		_hif_dma_debug("interr_isr: 0x%X\n", status); */

		/* 0: highest priority */
		for (channel = 0 ; channel < HIF_DMA_CHANNEL_MAX ; channel++)
		{
			if (status & (1 << channel))
			{
/*				_hif_dma_channel_regs_print(channel); */

				if (g_dmac_interr_isr[channel])
					g_dmac_interr_isr[channel](channel);
			}
		}
	}
}

static void _hif_dma_inttc_clear (int channel)
{
	volatile rDMAC_t *dmac = g_dma_regs;

	if (_hif_dma_valid_channel(channel))
		dmac->IntTCClear |= 1 << channel;
	else if (channel == HIF_DMA_CHANNEL_MAX)
		dmac->IntTCClear = ~0;
}

static void _hif_dma_interr_clear (int channel)
{
	volatile rDMAC_t *dmac = g_dma_regs;

	if (_hif_dma_valid_channel(channel))
		dmac->IntErrClr |= 1 << channel;
	else if (channel == HIF_DMA_CHANNEL_MAX)
		dmac->IntErrClr = ~0;
}

static void _hif_dma_interrupt_enable (void)
{
	volatile rDMAC_t *dmac = g_dma_regs;

	_hif_dma_inttc_clear(HIF_DMA_CHANNEL_MAX);
	_hif_dma_interr_clear(HIF_DMA_CHANNEL_MAX);

	system_register_isr(EV_DMACINTTC, _hif_dma_inttc_isr);
	system_register_isr(EV_DMACINTERR, _hif_dma_interr_isr);

	system_irq_unmask(EV_DMACINTTC); // priority: 3
	system_irq_unmask(EV_DMACINTERR); // priority: 1
}

static void _hif_dma_interrupt_disable (void)
{
	volatile rDMAC_t *dmac = g_dma_regs;

	_hif_dma_inttc_clear(HIF_DMA_CHANNEL_MAX);
	_hif_dma_interr_clear(HIF_DMA_CHANNEL_MAX);

	system_register_isr(EV_DMACINTTC, NULL);
	system_register_isr(EV_DMACINTERR, NULL);
}

static int _hif_dma_config (int channel,
							dma_peri_t *src_peri, dma_peri_t *dest_peri,
							dma_isr_t inttc_isr, dma_isr_t interr_isr,
							bool locked_transfer)
{
	const char *str_transfer_type[] =
	{
		"M2M", "M2P", "P2M", "P2P",
		"P2P(FC)", "M2P(FC)", "(FC)P2M", "(FC)P2P",
	};
	volatile rDMACC_Config_t *dmacc_config;

	if (!_hif_dma_is_enabled())
		return HIF_DMA_EPERM;

	if (!_hif_dma_valid_channel(channel))
		return HIF_DMA_EINVAL;

	dmacc_config = &g_dma_regs->Channel[channel].Config;

	if (dmacc_config->Enable)
		return HIF_DMA_EBUSY;

	dmacc_config->FlowCntrl = 0;

	if (!src_peri)
		dmacc_config->SrcPeri = 0; // memory
	else
	{
		if (src_peri->ID >= HIF_DMA_PERI_ID_MAX)
			return HIF_DMA_EINVAL;

		dmacc_config->SrcPeri = src_peri->ID;
		dmacc_config->FlowCntrl |= (1 << 1);

		if (src_peri->FlowCtrl)
		{
			if (dest_peri && dest_peri->FlowCtrl)
			{
				_hif_dma_error("%s: src_peri->FlowCtrl && dest_peri->FlowCtrl\n", __func__);
				return HIF_DMA_EINVAL;
			}

			dmacc_config->FlowCntrl |= (1 << 2);
		}
	}

	if (!dest_peri)
		dmacc_config->DestPeri = 0; // memory
	else
	{
		if (dest_peri->ID >= HIF_DMA_PERI_ID_MAX)
			return HIF_DMA_EINVAL;

		dmacc_config->DestPeri = dest_peri->ID;
		dmacc_config->FlowCntrl |= (1 << 0);

		if (dest_peri->FlowCtrl)
		{
			if (src_peri && src_peri->FlowCtrl)
				return HIF_DMA_EINVAL;

			dmacc_config->FlowCntrl |= (1 << 2);

			if (dmacc_config->FlowCntrl == 7)
				dmacc_config->FlowCntrl = 4; // peripheral to peripheral(fc)
		}
	}

	dmacc_config->IntTC = inttc_isr ? 1 : 0;
	dmacc_config->IntErr = interr_isr ? 1 : 0;
	dmacc_config->Lock = locked_transfer ? 1 : 0;

	if (inttc_isr)
		g_dmac_inttc_isr[channel] = inttc_isr;

	if (interr_isr)
		g_dmac_interr_isr[channel] = interr_isr;

	_hif_dma_info("DMA_CONFIG: channel=%d type=%s\n", channel,
				str_transfer_type[dmacc_config->FlowCntrl]);

	return 0;
}

/**********************************************************************************************/

void _hif_dma_enable (void)
{
	volatile rDMAC_t *dmac = g_dma_regs;

	if (!dmac->Config.Enable)
	{
		_hif_dma_interrupt_enable();

		dmac->Config.Enable = 1;
		dmac->Config.AHBM1_Endian = 0; // little endian
		dmac->Config.AHBM2_Endian = 0; // little endian
		dmac->Sync = 0; // disable

		_hif_dma_info("DMA_ENABLE\n");
	}
}

void _hif_dma_disable (void)
{
	volatile rDMAC_t *dmac = g_dma_regs;

	if (dmac->Config.Enable)
	{
		int channel;

		for (channel = 0 ; channel < HIF_DMA_CHANNEL_MAX ; channel++)
		{
			if (dmac->EnabledChannels & (1 << channel))
				_hif_dma_stop(channel);
		}

		dmac->Config.Enable = 0;

		_hif_dma_interrupt_disable();

		_hif_dma_info("DMA_DISABLE\n");
	}
}

bool _hif_dma_is_enabled (void)
{
	volatile rDMAC_t *dmac = g_dma_regs;

	return !!dmac->Config.Enable;
}

int _hif_dma_get_channel (int highest)
{
	volatile rDMAC_t *dmac = g_dma_regs;
	int channel;

	if (highest)
	{
		for (channel = 0 ; channel < HIF_DMA_CHANNEL_MAX ; channel++)
		{
			if (!(dmac->EnabledChannels & (1 << channel)))
				return channel;
		}
	}
	else
	{
		for (channel = HIF_DMA_CHANNEL_MAX - 1 ; channel >= 0 ; channel--)
		{
			if (!(dmac->EnabledChannels & (1 << channel)))
				return channel;
		}
	}

	return -1;
}

bool _hif_dma_valid_channel (int channel)
{
	if (channel < 0 || channel >= HIF_DMA_CHANNEL_MAX)
		return false;

	return true;
}

int _hif_dma_config_m2m (int channel,
						dma_isr_t inttc_isr, dma_isr_t interr_isr)
{
	return _hif_dma_config(channel, NULL, NULL, inttc_isr, interr_isr, false);
}

int _hif_dma_config_m2p (int channel,
						dma_peri_t *dest,
						dma_isr_t inttc_isr, dma_isr_t interr_isr)
{
	return _hif_dma_config(channel, NULL, dest, inttc_isr, interr_isr, false);
}

int _hif_dma_config_p2m (int channel,
						dma_peri_t *src,
						dma_isr_t inttc_isr, dma_isr_t interr_isr)
{
	return _hif_dma_config(channel, src, NULL, inttc_isr, interr_isr, false);
}

int _hif_dma_config_p2p (int channel,
						dma_peri_t *src, dma_peri_t *dest,
						dma_isr_t inttc_isr, dma_isr_t interr_isr)
{
	return _hif_dma_config(channel, src, dest, inttc_isr, interr_isr, false);
}

int _hif_dma_start (int channel, dma_desc_t *desc)
{
	volatile rDMACC_t *dmacc;

	if (!_hif_dma_is_enabled())
		return HIF_DMA_EPERM;

	if (!_hif_dma_valid_channel(channel) || !desc)
		return HIF_DMA_EINVAL;

	dmacc = &g_dma_regs->Channel[channel];

	if (dmacc->Config.Enable)
		return HIF_DMA_EBUSY;

	_hif_dma_inttc_clear(channel);
	_hif_dma_interr_clear(channel);

	memcpy((void *)dmacc, (void *)desc, sizeof(dma_desc_t));

	_hif_dma_info("DMA_START: channel=%d\n", channel);
/*	_hif_dma_channel_regs_print(channel); */

	dmacc->Config.Enable = 1;

	return 0;
}

int _hif_dma_stop (int channel)
{
	volatile rDMACC_t *dmacc;

	if (!_hif_dma_is_enabled())
		return HIF_DMA_EPERM;

	if (!_hif_dma_valid_channel(channel))
		return HIF_DMA_EINVAL;

	dmacc = &g_dma_regs->Channel[channel];

	if (dmacc->Config.Enable)
	{
		if (dmacc->Config.Active)
		{
			dmacc->Config.Halt = 1;
			while (dmacc->Config.Active) _delay_ms(1);
			dmacc->Config.Halt = 0;
		}

		dmacc->Config.Enable = 0;

		_hif_dma_inttc_clear(channel);
		_hif_dma_interr_clear(channel);

	}

	_hif_dma_info("DMA_STOP: channel=%d\n", channel);

	return 0;
}

bool _hif_dma_busy (int channel)
{
	volatile rDMACC_t *dmacc;

	if (!_hif_dma_is_enabled())
		return HIF_DMA_EPERM;

	if (!_hif_dma_valid_channel(channel))
		return HIF_DMA_EINVAL;

	dmacc = &g_dma_regs->Channel[channel];

	return !!dmacc->Config.Enable;
}

uint32_t _hif_dma_src_addr (int channel)
{
	volatile rDMACC_t *dmacc = &g_dma_regs->Channel[channel];

	return dmacc->SrcAddr;
}

uint32_t _hif_dma_dest_addr (int channel)
{
	volatile rDMACC_t *dmacc = &g_dma_regs->Channel[channel];

	return dmacc->DestAddr;
}

/**********************************************************************************************/

int _hif_dma_peri_init (dma_peri_t *peri, int id, uint32_t addr, bool addr_inc, bool flow_ctrl)
{
	if (!peri)
		return HIF_DMA_EINVAL;

	if (id < 0 || id >= HIF_DMA_PERI_ID_MAX)
		return HIF_DMA_EINVAL;

	peri->ID = id;
	peri->Addr = addr;
	peri->AddrInc = addr_inc ? 1 : 0;
	peri->FlowCtrl = flow_ctrl ? 1 : 0;

	return 0;
}

/**********************************************************************************************/

void _hif_dma_desc_print (dma_desc_t *desc)
{
	const char *str_onoff[2] = { "off", "on" };
	dma_desc_t *_desc = desc;
	int i;

	for (i = 0 ; desc ; i++, desc = desc->Next == _desc ? NULL : desc->Next)
	{
		_hif_dma_info("[ DMA Descriptor %d : 0x%X ]\n", i, desc);
		_hif_dma_info(" - SrcAddr: 0x%X\n", desc->SrcAddr);
		_hif_dma_info(" - DestAddr: 0x%X\n", desc->DestAddr);
		_hif_dma_info(" - NextDesc: 0x%X\n", desc->Next);
		_hif_dma_info(" - XferSize: %.f-byte\n", desc->XferSize * pow(2, desc->SWidth));
		_hif_dma_info(" - SBSize: %.f-byte\n", desc->SBSize ? pow(2, desc->SBSize + 1) : 1);
		_hif_dma_info(" - DBSize: %.f-byte\n", desc->DBSize ? pow(2, desc->SBSize + 1) : 1);
		_hif_dma_info(" - SWidth: %.f-bit\n", 8 * pow(2, desc->SWidth));
		_hif_dma_info(" - DWidth: %.f-bit\n", 8 * pow(2, desc->DWidth));
		_hif_dma_info(" - SAHBM: %u\n", desc->SAHBM);
		_hif_dma_info(" - DAHBM: %u\n", desc->DAHBM);
		_hif_dma_info(" - SAInc: %s\n", str_onoff[desc->SAInc]);
		_hif_dma_info(" - DAInc: %s\n", str_onoff[desc->DAInc]);
		_hif_dma_info(" - Privileged: %s\n", str_onoff[desc->Privileged]);
		_hif_dma_info(" - Bufferable: %s\n", str_onoff[desc->Bufferable]);
		_hif_dma_info(" - Cacheable: %s\n", str_onoff[desc->Cacheable]);
		_hif_dma_info(" - IntTC: %s\n", str_onoff[desc->IntTC]);
		_hif_dma_info("\r\n");
	}
}

int _hif_dma_desc_init (dma_desc_t *desc, uint32_t src_addr, uint32_t dest_addr, uint16_t size)
{
	if (!desc || !src_addr || !dest_addr)
		return HIF_DMA_EINVAL;

	if (size >= HIF_DMA_XFER_SIZE_MAX)
		return HIF_DMA_EINVAL;

	desc->SrcAddr = src_addr;
	desc->DestAddr = dest_addr;
	desc->Next = 0;
	desc->XferSize = size;
	desc->SBSize = HIF_DMA_BSIZE_1;
	desc->DBSize = HIF_DMA_BSIZE_1;
	desc->SWidth = HIF_DMA_WIDTH_8;
	desc->DWidth = HIF_DMA_WIDTH_8;
	desc->SAHBM = HIF_DMA_AHB_M1;
	desc->DAHBM = HIF_DMA_AHB_M1;
	desc->SAInc = 0;
	desc->DAInc = 0;
	desc->Privileged = 0;
	desc->Bufferable = 0;
	desc->Cacheable = 0;
	desc->IntTC = 0;

	return 0;
}

int _hif_dma_desc_link (dma_desc_t *desc, dma_desc_t *next)
{
	if (!desc || !next)
		return HIF_DMA_EINVAL;

	if ((uint32_t)next & 0x3)
	{
		_hif_dma_error("next=0x%X\n", next);
		return HIF_DMA_EINVAL;
	}

	desc->Next = next;

	return 0;
}

int _hif_dma_desc_set_addr (dma_desc_t *desc, uint32_t src_addr, uint32_t dest_addr)
{
	if (!desc || !src_addr | !dest_addr)
		return HIF_DMA_EINVAL;

	desc->SrcAddr = src_addr;
	desc->DestAddr = dest_addr;

	return 0;
}

int _hif_dma_desc_set_addr_inc (dma_desc_t *desc, bool src_inc, bool dest_inc)
{
	if (!desc)
		return HIF_DMA_EINVAL;

	desc->SAInc = src_inc ? 1 : 0;
	desc->DAInc = dest_inc ? 1 : 0;

	return 0;
}

int _hif_dma_desc_set_size (dma_desc_t *desc, uint16_t size)
{
	if (!desc)
		return HIF_DMA_EINVAL;

	if (size >= HIF_DMA_XFER_SIZE_MAX)
		return HIF_DMA_EINVAL;

	desc->XferSize = size >> desc->SWidth;

	return 0;
}

int _hif_dma_desc_set_width (dma_desc_t *desc, uint8_t src_width, uint8_t dest_width)
{
	if (!desc)
		return HIF_DMA_EINVAL;

	if ((src_width >= HIF_DMA_WIDTH_MAX) || (dest_width >= HIF_DMA_WIDTH_MAX))
		return HIF_DMA_EINVAL;

	if (src_width != desc->SWidth)
	{
		desc->XferSize <<= desc->SWidth;
		desc->XferSize >>= src_width;
	}

	desc->SWidth = src_width;
	desc->DWidth = dest_width;

	return 0;
}

int _hif_dma_desc_set_bsize (dma_desc_t *desc, uint8_t src_bsize, uint8_t dest_bsize)
{
	if (!desc)
		return HIF_DMA_EINVAL;

	if ((src_bsize >= HIF_DMA_BSIZE_MAX) || (dest_bsize >= HIF_DMA_BSIZE_MAX))
		return HIF_DMA_EINVAL;

	desc->SBSize = src_bsize;
	desc->DBSize = dest_bsize;

	return 0;
}

int _hif_dma_desc_set_inttc (dma_desc_t *desc, bool inttc)
{
	if (!desc)
		return HIF_DMA_EINVAL;

	desc->IntTC = inttc ? 1 : 0;

	return 0;
}

int _hif_dma_desc_set_ahb_master (dma_desc_t *desc, int src_ahbm, int dest_ahbm)
{
	if (!desc)
		return HIF_DMA_EINVAL;

	if ((src_ahbm >= HIF_DMA_AHB_MAX) || (dest_ahbm >= HIF_DMA_AHB_MAX))
		return HIF_DMA_EINVAL;

	desc->SAHBM = src_ahbm;
	desc->DAHBM = dest_ahbm;

	return 0;
}

int _hif_dma_desc_set_protection (dma_desc_t *desc, bool privileged, bool bufferable, bool cacheable)
{
	if (!desc)
		return HIF_DMA_EINVAL;

	desc->Privileged = privileged ? 1 : 0;
	desc->Bufferable = bufferable ? 1 : 0;
	desc->Cacheable = cacheable ? 1 : 0;

	return 0;
}

