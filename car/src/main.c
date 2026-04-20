#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "motor.h"
#include "uart_rx.h"

/* All motion uses drive_all(left, right) so both TB6612 drivers always get a command (four wheels). */

// Commanded speed profile (percent).
#ifndef DRIVE_SPEED
#define DRIVE_SPEED      75.0f
#endif
#ifndef TURN_SPEED
#define TURN_SPEED       62.0f
#endif
#ifndef COMMAND_TIMEOUT_MS
/* Marginal HC-12 links need slack; glove also retransmits on this scale. */
#define COMMAND_TIMEOUT_MS 800
#endif
#ifndef MOTOR_ONLY_TEST_MODE
#define MOTOR_ONLY_TEST_MODE 0
#endif
#ifndef MOTOR_WIRING_TEST_MODE
#define MOTOR_WIRING_TEST_MODE 0
#endif
// 1 = USB serial logs: every RX byte (which UART), active command, heartbeat.
#ifndef CAR_UART_DEBUG_LOG
#define CAR_UART_DEBUG_LOG 0
#endif

/*
 * Manual motor bench test (no radio): change 0 → 1, Upload in PlatformIO, wheels should spin
 * forward continuously. Put car on blocks. Set back to 0 and Upload for normal glove driving.
 */
#ifndef RUN_MOTOR_SMOKE_TEST
#define RUN_MOTOR_SMOKE_TEST 0
#endif

// Visual debug: if the board definition provides an onboard LED pin, blink patterns
// reflect current command even when USB serial is unreliable.
#ifdef PICO_DEFAULT_LED_PIN
#define CAR_STATUS_LED_PIN PICO_DEFAULT_LED_PIN
#endif

#ifdef CAR_STATUS_LED_PIN
static void status_led_init(void) {
    gpio_init(CAR_STATUS_LED_PIN);
    gpio_set_dir(CAR_STATUS_LED_PIN, GPIO_OUT);
    gpio_put(CAR_STATUS_LED_PIN, 0);
}

static void status_led_tick(command_t cmd) {
    // Simple pattern per command (repeats ~every second):
    // STOP: off
    // FORWARD: 1 blink
    // BACKWARD: 2 blinks
    // LEFT: 3 blinks
    // RIGHT: 4 blinks
    static absolute_time_t t0;
    static bool inited;
    if (!inited) {
        t0 = get_absolute_time();
        inited = true;
    }

    int64_t us = absolute_time_diff_us(t0, get_absolute_time());
    // 1.2s frame: [0..1.0s] patterns, [1.0..1.2s] idle
    int64_t frame = us % 1200000;

    unsigned blinks = 0;
    switch (cmd) {
        case CMD_FORWARD:  blinks = 1; break;
        case CMD_BACKWARD: blinks = 2; break;
        case CMD_LEFT:     blinks = 3; break;
        case CMD_RIGHT:    blinks = 4; break;
        case CMD_STOP:
        default:           blinks = 0; break;
    }

    bool on = false;
    // Each blink: 80ms on, 120ms off. Blink windows start at 0ms.
    for (unsigned i = 0; i < blinks; i++) {
        int64_t start = (int64_t)i * 200000;
        if (frame >= start && frame < start + 80000) {
            on = true;
            break;
        }
    }
    gpio_put(CAR_STATUS_LED_PIN, on ? 1 : 0);
}
#else
static void status_led_init(void) {}
static void status_led_tick(command_t cmd) {(void)cmd;}
#endif

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
#if RUN_MOTOR_SMOKE_TEST
    motor_init();
    while (true) {
        /* Lower % on purpose for bench test if a motor is weak. */
        drive_all(30.0f, 30.0f);
        sleep_ms(10);
    }
#else
#if CAR_UART_DEBUG_LOG
    stdio_init_all();
    // Match glove startup behavior to make USB CDC logs reliable after reset.
    setvbuf(stdout, NULL, _IONBF, 0);
    // Some setups need a moment after SWD reset before USB CDC output is stable.
    sleep_ms(2000);
#endif
    status_led_init();
    motor_init();
#if !MOTOR_ONLY_TEST_MODE
    uart_rx_init();
#endif

#if MOTOR_WIRING_TEST_MODE
    // Step-by-step motor sanity test to validate wiring and polarity.
    while (true) {
        drive_all(35.0f, 0.0f);   // left side only
        sleep_ms(1500);
        drive_all(0.0f, 35.0f);   // right side only
        sleep_ms(1500);
        drive_all(35.0f, 35.0f);  // both forward
        sleep_ms(1500);
        drive_all(-30.0f, 30.0f); // left turn
        sleep_ms(1200);
        drive_all(30.0f, -30.0f); // right turn
        sleep_ms(1200);
        motor_stop_all();
        sleep_ms(1000);
    }
#elif MOTOR_ONLY_TEST_MODE
    while (true) {
        drive_all(DRIVE_SPEED, DRIVE_SPEED);
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
#if CAR_UART_DEBUG_LOG
        unsigned rx_uart;
#endif
        for (;;) {
#if CAR_UART_DEBUG_LOG
            if (!uart_receive_command(&cmd, &rx_uart))
#else
            if (!uart_receive_command(&cmd, NULL))
#endif
                break;
            last_cmd = cmd;
            last_rx_time = get_absolute_time();
#if CAR_UART_DEBUG_LOG
            printf("RX uart%u (%s) cmd=%s byte=%u\n",
                   rx_uart,
                   rx_uart ? "uart1" : "uart0",
                   cmd_str(cmd), (unsigned)cmd);
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
                /* Same speed both sides → all four wheels forward (two motors per TB6612). */
                drive_all(DRIVE_SPEED, DRIVE_SPEED);
                break;
            case CMD_BACKWARD:
                drive_all(-DRIVE_SPEED, -DRIVE_SPEED);
                break;
            case CMD_LEFT:
                /* All four wheels powered; differential turn. */
                drive_all(-TURN_SPEED, TURN_SPEED);
                break;
            case CMD_RIGHT:
                drive_all(TURN_SPEED, -TURN_SPEED);
                break;
            case CMD_STOP:
            default:
                motor_stop_all();
                break;
        }

        status_led_tick(last_cmd);
        sleep_ms(5);
    }
#endif
#endif /* RUN_MOTOR_SMOKE_TEST */
}