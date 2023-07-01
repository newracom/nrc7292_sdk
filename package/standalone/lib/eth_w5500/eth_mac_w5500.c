// Copyright 2020 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "eth_mac.h"
#include "w5500.h"
#include "nrc_sdk.h"
#include "util_trace.h"
#ifdef SUPPORT_ETHERNET_SPI_DMA
#include "nrc_spi_dma.h"
#endif
#define W5500_SPI_LOCK_TIMEOUT_MS (50)
#define W5500_TX_MEM_SIZE (0x4000)
#define W5500_RX_MEM_SIZE (0x4000)

#ifdef NRC7394
#define GPIO_INT_PIN  GPIO_20
#else
#define GPIO_INT_PIN  GPIO_10
#endif
static void *isr_arg;
#ifdef ENABLE_ETHERNET_INTERRUPT
static int interrupt_vector = -1;
#endif

spi_device_t w5500_spi;

typedef struct {
    esp_eth_mac_t parent;
    esp_eth_mediator_t *eth;
    SemaphoreHandle_t spi_lock;
    TaskHandle_t rx_task_hdl;
    uint32_t sw_reset_timeout_ms;
    int int_gpio_num;
    uint8_t addr[6];
    bool packets_remain;
    eth_link_t link;
} emac_w5500_t;

#define MAC_CHECK(x, goto_tag, format, ...) do {                                                           \
        nrc_err_t err_rc_ = (x);                                                                           \
        if (err_rc_ != NRC_SUCCESS) {                                                                      \
            E(TT_NET, "%s(%d): " format, __FUNCTION__, __LINE__, ##__VA_ARGS__);                           \
            ret = err_rc_;                                                                                 \
            goto goto_tag;                                                                                 \
        }                                                                                                  \
    } while(0)

#define MAC_CHECK_ON_FALSE(a, err_code, goto_tag, format, ...) do {                                        \
        if (!(a)) {                                                                                        \
            E(TT_NET, "%s(%d): " format, __FUNCTION__, __LINE__,  ##__VA_ARGS__);	                       \
            ret = err_code;                                                                                \
            goto goto_tag;                                                                                 \
        }                                                                                                  \
    } while (0)

ATTR_NC __attribute__((optimize("O3"))) static inline bool w5500_lock(emac_w5500_t *emac)
{
    return xSemaphoreTake(emac->spi_lock, portMAX_DELAY) == pdTRUE;
}

ATTR_NC __attribute__((optimize("O3"))) static inline bool w5500_unlock(emac_w5500_t *emac)
{
    return xSemaphoreGive(emac->spi_lock) == pdTRUE;
}

ATTR_NC __attribute__((optimize("O3"))) static nrc_err_t w5500_write(emac_w5500_t *emac, uint32_t address, const void *value, uint32_t len)
{
    nrc_err_t ret = NRC_SUCCESS;
    uint8_t *buffer = NULL;
    uint8_t addr[4];

	buffer = pvPortMalloc(len + 3);
    buffer[0] = (address >> W5500_ADDR_OFFSET) >> 8;
    buffer[1] = address >> W5500_ADDR_OFFSET;
    buffer[2] = ((address & 0xFFFF) | (W5500_ACCESS_MODE_WRITE << W5500_RWB_OFFSET) | W5500_SPI_OP_MODE_VDM);

    memcpy(buffer + 3, value, len);
    V(TT_NET, "[%s] addr[0] = 0x%02x; addr[1] = 0x%02x; addr[2] = 0x%02x; addr[3] = 0x%02x; len = %d\n",
      __func__, buffer[0], buffer[1], buffer[2], buffer[3], len);
    if (w5500_lock(emac)) {
#ifdef SUPPORT_ETHERNET_SPI_DMA
        spi_dma_write(buffer, len + 3);
#else
        nrc_spi_start_xfer(&w5500_spi);
        nrc_spi_xfer(&w5500_spi, buffer, NULL, len + 3);
        nrc_spi_stop_xfer(&w5500_spi);
#endif
        w5500_unlock(emac);
    } else {
        ret = NRC_FAIL;
    }

	vPortFree(buffer);
    return ret;
}

ATTR_NC __attribute__((optimize("O3"))) static nrc_err_t w5500_read(emac_w5500_t *emac, uint32_t address, void *value, uint32_t len)
{
    nrc_err_t ret = NRC_SUCCESS;
    uint8_t addr[3];
#ifdef SUPPORT_ETHERNET_SPI_DMA
	uint8_t *buffer = NULL;
#else
    uint8_t rx[4] = {0xff, 0xff, 0xff, 0xff};
    uint8_t transfer_bytes = 4;
    uint32_t loop = (len - 1)/ transfer_bytes;
    uint32_t remain = (len - 1) % transfer_bytes;
#endif // SUPPORT_ETHERNET_SPI_DMA
    addr[0] = (address >> W5500_ADDR_OFFSET) >> 8;
    addr[1] = address >> W5500_ADDR_OFFSET;
    addr[2] = ((address & 0xFFFF) | (W5500_ACCESS_MODE_READ << W5500_RWB_OFFSET) | W5500_SPI_OP_MODE_VDM);

    V(TT_NET, "[%s] addr[0] = 0x%02x addr[1] = 0x%02x addr[2] = 0x%02x, len = %d\n",
      __func__, addr[0], addr[1], addr[2], len);

    if (w5500_lock(emac)) {
#ifdef SUPPORT_ETHERNET_SPI_DMA
		buffer = pvPortMalloc(len + 3);
        spi_dma_read(addr, buffer, len);
        memcpy(value, &buffer[3], len);
        V(TT_NET, "[%s] rx[0] = 0x%02x, rx[1] = 0x%02x, rx[2] = 0x%02x, rx[3] = 0x%02x\n\n",
          __func__, buffer[0], buffer[1], buffer[2], buffer[3]);

		vPortFree(buffer);
#else // SUPPORT_ETHERNET_SPI_DMA
        nrc_spi_start_xfer(&w5500_spi);
        nrc_spi_xfer(&w5500_spi, addr, rx, 4);
        for (int i = 0; i < loop; i++) {
            nrc_spi_xfer(&w5500_spi, NULL, (value + 1) + (i * transfer_bytes), transfer_bytes);
        }
        if (remain) {
            nrc_spi_xfer(&w5500_spi, NULL, (value + 1) + (loop * transfer_bytes), remain);
        }
        nrc_spi_stop_xfer(&w5500_spi);

        V(TT_NET, "[%s] rx[0] = 0x%02x, rx[1] = 0x%02x, rx[2] = 0x%02x, rx[3] = 0x%02x\n",
          __func__, rx[0], rx[1], rx[2], rx[3]);

        memcpy(value, &rx[3], 1);
#endif // SUPPORT_ETHERNET_SPI_DMA
        w5500_unlock(emac);
    } else {
        ret = NRC_FAIL;
    }
    return ret;
}

ATTR_NC __attribute__((optimize("O3"))) static nrc_err_t w5500_send_command(emac_w5500_t *emac, uint8_t command, uint32_t timeout_ms)
{
    nrc_err_t ret = NRC_SUCCESS;
    MAC_CHECK(w5500_write(emac, W5500_REG_SOCK_CR(0), &command, sizeof(command)), err, "write SCR failed\n");
    // after W5500 accepts the command, the command register will be cleared automatically
    uint32_t to = 0;
    for (to = 0; to < timeout_ms / 10; to++) {
        MAC_CHECK(w5500_read(emac, W5500_REG_SOCK_CR(0), &command, sizeof(command)), err, "read SCR failed\n");
        if (!command) {
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    MAC_CHECK_ON_FALSE(to < timeout_ms / 10, NRC_FAIL, err, "send command timeout\n");

err:
    return ret;
}

ATTR_NC __attribute__((optimize("O3"))) static nrc_err_t w5500_get_tx_free_size(emac_w5500_t *emac, uint16_t *size)
{
    nrc_err_t ret = NRC_SUCCESS;
    uint16_t free0, free1 = 0;
    // read TX_FSR register more than once, until we get the same value
    // this is a trick because we might be interrupted between reading the high/low part of the TX_FSR register (16 bits in length)
    do {
        MAC_CHECK(w5500_read(emac, W5500_REG_SOCK_TX_FSR(0), &free0, sizeof(free0)), err, "read TX FSR failed\n");
        MAC_CHECK(w5500_read(emac, W5500_REG_SOCK_TX_FSR(0), &free1, sizeof(free1)), err, "read TX FSR failed\n");
    } while (free0 != free1);

    *size = __builtin_bswap16(free0);

err:
    return ret;
}

ATTR_NC __attribute__((optimize("O3"))) static nrc_err_t w5500_get_rx_received_size(emac_w5500_t *emac, uint16_t *size)
{
    nrc_err_t ret = NRC_SUCCESS;
    uint16_t received0, received1 = 0;
    do {
        MAC_CHECK(w5500_read(emac, W5500_REG_SOCK_RX_RSR(0), &received0, sizeof(received0)), err, "read RX RSR failed\n");
        MAC_CHECK(w5500_read(emac, W5500_REG_SOCK_RX_RSR(0), &received1, sizeof(received1)), err, "read RX RSR failed\n");
    } while (received0 != received1);
    *size = __builtin_bswap16(received0);

err:
    return ret;
}

ATTR_NC __attribute__((optimize("O3"))) static nrc_err_t w5500_write_buffer(emac_w5500_t *emac, const void *buffer, uint32_t len, uint16_t offset)
{
    nrc_err_t ret = NRC_SUCCESS;
    uint32_t remain = len;
    const uint8_t *buf = buffer;
    offset %= W5500_TX_MEM_SIZE;
    if (offset + len > W5500_TX_MEM_SIZE) {
        remain = (offset + len) % W5500_TX_MEM_SIZE;
        len = W5500_TX_MEM_SIZE - offset;
        MAC_CHECK(w5500_write(emac, W5500_MEM_SOCK_TX(0, offset), buf, len), err, "write TX buffer failed\n");
        offset += len;
        buf += len;
    }
    MAC_CHECK(w5500_write(emac, W5500_MEM_SOCK_TX(0, offset), buf, remain), err, "write TX buffer failed\n");

err:
    return ret;
}

ATTR_NC __attribute__((optimize("O3"))) static nrc_err_t w5500_read_buffer(emac_w5500_t *emac, void *buffer, uint32_t len, uint16_t offset)
{
    nrc_err_t ret = NRC_SUCCESS;
    uint32_t remain = len;
    uint8_t *buf = buffer;
    offset %= W5500_RX_MEM_SIZE;
    if (offset + len > W5500_RX_MEM_SIZE) {
        remain = (offset + len) % W5500_RX_MEM_SIZE;
        len = W5500_RX_MEM_SIZE - offset;
        MAC_CHECK(w5500_read(emac, W5500_MEM_SOCK_RX(0, offset), buf, len), err, "read RX buffer failed\n");
        offset += len;
        buf += len;
    }
    MAC_CHECK(w5500_read(emac, W5500_MEM_SOCK_RX(0, offset), buf, remain), err, "read RX buffer failed\n");

err:
    return ret;
}

static nrc_err_t w5500_set_mac_addr(emac_w5500_t *emac)
{
    nrc_err_t ret = NRC_SUCCESS;
    MAC_CHECK(w5500_write(emac, W5500_REG_MAC, emac->addr, 6), err, "write MAC address register failed\n");

err:
    return ret;
}

static nrc_err_t w5500_reset(emac_w5500_t *emac)
{
    nrc_err_t ret = NRC_SUCCESS;
    /* software reset */
    uint8_t mr = W5500_MR_RST; // Set RST bit (auto clear)
    MAC_CHECK(w5500_write(emac, W5500_REG_MR, &mr, sizeof(mr)), err, "write MR failed\n");
    uint32_t to = 0;
    for (to = 0; to < emac->sw_reset_timeout_ms / 10; to++) {
        MAC_CHECK(w5500_read(emac, W5500_REG_MR, &mr, sizeof(mr)), err, "read MR failed\n");
        if (!(mr & W5500_MR_RST)) {
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    MAC_CHECK_ON_FALSE(to < emac->sw_reset_timeout_ms / 10, NRC_FAIL, err, "reset timeout\n");

err:
    return ret;
}

static nrc_err_t w5500_verify_id(emac_w5500_t *emac)
{
    nrc_err_t ret = NRC_SUCCESS;
    uint8_t version = 0;
    MAC_CHECK(w5500_read(emac, W5500_REG_VERSIONR, &version, sizeof(version)), err, "read VERSIONR failed\n");
    // W5500 doesn't have chip ID, we just print the version number instead
    I(TT_NET, "version=%x\n", version);

err:
    return ret;
}

static nrc_err_t w5500_setup_default(emac_w5500_t *emac)
{
    nrc_err_t ret = NRC_SUCCESS;
    uint8_t reg_value = 16;

    // Only SOCK0 can be used as MAC RAW mode, so we give the whole buffer (16KB TX and 16KB RX) to SOCK0
    MAC_CHECK(w5500_write(emac, W5500_REG_SOCK_RXBUF_SIZE(0), &reg_value, sizeof(reg_value)), err, "set rx buffer size failed\n");
    MAC_CHECK(w5500_write(emac, W5500_REG_SOCK_TXBUF_SIZE(0), &reg_value, sizeof(reg_value)), err, "set tx buffer size failed\n");
    reg_value = 0;
    for (int i = 1; i < 8; i++) {
        MAC_CHECK(w5500_write(emac, W5500_REG_SOCK_RXBUF_SIZE(i), &reg_value, sizeof(reg_value)), err, "set rx buffer size failed\n");
        MAC_CHECK(w5500_write(emac, W5500_REG_SOCK_TXBUF_SIZE(i), &reg_value, sizeof(reg_value)), err, "set tx buffer size failed\n");
    }

    /* Enable ping block, disable PPPoE, WOL */
    reg_value = W5500_MR_PB;
    MAC_CHECK(w5500_write(emac, W5500_REG_MR, &reg_value, sizeof(reg_value)), err, "write MR failed\n");
    /* Disable interrupt for all sockets by default */
    reg_value = 0;
    MAC_CHECK(w5500_write(emac, W5500_REG_SIMR, &reg_value, sizeof(reg_value)), err, "write SIMR failed\n");
    /* Enable MAC RAW mode for SOCK0, enable MAC filter, no blocking broadcast and multicast */
    reg_value = W5500_SMR_MAC_RAW | W5500_SMR_MAC_FILTER;
    MAC_CHECK(w5500_write(emac, W5500_REG_SOCK_MR(0), &reg_value, sizeof(reg_value)), err, "write SMR failed\n");
    /* Enable receive event for SOCK0 */
    reg_value = W5500_SIR_RECV;
    MAC_CHECK(w5500_write(emac, W5500_REG_SOCK_IMR(0), &reg_value, sizeof(reg_value)), err, "write SOCK0 IMR failed\n");
    /* Set the interrupt re-assert level to maximum (~1.5ms) to lower the chances of missing it */
    uint16_t int_level = __builtin_bswap16(0xFFFF);
    MAC_CHECK(w5500_write(emac, W5500_REG_INTLEVEL, &int_level, sizeof(int_level)), err, "write INTLEVEL failed\n");

err:
    return ret;
}

static nrc_err_t emac_w5500_start(esp_eth_mac_t *mac)
{
    nrc_err_t ret = NRC_SUCCESS;
    emac_w5500_t *emac = __containerof(mac, emac_w5500_t, parent);
    uint8_t reg_value = 0;
    /* open SOCK0 */
    MAC_CHECK(w5500_send_command(emac, W5500_SCR_OPEN, 100), err, "issue OPEN command failed\n");
    /* enable interrupt for SOCK0 */
    reg_value = W5500_SIMR_SOCK0;
    MAC_CHECK(w5500_write(emac, W5500_REG_SIMR, &reg_value, sizeof(reg_value)), err, "write SIMR failed\n");

err:
    return ret;
}

static nrc_err_t emac_w5500_stop(esp_eth_mac_t *mac)
{
    nrc_err_t ret = NRC_SUCCESS;
    emac_w5500_t *emac = __containerof(mac, emac_w5500_t, parent);
    uint8_t reg_value = 0;
    /* disable interrupt */
    MAC_CHECK(w5500_write(emac, W5500_REG_SIMR, &reg_value, sizeof(reg_value)), err, "write SIMR failed\n");
    /* close SOCK0 */
    MAC_CHECK(w5500_send_command(emac, W5500_SCR_CLOSE, 100), err, "issue CLOSE command failed\n");

err:
    return ret;
}

#ifdef ENABLE_ETHERNET_INTERRUPT
ATTR_NC __attribute__((optimize("O3"))) static void w5500_intr_handler(int vector)
{
    int input_high;

    if (nrc_gpio_inputb(GPIO_INT_PIN, &input_high) < 0) {
        return;
    }

#ifdef NRC7292
    if (input_high) {
        V(TT_NET, "[%s] input high\n", __func__);
        V(TT_NET, "[%s] system_irq_mask. vector = 0x%x\n", __func__, interrupt_vector);
#endif
        interrupt_vector = vector;
        system_irq_mask(interrupt_vector);
//        w5500_isr_handler(isr_arg);
		emac_w5500_t *emac = (emac_w5500_t *)isr_arg;
		BaseType_t high_task_wakeup = pdFALSE;
		/* notify w5500 task */
		vTaskNotifyGiveFromISR(emac->rx_task_hdl, &high_task_wakeup);
		if (high_task_wakeup != pdFALSE) {
			V(TT_NET, "[%s] calling portYIELD_FROM_ISR\n", __func__);
			portYIELD_FROM_ISR(high_task_wakeup);
		}

#ifdef NRC7394
		V(TT_NET, "[%s] Clear EINT00STAT...\n", __func__);
		volatile uint32_t *eint0_stat;
		if (vector == EV_EXT0) {
			eint0_stat = (uint32_t *) (0x40005000 + 0x844);
		} else {
			eint0_stat = (uint32_t *) (0x40005000 + 0x84C);
		}
		*eint0_stat |= 1;
		system_irq_unmask(vector);
#endif
#ifdef NRC7292
    } else {
        V(TT_NET, "[%s] input low\n", __func__);
    }
#endif
}
#endif

ATTR_NC __attribute__((optimize("O3"))) static void emac_w5500_task(void *arg)
{
    emac_w5500_t *emac = (emac_w5500_t *)arg;
    uint8_t status = 0;
    uint8_t *buffer = NULL;
    uint32_t length = 0;
    int int_bit = 0;

    while (1) {
#ifdef ENABLE_ETHERNET_INTERRUPT
        // check if the task receives any notification
        if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(1000)) == 0) {    // if no notification for 1 sec...
//        if (ulTaskNotifyTake(pdTRUE, portMAX_DELAY) == 0) {
            nrc_gpio_inputb(GPIO_INT_PIN, &int_bit);
#ifdef NRC7292
            if (int_bit == 0) {                                      // ...and no interrupt asserted
                V(TT_NET, "[%s] continue int_bit = %d\n", __func__, int_bit);
                continue;                                            // -> just continue to check again
            }
#endif
        }
#else
        /* Polling Interrupt GPIO.*/
        vTaskDelay(pdMS_TO_TICKS(1000) / 20);
        nrc_gpio_inputb(GPIO_INT_PIN, &int_bit);
        if (int_bit == 1) {
            continue;
        }
#endif
        V(TT_NET, "[%s] interrupt asserted bit = %d\n", __func__, int_bit);
        /* read interrupt status */
        w5500_read(emac, W5500_REG_SOCK_IR(0), &status, sizeof(status));
        /* packet received */
        if (status & W5500_SIR_RECV) {
            status = W5500_SIR_RECV;
            // clear interrupt status
            w5500_write(emac, W5500_REG_SOCK_IR(0), &status, sizeof(status));
#ifdef ENABLE_ETHERNET_INTERRUPT
#ifdef NRC7292
            if (interrupt_vector != -1) {
                V(TT_NET, "[%s] system_irq_UNmask vector 0x%x\n", __func__, interrupt_vector);
                system_irq_unmask(interrupt_vector);
            }
#endif
#endif
            do {
                /* read while there's packets remain in w5500 */
                do {
                    length = ETH_MAX_PACKET_SIZE;
                    V(TT_NET, "[%s] allocate buffer length = %d\n", __func__, length);
                    buffer = pvPortMalloc(length);
                    if (!buffer) {
                        E(TT_NET, "[%s] buffer allocation failed, waiting 1000ms to recover...\n", __func__);
                        _delay_ms(1000);
                    }
                } while (!buffer);
                V(TT_NET, "[%s] Receive packet\n", __func__);
                if (emac->parent.receive(&emac->parent, buffer, &length) == NRC_SUCCESS) {
                    /* pass the buffer to stack (e.g. TCP/IP layer) */
                    V(TT_NET, "[%s] Packet received length = %d\n",
                      __func__, length);
                    if (length) {
                        emac->eth->stack_input(emac->eth, buffer, length);
                        V(TT_NET, "[%s] after stack_input\n", __func__);
                    } else {
                        vPortFree(buffer);
                    }
                } else {
                    vPortFree(buffer);
                }
            } while (emac->packets_remain);
        }
    }
    vTaskDelete(NULL);
}

static nrc_err_t emac_w5500_set_mediator(esp_eth_mac_t *mac, esp_eth_mediator_t *eth)
{
    nrc_err_t ret = NRC_SUCCESS;
    MAC_CHECK_ON_FALSE(eth, NRC_FAIL, err, "can't set mac's mediator to null\n");
    emac_w5500_t *emac = __containerof(mac, emac_w5500_t, parent);
    emac->eth = eth;
    return NRC_SUCCESS;
err:
    return ret;
}

static nrc_err_t emac_w5500_write_phy_reg(esp_eth_mac_t *mac, uint32_t phy_addr, uint32_t phy_reg, uint32_t reg_value)
{
    nrc_err_t ret = NRC_SUCCESS;
    emac_w5500_t *emac = __containerof(mac, emac_w5500_t, parent);
    // PHY register and MAC registers are mixed together in W5500
    // The only PHY register is PHYCFGR
    MAC_CHECK_ON_FALSE(phy_reg == W5500_REG_PHYCFGR, NRC_FAIL, err, "wrong PHY register\n");
    MAC_CHECK(w5500_write(emac, W5500_REG_PHYCFGR, &reg_value, sizeof(uint8_t)), err, "write PHY register failed\n");

err:
    return ret;
}

static nrc_err_t emac_w5500_read_phy_reg(esp_eth_mac_t *mac, uint32_t phy_addr, uint32_t phy_reg, uint32_t *reg_value)
{
    nrc_err_t ret = NRC_SUCCESS;
    MAC_CHECK_ON_FALSE(reg_value, NRC_FAIL, err, "can't set reg_value to null\n");
    emac_w5500_t *emac = __containerof(mac, emac_w5500_t, parent);
    // PHY register and MAC registers are mixed together in W5500
    // The only PHY register is PHYCFGR
    MAC_CHECK_ON_FALSE(phy_reg == W5500_REG_PHYCFGR, NRC_FAIL, err, "wrong PHY register\n");
    MAC_CHECK(w5500_read(emac, W5500_REG_PHYCFGR, reg_value, sizeof(uint8_t)), err, "read PHY register failed\n");

err:
    return ret;
}

static nrc_err_t emac_w5500_set_addr(esp_eth_mac_t *mac, uint8_t *addr)
{
    nrc_err_t ret = NRC_SUCCESS;
    MAC_CHECK_ON_FALSE(addr, NRC_FAIL, err, "invalid argument\n");
    emac_w5500_t *emac = __containerof(mac, emac_w5500_t, parent);
    memcpy(emac->addr, addr, 6);
    MAC_CHECK(w5500_set_mac_addr(emac), err, "set mac address failed\n");

err:
    return ret;
}

static nrc_err_t emac_w5500_get_addr(esp_eth_mac_t *mac, uint8_t *addr)
{
    nrc_err_t ret = NRC_SUCCESS;
    MAC_CHECK_ON_FALSE(addr, NRC_FAIL, err, "invalid argument\n");
    emac_w5500_t *emac = __containerof(mac, emac_w5500_t, parent);
    memcpy(addr, emac->addr, 6);

err:
    return ret;
}

static nrc_err_t emac_w5500_set_link(esp_eth_mac_t *mac, eth_link_t link)
{
    nrc_err_t ret = NRC_SUCCESS;
    emac_w5500_t *emac = __containerof(mac, emac_w5500_t, parent);

    emac->link = link;

    switch (link) {
    case ETH_LINK_UP:
        I(TT_NET, "link is up\n");
        MAC_CHECK(mac->start(mac), err, "w5500 start failed\n");
        break;
    case ETH_LINK_DOWN:
        I(TT_NET, "link is down\n");
        MAC_CHECK(mac->stop(mac), err, "w5500 stop failed\n");
        break;
    default:
        MAC_CHECK_ON_FALSE(false, NRC_FAIL, err, "unknown link status\n");
        break;
    }

err:
    return ret;
}

static nrc_err_t emac_w5500_set_speed(esp_eth_mac_t *mac, eth_speed_t speed)
{
    nrc_err_t ret = NRC_SUCCESS;
    switch (speed) {
    case ETH_SPEED_10M:
        I(TT_NET, "working in 10Mbps\n");
        break;
    case ETH_SPEED_100M:
        I(TT_NET, "working in 100Mbps\n");
        break;
    default:
        MAC_CHECK_ON_FALSE(false, NRC_FAIL, err, "unknown speed\n");
        break;
    }

err:
    return ret;
}

static nrc_err_t emac_w5500_set_duplex(esp_eth_mac_t *mac, eth_duplex_t duplex)
{
    nrc_err_t ret = NRC_SUCCESS;
    switch (duplex) {
    case ETH_DUPLEX_HALF:
        I(TT_NET, "working in half duplex\n");
        break;
    case ETH_DUPLEX_FULL:
        I(TT_NET, "working in full duplex\n");
        break;
    default:
        MAC_CHECK_ON_FALSE(false, NRC_FAIL, err, "unknown duplex\n");
        break;
    }

err:
    return ret;
}

static nrc_err_t emac_w5500_set_promiscuous(esp_eth_mac_t *mac, bool enable)
{
    nrc_err_t ret = NRC_SUCCESS;
    emac_w5500_t *emac = __containerof(mac, emac_w5500_t, parent);
    uint8_t smr = 0;
    MAC_CHECK(w5500_read(emac, W5500_REG_SOCK_MR(0), &smr, sizeof(smr)), err, "read SMR failed\n");
    if (enable) {
        smr &= ~W5500_SMR_MAC_FILTER;
    } else {
        smr |= W5500_SMR_MAC_FILTER;
    }
    MAC_CHECK(w5500_write(emac, W5500_REG_SOCK_MR(0), &smr, sizeof(smr)), err, "write SMR failed\n");

err:
    return ret;
}

static nrc_err_t emac_w5500_enable_flow_ctrl(esp_eth_mac_t *mac, bool enable)
{
    /* w5500 doesn't support flow control function, so accept any value */
    return NRC_FAIL;
}

static nrc_err_t emac_w5500_set_peer_pause_ability(esp_eth_mac_t *mac, uint32_t ability)
{
    /* w5500 doesn't suppport PAUSE function, so accept any value */
    return NRC_FAIL;
}

ATTR_NC __attribute__((optimize("O3"))) static inline bool is_w5500_sane_for_rxtx(emac_w5500_t *emac)
{
    uint8_t phycfg;
    /* phy is ok for rx and tx operations if bits RST and LNK are set (no link down, no reset) */
    if (w5500_read(emac, W5500_REG_PHYCFGR, &phycfg, 1) == NRC_SUCCESS && (phycfg & 0x8001)) {
        return true;
    }
    return false;
}

ATTR_NC  __attribute__((optimize("O3"))) static nrc_err_t emac_w5500_transmit(esp_eth_mac_t *mac, uint8_t *buf, uint32_t length)
{
    nrc_err_t ret = NRC_SUCCESS;
    emac_w5500_t *emac = __containerof(mac, emac_w5500_t, parent);
    uint16_t offset = 0;

    if (emac->link == ETH_LINK_DOWN) {
        return NRC_FAIL;
    }
    // check if there're free memory to store this packet
    uint16_t free_size = 0;
    MAC_CHECK(w5500_get_tx_free_size(emac, &free_size), err, "get free size failed\n");
    MAC_CHECK_ON_FALSE(length <= free_size, NRC_FAIL, err, "free size (%d) < send length (%d)\n", length, free_size);
    // get current write pointer
    MAC_CHECK(w5500_read(emac, W5500_REG_SOCK_TX_WR(0), &offset, sizeof(offset)), err, "read TX WR failed\n");
    offset = __builtin_bswap16(offset);
    // copy data to tx memory
    MAC_CHECK(w5500_write_buffer(emac, buf, length, offset), err, "write frame failed\n");
    // update write pointer
    offset += length;
    offset = __builtin_bswap16(offset);
    MAC_CHECK(w5500_write(emac, W5500_REG_SOCK_TX_WR(0), &offset, sizeof(offset)), err, "write TX WR failed\n");
    // issue SEND command
    MAC_CHECK(w5500_send_command(emac, W5500_SCR_SEND, 100), err, "issue SEND command failed\n");

    // pooling the TX done event
    int retry = 0;
    uint8_t status = 0;
    while (!(status & W5500_SIR_SEND)) {
        MAC_CHECK(w5500_read(emac, W5500_REG_SOCK_IR(0), &status, sizeof(status)), err, "read SOCK0 IR failed\n");
        if ((retry++ > 3 && !is_w5500_sane_for_rxtx(emac)) || retry > 10) {
            V(TT_NET, "[%s] error transmission, retry = %d...\n", __func__, retry);
            return NRC_FAIL;
        }
    }
    // clear the event bit
    status  = W5500_SIR_SEND;
    MAC_CHECK(w5500_write(emac, W5500_REG_SOCK_IR(0), &status, sizeof(status)), err, "write SOCK0 IR failed\n");

err:
    return ret;
}

ATTR_NC __attribute__((optimize("O3"))) static nrc_err_t emac_w5500_receive(esp_eth_mac_t *mac, uint8_t *buf, uint32_t *length)
{
    nrc_err_t ret = NRC_SUCCESS;
    emac_w5500_t *emac = __containerof(mac, emac_w5500_t, parent);
    uint16_t offset = 0;
    uint16_t rx_len = 0;
    uint16_t remain_bytes = 0;
    emac->packets_remain = false;

    w5500_get_rx_received_size(emac, &remain_bytes);
    if (remain_bytes) {
        // get current read pointer
        MAC_CHECK(w5500_read(emac, W5500_REG_SOCK_RX_RD(0), &offset, sizeof(offset)), err, "read RX RD failed\n");
        offset = __builtin_bswap16(offset);
        // read head first
        MAC_CHECK(w5500_read_buffer(emac, &rx_len, sizeof(rx_len), offset), err, "read frame header failed\n");
        rx_len = __builtin_bswap16(rx_len) - 2; // data size includes 2 bytes of header
        offset += 2;
        // read the payload
        MAC_CHECK(w5500_read_buffer(emac, buf, rx_len, offset), err, "read payload failed, len=%d, offset=%d\n", rx_len, offset);
        offset += rx_len;
        // update read pointer
        offset = __builtin_bswap16(offset);
        MAC_CHECK(w5500_write(emac, W5500_REG_SOCK_RX_RD(0), &offset, sizeof(offset)), err, "write RX RD failed\n");
        /* issue RECV command */
        MAC_CHECK(w5500_send_command(emac, W5500_SCR_RECV, 100), err, "issue RECV command failed\n");
        // check if there're more data need to process
        remain_bytes -= rx_len + 2;
        emac->packets_remain = remain_bytes > 0;
    }

    *length = rx_len;
err:
    return ret;
}

#ifdef ENABLE_ETHERNET_INTERRUPT
#ifdef NRC7394
nrc_err_t eth_gpio_register_interrupt_handler(int intr_src, int pin, intr_handler_fn cb)
{
	volatile uint32_t *gpio_ctrl;
	gpio_io_t gpio;
	gpio_io_t dir;
	uio_sel_t uio;
	int ext_intr_src;
	int ret;

	if (!cb)
		return NRC_FAIL;

	A("[%s] Registering interrupt handler for pin %d...\n", __func__, pin);
	nrc_gpio_get_int_enable(&gpio);
	gpio.word |=  (0x00000001L << pin);
	nrc_gpio_set_int_enable(&gpio);

	nrc_gpio_get_alt(&gpio);
	gpio.word |=  (0x00000001L << pin);
	nrc_gpio_set_alt(&gpio);

	nrc_gpio_get_dir(&dir);
	gpio.word &=  ~(0x00000001L << pin);
	nrc_gpio_config_dir(&dir);

	A("[%s] interrupt src : %d\n", __func__, intr_src);
	switch(intr_src){
	case EV_EXT0:
		nrc_gpio_get_uio_sel(UIO_SEL_EINT3_EINT0, &uio);
		uio.bit.sel7_0 = pin;
		nrc_gpio_set_uio_sel(UIO_SEL_EINT3_EINT0, &uio);
		system_register_isr(EV_EXT0, cb);

		/* set EINT01CTL */
		gpio_ctrl = (uint32_t *) (0x40005000 + 0x840);
		*gpio_ctrl |= ((1 << 1) | 1);
		system_printf("GPIO_EINT02CTL_U : %p = 0x%x\n", gpio_ctrl, *gpio_ctrl);

		system_irq_unmask(EV_EXT0);
		break;

	case EV_EXT1:
		nrc_gpio_get_uio_sel(UIO_SEL_EINT3_EINT0, &uio);
		uio.bit.sel15_8 = pin;
		nrc_gpio_set_uio_sel(UIO_SEL_EINT3_EINT0, &uio);
		system_register_isr(EV_EXT1, cb);

		/* set EINT01CTL */
		gpio_ctrl = (uint32_t *) (0x40005000 + 0x848);
		*gpio_ctrl |= ((1 << 1) | 1);

		system_irq_unmask(EV_EXT1);
		break;

	default:
		E(TT_SDK_GPIO, "[%s] Failed!! Not available external interrupt\n", __func__);
	}

	return NRC_SUCCESS;
}
#endif
#endif

static void emac_gpio_interrupt_init()
{
    NRC_GPIO_CONFIG gpio_config;

    V(TT_NET, "[%s] init ...\n", __func__);
#ifdef ENABLE_ETHERNET_INTERRUPT
#ifdef NRC7394
    if (eth_gpio_register_interrupt_handler(EV_EXT0, GPIO_INT_PIN, w5500_intr_handler) == NRC_SUCCESS) {
#else // NRC7394
    gpio_config.gpio_pin = GPIO_INT_PIN;
    gpio_config.gpio_dir = GPIO_INPUT;
	gpio_config.gpio_mode = GPIO_PULL_DOWN;
    gpio_config.gpio_alt = GPIO_FUNC;
    nrc_gpio_config(&gpio_config);

	if (nrc_gpio_register_interrupt_handler(INT_VECTOR0, GPIO_INT_PIN, w5500_intr_handler) == NRC_SUCCESS) {
#endif
        system_printf("[%s] Interrupt handler installed...\n", __func__);
    } else {
        system_printf("[%s] Interrupt handler install failed...\n", __func__);
    }
#else // ENABLE_ETHERNET_INTERRUPT
    gpio_config.gpio_pin = GPIO_INT_PIN;
    gpio_config.gpio_dir = GPIO_INPUT;
	gpio_config.gpio_mode = GPIO_PULL_UP;
    gpio_config.gpio_alt = GPIO_FUNC;
    nrc_gpio_config(&gpio_config);
#endif
    V(TT_NET, "[%s] return ...\n", __func__);
}

static nrc_err_t emac_w5500_init(esp_eth_mac_t *mac)
{
    nrc_err_t ret = NRC_SUCCESS;
    emac_w5500_t *emac = __containerof(mac, emac_w5500_t, parent);
    esp_eth_mediator_t *eth = emac->eth;

    isr_arg = (void *) emac;
    /* Initialize GPIO for interrupt or polling */
    emac_gpio_interrupt_init();

    V(TT_NET, "[%s] call on_state_changed\n", __func__);
    MAC_CHECK(eth->on_state_changed(eth, ETH_STATE_LLINIT, NULL), err, "lowlevel init failed\n");
    /* reset w5500 */
    V(TT_NET, "[%s] call reset\n", __func__);
    MAC_CHECK(w5500_reset(emac), err, "reset w5500 failed\n");
    /* verify chip id */
    V(TT_NET, "[%s] call verify ID\n", __func__);
    MAC_CHECK(w5500_verify_id(emac), err, "verify chip ID failed\n");
    /* default setup of internal registers */
    V(TT_NET, "[%s] call w5500_setup_default\n", __func__);
    MAC_CHECK(w5500_setup_default(emac), err, "w5500 default setup failed\n");
    return NRC_SUCCESS;
err:
//    gpio_isr_handler_remove(emac->int_gpio_num);
//    gpio_reset_pin(emac->int_gpio_num);
    eth->on_state_changed(eth, ETH_STATE_DEINIT, NULL);
    return ret;
}

static nrc_err_t emac_w5500_deinit(esp_eth_mac_t *mac)
{
    emac_w5500_t *emac = __containerof(mac, emac_w5500_t, parent);
    esp_eth_mediator_t *eth = emac->eth;
    mac->stop(mac);
//    gpio_isr_handler_remove(emac->int_gpio_num);
//    gpio_reset_pin(emac->int_gpio_num);
    eth->on_state_changed(eth, ETH_STATE_DEINIT, NULL);
    return NRC_SUCCESS;
}

static nrc_err_t emac_w5500_del(esp_eth_mac_t *mac)
{
    emac_w5500_t *emac = __containerof(mac, emac_w5500_t, parent);
    vTaskDelete(emac->rx_task_hdl);
    vSemaphoreDelete(emac->spi_lock);
    vPortFree(emac);
    return NRC_SUCCESS;
}

esp_eth_mac_t *esp_eth_mac_new_w5500(const eth_mac_config_t *mac_config)
{
    esp_eth_mac_t *ret = NULL;
    emac_w5500_t *emac = NULL;

#ifdef SUPPORT_ETHERNET_SPI_DMA
    I(TT_NET, "[%s] spi_dma_init ....\n", __func__);
    spi_dma_init();
#else // SUPPORT_ETHERNET_SPI_DMA
    I(TT_NET, "[%s] nrc_spi_master_init ....\n", __func__);

#ifdef NRC7394
    spi_dma.pin_miso = 30;
    spi_dma.pin_mosi = 29;
    spi_dma.pin_cs =25;
    spi_dma.pin_sclk = 28;
#else
    w5500_spi.pin_miso = 12;
    w5500_spi.pin_mosi = 13;
    w5500_spi.pin_cs =14;
    w5500_spi.pin_sclk = 15;
#endif
    w5500_spi.frame_bits = SPI_BIT8;
    w5500_spi.clock = 6000000;
    w5500_spi.mode = SPI_MODE0;
    w5500_spi.controller = SPI_CONTROLLER_SPI0;
    w5500_spi.irq_save_flag = 0;
    w5500_spi.isr_handler = NULL;

    nrc_spi_master_init(&w5500_spi);
    nrc_spi_enable(&w5500_spi, true);
#endif // SUPPORT_ETHERNET_SPI_DMA
    _delay_ms(100);

    MAC_CHECK_ON_FALSE(mac_config, NULL, err, "invalid argument\n");
    emac = pvPortCalloc(1, sizeof(emac_w5500_t));
    MAC_CHECK_ON_FALSE(emac, NULL, err, "no mem for MAC instance\n");

    /* bind methods and attributes */
    emac->sw_reset_timeout_ms = mac_config->sw_reset_timeout_ms;
    emac->parent.set_mediator = emac_w5500_set_mediator;
    emac->parent.init = emac_w5500_init;
    emac->parent.deinit = emac_w5500_deinit;
    emac->parent.start = emac_w5500_start;
    emac->parent.stop = emac_w5500_stop;
    emac->parent.del = emac_w5500_del;
    emac->parent.write_phy_reg = emac_w5500_write_phy_reg;
    emac->parent.read_phy_reg = emac_w5500_read_phy_reg;
    emac->parent.set_addr = emac_w5500_set_addr;
    emac->parent.get_addr = emac_w5500_get_addr;
    emac->parent.set_speed = emac_w5500_set_speed;
    emac->parent.set_duplex = emac_w5500_set_duplex;
    emac->parent.set_link = emac_w5500_set_link;
    emac->parent.set_promiscuous = emac_w5500_set_promiscuous;
    emac->parent.set_peer_pause_ability = emac_w5500_set_peer_pause_ability;
    emac->parent.enable_flow_ctrl = emac_w5500_enable_flow_ctrl;
    emac->parent.transmit = emac_w5500_transmit;
    emac->parent.receive = emac_w5500_receive;
    /* create mutex */
    emac->spi_lock = xSemaphoreCreateMutex();
    MAC_CHECK_ON_FALSE(emac->spi_lock, NULL, err, "create lock failed\n");
    emac->link = ETH_LINK_DOWN;

    /* create w5500 task */
    I(TT_NET, "[%s] w5500_tsk running with priority: %d\n", __func__, LWIP_TASK_PRIORITY);
    BaseType_t xReturned = xTaskCreate(emac_w5500_task, "w5500_tsk", mac_config->rx_task_stack_size, emac,
                                       LWIP_TASK_PRIORITY, &emac->rx_task_hdl);
    MAC_CHECK_ON_FALSE(xReturned == pdPASS, NULL, err, "create w5500 task failed\n");
    return &(emac->parent);

err:
    if (emac) {
        if (emac->rx_task_hdl) {
            vTaskDelete(emac->rx_task_hdl);
        }
        if (emac->spi_lock) {
            vSemaphoreDelete(emac->spi_lock);
        }
        vPortFree(emac);
    }
    return ret;
}
