#include <hal.h>
#include "behaviors.h"
#include "motors.h"
#include "sensors/proximity.h"

#define OA_ACTIVE_THRESHOLD 300 // If the proximity is higher than this threshold, then activate obstacles avoidance, otherwise move based on user speed settings.

static thread_t *behaviorsThd;
uint8_t oa_enabled = 0;
int16_t target_speed = 0;

void enable_obstacle_avoidance(int speed) {
	oa_enabled = 1;
	target_speed = speed;
}

void disable_obstacle_avoidance(void) {
	oa_enabled = 0;
}

static THD_WORKING_AREA(behaviors_thd_wa, 128);
static THD_FUNCTION(behaviors_thd, arg) {
    (void) arg;
    chRegSetThreadName(__FUNCTION__);
    systime_t time;

    int16_t left_speed = 0, right_speed = 0;
    int16_t prox_values_temp[8];
    uint8_t i = 0;

    while (chThdShouldTerminateX() == false) {
        time = chVTGetSystemTime();
        if(oa_enabled) {
        	// Simple obstacles avoidance implementation (only for forward motion).
        	for(i=0; i<8; i++) {
        		prox_values_temp[i] = get_calibrated_prox(i);
        	}
        	if(target_speed > 0) {
        		left_speed = target_speed - prox_values_temp[0]*8 - prox_values_temp[1]*4 - prox_values_temp[2]*2;
        		right_speed = target_speed - prox_values_temp[7]*8 - prox_values_temp[6]*4 - prox_values_temp[5]*2;
        	}
			right_motor_set_speed(right_speed);
			left_motor_set_speed(left_speed);

        }

        chThdSleepUntilWindowed(time, time + MS2ST(10)); // Refresh @ 100 Hz
    }
}

void behaviors_start(void) {
	behaviorsThd = chThdCreateStatic(behaviors_thd_wa, sizeof(behaviors_thd_wa), NORMALPRIO, behaviors_thd, NULL);
}

