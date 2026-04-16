#include "gesture.h"

#define FORWARD_THRESH   0.35f
#define TURN_THRESH      0.35f
#define STOP_BAND        0.20f

command_t detect_gesture(const imu_data_t *imu) {
    if (!imu) {
        return CMD_STOP;
    }

    // Neutral/upright
    if ((imu->ax > -STOP_BAND && imu->ax < STOP_BAND) &&
        (imu->ay > -STOP_BAND && imu->ay < STOP_BAND)) {
        return CMD_STOP;
    }

    // Forward tilt
    if (imu->ax > FORWARD_THRESH) {
        return CMD_FORWARD;
    }

    // Left/right tank turn
    if (imu->ay > TURN_THRESH) {
        return CMD_RIGHT;
    }

    if (imu->ay < -TURN_THRESH) {
        return CMD_LEFT;
    }

    return CMD_STOP;
}