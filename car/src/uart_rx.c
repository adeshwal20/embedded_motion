#include "uart_rx.h"
#include "hardware/gpio.h"
#include "hardware/uart.h"

#define HC12_UART       uart0
#define HC12_BAUD       9600

// Your car-side HC-12 wiring is the same:
#define HC12_TX_PIN     0
#define HC12_RX_PIN     1

void uart_rx_init(void) {
    uart_init(HC12_UART, HC12_BAUD);
    gpio_set_function(HC12_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(HC12_RX_PIN, GPIO_FUNC_UART);
    uart_set_format(HC12_UART, 8, 1, UART_PARITY_NONE);
    uart_set_fifo_enabled(HC12_UART, true);
}

bool uart_receive_command(command_t *out) {
    if (!out) {
        return false;
    }

    if (!uart_is_readable(HC12_UART)) {
        return false;
    }

    uint8_t b = (uint8_t)uart_getc(HC12_UART);
    *out = byte_to_command(b);
    return true;
}