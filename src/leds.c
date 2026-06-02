#include "leds.h"
#include <string.h>

static uint8_t rgb_led[NUM_RGB_LED][NUM_COLOR_LED];

/*! \brief turn on/off the red LEDs
 *
 * The e-puck2 has 4 red LEDs placed on front, right, back and left; these LEDs are directly controllable from the main processor (F407).
 * There are also 4 RGB LEDs placed at 45, 135, 225, 315 degrees; these LEDs are connected to the ESP32 and can be controlled through SPI.
 * With this function, you can change the state of the 4 red LEDs, not the RGB LEDs.
 * \param led_number: LED1, LED3, LED5, LED7 (LED1 is the front led, then continue clockwise) or ALL_LEDS for all LEDs
 * \param value 0 (off), 1 (on), any other value will toggle the state

*/
void set_led(led_name_t led_number, unsigned int value) {
	switch(led_number) {
		case LED1: 
			if(value>1)  {
				palTogglePad(GPIOD, GPIOD_LED1);
			} else {
				value?palClearPad(GPIOD, GPIOD_LED1):palSetPad(GPIOD, GPIOD_LED1);
			}
			break;
		case LED3:
			if(value>1) {
				palTogglePad(GPIOD, GPIOD_LED3);
			} else {
				value?palClearPad(GPIOD, GPIOD_LED3):palSetPad(GPIOD, GPIOD_LED3);
			}
			break;
		case LED5:
			if(value>1) {
				palTogglePad(GPIOD, GPIOD_LED5);
			} else {
				value?palClearPad(GPIOD, GPIOD_LED5):palSetPad(GPIOD, GPIOD_LED5);
			}
			break;
		case LED7:
			if(value>1) {
				palTogglePad(GPIOD, GPIOD_LED7);
			} else {
				value?palClearPad(GPIOD, GPIOD_LED7):palSetPad(GPIOD, GPIOD_LED7);
			}
			break;
		case ALL_LEDS:
		default:
			for(int i=0; i<NUM_LED; i++) {
				set_led(i, value);
			}
	}
}

void set_rgb_led(rgb_led_name_t led_number, uint8_t red_val, uint8_t green_val, uint8_t blue_val) {
	rgb_led[led_number][RED_LED] = red_val;
	rgb_led[led_number][GREEN_LED] = green_val;
	rgb_led[led_number][BLUE_LED] = blue_val;
}

void toggle_rgb_led(rgb_led_name_t led_number, color_led_name_t led, uint8_t intensity) {
	if(rgb_led[led_number][led] > 0) {
		rgb_led[led_number][led] = 0;
	} else {
		rgb_led[led_number][led] = intensity;
	}
}

/*! \brief turn on/off the body LED
 *
 * The e-puck has a green LED that illuminate his body. With this function,
 * you can change the state of this LED.
 * \param value 0 (off), 1 (on) otherwise change the state
 */
void set_body_led(unsigned int value)
{
	if(value>1)
		palTogglePad(GPIOB, GPIOB_LED_BODY);
	else
		value?palSetPad(GPIOB, GPIOB_LED_BODY):palClearPad(GPIOB, GPIOB_LED_BODY);
}

/*! \brief turn on/off the front LED
 *
 * The e-puck has a red LED in the front. With this function, you can
 * change the state of this LED.
 * \param value 0 (off), 1 (on), any other value will toggle the state
 */
void set_front_led(unsigned int value)
{
	if(value>1)
		palTogglePad(GPIOD, GPIOD_LED_FRONT);
	else
		value?palSetPad(GPIOD, GPIOD_LED_FRONT):palClearPad(GPIOD, GPIOD_LED_FRONT);
}

/*! \brief Turn off all the LEDs around the robot
 *
 * The e-puck2 has 4 red LEDs and 4 RGB LEDs placed on top of it. This function turns them all off.
 * \warning this function does not turn off the "body LED" and "front LED".
 */
void clear_leds(void)
{
	for(int i=0; i<NUM_LED; i++) {
		set_led(i, 0);
	}
	for (int i=0; i<NUM_RGB_LED; i++) {
		set_rgb_led(i, 0, 0, 0);
	}
}

void get_all_rgb_state(uint8_t* values) {
	memcpy(values, rgb_led, NUM_RGB_LED * NUM_COLOR_LED);
}

