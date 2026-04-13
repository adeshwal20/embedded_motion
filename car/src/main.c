#include "motor.h"
#include "pico/stdlib.h"
#include "protocol.h"
#include "uart_rx.h"

int main(void) {
    stdio_init_all();

    motor_init();
    uart_rx_init();

    command_t current_cmd = CMD_STOP;

    while (true) {
        command_t rx_cmd;
        if (uart_receive_command(&rx_cmd)) {
            current_cmd = rx_cmd;
        }

        switch (current_cmd) {
            case CMD_FORWARD:
                drive_all(70.0f, 70.0f);
                break;

            case CMD_BACKWARD:
                drive_all(-70.0f, -70.0f);
                break;

            case CMD_LEFT:
                drive_all(35.0f, 70.0f);
                break;

            case CMD_RIGHT:
                drive_all(70.0f, 35.0f);
                break;

            case CMD_STOP:
            default:
                motor_stop_all();
                break;
        }

        sleep_ms(10);
    }
}