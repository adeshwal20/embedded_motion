#ifndef GESTURE_H
#define GESTURE_H

#include "imu.h"
#include "protocol.h"

command_t detect_gesture(const imu_data_t *imu);

#endif