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

		//Save Char into buffer except Carriage return
		if (ch != '\r') {
			if (count_buf >= MAX_BUFFER_BYTE) {
				++discard_count;
				nrc_usr_print("[%s] Buffer Overflow!! Discard:%c (count:%d vs MAX:%d)\n",
						__func__, ch, count_buf, MAX_BUFFER_BYTE);
			} else {
				buffer[count_buf++] = ch;
			}
		} else {
			if (!progress_tx) {
				if (count_buf) {
					//Buffer is Ready. Just send buffer.
					//nrc_usr_print("[%s] Ready to send (len:%d, progress_tx:%d)\n", __func__, count_buf, progress_tx);
					if (count_buf > MAX_BUFFER_BYTE) {
						buffer_len = MAX_BUFFER_BYTE;
					} else {
						buffer_len = count_buf;
					}
					progress_tx = true;
					count_buf =0;
				} else {
					nrc_usr_print("[%s] Get CR but no Buffer Data (len:%d, progress_tx:%d)\n", __func__, count_buf, progress_tx);
				}
			}
		}
	}
}
#endif

static int uart_init(void)
{
	NRC_UART_CONFIG uart_config;
	uart_config.ch = s_uart_ch;
	uart_config.db = NRC_UART_DB8;
	uart_config.br = 115200;
	uart_config.stop_bit = NRC_UART_SB1;
	uart_config.parity_bit = NRC_UART_PB_NONE;
	uart_config.hw_flow_ctrl = NRC_UART_HFC_DISABLE;
	uart_config.fifo = NRC_UART_FIFO_DISABLE;
	//uart_config.fifo = NRC_UART_FIFO_ENABLE;

	if (nrc_uart_set_channel(s_uart_ch) == 0) {
		nrc_uart_set_config(&uart_config);

#if USE_UART_INTERRUPT
		nrc_uart_clear_interrupt(s_uart_ch, true, true , true);
		nrc_uart_register_interrupt_handler(s_uart_ch, uart_intr_handler);
		nrc_uart_set_interrupt(s_uart_ch, true, true);
#else
		nrc_uart_clear_interrupt(s_uart_ch, true, true , true);
		nrc_uart_set_interrupt(s_uart_ch, false, false);
#endif
	} else {
		nrc_usr_print("[%s] Fail to set channel (%d) \n", __func__, s_uart_ch);
		return -1;
	}

	return 0;
}

#if READ_RSSI_SNR
void report_signal(const timer_id id)
{
	nrc_usr_print("[%s] RSSI:%d, SNR:%d\n", __func__, nrc_wifi_get_rssi(), nrc_wifi_get_snr());
}
#endif

#if PERIODIC_SCAN
bool g_scan_trigger = false;
void periodic_scan(const timer_id id)
{
	g_scan_trigger = true;
}

void scan_once(void)
{
	tWIFI_STATE_ID wifi_state = WIFI_STATE_INIT;

	if (!g_scan_trigger)
		return;

	SCAN_RESULTS results;

	nrc_wifi_get_state(&wifi_state);
	if(wifi_state != WIFI_STATE_GET_IP)
		return;

	if (nrc_wifi_scan()!= WIFI_SUCCESS) {
		nrc_usr_print ("[%s] Fail to scan\n", __func__);
		return;
	}

	if (nrc_wifi_scan_results(&results)!= WIFI_SUCCESS) {
		nrc_usr_print ("[%s] Fail to scan\n", __func__);
		return;
	} else {
		nrc_usr_print("\n[%s] scan results(num_ap:%d)\n", __func__, results.n_result);
		for (int i=0; i <results.n_result ;i++) {
			nrc_usr_print("[%s] bssid:%s, freq:%s, sig_level:%s, flags:%s, ssid:%s\n", __func__,
				results.result[i].bssid, results.result[i].freq,
				results.result[i].sig_level, results.result[i].flags,results.result[i].ssid);
		}
	}

	g_scan_trigger = false;
}
#endif

static int recv_from_timeout(int sockfd, long sec, long usec)
{
	/* Setup timeval variable */
	struct timeval timeout;
	timeout.tv_sec = sec;
	timeout.tv_usec = usec;

	/* Setup fd_set structure */
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(sockfd, &fds);

	/* Return value: -1(error occured), 0(timed out), >0(data ready to be read) */
	return select(sockfd+1 , &fds, NULL, NULL, &timeout);
}

static int error_val = 0;
static void udp_uart_send_task(void *pvParameters)
{
	char rx_buffer[RECV_BUF_SIZE];
	int n = 0;
	int destlen;
	int sockfd;
	struct sockaddr_in dest_addr;
	WIFI_CONFIG *param = pvParameters;
	int count = param->count;
	int recv_count = 0;


	uint32_t total_sent =0;
	uint32_t start_time =0;
	uint32_t end_time =0;

	uart_init(); // UART INIT

	/* Assign test is running */
	param->test_running = 1;

	start_time = sys_now();

	sockfd =  socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) {
		error_val = -1;
		nrc_usr_print("Fail to create socket\n");
		goto exit;
	}

	/* build the destination's Internet address */
	bzero((char *) &dest_addr, sizeof(dest_addr));

	// Filling server information
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htons(REMOTE_PORT);
	dest_addr.sin_addr.s_addr = inet_addr((const char *)param->remote_addr);

	while (1) {

#if USE_UART_INTERRUPT
		if(progress_tx) {
			nrc_usr_print("[%s][%d]\n",__func__,__LINE__);
			/* send data with the specified port */
			n = sendto(sockfd, (const char *)buffer, buffer_len, 0,
				(const struct sockaddr *) &dest_addr, sizeof(dest_addr));

			if (n < 0) {
				error_val = -1;
				nrc_usr_print("Error occurred during sending\n");
				goto exit;
			}
			progress_tx = false;

			if ( count == ++recv_count) {
				goto exit;
			}
		}
#else
		char ch0;
		if (nrc_uart_get(s_uart_ch, &ch0) == 0) {
			nrc_uart_put(s_uart_ch, ch0);
			/* Send UDP Data */
			if (ch != '\r') {
				buffer[count_buf++] = ch0;
			} else {
				if (count_buf > MAX_BUFFER_BYTE)
					count_buf = MAX_BUFFER_BYTE;
				nrc_usr_print("[%s] Send UDP data (len:%d)\n", __func__, count_buf);
				n = sendto(sockfd, (const char *)buffer, count_buf, 0,
					(const struct sockaddr *) &dest_addr, sizeof(dest_addr));

				if (n < 0) {
					error_val = -1;
					nrc_usr_print("Error occurred during sending\n");
					goto exit;
				}
				count_buf = 0;
				if ( count == ++recv_count) {
					goto exit;
				}
			}
		}
#endif


		_delay_ms(10);
	}
	error_val = 0;


exit:
	if (sockfd >= 0) {
		nrc_usr_print("Shutting down and close socket\n");
		shutdown(sockfd, SHUT_RDWR);
		close(sockfd);
	}
	param->test_running = 0;
	vTaskDelete(NULL);
}

/******************************************************************************
 * FunctionName : run_sample_uart
 * Description  : sample test for udp client
 * Parameters   : WIFI_CONFIG
 * Returns      : 0 or -1 (0: success, -1: fail)
 *******************************************************************************/
nrc_err_t run_sample_uart(WIFI_CONFIG *param)
{
	int count = 0;
	int interval = 0;
	int network_index = 0;
	tWIFI_STATE_ID wifi_state = WIFI_STATE_INIT;
	SCAN_RESULTS results;

	nrc_usr_print("[%s] Sample App for run_sample_uart \n",__func__);

	count = param->count;
	interval = param->interval;

	int i = 0;
	int ssid_found =false;

	/* set initial wifi configuration */
	while(1){
		if (wifi_init(param)== WIFI_SUCCESS) {
			nrc_usr_print ("[%s] wifi_init Success !! \n", __func__);
			break;
		} else {
			nrc_usr_print ("[%s] wifi_init Failed !! \n", __func__);
			_delay_ms(1000);
		}
	}

	/* find AP */
	while(1){
		if (nrc_wifi_scan() == WIFI_SUCCESS){
			if (nrc_wifi_scan_results(&results)== WIFI_SUCCESS) {
				/* Find the ssid in scan results */
				for(i=0; i<results.n_result ; i++){
					if(strcmp((char*)param->ssid, (char*)results.result[i].ssid)== 0 ){
						ssid_found = true;
						break;
					}
				}

				if(ssid_found){
					nrc_usr_print ("[%s] %s is found \n", __func__, param->ssid);
					break;
				}
			}
		} else {
			nrc_usr_print ("[%s] Scan fail !! \n", __func__);
			_delay_ms(1000);
		}
	}

	/* connect to AP */
	while(1) {
		if (wifi_connect(param)== WIFI_SUCCESS) {
			nrc_usr_print ("[%s] connect to %s successfully !! \n", __func__, param->ssid);
			break;
		} else{
			nrc_usr_print ("[%s] Fail for connection %s\n", __func__, param->ssid);
			_delay_ms(1000);
		}
	}

	nrc_wifi_get_network_index(&network_index );

	/* check the IP is ready */
	while(1){
		nrc_wifi_get_state(&wifi_state);
		if (wifi_state == WIFI_STATE_GET_IP) {
			nrc_usr_print("[%s] IP ...\n",__func__);
			break;
		} else{
			nrc_usr_print("[%s] Current State : %d...\n",__func__, wifi_state);
		}
		_delay_ms(1000);
	}

#if READ_RSSI_SNR || PERIODIC_SCAN
	nrc_timers_init();
#endif

#if READ_RSSI_SNR
	/* print RSSI and SNR every 3 seconds */
	nrc_timer_create(3000000, 1, report_signal);
#endif
#if PERIODIC_SCAN
	/* scan every 10 seconds */
	nrc_timer_create(10000000, 1, periodic_scan);
	scan_once();
#endif


	xTaskCreate(udp_uart_send_task, "udp_uart_send", 4096, (void*)param, 5, NULL);
	while(param->test_running){
		_delay_ms(1);
	}

	nrc_wifi_get_state(&wifi_state);
	if (wifi_state == WIFI_STATE_GET_IP || wifi_state == WIFI_STATE_CONNECTED) {
		nrc_usr_print("[%s] Trying to DISCONNECT... for exit\n",__func__);
		if (nrc_wifi_disconnect(network_index) != WIFI_SUCCESS) {
			nrc_usr_print ("[%s] Fail for Wi-Fi disconnection (results:%d)\n", __func__);
			return NRC_FAIL;
		}
	}

	if (error_val < 0)
		return NRC_FAIL;

	nrc_usr_print("[%s] End of run_sample_uart!! \n",__func__);

	return NRC_SUCCESS;
}



/******************************************************************************
 * FunctionName : user_init
 * Description  : Start Code for User Application, Initialize User function
 * Parameters   : none
 * Returns      : none
 *******************************************************************************/
void user_init(void)
{
	nrc_err_t ret;
	WIFI_CONFIG* param;

	nrc_uart_console_enable();

	param = nrc_mem_malloc(WIFI_CONFIG_SIZE);
	memset(param, 0x0, WIFI_CONFIG_SIZE);

	set_wifi_config(param);
	ret = run_sample_uart(param);
	nrc_usr_print("[%s] test result!! %s \n",__func__, (ret==0) ?  "Success" : "Fail");
	if(param){
		nrc_mem_free(param);
	}
}
