#include "motor.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "pico/stdlib.h"
#include <stdint.h>

// STBY1/STBY2 are tied high in hardware (not MCU GPIO).
//
// --- Software PWM (~1 kHz) ---------------------------------------------------
// Each TB6612: PWMA and PWMB tied to one GPIO; AIN1/BIN1 and AIN2/BIN2 tied.
// One speed + one direction per driver. drive_all(left,right): driver 1 = left,
// driver 2 = right (not per half-bridge A/B on the same chip).
//
// If the car spins in place with drive_all(70,70), one side is wired opposite: set
// MOTOR_DRIVER2_SWAP_DIRECTION to 0 and use 1 for driver 1 instead (or vice versa).
#define MOTOR_DRIVER2_SWAP_DIRECTION 1

#define SOFT_PWM_TICK_US   50u
#define SOFT_PWM_PHASES    20u

// ---------------- Driver 1 (one net per signal) ----------------------------
#define PWMA1           12
#define PWMB1           12

#define AIN1_1          10
#define BIN1_1          10
#define AIN2_1          11
#define BIN2_1          11

// ---------------- Driver 2 -------------------------------------------------
#define PWMA2           29
#define PWMB2           29

#define AIN1_2          30
#define BIN1_2          30
#define AIN2_2          31
#define BIN2_2          31

/* One soft-PWM level per physical PWM pin: GPIO 12 (driver 1), GPIO 29 (driver 2). */
static volatile uint8_t s_soft_duty[2];
static repeating_timer_t s_pwm_timer;

static void init_output(uint pin) {
    gpio_init(pin);
    gpio_disable_pulls(pin);
    gpio_set_dir(pin, GPIO_OUT);
    gpio_put(pin, 0);
}

static void set_soft_duty_for_pwm_pin(uint pwm_pin, uint8_t duty_0_20) {
    if (pwm_pin == PWMA1 || pwm_pin == PWMB1) {
        s_soft_duty[0] = duty_0_20;
    } else if (pwm_pin == PWMA2 || pwm_pin == PWMB2) {
        s_soft_duty[1] = duty_0_20;
    }
}

static uint8_t speed_to_duty(float speed_abs_pct) {
    if (speed_abs_pct < 0.0f) {
        speed_abs_pct = 0.0f;
    }
    if (speed_abs_pct > 100.0f) {
        speed_abs_pct = 100.0f;
    }
    return (uint8_t)((speed_abs_pct / 100.0f) * (float)SOFT_PWM_PHASES + 0.5f);
}

static bool soft_pwm_timer_cb(repeating_timer_t *rt) {
    (void)rt;
    static uint8_t phase;

    phase = (uint8_t)((phase + 1u) % SOFT_PWM_PHASES);

    gpio_put(PWMA1, phase < s_soft_duty[0]);
    gpio_put(PWMA2, phase < s_soft_duty[1]);
    return true;
}

static void set_one_motor(uint pwm_pin, uint in1_pin, uint in2_pin, float speed_pct,
                          bool swap_direction) {
    uint8_t d = 0;

    if (speed_pct > 0.0f) {
        if (!swap_direction) {
            gpio_put(in1_pin, 1);
            gpio_put(in2_pin, 0);
        } else {
            gpio_put(in1_pin, 0);
            gpio_put(in2_pin, 1);
        }
        d = speed_to_duty(speed_pct);
    } else if (speed_pct < 0.0f) {
        if (!swap_direction) {
            gpio_put(in1_pin, 0);
            gpio_put(in2_pin, 1);
        } else {
            gpio_put(in1_pin, 1);
            gpio_put(in2_pin, 0);
        }
        d = speed_to_duty(-speed_pct);
    } else {
        gpio_put(in1_pin, 0);
        gpio_put(in2_pin, 0);
        d = 0;
    }

    set_soft_duty_for_pwm_pin(pwm_pin, d);
}

void motor_init(void) {
    s_soft_duty[0] = 0;
    s_soft_duty[1] = 0;

    init_output(10);
    init_output(11);
    init_output(30);
    init_output(31);

    init_output(PWMA1);
    init_output(PWMA2);

    if (!add_repeating_timer_us(-(int64_t)SOFT_PWM_TICK_US, soft_pwm_timer_cb, NULL, &s_pwm_timer)) {
        /* Timer failed; outputs stay low */
    }
}

void drive_all(float left_percent, float right_percent) {
    if (left_percent > 100.0f) {
        left_percent = 100.0f;
    }
    if (left_percent < -100.0f) {
        left_percent = -100.0f;
    }
    if (right_percent > 100.0f) {
        right_percent = 100.0f;
    }
    if (right_percent < -100.0f) {
        right_percent = -100.0f;
    }

    set_one_motor(PWMA1, AIN1_1, AIN2_1, left_percent, false);
    set_one_motor(PWMA2, AIN1_2, AIN2_2, right_percent,
                  MOTOR_DRIVER2_SWAP_DIRECTION != 0);
}

void drive_forward(float speed_percent) {
    drive_all(speed_percent, speed_percent);
}

void drive_single_motor(unsigned motor_index, float speed_percent) {
    if (speed_percent > 100.0f) {
        speed_percent = 100.0f;
    }
    if (speed_percent < -100.0f) {
        speed_percent = -100.0f;
    }

    float z = 0.0f;
    /* 0,1 -> driver 1; 2,3 -> driver 2 (A/B indices alias when wired together). */
    set_one_motor(PWMA1, AIN1_1, AIN2_1,
                  (motor_index == 0u || motor_index == 1u) ? speed_percent : z, false);
    set_one_motor(PWMA2, AIN1_2, AIN2_2,
                  (motor_index == 2u || motor_index == 3u) ? speed_percent : z,
                  MOTOR_DRIVER2_SWAP_DIRECTION != 0);
}

void drive_motor1_channel_a(float speed_percent) {
    drive_single_motor(0u, speed_percent);
}

void drive_motor1_channel_b(float speed_percent) {
    drive_single_motor(1u, speed_percent);
}

void motor_stop_all(void) {
    drive_all(0.0f, 0.0f);
}
