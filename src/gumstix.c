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
#include "ir_remote.h"

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

#define ACTUATORS_SIZE 19
#define SENSORS_SIZE 30

uint8_t rx_flag = 0;

I2CSlaveMsgCB messageProcessor, clearAfterSend, catchError;

static const I2CConfig slaveI2Cconfig = {
		OPMODE_I2C,
    //400000,
	//FAST_DUTY_CYCLE_2
	100000,
	STD_DUTY_CYCLE
};

uint8_t i2c_rx_buff[ACTUATORS_SIZE];	// Stores last message master sent us.
uint8_t work_buffer[ACTUATORS_SIZE];

/* Return message buffer for computed replies */
uint8_t i2c_tx_buff[SENSORS_SIZE] = {
	// Sensors
	0,100, 0,101, 0,102, 0,103, 0,104, 0,105, 0,106, 0,107, // Proximity (16 bytes)
	0,200, 0,201, 0,202, 0,203, // Mic (8 bytes)
	10, // Selector (1 byte)
	0,150, 0,151, // Mot steps (4 bytes)
	5, // Tv remote (1 byte)
};

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
	clearAfterSend,       /* Clear receive buffer once replied */
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

	uint8_t j, i=0;
	int16_t n;
//  uint8_t *txPtr = txBody + 8;
//  uint8_t txLen;
//  uint32_t curCount;
//
//  size_t len = i2cSlaveBytes(i2cp);         // Number of bytes received
//  if (len >= sizeof(rxBody))
//      len = sizeof(rxBody)-1;
//  rxBody[len]=0;                            // String termination sometimes useful
//
//  /* A real-world application would read and decode the message in rxBody, then generate an appropriate reply in txBody */
//
//  curCount = ++messageCounter;
//  txLen = len + 11;                         // Add in the overhead
//
//  for (i = 0; i < 8; i++)
//  {
//    *--txPtr = hexString[curCount & 0xf];
//    curCount = curCount >> 4;
//  }
//
//  txPtr = txBody + 8;
//  *txPtr++ = ' ';
//  *txPtr++ = '[';
//  memcpy(txPtr, rxBody, len);               // Echo received message
//  txPtr += len;
//  *txPtr++ = ']';
//  *txPtr = '\0';

	memcpy(work_buffer, i2c_rx_buff, ACTUATORS_SIZE);
	rx_flag = 1;

    for (j=0; j<8; j++) {
        n = get_calibrated_prox(j);
        i2c_tx_buff[i++] = n & 0xff;
        i2c_tx_buff[i++] = n >> 8;
    }
    for(j=0; j<4; j++) {
    	n = mic_get_volume(j);
        i2c_tx_buff[i++] = n & 0xff;
        i2c_tx_buff[i++] = n >> 8;
    }
    i2c_tx_buff[i++] = get_selector();
    n = left_motor_get_pos();
    i2c_tx_buff[i++] = n & 0xff;
    i2c_tx_buff[i++] = n >> 8;
    n = right_motor_get_pos();
    i2c_tx_buff[i++] = n & 0xff;
    i2c_tx_buff[i++] = n >> 8;
    i2c_tx_buff[i++] = ir_remote_get_data();

	/** Message ready to go here */
	i2c_tx_msg.size = SENSORS_SIZE;
	chSysLockFromISR();
	i2cSlaveReplyI(i2cp, &i2c_tx_msg);
	chSysUnlockFromISR();
}

/**
 * Callback after sending of response complete - restores default reply in case polled
 */
void clearAfterSend(I2CDriver *i2cp)
{
	i2c_tx_msg.size = 0;
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

//  //simulate 16 clock pulses to unblock potential I2C periph blocked
//  //take control of the pin
//  palSetPadMode(GPIOB, GPIOB_SCL , PAL_MODE_OUTPUT_OPENDRAIN );
//  //16 clock pulses
//  for(uint8_t i = 0 ; i < 32 ; i++){
//  	palTogglePad(GPIOB, GPIOB_SCL);
//  	chThdSleepMilliseconds(1);
//  }
//  //make sure the output is high
//  palSetPad(GPIOB, GPIOB_SCL);
//  //give the control of the pin to the I2C machine
//  palSetPadMode(GPIOB, GPIOB_SCL , PAL_MODE_ALTERNATE(4) | PAL_STM32_OTYPE_OPENDRAIN);

  i2cStart(&slaveI2cPort, &slaveI2Cconfig);
#if HAL_USE_I2C1_MS_SLAVE
  slaveI2cPort.slaveTimeout = MS2ST(100);       // Time for complete message
#endif

  i2cSlaveConfigure(&slaveI2cPort, &i2c_rx_msg, &i2c_tx_msg); //&initialReply);

  // Enable match address after everything else set up
  i2cMatchAddress(&slaveI2cPort, slaveI2Caddress/2);
//  i2cMatchAddress(&slaveI2cPort, myOtherI2Caddress/2);
//  i2cMatchAddress(&slaveI2cPort, 0);  /* "all call" */

//  chprintf(chp, "Slave I2C started\n\r");

  /**
   * Just loop checking the error flag here.
   *
   * A real-world application might do the more heavyweight processing on received messages
   */
  while(true)
  {
    chThdSleepMilliseconds(10);
    if (gotI2cError)
    {
      gotI2cError = 0;
        chprintf(chp, "I2cError: %04x\r\n", lastI2cErrorFlags);
    }

    if(rx_flag) {
    	rx_flag = 0;

		speedl = (unsigned char)work_buffer[0] + ((unsigned int)work_buffer[1] << 8);
    	speedr = (unsigned char)work_buffer[2] + ((unsigned int)work_buffer[3] << 8);
    	left_motor_set_speed(speedl);
    	right_motor_set_speed(speedr);

    	// work_buffer[4] ==> speaker

    	set_led(LED1, (work_buffer[5])&0x01);
    	set_led(LED3, (work_buffer[5]>>1)&0x01);
    	set_led(LED5, (work_buffer[5]>>2)&0x01);
    	set_led(LED7, (work_buffer[5]>>3)&0x01);

		set_rgb_led(0, work_buffer[6], work_buffer[7], work_buffer[8]);
		set_rgb_led(1, work_buffer[9], work_buffer[10], work_buffer[11]);
		set_rgb_led(2, work_buffer[12], work_buffer[13], work_buffer[14]);
		set_rgb_led(3, work_buffer[15], work_buffer[16], work_buffer[17]);

		// work_buffer[18] ==> Additional future usage

    	//chprintf(chp, "SR2: %04x\r\n", (&slaveI2cPort)->i2c->SR2);
    }

  }
}
