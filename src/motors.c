#include <ch.h>
#include <hal.h>
#include "motors.h"
#include "leds.h"

static const uint8_t step_halt[4] = {0, 0, 0, 0};
static const uint8_t step_table[8][4] = {
    {1, 0, 1, 0},
	{0, 0, 1, 0},
    {0, 1, 1, 0},
	{0, 1, 0, 0},
    {0, 1, 0, 1},
	{0, 0, 0, 1},
    {1, 0, 0, 1},
	{1, 0, 0, 0},
};

struct stepper_motor_s right_motor;
struct stepper_motor_s left_motor;

static void right_motor_update(const uint8_t *out)
{
    /* right motor */
    out[0] ? palSetPad(GPIOE, GPIOE_MOT_R_IN1) : palClearPad(GPIOE, GPIOE_MOT_R_IN1);
    out[1] ? palSetPad(GPIOE, GPIOE_MOT_R_IN2) : palClearPad(GPIOE, GPIOE_MOT_R_IN2);
    out[2] ? palSetPad(GPIOE, GPIOE_MOT_R_IN3) : palClearPad(GPIOE, GPIOE_MOT_R_IN3);
    out[3] ? palSetPad(GPIOE, GPIOE_MOT_R_IN4) : palClearPad(GPIOE, GPIOE_MOT_R_IN4);
}

static void left_motor_update(const uint8_t *out)
{
    /* left motor */
    out[0] ? palSetPad(GPIOE, GPIOE_MOT_L_IN1) : palClearPad(GPIOE, GPIOE_MOT_L_IN1);
    out[1] ? palSetPad(GPIOE, GPIOE_MOT_L_IN2) : palClearPad(GPIOE, GPIOE_MOT_L_IN2);
    out[2] ? palSetPad(GPIOE, GPIOE_MOT_L_IN3) : palClearPad(GPIOE, GPIOE_MOT_L_IN3);
    out[3] ? palSetPad(GPIOE, GPIOE_MOT_L_IN4) : palClearPad(GPIOE, GPIOE_MOT_L_IN4);
}

static void right_motor_timer_callback(PWMDriver *gptp)
{
    (void) gptp;
    uint8_t i;
    if (right_motor.direction == BACKWARD) {
        i = (right_motor.step_index + 1) & 7;
        right_motor.update(step_table[i]);
        right_motor.count += 1;
        right_motor.step_index = i;
    } else if (right_motor.direction == FORWARD) {
        i = (right_motor.step_index - 1) & 7;
        right_motor.update(step_table[i]);
        right_motor.count -= 1;
        right_motor.step_index = i;
    } else {
        right_motor.update(step_halt);
    }
}

static void left_motor_timer_callback(PWMDriver *gptp)
{
    (void) gptp;
    uint8_t i;
    if (left_motor.direction == FORWARD) { // Inverted for the two motors
        i = (left_motor.step_index + 1) & 7;
        left_motor.update(step_table[i]);
        left_motor.count += 1;
        left_motor.step_index = i;
    } else if (left_motor.direction == BACKWARD) {
        i = (left_motor.step_index - 1) & 7;
        left_motor.update(step_table[i]);
        left_motor.count -= 1;
        left_motor.step_index = i;
    } else {
        left_motor.update(step_halt);
    }
}

static void right_motor_pwm_ch1_cb(PWMDriver *pwmp) {
	(void)pwmp;
    palClearPad(GPIOE, GPIOE_MOT_R_IN1);
    palClearPad(GPIOE, GPIOE_MOT_R_IN2);
    palClearPad(GPIOE, GPIOE_MOT_R_IN3);
    palClearPad(GPIOE, GPIOE_MOT_R_IN4);
}

static void left_motor_pwm_ch1_cb(PWMDriver *pwmp) {
	(void)pwmp;
    palClearPad(GPIOE, GPIOE_MOT_L_IN1);
    palClearPad(GPIOE, GPIOE_MOT_L_IN2);
    palClearPad(GPIOE, GPIOE_MOT_L_IN3);
    palClearPad(GPIOE, GPIOE_MOT_L_IN4);
}

void right_motor_enable_power_save(void) {
    pwmEnableChannel(&PWMD3, 0, (pwmcnt_t) (MOTOR_TIMER_FREQ/THRESV)); // Enable channel 1 to set duty cycle for power save.
	pwmEnableChannelNotification(&PWMD3, 0); // Channel 1 interrupt enable to handle motor shutdown.
}

void left_motor_enable_power_save(void) {
    pwmEnableChannel(&PWMD4, 0, (pwmcnt_t) (MOTOR_TIMER_FREQ/THRESV)); // Enable channel 1 to set duty cycle for power save.
	pwmEnableChannelNotification(&PWMD4, 0); // Channel 1 interrupt enable to handle motor shutdown.
}

void right_motor_disable_power_save(void) {
	pwmDisableChannel(&PWMD3, 0);
}

void left_motor_disable_power_save(void) {
	pwmDisableChannel(&PWMD4, 0);
}

void motor_set_speed(struct stepper_motor_s *m, int speed)
{
    /* limit motor speed */
    if (speed > MOTOR_SPEED_LIMIT) {
        speed = MOTOR_SPEED_LIMIT;
    } else if (speed < -MOTOR_SPEED_LIMIT) {
        speed = -MOTOR_SPEED_LIMIT;
    }

    uint16_t interval;
    if (speed == 0) {
        m->direction = HALT;
        interval = 0xffff;
        m->disable_power_save();
    } else {
        if (speed > 0) {
            m->direction = FORWARD;
        } else {
            m->direction = BACKWARD;
            speed = -speed;
        }
        interval = MOTOR_TIMER_FREQ / speed;

        if(speed < THRESV) {
        	m->enable_power_save();
        } else {
        	m->disable_power_save();
        }
    }

    /* change motor step interval */
    pwmChangePeriod(m->timer, interval);
}

void right_motor_set_speed(int speed) {
	motor_set_speed(&right_motor, speed);
}

void left_motor_set_speed(int speed) {
	motor_set_speed(&left_motor, speed);
}

uint32_t right_motor_get_pos(void) {
	return right_motor.count;
}

uint32_t left_motor_get_pos(void) {
	return left_motor.count;
}

void motors_init(void)
{
    /* motor struct init */
    right_motor.direction = HALT;
    right_motor.step_index = 0;
    right_motor.count = 0;
    right_motor.update = right_motor_update;
    right_motor.enable_power_save = right_motor_enable_power_save;
    right_motor.disable_power_save = right_motor_disable_power_save;
    right_motor.timer = &PWMD3;

    left_motor.direction = HALT;
    left_motor.step_index = 0;
    left_motor.count = 0;
    left_motor.update = left_motor_update;
    left_motor.enable_power_save = left_motor_enable_power_save;
    left_motor.disable_power_save = left_motor_disable_power_save;
    left_motor.timer = &PWMD4;

    /* motor init halted*/
    right_motor_update(step_halt);
    left_motor_update(step_halt);

    /* timer init */
    static const PWMConfig pwmcfg_right_motor = {
        .frequency = MOTOR_TIMER_FREQ,
        .period = 0xFFFF,
        .cr2 = 0,
        .callback = right_motor_timer_callback,
        .channels = {
            // Channel 1 is used to handling motor power save feature.
            {.mode = PWM_OUTPUT_ACTIVE_HIGH, .callback = right_motor_pwm_ch1_cb},
            {.mode = PWM_OUTPUT_DISABLED, .callback = NULL},
            {.mode = PWM_OUTPUT_DISABLED, .callback = NULL},
            {.mode = PWM_OUTPUT_DISABLED, .callback = NULL},
        },
    };
    pwmStart(&PWMD3, &pwmcfg_right_motor);
    pwmEnablePeriodicNotification(&PWMD3); // PWM general interrupt at the beginning of the period to handle motor steps.

    static const PWMConfig pwmcfg_left_motor = {
        .frequency = MOTOR_TIMER_FREQ,
        .period = 0xFFFF,
        .cr2 = 0,
        .callback = left_motor_timer_callback,
        .channels = {
            // Channel 1 is used to handling motor power save feature.
            {.mode = PWM_OUTPUT_ACTIVE_HIGH, .callback = left_motor_pwm_ch1_cb},
            {.mode = PWM_OUTPUT_DISABLED, .callback = NULL},
            {.mode = PWM_OUTPUT_DISABLED, .callback = NULL},
            {.mode = PWM_OUTPUT_DISABLED, .callback = NULL},
        },
    };
    pwmStart(&PWMD4, &pwmcfg_left_motor);
    pwmEnablePeriodicNotification(&PWMD4); // PWM general interrupt at the beginning of the period to handle motor steps.

}
