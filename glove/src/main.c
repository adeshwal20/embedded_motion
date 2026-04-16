#include <stdio.h>
#include "pico/stdlib.h"
#include "imu.h"

int main(void) {
    stdio_init_all();
    sleep_ms(3000);
    printf("glove booted\r\n");

    if (!imu_init()) {
        while (1) {
            printf("imu init failed\r\n");
            sleep_ms(500);
        }
    }

    printf("imu init complete\r\n");

    while (1) {
        imu_data_t imu;
        if (imu_read(&imu)) {
            // Print only accel, no UART, no gesture, no gyro
            printf("ax=%.2f ay=%.2f az=%.2f\r\n", imu.ax, imu.ay, imu.az);
        } else {
            printf("imu read failed\r\n");
        }
        sleep_ms(300);
    }
}