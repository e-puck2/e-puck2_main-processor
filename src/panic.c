#include <ch.h>
#include <hal.h>
#include "leds.h"

void panic_handler(const char *reason)
{
    (void)reason;
	clear_leds();
	set_body_led(0);
	set_front_led(0);
	set_led(ALL_LEDS, 1);
	
    while (true) {

    }
}
