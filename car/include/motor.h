#ifndef MOTOR_H
#define MOTOR_H

void motor_init(void);
void drive_all(float left_percent, float right_percent);
void motor_stop_all(void);

#endif