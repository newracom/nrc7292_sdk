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

#include "nrc_sdk.h"
#include "lwip/sys.h"
#include "lwip/sockets.h"
#include "lwip/errno.h"
#include "wifi_config_setup.h"
#include "wifi_connect_common.h"

#define READ_RSSI_SNR 0
#define PERIODIC_SCAN 0

#define USE_UART_INTERRUPT 1

#define MAX_BUFFER_BYTE 1000


/* Connection param */
#define REMOTE_PORT 8088
#define RECV_BUF_SIZE (1024 * 2)
#define RECVFROM_WAITING_TIME 0 /* seconds */

static int s_uart_ch = NRC_UART_CH2;	// USE UART2
char buffer[MAX_BUFFER_BYTE];
int buffer_len = 0;
int count_buf = 0;
uint32_t discard_count = 0;
bool progress_tx = false;

#if USE_UART_INTERRUPT
static void uart_intr_handler(int vector)
{
	char ch;

	while (1) {
		if (nrc_uart_get(s_uart_ch, &ch) < 0) {
			break;
		}
		nrc_uart_put(s_uart_ch, ch);	 //Echo for test
	}
}
#endif

static int uart_init(NRC_UART_CONFIG* conf)
{
	if (nrc_uart_set_channel(conf->ch) == 0) {
		nrc_uart_set_config(conf);

#if USE_UART_INTERRUPT
		nrc_uart_clear_interrupt(conf->ch, true, true , true);
		nrc_uart_register_interrupt_handler(conf->ch, uart_intr_handler);
		nrc_uart_set_interrupt(conf->ch, true, true);
#else
		nrc_uart_clear_interrupt(conf->ch, true, true , true);
		nrc_uart_set_interrupt(conf->ch, false, false);
#endif
	} else {
		nrc_usr_print("[%s] Fail to set channel (%d) \n", __func__, conf->ch);
		return -1;
	}

	return 0;
}


char HalowRxBuffer[10]={'0', '1','2','3','4','5','6','7','8','9'};
void SendtoDevice(NRC_UART_CONFIG* conf, int packetLen)
{
	unsigned char ucLoopCount = 0;
	nrc_usr_print("[%s] SendtoDevice :%d CH:%d\n",__func__, packetLen, conf->ch);

	for(ucLoopCount=0; ucLoopCount<packetLen; ucLoopCount++){
		if(NRC_FAIL == nrc_uart_put(conf->ch,HalowRxBuffer[(ucLoopCount)%10]))
			nrc_usr_print("Fail\n");
		_delay_ms(10);
	}
}

/******************************************************************************
 * FunctionName : user_init
 * Description  : Start Code for User Application, Initialize User function
 * Parameters   : none
 * Returns      : none
 *******************************************************************************/
void user_init(void)
{
	NRC_UART_CONFIG uart_config;

	nrc_uart_console_enable(true);

	uart_config.ch = NRC_UART_CH2;
	uart_config.db = NRC_UART_DB8;
	uart_config.br = 115200;
	uart_config.stop_bit = NRC_UART_SB1;
	uart_config.parity_bit = NRC_UART_PB_NONE;
	uart_config.hw_flow_ctrl = NRC_UART_HFC_DISABLE;
	//uart_config.fifo = NRC_UART_FIFO_DISABLE;
	uart_config.fifo = NRC_UART_FIFO_ENABLE;

	uart_init(&uart_config); // UART INIT
	SendtoDevice(&uart_config, 50000);

}
