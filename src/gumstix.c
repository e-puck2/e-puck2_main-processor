#include <hal.h>
#include "gumstix.h"

#include <string.h>
#include "i2c_ms_slave.h"
#include "memstreams.h"
#include "leds.h"
#include "sensors/proximity.h"
#include "motors.h"
#include "selector.h"
#include "audio/microphone.h"
#include "audio/play_melody.h"
#include "ir_remote.h"
#include "button.h"
#include "epuck1x/motor_led/advance_one_timer/e_led.h"

#define BUFF_SIZE 32

uint8_t leds_outer = 0;
uint8_t leds_front_body = 0;
int16_t motor_left = 0;
int16_t motor_right = 0;
uint8_t test_err = 0;

/**
 * I2C slave test routine.
 *
 * To use: Add file to a project, call startComms() with the address of a serial stream
 *
 * There are two different responses:
 *  a) A read-only transaction - returns the "Initial Reply" message
 *  b) A write then read transaction - calls a message processor and returns the generated reply.
 *          Stretches clock until reply available.
 */

#define slaveI2cPort    I2CD1
#define slaveI2Caddress  0x3E	// 8 bits address (0x1F is the 7-bit address).

static BSEMAPHORE_DECL(i2c_rx_ready, true);
binary_semaphore_t *sem = &i2c_rx_ready;

I2CSlaveMsgCB messageProcessor, clearAfterSend, catchError, addrMatchCB;

static const I2CConfig slaveI2Cconfig = {
		OPMODE_I2C,
    //400000,
	//FAST_DUTY_CYCLE_2
	100000,
	STD_DUTY_CYCLE
};

uint8_t i2c_rx_buff[BUFF_SIZE];
uint8_t work_buffer[BUFF_SIZE];
uint8_t i2c_tx_buff[BUFF_SIZE] = {0};

BaseSequentialStream *chp = NULL;           // Used for serial logging

// Handler when something sent to us
const I2CSlaveMsg i2c_rx_msg = {
	BUFF_SIZE,						/* max sizeof received msg body */
	i2c_rx_buff,			/* body of received msg */
	NULL,					/* do nothing on address match */
	messageProcessor,		/* Routine to process received messages */
	catchError				/* Error hook */
};

// Response to received messages
I2CSlaveMsg i2c_tx_msg = { 	/* this is in RAM so size may be updated */
	BUFF_SIZE,						/* filled in with the length of the message to send */
	i2c_tx_buff,			/* Response message */
	NULL,					/* do nothing special on address match */
	NULL, //clearAfterSend,       /* Clear receive buffer once replied */
	catchError				/* Error hook */
};


/**
 * Track I2C errors
 */
uint8_t gotI2cError = 0;
uint32_t lastI2cErrorFlags = 0;

// Called from ISR to log error
void noteI2cError(uint32_t flags)
{
  lastI2cErrorFlags = flags;
  gotI2cError = 1;
}


/**
 * Generic error handler
 *
 * Called in interrupt context, so need to watch what we do
 */
void catchError(I2CDriver *i2cp)
{
  noteI2cError(i2cp->errors);
}

/**
 * Callback on address match.
 */
void addrMatchCB(I2CDriver *i2cp)
{
	if((i2cp->i2c->SR2 & I2C_SR2_TRA) > 0) {
		set_body_led(0);
		set_front_led(1);
	} else {
		set_body_led(1);
		set_front_led(0);
	}
}


/**
 *  Message processor - looks at received message, determines reply as quickly as possible
 *
 *  Responds with the value of the messageCounter (in hex), followed by the received message in [..]
 *
 *  Note: Called in interrupt context, so need to be quick!
 */
void messageProcessor(I2CDriver *i2cp) {
	uint8_t len = i2cSlaveBytes(i2cp);

	switch(i2c_rx_buff[0]) {
		case 0: // Outer leds
			if(len == 1) { // Read request
				i2c_tx_buff[0] = leds_outer;
				i2c_tx_msg.size = 1;
				i2cSlaveReplyI(&I2CD1, &i2c_tx_msg);
			} else { // Write request
				leds_outer = i2c_rx_buff[1];
				memcpy(work_buffer, i2c_rx_buff, BUFF_SIZE);
			    chSysLockFromISR();
			    chBSemSignalI(sem);
			    chSysUnlockFromISR();
			}
			break;
		case 1: // Body, front leds
			if(len == 1) { // Read request
				i2c_tx_buff[0] = leds_front_body;
				i2c_tx_msg.size = 1;
				i2cSlaveReplyI(&I2CD1, &i2c_tx_msg);
			} else { // Write request
				leds_front_body = i2c_rx_buff[1];
				memcpy(work_buffer, i2c_rx_buff, BUFF_SIZE);
			    chSysLockFromISR();
			    chBSemSignalI(sem);
			    chSysUnlockFromISR();
			}
			break;
		case 2: // Left speed
			if(len == 1) { // Read request
				i2c_tx_buff[0] = motor_left&0xFF;
				i2c_tx_buff[1] = (motor_left>>8)&0xFF;
				i2c_tx_msg.size = 2;
				i2cSlaveReplyI(&I2CD1, &i2c_tx_msg);
			} else { // Write request
				motor_left = i2c_rx_buff[1]|(i2c_rx_buff[2]<<8);
				if(motor_left > 1000) {
					motor_left = 1000;
				} else if(motor_left < -1000) {
					motor_left = -1000;
				}
				memcpy(work_buffer, i2c_rx_buff, BUFF_SIZE);
			    chSysLockFromISR();
			    chBSemSignalI(sem);
			    chSysUnlockFromISR();
			}
			break;
		case 3: // Right speed
			if(len == 1) { // Read request
				i2c_tx_buff[0] = motor_right&0xFF;
				i2c_tx_buff[1] = (motor_right>>8)&0xFF;
				i2c_tx_msg.size = 2;
				i2cSlaveReplyI(&I2CD1, &i2c_tx_msg);
			} else { // Write request
				motor_right = i2c_rx_buff[1]|(i2c_rx_buff[2]<<8);
				if(motor_right > 1000) {
					motor_right = 1000;
				} else if(motor_right < -1000) {
					motor_right = -1000;
				}
				memcpy(work_buffer, i2c_rx_buff, BUFF_SIZE);
			    chSysLockFromISR();
			    chBSemSignalI(sem);
			    chSysUnlockFromISR();
			}
			break;
		case 4: // Left steps
			if(len == 1) { // Read request
				int16_t steps = left_motor_get_pos();
				i2c_tx_buff[0] = steps&0xFF;
				i2c_tx_buff[1] = (steps>>8)&0xFF;
				i2c_tx_msg.size = 2;
				i2cSlaveReplyI(&I2CD1, &i2c_tx_msg);
			}
			break;
		case 5: // Right steps
			if(len == 1) { // Read request
				int16_t steps = right_motor_get_pos();
				i2c_tx_buff[0] = steps&0xFF;
				i2c_tx_buff[1] = (steps>>8)&0xFF;
				i2c_tx_msg.size = 2;
				i2cSlaveReplyI(&I2CD1, &i2c_tx_msg);
			}
			break;
		case 6: // IR sensors control => not implemented
			break;
		case 7: // Prox0
			if(len == 1) { // Read request
				uint16_t prox = get_prox(0);
				i2c_tx_buff[0] = prox&0xFF;
				i2c_tx_buff[1] = (prox>>8)&0xFF;
				i2c_tx_msg.size = 2;
				i2cSlaveReplyI(&I2CD1, &i2c_tx_msg);
			}
			break;
		case 8: // Prox1
			if(len == 1) { // Read request
				uint16_t prox = get_prox(1);
				i2c_tx_buff[0] = prox&0xFF;
				i2c_tx_buff[1] = (prox>>8)&0xFF;
				i2c_tx_msg.size = 2;
				i2cSlaveReplyI(&I2CD1, &i2c_tx_msg);
			}
			break;
		case 9: // Prox2
			if(len == 1) { // Read request
				uint16_t prox = get_prox(2);
				i2c_tx_buff[0] = prox&0xFF;
				i2c_tx_buff[1] = (prox>>8)&0xFF;
				i2c_tx_msg.size = 2;
				i2cSlaveReplyI(&I2CD1, &i2c_tx_msg);
			}
			break;
		case 10: // Prox3
			if(len == 1) { // Read request
				uint16_t prox = get_prox(3);
				i2c_tx_buff[0] = prox&0xFF;
				i2c_tx_buff[1] = (prox>>8)&0xFF;
				i2c_tx_msg.size = 2;
				i2cSlaveReplyI(&I2CD1, &i2c_tx_msg);
			}
			break;
		case 11: // Prox4
			if(len == 1) { // Read request
				uint16_t prox = get_prox(4);
				i2c_tx_buff[0] = prox&0xFF;
				i2c_tx_buff[1] = (prox>>8)&0xFF;
				i2c_tx_msg.size = 2;
				i2cSlaveReplyI(&I2CD1, &i2c_tx_msg);
			}
			break;
		case 12: // Prox5
			if(len == 1) { // Read request
				uint16_t prox = get_prox(5);
				i2c_tx_buff[0] = prox&0xFF;
				i2c_tx_buff[1] = (prox>>8)&0xFF;
				i2c_tx_msg.size = 2;
				i2cSlaveReplyI(&I2CD1, &i2c_tx_msg);
			}
			break;
		case 13: // Prox6
			if(len == 1) { // Read request
				uint16_t prox = get_prox(6);
				i2c_tx_buff[0] = prox&0xFF;
				i2c_tx_buff[1] = (prox>>8)&0xFF;
				i2c_tx_msg.size = 2;
				i2cSlaveReplyI(&I2CD1, &i2c_tx_msg);
			}
			break;
		case 14: // Prox7
			if(len == 1) { // Read request
				uint16_t prox = get_prox(7);
				i2c_tx_buff[0] = prox&0xFF;
				i2c_tx_buff[1] = (prox>>8)&0xFF;
				i2c_tx_msg.size = 2;
				i2cSlaveReplyI(&I2CD1, &i2c_tx_msg);
			}
			break;
		case 15: // Ambient0
			if(len == 1) { // Read request
				uint16_t prox = get_ambient_light(0);
				i2c_tx_buff[0] = prox&0xFF;
				i2c_tx_buff[1] = (prox>>8)&0xFF;
				i2c_tx_msg.size = 2;
				i2cSlaveReplyI(&I2CD1, &i2c_tx_msg);
			}
			break;
		case 16: // Ambient1
			if(len == 1) { // Read request
				uint16_t prox = get_ambient_light(1);
				i2c_tx_buff[0] = prox&0xFF;
				i2c_tx_buff[1] = (prox>>8)&0xFF;
				i2c_tx_msg.size = 2;
				i2cSlaveReplyI(&I2CD1, &i2c_tx_msg);
			}
			break;
		case 17: // Ambient2
			if(len == 1) { // Read request
				uint16_t prox = get_ambient_light(2);
				i2c_tx_buff[0] = prox&0xFF;
				i2c_tx_buff[1] = (prox>>8)&0xFF;
				i2c_tx_msg.size = 2;
				i2cSlaveReplyI(&I2CD1, &i2c_tx_msg);
			}
			break;
		case 18: // Ambient3
			if(len == 1) { // Read request
				uint16_t prox = get_ambient_light(3);
				i2c_tx_buff[0] = prox&0xFF;
				i2c_tx_buff[1] = (prox>>8)&0xFF;
				i2c_tx_msg.size = 2;
				i2cSlaveReplyI(&I2CD1, &i2c_tx_msg);
			}
			break;
		case 19: // Ambient4
			if(len == 1) { // Read request
				uint16_t prox = get_ambient_light(4);
				i2c_tx_buff[0] = prox&0xFF;
				i2c_tx_buff[1] = (prox>>8)&0xFF;
				i2c_tx_msg.size = 2;
				i2cSlaveReplyI(&I2CD1, &i2c_tx_msg);
			}
			break;
		case 20: // Ambient5
			if(len == 1) { // Read request
				uint16_t prox = get_ambient_light(5);
				i2c_tx_buff[0] = prox&0xFF;
				i2c_tx_buff[1] = (prox>>8)&0xFF;
				i2c_tx_msg.size = 2;
				i2cSlaveReplyI(&I2CD1, &i2c_tx_msg);
			}
			break;
		case 21: // Ambient6
			if(len == 1) { // Read request
				uint16_t prox = get_ambient_light(6);
				i2c_tx_buff[0] = prox&0xFF;
				i2c_tx_buff[1] = (prox>>8)&0xFF;
				i2c_tx_msg.size = 2;
				i2cSlaveReplyI(&I2CD1, &i2c_tx_msg);
			}
			break;
		case 22: // Ambient7
			if(len == 1) { // Read request
				uint16_t prox = get_ambient_light(7);
				i2c_tx_buff[0] = prox&0xFF;
				i2c_tx_buff[1] = (prox>>8)&0xFF;
				i2c_tx_msg.size = 2;
				i2cSlaveReplyI(&I2CD1, &i2c_tx_msg);
			}
			break;
		case 254: // Testing purpose
			if(len == 1) { // Read request
				i2c_tx_buff[0] = test_err;
				i2c_tx_msg.size = 1;
				i2cSlaveReplyI(&I2CD1, &i2c_tx_msg);
				if(test_err == 255) {
					test_err = 0;
				} else {
					test_err++;
				}
			}
			break;
		default:
			break;
	}

}

/**
 * Callback after sending of response complete - restores default reply in case polled
 */
void clearAfterSend(I2CDriver *i2cp)
{
	(void)i2cp;
	//i2c_tx_msg.size = 0;
}


/**
 * Start the I2C Slave port to accept comms from master CPU
 *
 * We then go into a loop checking for errors, and never return
 */
void start_gumstix_comm(BaseSequentialStream *serport)
{
  chp = serport;

  i2cStart(&slaveI2cPort, &slaveI2Cconfig);
#if HAL_USE_I2C1_MS_SLAVE
  slaveI2cPort.slaveTimeout = MS2ST(100);       // Time for complete message.
#endif

  i2cSlaveConfigure(&slaveI2cPort, &i2c_rx_msg, &i2c_tx_msg); // Initial reply.

  // Enable match address after everything else set up.
  i2cMatchAddress(&slaveI2cPort, slaveI2Caddress/2);
//  i2cMatchAddress(&slaveI2cPort, 0);  /* "all call" */


  // Prepare the reply for a "write+read" transaction.
  i2c_tx_msg.size = 1;
  i2cSlaveReplyI(&I2CD1, &i2c_tx_msg);

  while(true) {
    chBSemWait(&i2c_rx_ready);

    if (gotI2cError) {
    	gotI2cError = 0;
        //chprintf(chp, "I2cError: %04x\r\n", lastI2cErrorFlags);
    } else {
    	switch(work_buffer[0]) {
    		case 0: // outer leds
				e_set_led(0, leds_outer&0x01);
				e_set_led(1, leds_outer&0x02);
				e_set_led(2, leds_outer&0x04);
				e_set_led(3, leds_outer&0x08);
				e_set_led(4, leds_outer&0x10);
				e_set_led(5, leds_outer&0x20);
				e_set_led(6, leds_outer&0x40);
				e_set_led(7, leds_outer&0x80);
    			break;
    		case 1: // body, front leds
				set_front_led(leds_front_body&0x01);
				set_body_led(leds_front_body&0x02);
    			break;
    		case 2: // left speed
    			left_motor_set_speed(motor_left);
    			break;
    		case 3: // right speed
				right_motor_set_speed(motor_right);
    			break;
    	}
    	//chprintf(chp, "SR2: %04x\r\n", (&slaveI2cPort)->i2c->SR2);
    }

  }

}
