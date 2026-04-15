#ifndef MOTOR_H
#define MOTOR_H

void motor_init(void);
/** Driver 1 = left_percent, driver 2 = right_percent (shared A/B nets per chip). */
void drive_all(float left_percent, float right_percent);
/** Same speed both drivers — use for straight line (see MOTOR_DRIVER2_SWAP_DIRECTION in motor.c). */
void drive_forward(float speed_percent);
/** Bench: one driver at speed; other off. Indices 0,1 = driver 1; 2,3 = driver 2. */
void drive_single_motor(unsigned motor_index, float speed_percent);
void drive_motor1_channel_a(float speed_percent);
void drive_motor1_channel_b(float speed_percent);
void motor_stop_all(void);

#endif