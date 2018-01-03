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

#define RESISTOR_R1				220	//kohm
#define RESISTOR_R2 			330 //kohm
#define VOLTAGE_DIVIDER 		(1.0f * RESISTOR_R2 / (RESISTOR_R1 + RESISTOR_R2))

#define VREF					3.0f //volt correspond to the voltage on the VREF+ pin
#define ADC_RESOLUTION			4096

#define COEFF_ADC_TO_VOLT		((1.0f * ADC_RESOLUTION * VOLTAGE_DIVIDER) / VREF) //convertion from adc value to voltage

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

/** Starts the battery measurement service. */
void battery_level_start(void);
uint16_t get_battery_raw(void);
float get_battery_voltage(void);
float get_battery_percentage(void);

#endif
