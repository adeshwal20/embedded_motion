#include "imu.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include <stdint.h>

#define IMU_BAUD            100000
#define IMU_I2C_TIMEOUT_US  10000

#define MPU6050_ADDR_A      0x68
#define MPU6050_ADDR_B      0x69

#define REG_PWR_MGMT_1      0x6B
#define REG_ACCEL_XOUT      0x3B

typedef struct {
    i2c_inst_t *bus;
    uint sda;
    uint scl;
} imu_bus_cfg_t;

static const imu_bus_cfg_t k_bus_candidates[] = {
    {i2c1, 26u, 27u}, // preferred wiring
    {i2c0, 24u, 25u}, // legacy wiring fallback
};

static i2c_inst_t *s_bus = i2c1;
static uint8_t s_addr = MPU6050_ADDR_A;
static bool imu_ready = false;

static int16_t read16_be(const uint8_t *p) {
    return (int16_t)((p[0] << 8) | p[1]);
}

static void setup_bus(const imu_bus_cfg_t *cfg) {
    i2c_init(cfg->bus, IMU_BAUD);
    gpio_set_function(cfg->sda, GPIO_FUNC_I2C);
    gpio_set_function(cfg->scl, GPIO_FUNC_I2C);
    gpio_pull_up(cfg->sda);
    gpio_pull_up(cfg->scl);
    sleep_ms(20);
}

static bool try_wake_mpu(i2c_inst_t *bus, uint8_t addr) {
    uint8_t wake[2] = {REG_PWR_MGMT_1, 0x00};
    int rc = i2c_write_timeout_us(bus, addr, wake, 2, false, IMU_I2C_TIMEOUT_US);
    return rc == 2;
}

bool imu_init(void) {
    imu_ready = false;

    for (size_t i = 0; i < (sizeof(k_bus_candidates) / sizeof(k_bus_candidates[0])); ++i) {
        const imu_bus_cfg_t *cfg = &k_bus_candidates[i];
        setup_bus(cfg);

        if (try_wake_mpu(cfg->bus, MPU6050_ADDR_A)) {
            s_bus = cfg->bus;
            s_addr = MPU6050_ADDR_A;
            imu_ready = true;
            sleep_ms(50);
            return true;
        }
        if (try_wake_mpu(cfg->bus, MPU6050_ADDR_B)) {
            s_bus = cfg->bus;
            s_addr = MPU6050_ADDR_B;
            imu_ready = true;
            sleep_ms(50);
            return true;
        }
    }

    return false;
}

bool imu_read(imu_data_t *out) {
    if (!imu_ready || out == NULL) {
        return false;
    }

    uint8_t reg = REG_ACCEL_XOUT;
    uint8_t raw[14];

    int rc = i2c_write_timeout_us(s_bus, s_addr, &reg, 1, true, IMU_I2C_TIMEOUT_US);
    if (rc != 1) {
        return false;
    }

    rc = i2c_read_timeout_us(s_bus, s_addr, raw, 14, false, IMU_I2C_TIMEOUT_US);
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