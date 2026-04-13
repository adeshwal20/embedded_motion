#include "gesture.h"
#include "imu.h"
#include "pico/stdlib.h"
#include "protocol.h"
#include "uart_tx.h"

int main(void) {
    stdio_init_all();

    imu_init();
    uart_tx_init();

    command_t last_cmd = CMD_STOP;

    while (true) {
        imu_tilt_t tilt;
        command_t cmd = CMD_STOP;

        if (imu_read_tilt(&tilt)) {
            cmd = detect_gesture(&tilt);
        } else {
            // Safe fallback until BNO085 packet parsing is implemented.
            cmd = CMD_STOP;
        }

        // Send every loop for simplicity and robustness.
        // You can later change this to only send on command changes.
        uart_send_command(cmd);
        last_cmd = cmd;
        (void)last_cmd;

        sleep_ms(50);
    }
}