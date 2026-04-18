#include <stdio.h>
#include "pico/stdlib.h"
#include "gesture.h"
#include "imu.h"
#include "uart_tx.h"

#define TERMINAL_DEBUG_ONLY_MODE  0

// Set via build flag (see platformio.ini envs) or here:
//   0 = normal IMU driving
//   1 = HC-12: constant FORWARD (re-flash glove with mode 2 to test LEFT — like your old workflow)
//   2 = HC-12: constant LEFT
//   3 = HC-12: repeat ~2s FORWARD then ~2s LEFT (one flash checks both)
//   4 = HC-12: alternate FORWARD / BACKWARD ~1.5s each
//   5 = HC-12: cycle FORWARD → LEFT → RIGHT → BACKWARD (~2s each, repeat) — wheel + link check
#ifndef GLOVE_HC12_TEST_MODE
#define GLOVE_HC12_TEST_MODE 0
#endif

#define HC12_TEST_TX_MS 50
#define HC12_TEST_PHASE_US 2000000

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

static void hc12_test_hold_command(command_t cmd) {
    uart_send_command(cmd);
    sleep_ms(HC12_TEST_TX_MS);
}

#if GLOVE_HC12_TEST_MODE != 0
static void hc12_test_run(void) {
    printf("GLOVE_HC12_TEST_MODE=%d (no IMU)\n", GLOVE_HC12_TEST_MODE);

#if GLOVE_HC12_TEST_MODE == 1
    printf("TX FORWARD only — car should drive straight forward.\n");
    while (true) {
        hc12_test_hold_command(CMD_FORWARD);
    }
#elif GLOVE_HC12_TEST_MODE == 2
    printf("TX LEFT only — car should turn left.\n");
    while (true) {
        hc12_test_hold_command(CMD_LEFT);
    }
#elif GLOVE_HC12_TEST_MODE == 3
    printf("TX FORWARD ~2s then LEFT ~2s (repeat) — HC-12 + steering check.\n");
    while (true) {
        absolute_time_t t0 = get_absolute_time();
        while (absolute_time_diff_us(t0, get_absolute_time()) < 2000000) {
            hc12_test_hold_command(CMD_FORWARD);
        }
        t0 = get_absolute_time();
        while (absolute_time_diff_us(t0, get_absolute_time()) < 2000000) {
            hc12_test_hold_command(CMD_LEFT);
        }
    }
#elif GLOVE_HC12_TEST_MODE == 4
    printf("TX FORWARD / BACKWARD alternate ~1.5s — straight line + reverse.\n");
    command_t test_cmd = CMD_FORWARD;
    absolute_time_t last_flip = get_absolute_time();
    while (true) {
        absolute_time_t now = get_absolute_time();
        if (absolute_time_diff_us(last_flip, now) > 1500000) {
            test_cmd = (test_cmd == CMD_FORWARD) ? CMD_BACKWARD : CMD_FORWARD;
            last_flip = now;
        }
        hc12_test_hold_command(test_cmd);
    }
#elif GLOVE_HC12_TEST_MODE == 5
    printf("TX wheel cycle FORWARD→LEFT→RIGHT→BACK (~2s each) — radio keeps changing cmd.\n");
    static const command_t k_wheel_cycle[] = {
        CMD_FORWARD,
        CMD_LEFT,
        CMD_RIGHT,
        CMD_BACKWARD,
    };
    while (true) {
        for (unsigned i = 0; i < sizeof(k_wheel_cycle) / sizeof(k_wheel_cycle[0]); i++) {
            command_t cmd = k_wheel_cycle[i];
            printf("phase %s\n", command_to_string(cmd));
            absolute_time_t t0 = get_absolute_time();
            while (absolute_time_diff_us(t0, get_absolute_time()) < (int64_t)HC12_TEST_PHASE_US) {
                hc12_test_hold_command(cmd);
            }
        }
    }
#else
#error "GLOVE_HC12_TEST_MODE must be 0..5"
#endif
}
#endif

int main(void) {
    stdio_init_all();
    setvbuf(stdout, NULL, _IONBF, 0);
    sleep_ms(1500);
    printf("glove booted\n");

    uart_tx_init();

#if GLOVE_HC12_TEST_MODE != 0
    hc12_test_run();
#endif

    if (!imu_init()) {
        while (1) {
            uart_send_command(CMD_STOP);
            printf("imu init failed\n");
            sleep_ms(200);
        }
    }

    command_t last_sent = CMD_STOP;
    absolute_time_t last_tx = get_absolute_time();
    absolute_time_t last_log = get_absolute_time();
    absolute_time_t last_hb = get_absolute_time();

    while (true) {
#if TERMINAL_DEBUG_ONLY_MODE
        imu_data_t imu;
        static absolute_time_t last_hb;
        if (imu_read(&imu)) {
            if (absolute_time_diff_us(last_log, get_absolute_time()) > 100000) {
                command_t cmd = detect_gesture(&imu);
                printf("imu ax=%.2f ay=%.2f az=%.2f gx=%.2f gy=%.2f gz=%.2f cmd=%s\n",
                       imu.ax, imu.ay, imu.az, imu.gx, imu.gy, imu.gz,
                       command_to_string(cmd));
                last_log = get_absolute_time();
            }
        } else if (absolute_time_diff_us(last_log, get_absolute_time()) > 250000) {
            printf("imu read timeout\n");
            last_log = get_absolute_time();
        }

        if (absolute_time_diff_us(last_hb, get_absolute_time()) > 1000000) {
            printf("glove alive\n");
            last_hb = get_absolute_time();
        }
        sleep_ms(10);
        continue;
#else
        imu_data_t imu;
        absolute_time_t now = get_absolute_time();

        if (imu_read(&imu)) {
            command_t cmd = detect_gesture(&imu);

            // Keep radio link alive (car timeout is 300 ms).
            if (cmd != last_sent || absolute_time_diff_us(last_tx, now) > 100000) {
                uart_send_command(cmd);
                last_sent = cmd;
                last_tx = now;
            }

            if (absolute_time_diff_us(last_log, now) > 100000) {
                printf("imu ax=%.2f ay=%.2f az=%.2f gx=%.2f gy=%.2f gz=%.2f cmd=%s\n",
                       imu.ax, imu.ay, imu.az, imu.gx, imu.gy, imu.gz,
                       command_to_string(cmd));
                last_log = now;
            }
        } else {
            if (absolute_time_diff_us(last_tx, now) > 100000) {
                uart_send_command(CMD_STOP);
                last_sent = CMD_STOP;
                last_tx = now;
            }
            if (absolute_time_diff_us(last_log, now) > 250000) {
                printf("imu read timeout\n");
                last_log = now;
            }
        }

        if (absolute_time_diff_us(last_hb, now) > 1000000) {
            printf("glove alive cmd=%s\n", command_to_string(last_sent));
            last_hb = now;
        }

        sleep_ms(10);
#endif
    }
}
