#include "uart_tx.h"
#include "hardware/gpio.h"
#include "hardware/uart.h"

#define HC12_BAUD       9600
// Must match car/src/uart_rx.c: car listens on uart0 @ 0/1 and uart1 @ 20/21.
#define HC12A_UART       uart0
#define HC12A_TX_PIN     0
#define HC12A_RX_PIN     1
#define HC12B_UART       uart1
#define HC12B_TX_PIN     20
#define HC12B_RX_PIN     21

void uart_tx_init(void) {
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

void uart_send_command(command_t cmd) {
    uint8_t b = command_to_byte(cmd);
    uart_putc_raw(HC12A_UART, b);
    uart_putc_raw(HC12B_UART, b);
}
