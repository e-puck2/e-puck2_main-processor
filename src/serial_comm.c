#include <stdlib.h>
#include <string.h>
#include <ch.h>
#include <hal.h>
#include "serial_comm.h"
#include "chprintf.h"
#include "usbcfg.h"

eventflags_t flags;

static THD_WORKING_AREA(serial_listener_thd_wa, 256);
static THD_FUNCTION(serial_listener_thd, arg) {
	(void) arg;

	event_listener_t serial_listener;

	/* Registering on the serial driver as event 1, interested in
     error flags and data-available only, other flags will not wakeup
     the thread.*/
	chEvtRegisterMaskWithFlags(&SD3.event, &serial_listener, EVENT_MASK(0), SD_OVERRUN_ERROR | SD_PARITY_ERROR | SD_FRAMING_ERROR | SD_NOISE_ERROR);

	while (true) {
		/* Waiting for any of the events we're registered on.*/
		chEvtWaitAny(EVENT_MASK(0));
		flags = chEvtGetAndClearFlags(&serial_listener);

		if (flags & (SD_OVERRUN_ERROR)) {
			chprintf((BaseSequentialStream *)&SDU1, "overrun err\r\n");
		}
		if (flags & (SD_PARITY_ERROR)) {
			chprintf((BaseSequentialStream *)&SDU1, "parity err\r\n");
		}
		if (flags & (SD_FRAMING_ERROR)) {
			chprintf((BaseSequentialStream *)&SDU1, "framing err\r\n");
		}
		if (flags & (SD_NOISE_ERROR)) {
			chprintf((BaseSequentialStream *)&SDU1, "noise err\r\n");
		}

	}
}

void serial_start(void) {
	static SerialConfig ser_cfg = {
	    115200,
	    0, // CR1
	    0, // CR2
	    0, // CR3
	};
	sdStart(&SD3, &ser_cfg); // UART3.

	chThdCreateStatic(serial_listener_thd_wa, sizeof(serial_listener_thd_wa), NORMALPRIO, serial_listener_thd, NULL);
}

uint32_t serial_get_last_errors(void) {
	return flags;
}

void serial_clear_last_errors(void) {
	flags = 0;
}
