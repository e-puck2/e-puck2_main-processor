#ifndef LEDS_H
#define LEDS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <hal.h>

#define RGB_MAX_INTENSITY 100	//percentage

//List of the RED LEDs present on the e-puck 2
typedef enum {
	ALL_LEDS = -1,
	LED1,
	LED3,
	LED5,
	LED7,
	NUM_LED,
} led_name_t;

//List of the RGB LEDs present on the e-puck 2
typedef enum {
	ALL_RGB_LEDS = -1,
	LED2,
	LED4,
	LED6,
	LED8,
	NUM_RGB_LED,
} rgb_led_name_t;

//List of the LEDs present on each RGB LED
typedef enum {
	RED_LED,
	GREEN_LED,
	BLUE_LED,
	NUM_COLOR_LED,
} color_led_name_t;

void set_led(led_name_t led_number, unsigned int value);
void clear_leds(void);

void set_body_led(unsigned int value);
void set_front_led(unsigned int value);

void get_all_rgb_state(uint8_t* values);
void toggle_rgb_led(rgb_led_name_t led_number, color_led_name_t led, uint8_t intensity);
void set_rgb_led(rgb_led_name_t led_number, uint8_t red_val, uint8_t green_val, uint8_t blue_val);


#ifdef __cplusplus
}
#endif

#endif
