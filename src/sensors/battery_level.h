#ifndef BATTERY_LEVEL_H
#define BATTERY_LEVEL_H

#include <hal.h>

/* BATTERY---[ R1 ]--*--- measure
                     |
                     |
                   [ R2 ]
                     |
                    GND
*/

#define RESISTOR_R1             220 //kohm
#define RESISTOR_R2             330 //kohm

#define MAX_VOLTAGE				4.2f	//volt
#define MIN_VOLTAGE				3.4f	//volt
#define MAX_PERCENTAGE			100
#define MIN_PERCENTAGE			0

/** Message reprensenting a measurement of the battery level. */
typedef struct {
    float voltage;
    float percentage;
    uint16_t raw_value;
} battery_msg_t;

 /**
 * @brief   Starts the battery measurement service.
 * 			Also broadcast a battery_msg_t message on the /battery_level topic
 */
void battery_level_start(void);

 /**
 * @brief   Returns the raw value measured on the battery
 * 
 * @return		raw value
 */
uint16_t get_battery_raw(void);

 /**
 * @brief   Returns the tension measured on the battery
 * 
 * @return		Battery voltage [V]
 */
float get_battery_voltage(void);

 /**
 * @brief   Returns the battery level measured
 * 
 * @return		Battery level [percentage]
 */
float get_battery_percentage(void);

#endif
