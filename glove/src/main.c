#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"

#define IMU_I2C     i2c0
#define IMU_SDA_PIN 24
#define IMU_SCL_PIN 25

int main(void) {
    stdio_init_all();
    sleep_ms(3000);
    printf("mpu scan start\r\n");

    i2c_init(IMU_I2C, 400000);
    gpio_set_function(IMU_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(IMU_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(IMU_SDA_PIN);
    gpio_pull_up(IMU_SCL_PIN);

    while (1) {
        int found = 0;
        for (int addr = 0x08; addr < 0x78; addr++) {
            int rc = i2c_write_blocking(IMU_I2C, addr, NULL, 0, false);
            if (rc >= 0) {
                printf("found 0x%02X\r\n", addr);
                found = 1;
            }
        }
        if (!found) {
            printf("no device found\r\n");
        }
        sleep_ms(2000);
    }
}