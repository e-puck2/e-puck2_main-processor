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
#include "audio/microphone.h"
#include "camera/po8030.h"
#include "epuck1x/Asercom.h"
#include "epuck1x/Asercom2.h"
#include "sensors/battery_level.h"
#include "sensors/imu.h"
#include "sensors/proximity.h"
#include "sensors/VL53L0X/VL53L0X.h"
#include "cmd.h"
#include "config_flash_storage.h"
#include "exti.h"
#include "i2c_bus.h"
#include "ir_remote.h"
#include "leds.h"
#include "main.h"
#include "memory_protection.h"
#include "motors.h"
#include "selector.h"
#include "spi_comm.h"
#include "usbcfg.h"
#include "communication.h"

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

    messagebus_topic_t *imu_topic = messagebus_find_topic_blocking(&bus, "/imu");
    imu_msg_t imu_values;

    uint8_t toEsp32 = 'c', fromEsp32 = 0;
    	int16_t len = 0;

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

			case 3: // Asercom protocol.
				run_asercom();
				stop_loop = 1;
				break;

			case 4: // Read IMU raw sensors values.
		    	messagebus_topic_wait(imu_topic, &imu_values, sizeof(imu_values));
		    	if (SDU1.config->usbp->state != USB_ACTIVE) { // Skip printing if port not opened.
		    		continue;
		    	}
		    	chprintf((BaseSequentialStream *)&SDU1, "%Ax=%-7d Ay=%-7d Az=%-7d Gx=%-7d Gy=%-7d Gz=%-7d (%x)\r\n", imu_values.acc_raw[0], imu_values.acc_raw[1], imu_values.acc_raw[2], imu_values.gyro_raw[0], imu_values.gyro_raw[1], imu_values.gyro_raw[2], imu_values.status);
		    	chThdSleepUntilWindowed(time, time + MS2ST(100)); // Refresh @ 10 Hz.
				break;

			case 5: // Distance sensor reading.
				chprintf((BaseSequentialStream *)&SDU1, "range=%d mm\r\n", VL53L0X_get_dist_mm());
				chThdSleepUntilWindowed(time, time + MS2ST(100)); // Refresh @ 10 Hz.
				break;

			case 6: // ESP32 UART communication test.
				sdPut(&SD3, toEsp32);
				len = sdReadTimeout(&SD3, &fromEsp32, 1, MS2ST(50));
				if(len > 0) {
					sdPut(&SDU1, fromEsp32);
				}
				chThdSleepUntilWindowed(time, time + MS2ST(10)); // Refresh @ 100 Hz.
				break;

			case 7:
				communication_start((BaseSequentialStream *)&SDU1);
				 while (1) {
			        chThdSleepMilliseconds(1000);
			    }
				break;

			case 8: // Asercom protocol v2.
				run_asercom2();
				stop_loop = 1;
				break;

			case 9:
				break;

			case 10: // Gumstix extension.
				//i2c_stop();
				run_asercom();
				stop_loop = 1;
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
	exti_start();
	imu_start();
	ir_remote_start();
	spi_comm_start();
	static SerialConfig ser_cfg = {
	    115200,
	    0,
	    0,
	    0,
	};
	sdStart(&SD3, &ser_cfg); // UART3.
	mic_start();
	static uint8_t sd_scratchpad[512]; // Working area for SDC driver.
	static const SDCConfig sdccfg = { //  SDIO configuration.
	  sd_scratchpad,
	  SDC_MODE_1BIT // Use 1-bit mode instead of 4-bit to avoid conflicts with the microphones.
	};
	sdcStart(&SDCD1, &sdccfg);

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
