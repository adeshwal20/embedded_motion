#include <stdio.h>
#include "pico/stdlib.h"
<<<<<<< HEAD
#include "gesture.h"
#include "imu.h"
=======
#include "imu.h"
#include "gesture.h"
>>>>>>> dbd89a8 (nothing' happening but communication)
#include "protocol.h"
#include "uart_tx.h"

static const char *command_to_string(command_t cmd) {
    switch (cmd) {
        case CMD_FORWARD:  return "FORWARD";
        case CMD_BACKWARD: return "BACKWARD";
        case CMD_LEFT:     return "LEFT";
        case CMD_RIGHT:    return "RIGHT";
        case CMD_STOP:
        default:           return "STOP";
    }
}

int main(void) {
    stdio_init_all();
<<<<<<< HEAD
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
=======
    sleep_ms(2000);
    printf("glove booted\n");

    uart_tx_init();

    if (!imu_init()) {
        while (true) {
            printf("imu init failed\n");
            uart_send_command(CMD_STOP);
            sleep_ms(200);
        }
    }

    while (true) {
        imu_tilt_t tilt;
        command_t cmd = CMD_STOP;

        if (imu_read_tilt(&tilt)) {
            cmd = detect_gesture(&tilt);
            printf("ax=%.2f ay=%.2f az=%.2f -> %s\n",
                   tilt.ax, tilt.ay, tilt.az, command_to_string(cmd));
        } else {
            printf("imu read failed -> STOP\n");
            cmd = CMD_STOP;
        }

        uart_send_command(cmd);
        sleep_ms(50);
>>>>>>> dbd89a8 (nothing' happening but communication)
    }
}