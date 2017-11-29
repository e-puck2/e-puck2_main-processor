#include <ch.h>
#include <hal.h>
#include "ir_remote.h"
#include "exti.h"
#include "motors.h"

#define DEFAULT_SPEED 600

uint8_t checkGlitch = 0;
uint8_t irWaitCmd = 0;
static uint8_t address_temp = 0;
static uint8_t data_temp = 0;
static uint8_t toggle_temp = 0;
uint8_t address = 0;
uint8_t data_ir = 0;
uint8_t toggle = 2;
uint8_t irCommand = 0;
static BSEMAPHORE_DECL(command_received, true);

uint8_t ir_remote_get_toggle(void) {
	return toggle;
}

uint8_t ir_remote_get_address(void) {
	return address;
}

uint8_t ir_remote_get_data(void) {
	return data_ir;
}

static void gpt11cb(GPTDriver *gptp) {
	(void)gptp;

	static int i = -1;

	if(checkGlitch) { // Checking if the interrupt was raised by a glitch.
		if(palReadPad(GPIOD, GPIOD_REMOTE)) { // This is a glitch.
			irWaitCmd = 1;
			i = -1;
		} else { // Not a glitch => real command received.
			checkGlitch = 0;

			// Activate the IR Receiver timer with a 2.1 ms cycle value to reach the second start bit.
			// The resolution of the timer is 0.032 ms so:
			// 2.1 / 0.032 = 64 to be set in the output compare register (=> 2.048 ms)
			// but we already wait 0.416 ms (=> 13), so 64-13=51 (=> 1.632 ms)
			chSysLockFromISR();
			gptStartOneShotI(&GPTD11, 51);
			chSysUnlockFromISR();
		}
	} else {
		if (i == -1) {
			if(palReadPad(GPIOD, GPIOD_REMOTE)) { // Double check => if high it is only noise.
				irWaitCmd = 1;
				i = -1;
			} else { // Start bit confirmed.
				// Cycle value is 0.889 ms to go to toggle bit so:
				// 0.889 / 0.032 = 28 (=> 0.896)
				chSysLockFromISR();
				gptStartOneShotI(&GPTD11, 28);
				chSysUnlockFromISR();
				toggle_temp = address_temp = data_temp = 0;
				i=0;
			}
		} else if (i == 1)	{ // Toggle bit read and change timer period.
			toggle_temp = palReadPad(GPIOD, GPIOD_REMOTE); // Read the toggle bit.
			// Cycle value is 1.778 ms to go to first address bit so:
			// 1.778 / 0.032 = 55 (=> 1.760 ms)
			// All the subsequent bits are read with 55 (=> 1.760 ms) cadence.
			chSysLockFromISR();
			gptStartOneShotI(&GPTD11, 55);
			chSysUnlockFromISR();
		} else if ((i > 1) && (i < 7)) { // Read address.
			chSysLockFromISR();
			gptStartOneShotI(&GPTD11, 55);
			chSysUnlockFromISR();
			unsigned char temp = palReadPad(GPIOD, GPIOD_REMOTE);
			temp <<= 6-i;
			address_temp += temp;
		} else if ((i > 6) && (i < 13 )) { // Read command.
			chSysLockFromISR();
			gptStartOneShotI(&GPTD11, 55);
			chSysUnlockFromISR();
			unsigned char temp = palReadPad(GPIOD, GPIOD_REMOTE);
			temp <<= 6+6-i;
			data_temp += temp;
		} else if (i == 13) { // Terminate.
			irWaitCmd = 1;
			i = -1;
			toggle = toggle_temp;
			address = address_temp;
			data_ir = data_temp;
		    /* Signal the remote motion thread that a new command is received. */
			binary_semaphore_t *sem = &command_received;
		    chSysLockFromISR();
		    chBSemSignalI(sem);
		    chSysUnlockFromISR();
		}
	}

	if(i!=-1) {
		i++;
	}
}

static const GPTConfig gpt11cfg = {
	31250,		/* 31.25kHz timer clock.*/
	gpt11cb,	/* Timer callback.*/
	0,
	0
};

static THD_FUNCTION(remote_motion_thd, arg)
{
    (void) arg;
    chRegSetThreadName(__FUNCTION__);
    systime_t time;

    while(1) {

    	time = chVTGetSystemTime();

    	chBSemWait(&command_received);

		irCommand = ir_remote_get_data();
		switch(irCommand) {
			// Sometimes there are two cases for the same command because two different
			// remote controls are used; one of this do not contain "numbers".
			case 5: // stop motors
			case 51:
				left_motor_set_speed(0);
				right_motor_set_speed(0);
				break;

			case 2: // Both mnotors forward.
			case 31:
				left_motor_set_speed(DEFAULT_SPEED);
				right_motor_set_speed(DEFAULT_SPEED);
				break;

			case 8: // Both mnotors backward.
			case 30:
				left_motor_set_speed(-DEFAULT_SPEED);
				right_motor_set_speed(-DEFAULT_SPEED);
				break;

			case 6: // Both mnotors right.
			case 47:
				left_motor_set_speed(DEFAULT_SPEED);
				right_motor_set_speed(-DEFAULT_SPEED);
				break;

			case 4: // Both mnotors left.
			case 46:
				left_motor_set_speed(-DEFAULT_SPEED);
				right_motor_set_speed(DEFAULT_SPEED);
				break;

			case 3: // Left motor forward.
				left_motor_set_speed(DEFAULT_SPEED);
				break;

			case 1: // Right motor forward.
				right_motor_set_speed(DEFAULT_SPEED);
				break;

			case 9: // Left motor backward.
				left_motor_set_speed(-DEFAULT_SPEED);
				break;

			case 7: // Right motor backward.
				right_motor_set_speed(-DEFAULT_SPEED);
				break;

		}

		chThdSleepUntilWindowed(time, time + MS2ST(200)); // Receive commands at most @ 5 Hz.
    }

}

static THD_FUNCTION(remote_cmd_recv_thd, arg)
{
    (void) arg;
    chRegSetThreadName(__FUNCTION__);

    event_listener_t remote_int;
    /* Starts waiting for the external interrupts. */
    chEvtRegisterMaskWithFlags(&exti_events, &remote_int,
                               (eventmask_t)EXTI_EVENT_IR_REMOTE_INT,
                               (eventflags_t)EXTI_EVENT_IR_REMOTE_INT);

    while(1) {
    	/* Wait for a command to come. */
    	chEvtWaitAny(EXTI_EVENT_IR_REMOTE_INT);

    	if(irWaitCmd == 1) {
			irWaitCmd = 0;
			// Check the pin change isn't due to a glitch; to check this verify that
			// the pin remain low for at least 400 us (the giltches last about 200 us)
			// 0.4 / 0.032 = 13 => 0.416 ms
			checkGlitch = 1;
			gptStartOneShot(&GPTD11, 13);
			toggle_temp = address_temp = data_temp = 0;
    	}

    }
}

void ir_remote_start(void) {
	irWaitCmd = 1;
	gptStart(&GPTD11, &gpt11cfg);

	static THD_WORKING_AREA(remote_motion_thd_wa, 512);
	chThdCreateStatic(remote_motion_thd_wa, sizeof(remote_motion_thd_wa), NORMALPRIO, remote_motion_thd, NULL);

	static THD_WORKING_AREA(remote_cmd_recv_thd_wa, 128);
	chThdCreateStatic(remote_cmd_recv_thd_wa, sizeof(remote_cmd_recv_thd_wa), NORMALPRIO, remote_cmd_recv_thd, NULL);
}
