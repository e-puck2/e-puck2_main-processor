#include <hal.h>
#include "leds.h"

/*! \brief turn on/off the specified LED
 *
 * The e-puck has 4 LEDs. With this function, you can
 * change the state of these LEDs.
 * \param led_number between 0 and 3
 * \param value 0 (off), 1 (on) otherwise change the state
 * \warning if led_number is other than 0-3, all leds are set
 * to the indicated value.
 */
void set_led(unsigned int led_number, unsigned int value)
{
	switch(led_number)
	{
		case 0: 
			{
			if(value>1)
				palTogglePad(GPIOD, GPIOD_LED1);
			else
				value?palClearPad(GPIOD, GPIOD_LED1):palSetPad(GPIOD, GPIOD_LED1);
			break;
			}
		case 1: 
			{
			if(value>1)
				palTogglePad(GPIOD, GPIOD_LED3);
			else
				value?palClearPad(GPIOD, GPIOD_LED3):palSetPad(GPIOD, GPIOD_LED3);
			break;
			}
		case 2: 
			{
			if(value>1)
				palTogglePad(GPIOD, GPIOD_LED5);
			else
				value?palClearPad(GPIOD, GPIOD_LED5):palSetPad(GPIOD, GPIOD_LED5);
			break;
			}
		case 3: 
			{
			if(value>1)
				palTogglePad(GPIOD, GPIOD_LED7);
			else
				value?palClearPad(GPIOD, GPIOD_LED7):palSetPad(GPIOD, GPIOD_LED7);
			break;
			}
		default:
			for(int i=0; i<4; i++) {
				set_led(i, value);
			}
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
 * \param value 0 (off), 1 (on) otherwise change the state
 */
void set_front_led(unsigned int value)
{
	if(value>1)
		palTogglePad(GPIOD, GPIOD_LED_FRONT);
	else
		value?palSetPad(GPIOD, GPIOD_LED_FRONT):palClearPad(GPIOD, GPIOD_LED_FRONT);
}

/*! \brief turn off the 8 LEDs
 *
 * The e-puck has 8 green LEDs. This function turn all off.
 * \warning this function doesn't turn off "body LED" and "front LED".
 */
void clear_leds(void)
{
	for(int i=0; i<4; i++) {
		set_led(i, 0);
	}
}
