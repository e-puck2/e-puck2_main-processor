#include <hal.h>
#include "behaviors.h"
#include "motors.h"
#include "sensors/proximity.h"
#include "../usbcfg.h"
#include "chprintf.h"
#include "ch.h"
#include <string.h>
#include <stdio.h>

#define OA_ACTIVE_THRESHOLD 300 // If the proximity is higher than this threshold, then activate obstacles avoidance, otherwise move based on user speed settings.
#define NOISE_THR 5
#define DIRECTION_FW 0
#define DIRECTION_LEFT 1
#define DIRECTION_RIGHT 2

static thread_t *behaviorsThd;
uint8_t oa_enabled = 0;
int32_t target_speed_left = 0;
int32_t target_speed_right = 0;

void enable_obstacle_avoidance(void) {
	oa_enabled = 1;
}

void disable_obstacle_avoidance(void) {
	oa_enabled = 0;
}

uint8_t obstacle_avoidance_enabled(void) {
	return oa_enabled;
}

void obstacle_avoidance_set_speed_left(int speed) {
	target_speed_left = speed;
}

void obstacle_avoidance_set_speed_right(int speed) {
	target_speed_right = speed;
}

static THD_WORKING_AREA(behaviors_thd_wa, 128);
static THD_FUNCTION(behaviors_thd, arg) {
    (void) arg;
    chRegSetThreadName(__FUNCTION__);
    systime_t time;

    int32_t left_speed = 0, right_speed = 0;
    int16_t prox_values_temp[8];
    uint8_t i = 0;
    int32_t sum_sensors_x = 0, sum_sensors_y = 0;

    while (chThdShouldTerminateX() == false) {
        time = chVTGetSystemTime();
        if(oa_enabled) {

        	// Obstacle avoidance using all the proximity sensors based on a simplified force field method.
        	// Position of the robot sensors:
        	//		forward
        	//
        	//		  7	  0 (15 deg)
        	//		6		1 (45 deg)
        	//	velL	x	 velR
        	//	  |		|	  |
        	//	  5	  y_0	  2
        	//
        	//		 4	   3 (150 deg)
        	//
        	// The following table shows the weights (simplified respect to the trigonometry) of all the proximity sensors for the resulting repulsive force:
        	//  Prox	0		1		2		3		4		5		6		7
        	//	x		-1		-0.5	0		0.75	0.75	0		-0.5	-1
        	//	y		0.5	0.5		1		0.5		-0.5	-1		-0.5	-0.5

        	// Consider small values to be noise thus set them to zero in order to not influence the resulting force.
        	for(i=0; i<8; i++) {
            	prox_values_temp[i] = get_calibrated_prox(i);
        		if(prox_values_temp[i] < NOISE_THR) {
        			prox_values_temp[i] = 0;
        		}
        	}

        	// Sum the contribution of each sensor (based on the previous weights table).
        	sum_sensors_x = -prox_values_temp[0] - (prox_values_temp[1]>>1) + (prox_values_temp[3]-(prox_values_temp[3]>>2)) + (prox_values_temp[4]-(prox_values_temp[4]>>2)) - (prox_values_temp[6]>>1) - prox_values_temp[7];
        	sum_sensors_y = (prox_values_temp[0]>>1) + (prox_values_temp[1]>>1) + prox_values_temp[2] + (prox_values_temp[3]>>1) - (prox_values_temp[4]>>1) - prox_values_temp[5] - (prox_values_temp[6]>>1) - (prox_values_temp[7]>>1);

        	// Modify the velocity components based on sensor values.
        	if(target_speed_left >= 0) {
        		left_speed = target_speed_left + ((sum_sensors_x>>1) - (sum_sensors_y<<2));
        	} else {
        		left_speed = target_speed_left - ((sum_sensors_x>>1) + (sum_sensors_y<<2));
        	}
        	if(target_speed_right >=0) {
        		right_speed = target_speed_right + ((sum_sensors_x>>1) + (sum_sensors_y<<2));
        	} else {
        		right_speed = target_speed_right - ((sum_sensors_x>>1) - (sum_sensors_y<<2));
        	}

        	motor_set_speed(&left_motor, left_speed);
        	motor_set_speed(&right_motor, right_speed);

        }

        chThdSleepUntilWindowed(time, time + MS2ST(20)); // Refresh @ 50 Hz
    }
}

void behaviors_start(void) {
	oa_enabled = 0;
	behaviorsThd = chThdCreateStatic(behaviors_thd_wa, sizeof(behaviors_thd_wa), NORMALPRIO, behaviors_thd, NULL);
}

