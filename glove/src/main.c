#include "pico/stdlib.h"
#include "protocol.h"
#include "uart_tx.h"

int main(void) {
    stdio_init_all();
    uart_tx_init();

    while (true) {
        uart_send_command(CMD_FORWARD);
        sleep_ms(2000);

        uart_send_command(CMD_LEFT);
        sleep_ms(1000);

        uart_send_command(CMD_RIGHT);
        sleep_ms(1000);

        uart_send_command(CMD_BACKWARD);
        sleep_ms(2000);

        uart_send_command(CMD_STOP);
        sleep_ms(2000);
    }
}