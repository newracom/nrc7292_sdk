#ifndef __HAL_UART_NRC7292_H__
#define __HAL_UART_NRC7292_H__

#define PRINT_BUFFER_SIZE		1024
#define INT_CONSOLE_BUFFER_MAX  8192
#define CLK_RESET_UART0		(0x00000001L << 8)

enum uart_data_bit {
    UART_DB5 = 0,
    UART_DB6 = 1,
    UART_DB7 = 2,
    UART_DB8 = 3,
};

enum uart_stop_bit {
    UART_SB1 = 0,
    UART_SB2 = 1,
};

enum uart_parity_bit {
    UART_PB_NONE = 0,
    UART_PB_ODD  = 1,
    UART_PB_EVEN = 2,
};

enum uart_hardware_flow_control {
    UART_HFC_DISABLE,
    UART_HFC_ENABLE,
};

enum uart_fifo {
    UART_FIFO_DISABLE,
    UART_FIFO_ENABLE,
};

enum nrc_uart_interrupt_e {
    UART_INT_NONE,
    UART_INT_ERROR,
    UART_INT_TIMEOUT,
    UART_INT_RX_DONE,
    UART_INT_TX_EMPTY,
    UART_INT_MAX
};

extern uint8_t g_print_dev;

void hal_uart_init(int vector, void(*cli_callback)(char));
void hal_uart_enable_print(bool enabled);
void hal_uart_printf(const char *f,...);
void hal_uart_vprintf(const char *,va_list );
void hal_uart_isr(int vector);

void nrc_hsuart_init(int ch, enum uart_data_bit db, int br,
                     enum uart_stop_bit sb, enum uart_parity_bit pb, enum uart_hardware_flow_control hfc, enum uart_fifo fifo);

bool nrc_hsuart_putch(int ch, const char c);
bool nrc_hsuart_getch(int ch, char *c);
void nrc_hsuart_str(int ch, const char *str);
void nrc_hsuart_str_cnt(int ch, const char *str, int cnt);
enum nrc_uart_interrupt_e nrc_hsuart_interrupt_type(int ch);
void nrc_hsuart_dma_enable(int ch, bool enable);
void nrc_hsuart_interrupt(int ch, bool tx_en, bool rx_en);
void nrc_hsuart_int_clr(int ch, bool tx_int, bool rx_int, bool to_int);
void nrc_hsuart_fifo_level(int ch, uint32_t tx_level, uint32_t rx_level);

#ifdef RELEASE
	#define system_printf
	#define system_vprintf
#else
	#define system_printf	hal_uart_printf
	#define system_vprintf	hal_uart_vprintf
#endif

#endif // __HAL_UART_NRC7292_H__
