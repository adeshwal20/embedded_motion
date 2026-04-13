#include "gesture.h"

#define AX_THRESHOLD  2.0f
#define AY_THRESHOLD  2.0f

command_t detect_gesture(const imu_tilt_t *tilt) {
    if (!tilt) {
        return CMD_STOP;
    }

    if (tilt->ax > AX_THRESHOLD) {
        return CMD_FORWARD;
    }
    if (tilt->ax < -AX_THRESHOLD) {
        return CMD_BACKWARD;
    }
    if (tilt->ay > AY_THRESHOLD) {
        return CMD_RIGHT;
    }
    if (tilt->ay < -AY_THRESHOLD) {
        return CMD_LEFT;
    }

    return CMD_STOP;
}