#include "imu.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include <stdint.h>

#define IMU_I2C         i2c0
#define IMU_SDA_PIN     24
#define IMU_SCL_PIN     25
#define IMU_BAUD        400000

#define MPU6050_ADDR    0x68

#define REG_PWR_MGMT_1  0x6B
#define REG_ACCEL_XOUT  0x3B

static bool imu_ready = false;

static int16_t read16_be(const uint8_t *p) {
    return (int16_t)((p[0] << 8) | p[1]);
}

bool imu_init(void) {
    i2c_init(IMU_I2C, IMU_BAUD);

    gpio_set_function(IMU_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(IMU_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(IMU_SDA_PIN);
    gpio_pull_up(IMU_SCL_PIN);

    sleep_ms(100);

    uint8_t wake[2] = {REG_PWR_MGMT_1, 0x00};
    int rc = i2c_write_blocking(IMU_I2C, MPU6050_ADDR, wake, 2, false);
    if (rc != 2) {
        imu_ready = false;
        return false;
    }

    sleep_ms(100);
    imu_ready = true;
    return true;
}

bool imu_read(imu_data_t *out) {
    if (!imu_ready || out == NULL) {
        return false;
    }

    uint8_t reg = REG_ACCEL_XOUT;
    uint8_t raw[14];

    int rc = i2c_write_blocking(IMU_I2C, MPU6050_ADDR, &reg, 1, true);
    if (rc != 1) {
        return false;
    }

    rc = i2c_read_blocking(IMU_I2C, MPU6050_ADDR, raw, 14, false);
    if (rc != 14) {
        return false;
    }

    int16_t ax_raw = read16_be(&raw[0]);
    int16_t ay_raw = read16_be(&raw[2]);
    int16_t az_raw = read16_be(&raw[4]);

    int16_t gx_raw = read16_be(&raw[8]);
    int16_t gy_raw = read16_be(&raw[10]);
    int16_t gz_raw = read16_be(&raw[12]);

    out->ax = (float)ax_raw / 16384.0f;
    out->ay = (float)ay_raw / 16384.0f;
    out->az = (float)az_raw / 16384.0f;

    out->gx = (float)gx_raw / 131.0f;
    out->gy = (float)gy_raw / 131.0f;
    out->gz = (float)gz_raw / 131.0f;

    return true;
}