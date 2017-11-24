#ifndef MOTORS_H
#define MOTORS_H

#include <stdint.h>
#include <hal.h>

#define MOTOR_TIMER_FREQ 100000 // [Hz]
#define MOTOR_SPEED_LIMIT 2200 // [steps/s]
#define THRESV 650 // This the speed under which the power save feature is active.

struct stepper_motor_s {
    enum {
        HALT=0,
        FORWARD=1,
        BACKWARD=2
    } direction;
    uint8_t step_index;
    int32_t count;
    void (*update)(const uint8_t *out);
    void (*enable_power_save)(void);
    void (*disable_power_save)(void);
    PWMDriver *timer;
};

/** Set motor speed in steps per second. */
void left_motor_set_speed(int speed);
void right_motor_set_speed(int speed);

/** Read motor position counter */
uint32_t left_motor_get_pos(void);
uint32_t right_motor_get_pos(void);

/** Initialize motors */
void motors_init(void);

#endif /* MOTOR_H */
