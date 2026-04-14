#include "pico/stdlib.h"
#include "motor.h"

int main(void) {
    stdio_init_all();
    motor_init();

    while (1) {
        drive_all(70.0f, 70.0f);
    }
}