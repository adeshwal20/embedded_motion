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
} imu_tilt_t;

bool imu_init(void);
bool imu_read_tilt(imu_tilt_t *out);

#endif