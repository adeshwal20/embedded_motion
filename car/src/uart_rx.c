#include "uart_rx.h"
#include "hardware/gpio.h"
#include "hardware/uart.h"

#define HC12_BAUD       9600

// Diagnostic mode: listen on both common UART routes.
// Route A: uart0 on GPIO0/1
#define HC12A_UART       uart0
#define HC12A_TX_PIN     0
#define HC12A_RX_PIN     1
// Route B: uart1 on GPIO20/21
#define HC12B_UART       uart1
#define HC12B_TX_PIN     20
#define HC12B_RX_PIN     21

void uart_rx_init(void) {
    uart_init(HC12A_UART, HC12_BAUD);
    gpio_set_function(HC12A_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(HC12A_RX_PIN, GPIO_FUNC_UART);
    uart_set_format(HC12A_UART, 8, 1, UART_PARITY_NONE);
    uart_set_fifo_enabled(HC12A_UART, true);

    uart_init(HC12B_UART, HC12_BAUD);
    gpio_set_function(HC12B_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(HC12B_RX_PIN, GPIO_FUNC_UART);
    uart_set_format(HC12B_UART, 8, 1, UART_PARITY_NONE);
    uart_set_fifo_enabled(HC12B_UART, true);
}

bool uart_receive_command(command_t *out, unsigned *which_uart) {
    if (!out) {
        return false;
    }

    if (uart_is_readable(HC12A_UART)) {
        uint8_t b = (uint8_t)uart_getc(HC12A_UART);
        *out = byte_to_command(b);
        if (which_uart) {
            *which_uart = 0U;
        }
        return true;
    }
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