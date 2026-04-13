#include "uart_tx.h"
#include "hardware/gpio.h"
#include "hardware/uart.h"

#define HC12_UART       uart1
#define HC12_BAUD       9600

// Your wiring:
// HC-12 TX -> GP1
// HC-12 RX -> GP0
// So MCU transmits on GP0 and receives on GP1.
#define HC12_TX_PIN     20
#define HC12_RX_PIN     21

void uart_tx_init(void) {
    uart_init(HC12_UART, HC12_BAUD);
    gpio_set_function(HC12_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(HC12_RX_PIN, GPIO_FUNC_UART);
    uart_set_format(HC12_UART, 8, 1, UART_PARITY_NONE);
    uart_set_fifo_enabled(HC12_UART, true);
}

void uart_send_command(command_t cmd) {
    uart_putc_raw(HC12_UART, command_to_byte(cmd));
}