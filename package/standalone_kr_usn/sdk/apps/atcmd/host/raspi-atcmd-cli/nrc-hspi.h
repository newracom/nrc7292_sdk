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

#ifndef __NRC_HSPI_H__
#define __NRC_HSPI_H__
/**********************************************************************************************/

#include "common.h"

/**
 *	H-SPI Host-Side Registers
 **********************************************************************************************/
#define HSPI_REG_SYS					0x00
#define HSPI_REG_WAKEUP					(HSPI_REG_SYS + 0x00)
#define HSPI_REG_DEVICE_STATUS			(HSPI_REG_SYS + 0x01)
#define HSPI_REG_CHIP_ID				(HSPI_REG_SYS + 0x02)
#define HSPI_REG_MODEM_ID				(HSPI_REG_SYS + 0x04)
#define HSPI_REG_SW_ID					(HSPI_REG_SYS + 0x08)
#define HSPI_REG_BOARD_ID				(HSPI_REG_SYS + 0x0C

#define HSPI_REG_IRQ					0x10
#define HSPI_REG_EIRQ_MODE				(HSPI_REG_IRQ + 0x00)
#define HSPI_REG_EIRQ_ENABLE			(HSPI_REG_IRQ + 0x01)
#define HSPI_REG_EIRQ_STATUS_LATCH		(HSPI_REG_IRQ + 0x02)
#define HSPI_REG_EIRQ_STATUS			(HSPI_REG_IRQ + 0x03)
#define HSPI_REG_QUEUE_STATUS			(HSPI_REG_IRQ + 0x04)

#define HSPI_REG_EIRQ					HSPI_REG_EIRQ_MODE
#define HSPI_REG_STATUS					HSPI_REG_EIRQ_STATUS_LATCH

#define HSPI_REG_MSG					0x20
#define HSPI_REG_DEV_MSG_0				(HSPI_REG_MSG + 0x00)
#define HSPI_REG_DEV_MSG_1				(HSPI_REG_MSG + 0x04)
#define HSPI_REG_DEV_MSG_2				(HSPI_REG_MSG + 0x08)
#define HSPI_REG_DEV_MSG_3				(HSPI_REG_MSG + 0x0C)

#define HSPI_REG_RXQ					0x30
#define HSPI_REG_RXQ_THRES				(HSPI_REG_RXQ + 0x00)
#define HSPI_REG_RXQ_WINDOW				(HSPI_REG_RXQ + 0x01)

#define HSPI_REG_TXQ					0x40
#define HSPI_REG_TXQ_THRES				(HSPI_REG_TXQ + 0x00)
#define HSPI_REG_TXQ_WINDOW				(HSPI_REG_TXQ + 0x01)

typedef	struct
{
	uint8_t wake_up; /* Write 0x79, write only */

	union
	{
		uint8_t reset; /* Write 0xC8 for reset, write only */

		struct
		{
			uint8_t ready:1;
			uint8_t sleep:1;
			uint8_t reserved:6;
		} status; /* read only */
	};

	uint16_t chip_id;  /* hw fix, read only */
	uint32_t modem_id; /* hw fix, read only */
	uint32_t sw_id;	   /* fw write, read only */
	uint32_t board_id; /* fw write, read only */
} hspi_sys_t;

#define HSPI_EIRQ_LOW		0
#define HSPI_EIRQ_HIGH		1
#define HSPI_EIRQ_LEVEL		0
#define HSPI_EIRQ_EDGE		(1<<1)

#define HSPI_EIRQ_TXQ		1
#define HSPI_EIRQ_RXQ		(1<<1)
#define HSPI_EIRQ_READY		(1<<2)
#define HSPI_EIRQ_SLEEP		(1<<3)
#define HSPI_EIRQ_ALL		0xF

typedef union
{
	uint8_t val[2];

	struct
	{
		struct
		{
			uint8_t active:1;	 /* 0:low, 1:high */
			uint8_t trigger:1; 	 /* 0:level, 1:edge */
			uint8_t io_enable:1; /* 0:disable, 1:enable */
			uint8_t reserved:5;
		} mode;

		struct
		{
			uint8_t txq:1;
			uint8_t rxq:1;
			uint8_t ready:1;
			uint8_t sleep:1;
			uint8_t reserved:4;
		} enable;
	};
} hspi_eirq_t;

typedef struct
{
#define HSPI_SLOT_CNT_MAX		0x3F

	uint8_t latch; /* Read others after read this value, and then interrupt is cleared. */

	struct
	{
		uint8_t txq:1;
		uint8_t rxq:1;
		uint8_t ready:1;
		uint8_t sleep:1;
		uint8_t reserved:4;
	} eirq;

	struct
	{
		uint8_t error; 	  /* valid bits : 7-bit */
		uint8_t slot_cnt; /* valid bits : 7-bit */
		uint16_t slot_size;
		uint16_t total_slot_size;
	} txq; /* target -> host */

	struct
	{
		uint8_t error; 	  /* valid bits : 7-bit */
		uint8_t slot_cnt; /* valid bits : 7-bit */
		uint16_t slot_size;
		uint16_t total_slot_size;
	} rxq; /* host -> target */

} hspi_status_t; /* read only */

typedef uint32_t hspi_msg_t[4]; /* read only */

typedef	struct
{
	uint8_t reserved;
	uint8_t window; /* rxq: write only, txq: read only */
} hspi_que_t;

typedef struct
{
	hspi_sys_t sys;
	hspi_eirq_t eirq;
	hspi_status_t status;
	hspi_msg_t msg;
	hspi_que_t rxq; /* uplink */
	hspi_que_t txq; /* downlink */
} hspi_regs_t;


/**
 *	H-SPI Opcode
 **********************************************************************************************/

#define HSPI_START				(0x50 << 24)
#define HSPI_BURST				(1 << 23)
#define HSPI_READ				(0 << 22)
#define HSPI_WRITE				(1 << 22)
#define HSPI_FIXED				(1 << 21)
#define HSPI_ADDR(addr)			(addr << 13)
#define HSPI_LENGTH(length)		(length)
#define HSPI_DATA(data)			(data)

#define HSPI_OPCODE_READ_REG(addr, length) \
	(HSPI_START|HSPI_ADDR(addr)|(length > 1 ? HSPI_BURST|HSPI_LENGTH(length) : HSPI_FIXED))

#define HSPI_OPCODE_WRITE_REG(addr, data) \
	(HSPI_START|HSPI_WRITE|HSPI_FIXED|HSPI_ADDR(addr)|HSPI_DATA(data))

#define HSPI_OPCODE_READ_DATA(addr, length) \
	(HSPI_START|HSPI_BURST|HSPI_FIXED|HSPI_ADDR(addr)|HSPI_LENGTH(length))

#define HSPI_OPCODE_WRITE_DATA(addr, length) \
	(HSPI_START|HSPI_BURST|HSPI_FIXED|HSPI_WRITE|HSPI_ADDR(addr)|HSPI_LENGTH(length))

typedef union
{
	uint32_t val;

	struct
	{
		uint32_t length:13;			/* burst length */
		/* uint32_t write_data:8; *//* single data */
		/* uint32_t reserved:5; */

		uint32_t address:8;			/* addresss */
		uint32_t fixed:1;			/* 0:incremental, 1:fixed */
		uint32_t write:1;			/* 0:read, 1:write */
		uint32_t burst:1; 			/* 0:single, 1:burst */
		uint32_t start:8; 			/* 0x50 */
	};
} hspi_opcode_t; /* 4-byte */


/**
 *	H-SPI Transfter
 **********************************************************************************************/

#define HSPI_ACK_VALUE			0x47

enum
{
	HSPI_TXQ = 0,	/* from target to host */
	HSPI_RXQ,	  	/* from host to target */

	HSPI_QUE_NUM,
	HSPI_QUE_ALL = HSPI_QUE_NUM
};

typedef struct
{
	hspi_opcode_t opcode;

	uint8_t crc;
	uint8_t reserved[3];
} hspi_cmd_t; /* 8-byte */

typedef struct
{
	uint8_t reserved[6];

	uint8_t data; /* single data */
	uint8_t ack;
} hspi_resp_t; /* 8-byte */

typedef struct
{
#define HSPI_XFER_NUM_MAX		10
#define HSPI_XFER_LEN_MAX		2048
#define HSPI_XFER_RETRY_MAX		5

	int len;
	char *tx_buf;
	char *rx_buf;
} hspi_xfer_t;

typedef struct
{
#define HSPI_SLOT_SIZE_MAX		512
#define HSPI_SLOT_HDR_SIZE		4 /* 2 + 1 + 1 */
#define HSPI_SLOT_START_SIZE	2
#define HSPI_SLOT_START			"HS"
#define HSPI_SLOT_SEQ_MAX		0x3F

	uint8_t start[2];
	uint16_t len:10;
	uint16_t seq:6;

	uint8_t data[0];
} hspi_slot_t;

typedef struct
{
	int (*spi_transfer) (char *tx_buf, char *rx_buf, int len);
} hspi_ops_t;

typedef struct
{
	int active;

	struct
	{
		struct
		{
			uint16_t num;
			uint16_t size;
		} slot[HSPI_QUE_NUM];

		hspi_status_t status;
	} queue;

	hspi_ops_t ops;
} hspi_info_t;


/**
 *	H-SPI Functions
 **********************************************************************************************/

extern int nrc_hspi_open (hspi_ops_t *ops);
extern void nrc_hspi_close (void);

extern int nrc_hspi_read (char *data, int len);
extern int nrc_hspi_write (char *data, int len);

/**********************************************************************************************/
#endif /* #ifndef __NRC_HSPI_H__ */

