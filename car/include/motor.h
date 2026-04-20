#ifndef MOTOR_H
#define MOTOR_H

void motor_init(void);
/**
 * Left = driver 1 (GPIO 12/10/11), right = driver 2 (GPIO 35/36/37). Each TB6612 runs two motors (A+B tied);
 * so non-zero left and right together spins all four wheels (skid-steer).
 */
void drive_all(float left_percent, float right_percent);
/** Straight line: both drivers at same speed → all four wheels forward. */
void drive_forward(float speed_percent);
/** Straight line: all four wheels reverse. */
void drive_backward(float speed_percent);
/** Bench: one driver at speed; other off. Indices 0,1 = driver 1; 2,3 = driver 2. */
void drive_single_motor(unsigned motor_index, float speed_percent);
void drive_motor1_channel_a(float speed_percent);
void drive_motor1_channel_b(float speed_percent);
void motor_stop_all(void);

#endif