#include <stdio.h>
#include "pico/stdlib.h"
#include "gesture.h"
#include "imu.h"
#include "protocol.h"
#include "uart_tx.h"

#define UART_ONLY_TEST_MODE 0
#define IMU_RX_LINK_TEST_MODE 0

static const char *command_to_string(command_t cmd) {
    switch (cmd) {
        case CMD_FORWARD:  return "FORWARD";
        case CMD_LEFT:     return "LEFT";
        case CMD_RIGHT:    return "RIGHT";
        case CMD_STOP:
        default:           return "STOP";
    }
}

int main(void) {
    stdio_init_all();
    sleep_ms(1500);
    printf("glove booted\n");

    uart_tx_init();
    imu_init();

    command_t last_sent = CMD_STOP;
    absolute_time_t last_imu_rx = get_absolute_time();
    absolute_time_t last_tx = get_absolute_time();
    absolute_time_t last_log = get_absolute_time();

    while (true) {
#if UART_ONLY_TEST_MODE
        command_t cmd = CMD_LEFT;
        if (absolute_time_diff_us(last_tx, get_absolute_time()) > 100000) {
            uart_send_command(cmd);
            last_sent = cmd;
            last_tx = get_absolute_time();
        }
        if (absolute_time_diff_us(last_log, get_absolute_time()) > 250000) {
            printf("uart-only test: sending %s\n", command_to_string(cmd));
            last_log = get_absolute_time();
        }
        sleep_ms(10);
        continue;
#endif
        imu_tilt_t tilt;
        if (imu_read_tilt(&tilt)) {
            last_imu_rx = get_absolute_time();
#if IMU_RX_LINK_TEST_MODE
            // Diagnostic: prove IMU packets are arriving by forcing a turn command.
            command_t cmd = CMD_LEFT;
#else
            command_t cmd = detect_gesture(&tilt);
#endif

            if (absolute_time_diff_us(last_log, get_absolute_time()) > 100000) {
                printf("imu ax=%.2f ay=%.2f az=%.2f gx=%.2f gy=%.2f gz=%.2f cmd=%s\n",
                       tilt.ax, tilt.ay, tilt.az, tilt.gx, tilt.gy, tilt.gz,
                       command_to_string(cmd));
                last_log = get_absolute_time();
            }

            // Send on change, and periodically re-send as link keepalive.
            if (cmd != last_sent ||
                absolute_time_diff_us(last_tx, get_absolute_time()) > 100000) {
                uart_send_command(cmd);
                last_sent = cmd;
                last_tx = get_absolute_time();
            }
        } else if (absolute_time_diff_us(last_imu_rx, get_absolute_time()) > 200000) {
            if (last_sent != CMD_STOP) {
                uart_send_command(CMD_STOP);
                last_sent = CMD_STOP;
                last_tx = get_absolute_time();
            }
            if (absolute_time_diff_us(last_log, get_absolute_time()) > 250000) {
                printf("imu read timeout\n");
                last_log = get_absolute_time();
            }
        }

        sleep_ms(10);
    }
}
