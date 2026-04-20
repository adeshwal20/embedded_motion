#include "uart_rx.h"
#include "hc12_uart.h"
#include "hardware/gpio.h"
#include "hardware/uart.h"

#define HC12_BAUD       9600

// Listen on uart0 @ 0/1 and uart1 @ HC12_UART1_* (set in shared/hc12_uart.h / car platformio.ini).
#define HC12A_UART       uart0
#define HC12A_TX_PIN     0
#define HC12A_RX_PIN     1
#define HC12B_UART       uart1
#define HC12B_TX_PIN     HC12_UART1_TX_PIN
#define HC12B_RX_PIN     HC12_UART1_RX_PIN

static void drain_uart(uart_inst_t *uart) {
    while (uart_is_readable(uart)) {
        (void)uart_getc(uart);
    }
}

void uart_rx_init(void) {
#if HC12_CAR_RX_ENABLE_UART0
    uart_init(HC12A_UART, HC12_BAUD);
    gpio_set_function(HC12A_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(HC12A_RX_PIN, GPIO_FUNC_UART);
    uart_set_format(HC12A_UART, 8, 1, UART_PARITY_NONE);
    uart_set_fifo_enabled(HC12A_UART, true);
#endif

    uart_init(HC12B_UART, HC12_BAUD);
    gpio_set_function(HC12B_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(HC12B_RX_PIN, GPIO_FUNC_UART);
    uart_set_format(HC12B_UART, 8, 1, UART_PARITY_NONE);
    uart_set_fifo_enabled(HC12B_UART, true);

    /* Clear noise / stale bytes from before HC-12 lock or power-up. */
#if HC12_CAR_RX_ENABLE_UART0
    drain_uart(HC12A_UART);
#endif
    drain_uart(HC12B_UART);
}

bool uart_receive_command(command_t *out, unsigned *which_uart) {
    if (!out) {
        return false;
    }

#if HC12_CAR_RX_ENABLE_UART0
    if (uart_is_readable(HC12A_UART)) {
        uint8_t b = (uint8_t)uart_getc(HC12A_UART);
        *out = byte_to_command(b);
        if (which_uart) {
            *which_uart = 0U;
        }
        return true;
    }
#endif
    if (uart_is_readable(HC12B_UART)) {
        uint8_t b = (uint8_t)uart_getc(HC12B_UART);
        *out = byte_to_command(b);
        if (which_uart) {
            *which_uart = 1U;
        }
        return true;
    }
    return false;
}
