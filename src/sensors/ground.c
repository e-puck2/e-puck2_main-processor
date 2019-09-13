#include <stdio.h>
#include <string.h>
#include "ch.h"
#include "hal.h"
#include "ground.h"
#include <main.h>
#include "i2c_bus.h"
#include "usbcfg.h"
#include "chprintf.h"

static ground_msg_t ground_values;
static bool ground_configured = false;
static thread_t *groundThd;

#define GROUND_ADDR 0x60

/***************************INTERNAL FUNCTIONS************************************/

 /**
 * @brief   Thread which updates the measures and publishes them
 */
static THD_WORKING_AREA(ground_thd_wa, 512);
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
	uint8_t temp[21]; // 3 x ground proximity (6 bytes) + 3 x ground ambient (6 bytes) + software revision (1 byte) + 2 x cliff proximity (4 bytes) + 2 x cliff ambient (4 bytes)

    while (chThdShouldTerminateX() == false) {
    	time = chVTGetSystemTime();

    	read_reg_multi(GROUND_ADDR, 0, temp, 21);
    	// Ground
        ground_values.delta[0] = (uint16_t)(temp[1] & 0xff) + ((uint16_t)temp[0] << 8);
        ground_values.delta[1] = (uint16_t)(temp[3] & 0xff) + ((uint16_t)temp[2] << 8);
        ground_values.delta[2] = (uint16_t)(temp[5] & 0xff) + ((uint16_t)temp[4] << 8);
        ground_values.ambient[0] = (uint16_t)(temp[7] & 0xff) + ((uint16_t)temp[6] << 8);
        ground_values.ambient[1] = (uint16_t)(temp[9] & 0xff) + ((uint16_t)temp[8] << 8);
        ground_values.ambient[2] = (uint16_t)(temp[11] & 0xff) + ((uint16_t)temp[10] << 8);
        // Cliff
        ground_values.delta[3] = (uint16_t)(temp[14] & 0xff) + ((uint16_t)temp[13] << 8);
        ground_values.delta[4] = (uint16_t)(temp[16] & 0xff) + ((uint16_t)temp[15] << 8);
        ground_values.ambient[3] = (uint16_t)(temp[18] & 0xff) + ((uint16_t)temp[17] << 8);
        ground_values.ambient[4] = (uint16_t)(temp[20] & 0xff) + ((uint16_t)temp[19] << 8);

        messagebus_topic_publish(&ground_topic, &ground_values, sizeof(ground_values));

    	//chprintf((BaseSequentialStream *)&SDU1, "prox: %d, %d, %d,\r\n", ground_values.delta[0], ground_values.delta[1], ground_values.delta[2]);
    	//chprintf((BaseSequentialStream *)&SDU1, "ambient: %d, %d, %d,\r\n", ground_values.ambient[0], ground_values.ambient[1], ground_values.ambient[2]);

        chThdSleepUntilWindowed(time, time + MS2ST(40)); //reduced the sample rate to 25Hz

    }
}

/*************************END INTERNAL FUNCTIONS**********************************/


/****************************PUBLIC FUNCTIONS*************************************/

void ground_start(void)
{
	uint8_t temp[21];
	uint8_t i = 0;
	int8_t err = 0;

	if(ground_configured) {
		return;
	}

	for(i=0; i<GROUND_NB_CHANNELS; i++) {
		ground_values.delta[i] = 0;
		ground_values.ambient[i] = 0;
	}

	i2c_start();

	if((err=read_reg_multi(GROUND_ADDR, 0, temp, 21)) != MSG_OK) {
		return;
	}
	// Ground
    ground_values.delta[0] = (uint16_t)(temp[1] & 0xff) + ((uint16_t)temp[0] << 8);
    ground_values.delta[1] = (uint16_t)(temp[3] & 0xff) + ((uint16_t)temp[2] << 8);
    ground_values.delta[2] = (uint16_t)(temp[5] & 0xff) + ((uint16_t)temp[4] << 8);
    ground_values.ambient[0] = (uint16_t)(temp[7] & 0xff) + ((uint16_t)temp[6] << 8);
    ground_values.ambient[1] = (uint16_t)(temp[9] & 0xff) + ((uint16_t)temp[8] << 8);
    ground_values.ambient[2] = (uint16_t)(temp[11] & 0xff) + ((uint16_t)temp[10] << 8);
    // Cliff
    ground_values.delta[3] = (uint16_t)(temp[14] & 0xff) + ((uint16_t)temp[13] << 8);
    ground_values.delta[4] = (uint16_t)(temp[16] & 0xff) + ((uint16_t)temp[15] << 8);
    ground_values.ambient[3] = (uint16_t)(temp[18] & 0xff) + ((uint16_t)temp[17] << 8);
    ground_values.ambient[4] = (uint16_t)(temp[20] & 0xff) + ((uint16_t)temp[19] << 8);

    ground_configured = true;
    groundThd = chThdCreateStatic(ground_thd_wa, sizeof(ground_thd_wa), NORMALPRIO, ground_thd, NULL);
	
}

void ground_stop(void) {
	if(ground_configured) {
		chThdTerminate(groundThd);
		chThdWait(groundThd);
		groundThd = NULL;
		ground_configured = false;
	}
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
