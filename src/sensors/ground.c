#include <stdio.h>
#include <string.h>
#include "ch.h"
#include "hal.h"
#include "ground.h"
#include <main.h>
#include "i2c_bus.h"

static ground_msg_t ground_values;
static bool ground_configured = false;
static thread_t *groundThd;

/***************************INTERNAL FUNCTIONS************************************/

 /**
 * @brief   Thread which updates the measures and publishes them
 */
static THD_FUNCTION(ground_thd, arg)
{
    (void) arg;
    chRegSetThreadName(__FUNCTION__);

    messagebus_topic_t ground_topic;
    MUTEX_DECL(ground_topic_lock);
    CONDVAR_DECL(ground_topic_condvar);
    messagebus_topic_init(&ground_topic, &ground_topic_lock, &ground_topic_condvar, &ground_values, sizeof(ground_values));
    messagebus_advertise_topic(&bus, &ground_topic, "/ground");
    systime_t time;
	uint8_t temp[6];
	uint8_t i = 0, j = 0;

    while (chThdShouldTerminateX() == false) {
    	time = chVTGetSystemTime();

        for (j=0; j<6; j++) {
            if (j%2 == 0) {
            	read_reg(0xC0, j+1, &temp[i++]);
            } else {
            	read_reg(0xC0, j-1, &temp[i++]);
            }
        }
        ground_values.delta[0] = (uint16_t)(temp[0] & 0xff) + ((uint16_t)temp[1] << 8);
        ground_values.delta[1] = (uint16_t)(temp[2] & 0xff) + ((uint16_t)temp[3] << 8);
        ground_values.delta[2] = (uint16_t)(temp[4] & 0xff) + ((uint16_t)temp[5] << 8);

        i=0;
        for (j=6; j<12; j++) {
            if (j%2 == 0) {
            	read_reg(0xC0, j+1, &temp[i++]);
            } else {
            	read_reg(0xC0, j-1, &temp[i++]);
            }
        }
        ground_values.ambient[0] = (uint16_t)(temp[0] & 0xff) + ((uint16_t)temp[1] << 8);
        ground_values.ambient[1] = (uint16_t)(temp[2] & 0xff) + ((uint16_t)temp[3] << 8);
        ground_values.ambient[2] = (uint16_t)(temp[4] & 0xff) + ((uint16_t)temp[5] << 8);

        messagebus_topic_publish(&ground_topic, &ground_values, sizeof(ground_values));

        chThdSleepUntilWindowed(time, time + MS2ST(40)); //reduced the sample rate to 25Hz

    }
}

/*************************END INTERNAL FUNCTIONS**********************************/


/****************************PUBLIC FUNCTIONS*************************************/

void ground_start(void)
{
	uint8_t temp[6];
	uint8_t i = 0, j = 0;

	if(ground_configured) {
		return;
	}

	i2c_start();

    for (j=0; j<6; j++) {
        if (j%2 == 0) {
        	read_reg(0xC0, j+1, &temp[i++]);
        } else {
        	read_reg(0xC0, j-1, &temp[i++]);
        }
    }
    ground_values.delta[0] = (uint16_t)(temp[0] & 0xff) + ((uint16_t)temp[1] << 8);
    ground_values.delta[1] = (uint16_t)(temp[2] & 0xff) + ((uint16_t)temp[3] << 8);
    ground_values.delta[2] = (uint16_t)(temp[4] & 0xff) + ((uint16_t)temp[5] << 8);

    i=0;
    for (j=6; j<12; j++) {
        if (j%2 == 0) {
        	read_reg(0xC0, j+1, &temp[i++]);
        } else {
        	read_reg(0xC0, j-1, &temp[i++]);
        }
    }
    ground_values.ambient[0] = (uint16_t)(temp[0] & 0xff) + ((uint16_t)temp[1] << 8);
    ground_values.ambient[1] = (uint16_t)(temp[2] & 0xff) + ((uint16_t)temp[3] << 8);
    ground_values.ambient[2] = (uint16_t)(temp[4] & 0xff) + ((uint16_t)temp[5] << 8);

    if(ground_values.delta[0]!=0 && ground_values.delta[1]!=0 && ground_values.delta[2]!=0) { // If ground sensor is present.
    	ground_configured = true;
    	static THD_WORKING_AREA(ground_thd_wa, 1024);
    	groundThd = chThdCreateStatic(ground_thd_wa, sizeof(ground_thd_wa), NORMALPRIO, ground_thd, NULL);
    }
	
}

void ground_stop(void) {
    chThdTerminate(groundThd);
    chThdWait(groundThd);
    groundThd = NULL;
    ground_configured = false;
}

int get_ground_prox(unsigned int sensor_number) {
	if (sensor_number > (GROUND_NB_CHANNELS-1)) {
		return 0;
	} else {
		return ground_values.delta[sensor_number];
	}
}

int get_ground_ambient_light(unsigned int sensor_number) {
	if (sensor_number > (GROUND_NB_CHANNELS-1)) {
		return 0;
	} else {
		return ground_values.ambient[sensor_number];
	}
}

/**************************END PUBLIC FUNCTIONS***********************************/
