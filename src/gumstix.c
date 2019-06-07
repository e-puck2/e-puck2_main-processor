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

#define ACTUATORS_SIZE (19+1) // Data + checksum.
#define SENSORS_SIZE (46+1) // Data + checksum.

static BSEMAPHORE_DECL(i2c_rx_ready, true);
binary_semaphore_t *sem = &i2c_rx_ready;

I2CSlaveMsgCB messageProcessor, clearAfterSend, catchError;

static const I2CConfig slaveI2Cconfig = {
		OPMODE_I2C,
    //400000,
	//FAST_DUTY_CYCLE_2
	100000,
	STD_DUTY_CYCLE
};

/* Last message received from the master:
 * | Left speed (2) | Right speed (2) | Speaker (1) | LED1, LED3, LED5, LED7 (1) | LED2 RGB (3) | LED4 RGB (3) | LED6 RGB (3) | LED8 RGB (3) | Settings (1) |
 */
uint8_t i2c_rx_buff[ACTUATORS_SIZE];
uint8_t work_buffer[ACTUATORS_SIZE];

/* Message to send to the master:
 * | 8 x Prox (16) | 8 x Amb (16) |	4 x Mic (8) | Selector + button (1) | Left steps (2) | Right steps (2) | TV remote (1) |
 */
uint8_t i2c_tx_buff[SENSORS_SIZE] = {0};

BaseSequentialStream *chp = NULL;           // Used for serial logging

// Handler when something sent to us
const I2CSlaveMsg i2c_rx_msg = {
	ACTUATORS_SIZE,			/* max sizeof received msg body */
	i2c_rx_buff,			/* body of received msg */
	NULL,					/* do nothing on address match */
	messageProcessor,		/* Routine to process received messages */
	catchError				/* Error hook */
};

// Response to received messages
I2CSlaveMsg i2c_tx_msg = { 	/* this is in RAM so size may be updated */
	SENSORS_SIZE,			/* filled in with the length of the message to send */
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
 *  Message processor - looks at received message, determines reply as quickly as possible
 *
 *  Responds with the value of the messageCounter (in hex), followed by the received message in [..]
 *
 *  Note: Called in interrupt context, so need to be quick!
 */
void messageProcessor(I2CDriver *i2cp) {
	(void)i2cp;
	uint8_t j, i=0;
	int16_t n;
	uint8_t checksum = 0;

	memcpy(work_buffer, i2c_rx_buff, ACTUATORS_SIZE);
    chSysLockFromISR();
    chBSemSignalI(sem);
    chSysUnlockFromISR();

    for(j=0; j<8; j++) {
        n = get_calibrated_prox(j);
        i2c_tx_buff[i++] = n & 0xff;
        i2c_tx_buff[i++] = n >> 8;
    }
    for(j=0; j<8; j++) {
        n = get_ambient_light(j);
        i2c_tx_buff[i++] = n & 0xff;
        i2c_tx_buff[i++] = n >> 8;
    }
    for(j=0; j<4; j++) {
    	n = mic_get_volume(j);
        i2c_tx_buff[i++] = n & 0xff;
        i2c_tx_buff[i++] = n >> 8;
    }
    i2c_tx_buff[i++] = get_selector(); // First 4 bits used for the selector.
    if(button_is_pressed()) {	// 5th bit used for the button state.
    	i2c_tx_buff[i-1] |= 0x10;
    } else {
    	i2c_tx_buff[i-1] &= 0xEF;
    }
    n = left_motor_get_pos();
    i2c_tx_buff[i++] = n & 0xff;
    i2c_tx_buff[i++] = n >> 8;
    n = right_motor_get_pos();
    i2c_tx_buff[i++] = n & 0xff;
    i2c_tx_buff[i++] = n >> 8;
    i2c_tx_buff[i++] = ir_remote_get_data();

    // Construct a cheksum (Longitudinal Redundancy Check) to put at the end of the message.
    checksum = 0;
    for(j=0; j<(SENSORS_SIZE-1); j++) {
    	checksum ^= i2c_tx_buff[j];
    }
    i2c_tx_buff[SENSORS_SIZE-1] = checksum;

    i2c_tx_msg.size = SENSORS_SIZE;
    i2cSlaveReplyI(&I2CD1, &i2c_tx_msg);
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

  int16_t speedl, speedr;
  uint8_t checksum = 0;

  i2cStart(&slaveI2cPort, &slaveI2Cconfig);
#if HAL_USE_I2C1_MS_SLAVE
  slaveI2cPort.slaveTimeout = MS2ST(100);       // Time for complete message.
#endif

  i2cSlaveConfigure(&slaveI2cPort, &i2c_rx_msg, &i2c_tx_msg); // Initial reply.

  // Enable match address after everything else set up.
  i2cMatchAddress(&slaveI2cPort, slaveI2Caddress/2);
//  i2cMatchAddress(&slaveI2cPort, 0);  /* "all call" */


  // Prepare the reply for a "write+read" transaction.
  i2c_tx_msg.size = SENSORS_SIZE;
  i2cSlaveReplyI(&I2CD1, &i2c_tx_msg);

  while(true) {
    chBSemWait(&i2c_rx_ready);

    if (gotI2cError) {
    	gotI2cError = 0;
        //chprintf(chp, "I2cError: %04x\r\n", lastI2cErrorFlags);
    } else {

    	// Verify the checksum (Longitudinal Redundancy Check) before applying the received command.
    	checksum = 0;
    	for(int k=0; k<(ACTUATORS_SIZE-1); k++) {
    		checksum ^= work_buffer[k];
    	}
    	if(checksum == work_buffer[ACTUATORS_SIZE-1]) {
    		speedl = (unsigned char)work_buffer[0] + ((unsigned int)work_buffer[1] << 8);
    		speedr = (unsigned char)work_buffer[2] + ((unsigned int)work_buffer[3] << 8);

    		if(work_buffer[4]>=1 && work_buffer[4]<=3) {
    			play_melody(work_buffer[4]-1);
    		}

    		set_led(LED1, (work_buffer[5])&0x01);
    		set_led(LED3, (work_buffer[5]>>1)&0x01);
    		set_led(LED5, (work_buffer[5]>>2)&0x01);
    		set_led(LED7, (work_buffer[5]>>3)&0x01);

    		set_rgb_led(0, work_buffer[6], work_buffer[7], work_buffer[8]);
    		set_rgb_led(1, work_buffer[9], work_buffer[10], work_buffer[11]);
    		set_rgb_led(2, work_buffer[12], work_buffer[13], work_buffer[14]);
    		set_rgb_led(3, work_buffer[15], work_buffer[16], work_buffer[17]);

    		// Handle behaviors and others commands.
    		if(work_buffer[18] & 0x01) { // Calibrate proximity.
    			calibrate_ir();
    		}
    		if(work_buffer[18] & 0x02) { // Enable obastacle avoidance.

    		} else { // Disable obstacle avoidance

    		}
    		if(work_buffer[18] & 0x04) { // Set steps.
    			left_motor_set_pos(speedl);
    			right_motor_set_pos(speedr);
    		} else { // Set speed.
    			left_motor_set_speed(speedl);
    			right_motor_set_speed(speedr);
    		}

		} else {
			chprintf(chp, "wrong checksum (%02x != %02x)\r\n", work_buffer[ACTUATORS_SIZE-1], checksum);
    	}

    	//chprintf(chp, "SR2: %04x\r\n", (&slaveI2cPort)->i2c->SR2);
    }

  }

}
