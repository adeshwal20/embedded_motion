#include "pico/stdlib.h"
#include "motor.h"
#include "uart_rx.h"

// Commanded speed profile (percent).
#define DRIVE_SPEED      35.0f
#define TURN_SPEED       30.0f
#define COMMAND_TIMEOUT_MS 300
#define MOTOR_ONLY_TEST_MODE 0
#define CAR_UART_DEBUG_LOG 0

#if CAR_UART_DEBUG_LOG
static const char *cmd_str(command_t c) {
    switch (c) {
        case CMD_FORWARD: return "FORWARD";
        case CMD_BACKWARD: return "BACKWARD";
        case CMD_LEFT: return "LEFT";
        case CMD_RIGHT: return "RIGHT";
        case CMD_STOP:
        default: return "STOP";
    }
}
#endif

int main(void) {
    stdio_init_all();
    motor_init();
#if !MOTOR_ONLY_TEST_MODE
    uart_rx_init();
#endif

#if MOTOR_ONLY_TEST_MODE
    while (true) {
        drive_forward(DRIVE_SPEED);
        sleep_ms(10);
    }
#else
    command_t last_cmd = CMD_STOP;
    command_t last_printed = CMD_STOP;
    absolute_time_t last_rx_time = get_absolute_time();
#if CAR_UART_DEBUG_LOG
    absolute_time_t last_hb = get_absolute_time();
    printf("car firmware running\n");
#endif

    while (true) {
        command_t cmd;
        if (uart_receive_command(&cmd)) {
            last_cmd = cmd;
            last_rx_time = get_absolute_time();
#if CAR_UART_DEBUG_LOG
            printf("RX cmd=%s (%u)\n", cmd_str(cmd), (unsigned)cmd);
#endif
        }

        // Fail-safe: if radio link is stale, stop the car.
        if (absolute_time_diff_us(last_rx_time, get_absolute_time()) >
            (int64_t)COMMAND_TIMEOUT_MS * 1000) {
            last_cmd = CMD_STOP;
        }

        if (last_cmd != last_printed) {
#if CAR_UART_DEBUG_LOG
            printf("ACT cmd=%s\n", cmd_str(last_cmd));
#endif
            last_printed = last_cmd;
        }

#if CAR_UART_DEBUG_LOG
        if (absolute_time_diff_us(last_hb, get_absolute_time()) > 1000000) {
            printf("HB waiting, active=%s\n", cmd_str(last_cmd));
            last_hb = get_absolute_time();
        }
#endif

        switch (last_cmd) {
            case CMD_FORWARD:
                drive_forward(DRIVE_SPEED);
                break;
            case CMD_BACKWARD:
                motor_stop_all();
                break;
            case CMD_LEFT:
                // Left turn: left wheels CCW, right wheels CW
                drive_all(-TURN_SPEED, TURN_SPEED);
                break;
            case CMD_RIGHT:
                // Right turn: right wheels CCW, left wheels CW
                drive_all(TURN_SPEED, -TURN_SPEED);
                break;
            case CMD_STOP:
            default:
                motor_stop_all();
                break;
        }

        sleep_ms(5);
    }
#endif
}