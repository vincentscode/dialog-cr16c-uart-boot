#include "uart.h"

#include <stddef.h>
#include <stdint.h>

#include "dma.h"
#include "gpio.h"
#include "irq.h"
#include "system.h"
#include "watchdog.h"

#define UART_STREAM_TIMEOUT 0x1000000

static uint8_t uart_rx_buf[1024];
static unsigned int uart_rx_dma_ptr = 0;
static unsigned int uart_rx_read_ptr = 0;

static void start_uart_rx_dma(void *ptr, const unsigned int len) {
    DMAX_A_STARTL_REG(DMA_UART_RX) = (uint16_t)(uintptr_t) & UART_RX_TX_REG;
    DMAX_A_STARTH_REG(DMA_UART_RX) = (uint16_t)((uint32_t) & UART_RX_TX_REG >> 16);
    DMAX_B_STARTL_REG(DMA_UART_RX) = (uint16_t)(uintptr_t) ptr;
    DMAX_B_STARTH_REG(DMA_UART_RX) = (uint16_t)((uint32_t) ptr >> 16);
    DMAX_INT_REG(DMA_UART_RX) = len;
    DMAX_LEN_REG(DMA_UART_RX) = len;
    DMAX_CTRL_REG(DMA_UART_RX) =
            DMAX_CTRL_REG_DMA_PRIO_MIDHIGH |
            DMAX_CTRL_REG_CIRCULAR |
            DMAX_CTRL_REG_BINC |
            DMAX_CTRL_REG_DREQ_MODE |
            DMAX_CTRL_REG_DINT_MODE |
            DMAX_CTRL_REG_BW_BYTE;
    DMAX_CTRL_REG(DMA_UART_RX) |= DMAX_CTRL_REG_DMA_ON;
}

void uart_rx_dma_reset(void) {
    start_uart_rx_dma(uart_rx_buf, sizeof(uart_rx_buf));
    uart_rx_read_ptr = 0;
}

void uart_init(void) {
    gpio_set_output(GPIO(0, 0));
    gpio_set_mode_bits(GPIO(0, 0), 1);
    UART_CTRL_REG = UART_CTRL_REG_UART_TEN | UART_CTRL_REG_UART_REN;

    INT2_PRIORITY_REG = 0x0002;

    uart_rx_dma_reset();
}

typedef struct uart_baudrate_config {
    unsigned long baudrate;
    unsigned int uart_ctrl;
} uart_baudrate_config_t;

static const uart_baudrate_config_t supported_baudrates[] = {
    {9600UL, UART_CTRL_REG_BAUDRATE_9600},
    {19200UL, UART_CTRL_REG_BAUDRATE_19200},
    {57600UL, UART_CTRL_REG_BAUDRATE_57600},
    {115200UL, UART_CTRL_REG_BAUDRATE_115200},
    {230400UL, UART_CTRL_REG_BAUDRATE_230400},
};

static const uart_baudrate_config_t *get_config_for_baudrate(const unsigned long baudrate) {
    for (unsigned int i = 0; i < ARRAY_SIZE(supported_baudrates); i++) {
        const uart_baudrate_config_t *cfg = &supported_baudrates[i];
        if (cfg->baudrate == baudrate) {
            return cfg;
        }
    }

    return NULL;
}

bool uart_is_baudrate_attainable(const unsigned long baudrate) {
    return !!get_config_for_baudrate(baudrate);
}

void uart_set_baudrate(const unsigned long baudrate) {
    const uart_baudrate_config_t *cfg = get_config_for_baudrate(baudrate);
    const unsigned int uart_enable = UART_CTRL_REG & (UART_CTRL_REG_UART_TEN | UART_CTRL_REG_UART_REN);
    UART_CTRL_REG &= ~(UART_CTRL_REG_UART_TEN | UART_CTRL_REG_UART_REN);
    unsigned int uart_ctrl = UART_CTRL_REG;
    uart_ctrl &= ~UART_CTRL_REG_BAUDRATE_MASK;
    uart_ctrl |= cfg->uart_ctrl;
    UART_CTRL_REG = uart_ctrl;
    UART_CTRL_REG |= uart_enable;
}

static void uart_putc(const char c) {
    UART_RX_TX_REG = (unsigned char) c;
    while (!(UART_CTRL_REG & UART_CTRL_REG_TI)) {}
    UART_CLEAR_TX_INT_REG = 1;
}

void uart_write(const void *ptr, unsigned int len) {
    const unsigned char *ptr8 = ptr;
    while (len--) {
        watchdog_reset();
        uart_putc((char) *ptr8++);
    }
}

void uart_flush(void) {
    for (volatile int i = 0; i < 100; i++) {}
}

unsigned int uart_rx_buffered_data(void) {
    const unsigned int data_in_transit = DMAX_IDX_REG(DMA_UART_RX);
    const unsigned int effective_rx_dma_ptr = uart_rx_dma_ptr + data_in_transit;
    if (effective_rx_dma_ptr >= uart_rx_read_ptr) {
        return effective_rx_dma_ptr - uart_rx_read_ptr;
    }
    return sizeof(uart_rx_buf) - uart_rx_read_ptr + effective_rx_dma_ptr;
}

bool uart_data_available(void) {
    const unsigned int data_in_transit = DMAX_IDX_REG(DMA_UART_RX);
    return (uart_rx_dma_ptr + data_in_transit) != uart_rx_read_ptr;
}

uint8_t uart_read_byte(void) {
    const uint8_t datum = uart_rx_buf[uart_rx_read_ptr++];
    uart_rx_read_ptr %= sizeof(uart_rx_buf);
    return datum;
}

uint8_t uart_stream_byte(void) {
    unsigned long timeout = UART_STREAM_TIMEOUT;
    while (!uart_data_available()) {
        watchdog_reset();
        if (!--timeout) {
            system_reset();
        }
    }
    return uart_read_byte();
}

uint32_t uart_read_u32_le(void) {
    return (uint32_t) uart_read_byte() |
           (uint32_t) uart_read_byte() << 8 |
           (uint32_t) uart_read_byte() << 16 |
           (uint32_t) uart_read_byte() << 24;
}

void uart_read(void *ptr, unsigned int len) {
    uint8_t *ptr8 = ptr;
    while (len--) {
        *ptr8++ = uart_read_byte();
    }
}

void *uart_get_read_ptr(void) {
    return &uart_rx_buf[uart_rx_read_ptr];
}

void uart_advance_read_ptr(const unsigned int increment) {
    uart_rx_read_ptr += increment;
}

void uart_rx_int(void) {
    // already handled by DMA
}
