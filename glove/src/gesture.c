#include "gesture.h"
#include <math.h>

// Orientation-state mapping:
// - Upright        -> STOP
// - Flat           -> FORWARD
// - Flat + twist   -> LEFT/RIGHT
#define UPRIGHT_Z_RATIO_STOP   0.75f
#define FLAT_Z_RATIO_ACTIVE    0.40f
#define TWIST_GZ_THRESHOLD     0.22f

static command_t s_last_cmd = CMD_STOP;

command_t detect_gesture(const imu_tilt_t *tilt) {
    if (!tilt) {
        return CMD_STOP;
    }

    // Normalize attitude against gravity magnitude.
    float g = sqrtf(tilt->ax * tilt->ax + tilt->ay * tilt->ay + tilt->az * tilt->az);
    if (g < 1.0f) {
        s_last_cmd = CMD_STOP;
        return CMD_STOP;
    }
    float z_ratio = fabsf(tilt->az) / g; // near 1.0 upright, near 0.0 horizontal

    if (z_ratio > UPRIGHT_Z_RATIO_STOP) {
        s_last_cmd = CMD_STOP;
        return CMD_STOP;
    }

    if (z_ratio < FLAT_Z_RATIO_ACTIVE) {
        if (tilt->gz > TWIST_GZ_THRESHOLD) {
            s_last_cmd = CMD_RIGHT;
            return CMD_RIGHT;
        }
        if (tilt->gz < -TWIST_GZ_THRESHOLD) {
            s_last_cmd = CMD_LEFT;
            return CMD_LEFT;
        }
        s_last_cmd = CMD_FORWARD;
        return CMD_FORWARD;
    }

    // Transition band: keep last non-stop motion briefly for smoother control.
    if (s_last_cmd != CMD_STOP) {
        return s_last_cmd;
    }
    s_last_cmd = CMD_STOP;
    return CMD_STOP;
}