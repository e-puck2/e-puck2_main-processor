#ifndef GROUND_H
#define GROUND_H

#ifdef __cplusplus
extern "C" {
#endif

#define GROUND_NB_CHANNELS 5 // 3 from gound + 2 from cliff

/** Struct containing a ground measurement message. */
typedef struct {
    /** Ambient light level (LED is OFF). */
    uint16_t ambient[GROUND_NB_CHANNELS];

    /** Difference between ambient and reflected. */
    uint16_t delta[GROUND_NB_CHANNELS];
} ground_msg_t;

 /**
 * @brief   Check the presence of the ground sensor and start the publisher.
 * 			It broadcast a ground_msg_t message on the /ground topic.
 */
void ground_start(void);

/**
* @brief   Stop the publisher.
*
*/
void ground_stop(void);

 /**
 * @brief   Returns the last value measured by the chosen sensor
 * 
 * @param sensor_number		0-4: 0=ground left, 1=ground center, 2=ground right, 3=cliff right, 4=cliff left
 * 
 * @return					Last value measured by the sensor
 */
int get_ground_prox(unsigned int sensor_number);


 /**
 * @brief   Returns the last ambiant light value measured by the chosen sensor
 * 
 * @param sensor_number		0-4: 0=ground left, 1=ground center, 2=ground right, 3=cliff right, 4=cliff left
 * 
 * @return					Last ambiant light value measured by the sensor
 */
int get_ground_ambient_light(unsigned int sensor_number);

#ifdef __cplusplus
}
#endif

#endif /* GROUND_H */
