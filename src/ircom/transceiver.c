#include <stdio.h>
#include <stdlib.h>
#include "ch.h"
#include "chprintf.h"
#include "hal.h"
#include "usbcfg.h"
#include "ircom/ircom.h"
#include "ircom/ircomTools.h"
#include "ircom/ircomReceive.h"
#include "ircom/ircomMessages.h"
#include "ircom/ircomSend.h"
#include "leds.h"

static THD_WORKING_AREA(transceiver_thd_wa, 256);
static THD_FUNCTION(transceiver_thd, arg) {
	(void) arg;
	chRegSetThreadName(__FUNCTION__);
	systime_t time;
	IrcomMessage imsg;				
	uint8_t txData = 0;

	while(1) {
		time = chVTGetSystemTime();

		/*// First way to get proximity sampled values if needed.
		messagebus_topic_wait(prox_topic, &prox_values, sizeof(prox_values));
		if (SDU1.config->usbp->state == USB_ACTIVE) { // Skip printing if port not opened.
			// Sensors info print: each line contains data related to a single sensor.
			for (uint8_t i = 0; i < sizeof(prox_values.ambient)/sizeof(prox_values.ambient[0]); i++) {
				chprintf((BaseSequentialStream *)&SDU1, "%4d,", prox_values.ambient[i]);
				chprintf((BaseSequentialStream *)&SDU1, "%4d,", prox_values.reflected[i]);
				chprintf((BaseSequentialStream *)&SDU1, "%4d", prox_values.delta[i]);
				chprintf((BaseSequentialStream *)&SDU1, "\r\n");
			}
			chprintf((BaseSequentialStream *)&SDU1, "\r\n");
		}
		*/
		/*
		// Second way to get proximity sampled values if needed.
		if (SDU1.config->usbp->state == USB_ACTIVE) { // Skip printing if port not opened.
			// Sensors info print: each line contains data related to a single sensor.
			for (uint8_t i = 0; i < 8; i++) {
				chprintf((BaseSequentialStream *)&SDU1, "%4d,", ircom_get_ambient_light(i));
				chprintf((BaseSequentialStream *)&SDU1, "%4d", ircom_get_prox(i));
				chprintf((BaseSequentialStream *)&SDU1, "\r\n");
			}
			chprintf((BaseSequentialStream *)&SDU1, "\r\n");
		}
		*/

		// Look for incoming messages and print related content if something is received.
		ircomPopMessage(&imsg);
		if(imsg.error == 0) {
			set_body_led(2);
			int val = (int) imsg.value;
			if (SDU1.config->usbp->state == USB_ACTIVE) { // Skip printing if port not opened.
				chprintf((BaseSequentialStream *)&SDU1, "Receive successful : %d  - distance=%f \t direction=%f\r\n", val, (double)imsg.distance, (double)imsg.direction);
			}
		} else if (imsg.error > 0) {
			if (SDU1.config->usbp->state == USB_ACTIVE) { // Skip printing if port not opened.
				chprintf((BaseSequentialStream *)&SDU1, "Receive failed\r\n");
			}
		}
		// else imsg.error == -1 -> no message available in the queue

		// Send fake data.
		ircomSend(txData);
		while (ircomSendDone() == 0);
		if(txData == 255) {
			txData = 0;
		} else {
			txData++;
		}

		chThdSleepUntilWindowed(time, time + MS2ST(100)); // Refresh @ 10 Hz.

	}

}

void startTransceiver(void) {

    // Initialize ircom and start listening.
    ircomStart();
    ircomEnableContinuousListening();
    ircomListen();

    chThdCreateStatic(transceiver_thd_wa, sizeof(transceiver_thd_wa), NORMALPRIO, transceiver_thd, NULL);

}

