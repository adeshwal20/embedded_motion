#include "gesture.h"

<<<<<<< HEAD
#define ACC_X_THRESHOLD   1.0f
#define ACC_Y_THRESHOLD   1.0f
#define GYRO_Z_THRESHOLD  0.7f
=======
#define AX_THRESHOLD  1.8f
#define AY_THRESHOLD  1.8f
>>>>>>> dbd89a8 (nothing' happening but communication)

command_t detect_gesture(const imu_tilt_t *tilt) {
    if (!tilt) {
        return CMD_STOP;
    }

<<<<<<< HEAD
    // Prioritize yaw/rotation gestures first.
    if (tilt->gz > GYRO_Z_THRESHOLD) {
        return CMD_RIGHT;
    }
    if (tilt->gz < -GYRO_Z_THRESHOLD) {
        return CMD_LEFT;
    }

    // Then use tilt/acceleration for forward/backward.
    if (tilt->ax > ACC_X_THRESHOLD) {
        return CMD_FORWARD;
    }
    if (tilt->ax < -ACC_X_THRESHOLD) {
        return CMD_BACKWARD;
    }
    if (tilt->ay > ACC_Y_THRESHOLD) {
        return CMD_RIGHT;
    }
    if (tilt->ay < -ACC_Y_THRESHOLD) {
        return CMD_LEFT;
    }
=======
    if (tilt->ax > AX_THRESHOLD)  return CMD_FORWARD;
    if (tilt->ax < -AX_THRESHOLD) return CMD_BACKWARD;
    if (tilt->ay > AY_THRESHOLD)  return CMD_RIGHT;
    if (tilt->ay < -AY_THRESHOLD) return CMD_LEFT;
>>>>>>> dbd89a8 (nothing' happening but communication)

    return CMD_STOP;
}