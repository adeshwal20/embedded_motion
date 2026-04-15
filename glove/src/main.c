#include "pico/stdlib.h"
#include "gesture.h"
#include "imu.h"
#include "protocol.h"
#include "uart_tx.h"

int main(void) {
    stdio_init_all();
    imu_init();
    uart_tx_init();

    command_t last_sent = CMD_STOP;
    absolute_time_t last_imu_rx = get_absolute_time();
    absolute_time_t last_tx = get_absolute_time();

    while (true) {
        imu_tilt_t tilt;
        if (imu_read_tilt(&tilt)) {
            last_imu_rx = get_absolute_time();
            command_t cmd = detect_gesture(&tilt);

            // Send on change, and periodically re-send as link keepalive.
            if (cmd != last_sent ||
                absolute_time_diff_us(last_tx, get_absolute_time()) > 100000) {
                uart_send_command(cmd);
                last_sent = cmd;
                last_tx = get_absolute_time();
            }
        } else if (absolute_time_diff_us(last_imu_rx, get_absolute_time()) > 200000) {
            // Fail-safe: stop car if IMU stream stalls.
            if (last_sent != CMD_STOP) {
                uart_send_command(CMD_STOP);
                last_sent = CMD_STOP;
                last_tx = get_absolute_time();
            }
        }

        sleep_ms(10);
    }
}