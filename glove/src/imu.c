#include "imu.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "pico/stdlib.h"
#include <string.h>

#define IMU_I2C         i2c0
#define IMU_BAUD        400000

// Your actual wiring:
#define IMU_SDA_PIN     24
#define IMU_SCL_PIN     25

// Common BNO08x I2C address when ADR is low.
// If your board is strapped differently, this may need to be 0x4B.
#define BNO085_ADDR     0x4A

static bool imu_ready = false;

bool imu_init(void) {
    i2c_init(IMU_I2C, IMU_BAUD);

    gpio_set_function(IMU_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(IMU_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(IMU_SDA_PIN);
    gpio_pull_up(IMU_SCL_PIN);

    sleep_ms(50);

    imu_ready = true;
    return true;
}

bool imu_read_tilt(imu_tilt_t *out) {
    if (!imu_ready || !out) {
        return false;
    }

    // IMPORTANT:
    // The BNO085 does not behave like a simple register IMU over I2C.
    // It uses SH-2 / SHTP packet transport. The guide you uploaded confirms
    // fused outputs and warns about its unusual I2C behavior, but it does not
    // include the full packet-level protocol needed to implement a correct
    // parser here from scratch. :contentReference[oaicite:3]{index=3}
    //
    // So for now this returns false until you add the specific BNO085 packet
    // decode routine you are using in your project.
    //
    // Once that parser is added, fill out:
    //   out->ax = <linear accel x in m/s^2>;
    //   out->ay = <linear accel y in m/s^2>;

    memset(out, 0, sizeof(*out));
    return false;
}