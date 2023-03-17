#ifndef PROXIMITY_H
#define PROXIMITY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define PROXIMITY_NB_CHANNELS 8
#define FAST_UPDATE 0	// Proximity sensors updated at 100 Hz
#define SLOW_UPDATE 1	// Proximity sensors updated at 20 Hz

/** Struct containing a proximity measurement message. */
typedef struct {
    /** Ambient light level (LED is OFF). */
    unsigned int ambient[PROXIMITY_NB_CHANNELS];

    /** Reflected light level (LED is ON). */
    unsigned int reflected[PROXIMITY_NB_CHANNELS];

    /** Difference between ambient and reflected. */
    unsigned int delta[PROXIMITY_NB_CHANNELS];

    /** Initial values saved during calibration. */
    unsigned int initValue[PROXIMITY_NB_CHANNELS];
} proximity_msg_t;

 /**
 * @brief   Starts the proximity measurement module. Make sure that the "ircom" module wasn't started when using this function otherwise there will be conflicts.
 * 			This module also broadcasts the measures through a proximity_msg_t message
 * 			on the /proximity topic
 *
 * @param freq	FAST_UPDATE or SLOW_UPDATE. SLOW_UPDATE only used when working with the "range and bearing" extension.
 */
void proximity_start(uint8_t mode);

/**
* @brief   Stop the proximity measurement module
*/
void proximity_stop(void);

 /**
 * @brief   Runs the IR sensor calibration process
 */
void calibrate_ir(void);

 /**
 * @brief   Returns the last value measured by the chosen sensor
 * 
 * @param sensor_number		0-7
 * 
 * @return					Last value measured by the sensor
 */
int get_prox(unsigned int sensor_number);

 /**
 * @brief   Returns the calibration value for the chosen sensor
 * 
 * @param sensor_number		0-7
 * 
 * @return					Calibration value of the sensor
 */
int get_calibrated_prox(unsigned int sensor_number);

 /**
 * @brief   Returns the last ambiant light value measured by the chosen sensor
 * 
 * @param sensor_number		0-7
 * 
 * @return					Last ambiant light value measured by the sensor
 */
int get_ambient_light(unsigned int sensor_number);

#ifdef __cplusplus
}
#endif

#endif /* PROXIMITY_H */
