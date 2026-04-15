#include "pico/stdlib.h"
#include "motor.h"

// Set to 1 to step through motors 0..3 at 100% for 2s each (wiring check).
// Set to 0 for normal driving.
#define MOTOR_DIAG_SEQUENCE 0

int main(void) {
    stdio_init_all();
    motor_init();

    while (1) {
#if MOTOR_DIAG_SEQUENCE
        for (unsigned i = 0; i < 4u; i++) {
            drive_single_motor(i, 100.0f);
            sleep_ms(2000);
        }
        motor_stop_all();
        sleep_ms(500);
#else
        drive_forward(10.0f);
#endif
    }
}