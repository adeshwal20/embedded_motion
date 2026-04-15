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

// Initializes I2C and the BNO085 interface.
bool imu_init(void);

// Reads tilt-related values used by gesture detection.
// Returns true when fresh data was read successfully.
// Returns false if no valid BNO085 packet was decoded yet.
bool imu_read_tilt(imu_tilt_t *out);

#endif