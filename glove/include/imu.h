#ifndef IMU_H
#define IMU_H

#include <stdbool.h>

typedef struct {
    float ax;
    float ay;
    float az;
    float gx;
    float gy;
    float gz;
} imu_data_t;

bool imu_init(void);
bool imu_read(imu_data_t *out);

#endif