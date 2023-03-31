
#ifndef _DEV_CONFIG_H_
#define _DEV_CONFIG_H_

#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "Debug.h"

/**
 * data
**/
#define UBYTE   uint8_t
#define UWORD   uint16_t
#define UDOUBLE uint32_t

/**
 * GPIOI config
**/
extern int EPD_RST_PIN;
extern int EPD_DC_PIN;
extern int EPD_CS_PIN;
extern int EPD_BUSY_PIN;

struct EPD_IO_PINS
{
	int BUSY;
	int RST;
	int DC;
	int CS;
	int SCL;
	int SDA;
	int PWR;
};

/*-----------------------------------------------------------------------*/

void DEV_Digital_Write(UWORD Pin, UBYTE Value);
UBYTE DEV_Digital_Read(UWORD Pin);

void DEV_SPI_WriteByte(UBYTE Value);
void DEV_SPI_Write_nByte(UBYTE *pData, UDOUBLE Len);
void DEV_Delay_ms(UDOUBLE xms);

int DEV_Module_Init(struct EPD_IO_PINS *pins);
void DEV_Module_Exit(void);

#define NRC_EPD_IO_Enable(pins)		DEV_Module_Init(pins)
#define NRC_EPD_IO_Disable(pins)	DEV_Module_Exit()

#endif /* #ifndef _DEV_CONFIG_H_ */
