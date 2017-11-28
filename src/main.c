#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "ch.h"
#include "chprintf.h"
#include "hal.h"
#include "shell.h"

#include "aseba_vm/aseba_node.h"
#include "aseba_vm/skel_user.h"
#include "aseba_vm/aseba_can_interface.h"
#include "aseba_vm/aseba_bridge.h"
#include "audio/audio_thread.h"
#include "camera/po8030.h"
#include "sensors/battery_level.h"
#include "sensors/proximity.h"
#include "sensors/VL53L0X/VL53L0X.h"
#include "cmd.h"
#include "config_flash_storage.h"
#include "i2c_bus.h"
#include "leds.h"
#include "main.h"
#include "memory_protection.h"
#include "motors.h"
#include "usbcfg.h"
#include "utility.h"

#define SHELL_WA_SIZE   THD_WORKING_AREA_SIZE(2048)

messagebus_t bus;
MUTEX_DECL(bus_lock);
CONDVAR_DECL(bus_condvar);

parameter_namespace_t parameter_root, aseba_ns;

static THD_WORKING_AREA(selector_thd_wa, 2048);

static bool load_config(void)
{
    extern uint32_t _config_start;

    return config_load(&parameter_root, &_config_start);
}

static THD_FUNCTION(selector_thd, arg)
{
    (void) arg;
    chRegSetThreadName(__FUNCTION__);

    uint8_t stop_loop = 0;
    systime_t time;

    messagebus_topic_t *prox_topic = messagebus_find_topic_blocking(&bus, "/proximity");
    proximity_msg_t prox_values;

    uint8_t tof_measuring = 0;

    while(stop_loop == 0) {
    	time = chVTGetSystemTime();

		switch(get_selector()) {
			case 0: // Aseba.
				aseba_vm_start();
				stop_loop = 1;
				break;

			case 1: // Shell.
				shell_start();
				stop_loop = 1;
				break;

			case 2: // Read proximity sensors.
				messagebus_topic_wait(prox_topic, &prox_values, sizeof(prox_values));

				if (SDU1.config->usbp->state != USB_ACTIVE) { // Skip printing if port not opened.
					continue;
				}

				// Sensors info print: each line contains data related to a single sensor.
		        for (uint8_t i = 0; i < sizeof(prox_values.ambient)/sizeof(prox_values.ambient[0]); i++) {
		        //for (uint8_t i = 0; i < PROXIMITY_NB_CHANNELS; i++) {
		        	chprintf((BaseSequentialStream *)&SDU1, "%4d,", prox_values.ambient[i]);
		        	chprintf((BaseSequentialStream *)&SDU1, "%4d,", prox_values.reflected[i]);
		        	chprintf((BaseSequentialStream *)&SDU1, "%4d", prox_values.delta[i]);
		        	chprintf((BaseSequentialStream *)&SDU1, "\r\n");
		        }
		        chprintf((BaseSequentialStream *)&SDU1, "\r\n");

				chThdSleepUntilWindowed(time, time + MS2ST(100)); // Refresh @ 10 Hz.
				break;

			case 3: // Distance sensor reading.
				if(tof_measuring == 0) {
					tof_measuring = 1;
					VL53L0X_init_demo();
				}
				chThdSleepUntilWindowed(time, time + MS2ST(100)); // Refresh @ 10 Hz.
				break;

			case 4:
				break;

			case 5:
				break;

			case 6:
				break;

			case 7:
				break;

			case 8:
				break;

			case 9:
				break;

			case 10:
				break;

			case 11:
				break;

			case 12:
				break;

			case 13:
				break;

			case 14:
				break;

			case 15:
				break;
		}
    }
}

int main(void)
{

    halInit();
    chSysInit();
    mpu_init();

    /** Inits the Inter Process Communication bus. */
    messagebus_init(&bus, &bus_lock, &bus_condvar);

    parameter_namespace_declare(&parameter_root, NULL, NULL);

    // Init the peripherals.
	clear_leds();
	set_body_led(0);
	set_front_led(0);
	usb_start();
	i2c_start();
	dcmi_start();
	po8030_start();
	motors_init();
	proximity_start();
	battery_level_start();
	dac_start();

	// Initialise Aseba system, declaring parameters
    parameter_namespace_declare(&aseba_ns, &parameter_root, "aseba");
    aseba_declare_parameters(&aseba_ns);

    /* Load parameter tree from flash. */
    load_config();

    /* Start AsebaCAN. Must be after config was loaded because the CAN id
     * cannot be changed at runtime. */
    aseba_vm_init();
    aseba_can_start(&vmState);

    chThdCreateStatic(selector_thd_wa, sizeof(selector_thd_wa), NORMALPRIO, selector_thd, NULL);

    /* Infinite loop. */
    while (1) {
        chThdSleepMilliseconds(1000);
    }
}

#define STACK_CHK_GUARD 0xe2dee396
uintptr_t __stack_chk_guard = STACK_CHK_GUARD;

void __stack_chk_fail(void)
{
    chSysHalt("Stack smashing detected");
}
