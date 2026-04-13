#include "motor.h"
#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "pico/stdlib.h"
#include <stdint.h>

#define PWM_FREQ_HZ     20000
#define PWM_WRAP        999

// ---------------- Driver 1 ----------------
#define STBY1           9

#define PWMA1           6
#define AIN1_1          8
#define AIN2_1          7

#define PWMB1           12
#define BIN1_1          10
#define BIN2_1          11

// ---------------- Driver 2 ----------------
#define STBY2           17

#define PWMA2           14
#define AIN1_2          16
#define AIN2_2          15

#define PWMB2           20
#define BIN1_2          18
#define BIN2_2          19

static void init_output(uint pin) {
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_OUT);
    gpio_put(pin, 0);
}

static void init_pwm_pin(uint pin) {
    gpio_set_function(pin, GPIO_FUNC_PWM);

    uint slice = pwm_gpio_to_slice_num(pin);
    pwm_config cfg = pwm_get_default_config();

    uint32_t clk_hz = clock_get_hz(clk_sys);
    float divider = (float)clk_hz / (PWM_FREQ_HZ * (PWM_WRAP + 1));

    if (divider < 1.0f) {
        divider = 1.0f;
    }

    pwm_config_set_clkdiv(&cfg, divider);
    pwm_config_set_wrap(&cfg, PWM_WRAP);

    pwm_init(slice, &cfg, true);
    pwm_set_gpio_level(pin, 0);
}

static uint16_t pct_to_level(float pct) {
    if (pct < 0.0f) {
        pct = 0.0f;
    }
    if (pct > 100.0f) {
        pct = 100.0f;
    }
    return (uint16_t)((pct / 100.0f) * PWM_WRAP);
}

static void set_one_motor(uint pwm_pin, uint in1_pin, uint in2_pin, float speed_pct) {
    uint16_t level = 0;

    if (speed_pct > 0.0f) {
        gpio_put(in1_pin, 1);
        gpio_put(in2_pin, 0);
        level = pct_to_level(speed_pct);
    } else if (speed_pct < 0.0f) {
        gpio_put(in1_pin, 0);
        gpio_put(in2_pin, 1);
        level = pct_to_level(-speed_pct);
    } else {
        // Coast/stop
        gpio_put(in1_pin, 0);
        gpio_put(in2_pin, 0);
        level = 0;
    }

    pwm_set_gpio_level(pwm_pin, level);
}

void motor_init(void) {
    // Direction / standby pins
    init_output(STBY1);
    init_output(AIN1_1);
    init_output(AIN2_1);
    init_output(BIN1_1);
    init_output(BIN2_1);

    init_output(STBY2);
    init_output(AIN1_2);
    init_output(AIN2_2);
    init_output(BIN1_2);
    init_output(BIN2_2);

    // PWM pins
    init_pwm_pin(PWMA1);
    init_pwm_pin(PWMB1);
    init_pwm_pin(PWMA2);
    init_pwm_pin(PWMB2);

    // Enable both TB6612FNG drivers
    gpio_put(STBY1, 1);
    gpio_put(STBY2, 1);
}

void drive_all(float left_percent, float right_percent) {
    if (left_percent > 100.0f) left_percent = 100.0f;
    if (left_percent < -100.0f) left_percent = -100.0f;
    if (right_percent > 100.0f) right_percent = 100.0f;
    if (right_percent < -100.0f) right_percent = -100.0f;

    // Left side = both A channels
    set_one_motor(PWMA1, AIN1_1, AIN2_1, left_percent);
    set_one_motor(PWMA2, AIN1_2, AIN2_2, left_percent);

    // Right side = both B channels
    set_one_motor(PWMB1, BIN1_1, BIN2_1, right_percent);
    set_one_motor(PWMB2, BIN1_2, BIN2_2, right_percent);
}

void motor_stop_all(void) {
    drive_all(0.0f, 0.0f);
}