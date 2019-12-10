#include <stdio.h>
#include <string.h>
#include "ch.h"
#include "hal.h"
#include "ircom.h"
#include "ircomReceive.h"
#include "ircomSend.h"
#include "ircomMessages.h"
#include <main.h>
#include <limits.h>
#include "leds.h"
#include "sensors/proximity.h"

// The proximity sensors sampling is designed in order to sample two sensors at one time, the couples are chosen
// in order to have as less interference as possible and divided as follow:
// - IR0 (front-right) + IR4 (back-left)
// - IR1 (front-right-45deg) + IR5 (left)
// - IR2 (right) + IR6 (front-left-45deg)
// - IR3 (back-right) + IR7 (front-left)
// A timer is used to handle the sampling (pwm mode), one channel is used to handle the pulse and a second channel
// is used to trigger the sampling at the right time.
// In each pwm period a single couple of sensors is sampled, either with the pulse active to measure the
// "reflected light" or with the pulse inactive to measure the ambient light; the sequence is:
// - cycle0: IR0 + IR4 ambient
// - cycle1: IR0 + IR4 reflected
// - cycle2: IR1 + IR5 ambient
// - cycle3: IR1 + IR5 reflected
// - cycle4: IR2 + IR6 ambient
// - cycle5: IR2 + IR6 reflected
// - cycle6: IR3 + IR7 ambient
// - cycle7: IR3 + IR7 reflected
// The pwm frequency is 800 Hz, the resulting update frequency for the sensors is 100 Hz (8 cycles to get all the sensors).
// The pulse is 300 us, the sampling is done at 260 us, this give time to sample both channels when the pulse is still
// active. Each sampling total time takes about 11.8 us:
// - ADC clock div = 8 => APB2/8 = 84/8 = 10.5 MHz
// - [sampling (112 cycles) + conversion (12 cycles)] x 1'000'000/10'500'000 = about 11.81 us

#define PWM_CLK_FREQ 1000000
#define PWM_FREQUENCY 12300 // => 81.3 us
#define PWM_CYCLE (PWM_CLK_FREQ / PWM_FREQUENCY)

#define PROXIMITY_ADC_SAMPLE_TIME ADC_SAMPLE_84
#define DMA_BUFFER_SIZE 1 // 1 sample for each ADC channel

#define EXTSEL_TIM2_CH2 0x03

#define IRCOM_PROX_TRIGGER 2460 // Trigger proximity sampling procedure every 81.3 usec * IRCOM_PROX_TRIGGER

volatile unsigned int ircom_last_ir_scan_id = 0;
volatile int ircom_samples_to_skip = 0;
volatile uint8_t array_filled = 0;
volatile Ircom ircomData;
volatile IrcomReceive ircomReceiveData;
volatile IrcomSend ircomSendData;

static BSEMAPHORE_DECL(adc2_ready, true);
static adcsample_t adc2_proximity_samples[PROXIMITY_NB_CHANNELS * DMA_BUFFER_SIZE];
static proximity_msg_t prox_values;
int16_t adc_buffer1[PROXIMITY_NB_CHANNELS * IRCOM_SAMPLING_WINDOW];
int16_t adc_buffer2[PROXIMITY_NB_CHANNELS * IRCOM_SAMPLING_WINDOW];
int16_t *window_sampling;
int16_t *window_data;
static uint8_t calibrationInProgress = 0;
static uint8_t calibrationState = 0;
static uint8_t calibrationNumSamples = 0;
static int32_t calibrationSum[PROXIMITY_NB_CHANNELS] = {0};
volatile uint8_t adc_mode; // Mode can be 0 for IR communication or 1 for proximity sampling.
volatile uint8_t prox_sampling_enabled;
volatile uint8_t ircom_enabled;
volatile int prox_trigger;
volatile int prox_counter;
volatile int prox_maxcounter;
volatile int prox_index;
volatile int prox_phase;

/***************************INTERNAL FUNCTIONS************************************/

/*
 * GPT10 callback.
 */
static void gpt14_callback(GPTDriver *gptp) {
	(void)gptp;
	//set_led(0, 2);
    // system is not on pause
    if (ircomData.paused == 0) {
		// use appropriate FSM, if any
		switch (ircomData.fsm) {
			case IRCOM_FSM_RECEIVE :
			    ircomReceiveMain();
			    break;
			case IRCOM_FSM_SEND :
			    ircomSendMain();
			    break;
			default:
				break;
		}
    }

    // update time counter
    if(ircomData.time < LONG_MAX) {
    	ircomData.time ++;
    }
}

//General Purpose Timer configuration
static const GPTConfig gpt14cfg = {
	1000000,		/* 1MHz timer clock in order to measure uS.*/
	gpt14_callback,			/* Timer callback.*/
	0,
	0
};

void ircom_interrupt(adcsample_t *samples) {
    if (ircom_enabled == 0) {
    	return;
    }

    // skip samples if needed to sync...
    if(ircom_samples_to_skip > 0) {
		ircom_samples_to_skip--;
		return;
    }

    // Copy the sampled values to the destination buffer.
    for (size_t i=0; i<PROXIMITY_NB_CHANNELS; i++) {
    	window_sampling[ircom_last_ir_scan_id*PROXIMITY_NB_CHANNELS+i] = samples[i];
    }

    ircom_last_ir_scan_id++;
    // Array full?
    if (ircom_last_ir_scan_id >= IRCOM_SAMPLING_WINDOW) {
		ircom_last_ir_scan_id = 0;
		array_filled = 1;

		// swap buffers
		int16_t* tmp = window_data;
		window_data = window_sampling;
		window_sampling = tmp;
    }
}

void ircom_scan_reset(void) {
    ircom_last_ir_scan_id = 0;      // reset array index
    array_filled = 0;	        // reset array filled flag
}

// sampling time = 81.3 usec
// counters limits are based on this value
void proximity_interrupt(adcsample_t *samples) {

	binary_semaphore_t *sem = &adc2_ready;

    // check if we can enter the interrupt
    prox_counter++;
    if(prox_counter < prox_maxcounter) {
    	return;
    }

    // reset counter
    prox_counter = 0;

    // acquisition of prox data is over, switch mode
    if(prox_index >= 4) {
    	ircom_scan_reset();
		adc_mode = 0;
		return;
    }

    // check current phase
    int idx = prox_index;
    if (prox_phase == 0) {
		// ok ir beam is off, we can measure ambient light
    	prox_values.ambient[idx] = samples[idx];
    	prox_values.ambient[idx+4] = samples[idx+4];

		switch (idx) {
			case 0 : palSetPad(GPIOB, GPIOB_PULSE_0); break;
			case 1 : palSetPad(GPIOB, GPIOB_PULSE_1); break;
			case 2 : palSetPad(GPIOE, GPIOE_PULSE_2); break;
			case 3 : palSetPad(GPIOE, GPIOE_PULSE_3); break;
		}
		prox_phase = 1;

		// set delay to enter inside interrupt next time (beam goes on, full power)
		prox_maxcounter = 4; // 325 usec = 81.3 * 4
    } else {
    	prox_values.reflected[idx] = samples[idx];
    	prox_values.reflected[idx+4] = samples[idx+4];

    	if(prox_values.reflected[idx] > prox_values.ambient[idx]) {
    		prox_values.delta[idx] = 0;
    	} else {
    		prox_values.delta[idx] = prox_values.ambient[idx] - prox_values.reflected[idx];
    	}
    	if(prox_values.reflected[idx+4] > prox_values.ambient[idx+4]) {
    		prox_values.delta[idx+4] = 0;
    	} else {
    		prox_values.delta[idx+4] = prox_values.ambient[idx+4] - prox_values.reflected[idx+4];
    	}
		switch (idx) {
			case 0 : palClearPad(GPIOB, GPIOB_PULSE_0); break;
			case 1 : palClearPad(GPIOB, GPIOB_PULSE_1); break;
			case 2 : palClearPad(GPIOE, GPIOE_PULSE_2); break;
			case 3 : palClearPad(GPIOE, GPIOE_PULSE_3); break;
		}
		prox_phase = 0;
		prox_index++;

		// set delay to enter inside interrupt next time (beam goes on, full power)
		prox_maxcounter = 26; // 2113 usec = 81.3 * 8
    }

    /* Signal the proximity thread that the ADC measurements are done. */
    chSysLockFromISR();
    chBSemSignalI(sem);
    chSysUnlockFromISR();
}

 /**
 * @brief   Calback called after an the ADC completes a measure.
 * 			Averages an copies the measure.
 * 
 * @param adcp		ADC pointer (not used)
 * @param samples	pointer to the buffer containing the samples
 * @param n			size of the buffer
 * 
 */
static void adc_cb(ADCDriver *adcp, adcsample_t *samples, size_t n)
{
    (void) adcp;
    (void) n;

    if(prox_sampling_enabled == 1) {
    	prox_trigger++;
    }

    // check if we switch mode
    if(prox_trigger >= IRCOM_PROX_TRIGGER) {	// every 81.3 * IRCOM_PROX_TRIGGER = about 200 ms
    	prox_trigger = 0;

    	// Shut down all IRs.
    	palClearPad(GPIOB, GPIOB_PULSE_0);
    	palClearPad(GPIOB, GPIOB_PULSE_1);
    	palClearPad(GPIOE, GPIOE_PULSE_2);
    	palClearPad(GPIOE, GPIOE_PULSE_3);

    	// Set delay till IR beam goes off.
    	prox_counter = 0;
    	prox_maxcounter = 26; // 2113 usec = 81.3 * 26
    	prox_phase = 0;
    	prox_index = 0;
		adc_mode = 1;

    }

    // Direct interrupt to proximity sampling or IR communication.
    switch (adc_mode) {
	    case 0:
	    	ircom_interrupt(samples);
	    	break;

	    case 1:
	    	proximity_interrupt(samples);
	    	break;
    }

}

//configuration of the ADC
static const ADCConversionGroup adcgrpcfg2 = {
    .circular = true,
    // One measure for each proximity is saved before raising the DMA interrupt (and call the adc callback).
    .num_channels = PROXIMITY_NB_CHANNELS,
    .end_cb = adc_cb,
    .error_cb = NULL,

    // Discontinuous mode with 8 conversions per trigger.
	// Every time the sampling is triggered by the timer, 8 channels are sampled:
	// - on trigger event: IR0 + IR1 + IR2 + IR3 + IR4 + IR5 + IR6 + IR7 sampling
    .cr1 = ADC_CR1_DISCEN | ADC_CR1_DISCNUM_0|ADC_CR1_DISCNUM_1|ADC_CR1_DISCNUM_2,

    /* External trigger on timer 2 CC2. */
    .cr2 = ADC_CR2_EXTEN_1 | ADC_CR2_EXTSEL_SRC(EXTSEL_TIM2_CH2),

    /* Sampling duration, all set to PROXIMITY_ADC_SAMPLE_TIME. */
    .smpr2 = ADC_SMPR2_SMP_AN0(PROXIMITY_ADC_SAMPLE_TIME) |
             ADC_SMPR2_SMP_AN1(PROXIMITY_ADC_SAMPLE_TIME) |
             ADC_SMPR2_SMP_AN2(PROXIMITY_ADC_SAMPLE_TIME) |
             ADC_SMPR2_SMP_AN3(PROXIMITY_ADC_SAMPLE_TIME) |
             ADC_SMPR2_SMP_AN4(PROXIMITY_ADC_SAMPLE_TIME) |
             ADC_SMPR2_SMP_AN5(PROXIMITY_ADC_SAMPLE_TIME) |
             ADC_SMPR2_SMP_AN6(PROXIMITY_ADC_SAMPLE_TIME) |
             ADC_SMPR2_SMP_AN7(PROXIMITY_ADC_SAMPLE_TIME) |
             ADC_SMPR2_SMP_AN8(PROXIMITY_ADC_SAMPLE_TIME) |
             ADC_SMPR2_SMP_AN9(PROXIMITY_ADC_SAMPLE_TIME),
    .smpr1 = ADC_SMPR1_SMP_AN10(PROXIMITY_ADC_SAMPLE_TIME) |
             ADC_SMPR1_SMP_AN11(PROXIMITY_ADC_SAMPLE_TIME) |
             ADC_SMPR1_SMP_AN12(PROXIMITY_ADC_SAMPLE_TIME) |
             ADC_SMPR1_SMP_AN13(PROXIMITY_ADC_SAMPLE_TIME) |
             ADC_SMPR1_SMP_AN14(PROXIMITY_ADC_SAMPLE_TIME) |
             ADC_SMPR1_SMP_AN15(PROXIMITY_ADC_SAMPLE_TIME),

    // Channels are defined starting from front-right sensor and turning clock wise.
	// Proximity sensors channels:
	// IR0 = ADC123_IN12 (PC2)
	// IR1 = ADC123_IN13 (PC3)
	// IR2 = ADC12_IN14 (PC4)
	// IR3 = ADC12_IN15 (PC5)
	// IR4 = ADC12_IN9 (PB1)
	// IR5 = ADC12_IN8 (PB0)
	// IR6 = ADC123_IN10 (PC0)
	// IR7 = ADC123_IN11 (PC1)
	// A single ADC peripheral (ADC1 or ADC2) can be used for all channels => chosen ADC2.
    .sqr3 = ADC_SQR3_SQ1_N(12) |
            ADC_SQR3_SQ2_N(13) |
			ADC_SQR3_SQ3_N(14) |
			ADC_SQR3_SQ4_N(15) |
            ADC_SQR3_SQ5_N(9) |
            ADC_SQR3_SQ6_N(8),
    .sqr2 = ADC_SQR2_SQ7_N(10) |
            ADC_SQR2_SQ8_N(11),
    .sqr1 = ADC_SQR1_NUM_CH(PROXIMITY_NB_CHANNELS)
};

 /**
 * @brief   Thread which updates the measures and publishes them
 */
static THD_WORKING_AREA(proximity_thd_wa, 256);
static THD_FUNCTION(proximity_thd, arg)
{
    (void) arg;
    chRegSetThreadName(__FUNCTION__);

    messagebus_topic_t proximity_topic;
    MUTEX_DECL(prox_topic_lock);
    CONDVAR_DECL(prox_topic_condvar);
    messagebus_topic_init(&proximity_topic, &prox_topic_lock, &prox_topic_condvar, &prox_values, sizeof(prox_values));
    messagebus_advertise_topic(&bus, &proximity_topic, "/proximity");

    while (true) {

    	chBSemWait(&adc2_ready);

        messagebus_topic_publish(&proximity_topic, &prox_values, sizeof(prox_values));

        if(calibrationInProgress) {
        	switch(calibrationState) {
				case 0:
					memset(prox_values.initValue, 0, PROXIMITY_NB_CHANNELS * sizeof(unsigned int));
					memset(calibrationSum, 0, PROXIMITY_NB_CHANNELS * sizeof(int32_t));
					calibrationNumSamples = 0;
					calibrationState = 1;
					break;

				case 1:
					for(int i=0; i<PROXIMITY_NB_CHANNELS; i++) {
						calibrationSum[i] += ircom_get_prox(i);
					}
					calibrationNumSamples++;
					if(calibrationNumSamples == 100) {
						for(int i=0; i<PROXIMITY_NB_CHANNELS; i++) {
							prox_values.initValue[i] = calibrationSum[i]/100;
						}
						calibrationInProgress = 0;
					}
					break;
        	}
        }

    }
}

/*************************END INTERNAL FUNCTIONS**********************************/


/****************************PUBLIC FUNCTIONS*************************************/

void ircomStart(void)
{
	if(ADCD1.state != ADC_STOP) {
		return;
	}

	// Clear all the pulse independently of the one that is actually active.
	palClearPad(GPIOB, GPIOB_PULSE_0);
	palClearPad(GPIOB, GPIOB_PULSE_1);
	palClearPad(GPIOE, GPIOE_PULSE_2);
	palClearPad(GPIOE, GPIOE_PULSE_3);

    ircomFlushMessages();
    ircomDisableContinuousListening();
    ircomPause(0);
    ircomSetOmnidirectional();

    // set a number of default values
    ircomData.fsm = IRCOM_FSM_IDLE;
    ircomData.time = 0;
    ircomReceiveData.receiving = 0;
    ircomReceiveData.done = 1;
    ircomSendData.done = 1;

    ircomSendData.markSwitchCount = 10; // How many times the signal switches for a "mark" bit in a sampling window.
    ircomSendData.spaceSwitchCount = 5; // How many times the signal switches for a "space" bit in sampling window.
    ircomSendData.markDuration = 260; // How much time (in T1 ticks) lasts a "mark" bit.
    ircomSendData.spaceDuration = 520; // How much time (in T1 ticks) lasts a "space" bit.

	prox_trigger = 0;
	prox_sampling_enabled = 1;
	ircom_enabled = 1;

	window_sampling = adc_buffer1;
	window_data = adc_buffer2;

	gptStart(&GPTD14, &gpt14cfg);
	gptStartContinuous(&GPTD14, 100);

    static const PWMConfig pwmcfg_proximity = {
        /* timer clock frequency */
        .frequency = PWM_CLK_FREQ, // 1MHz
        /* timer period */
        .period = PWM_CYCLE, // 12.3 KHz = 81.3 us
        .cr2 = 0,
        .callback = NULL,
        .channels = {
            {.mode = PWM_OUTPUT_DISABLED, .callback = NULL},
            // Channel 2 is used to generate ADC trigger for starting sampling.
        	// It must be in output mode, although it is not routed to any pin.
            {.mode = PWM_OUTPUT_ACTIVE_HIGH, .callback = NULL},
            {.mode = PWM_OUTPUT_DISABLED, .callback = NULL},
            {.mode = PWM_OUTPUT_DISABLED, .callback = NULL},
        },
    };
	
    adcStart(&ADCD1, NULL);
    //adcAcquireBus(&ADCD1);
    // ADC waiting for the trigger from the timer.
    adcStartConversion(&ADCD1, &adcgrpcfg2, adc2_proximity_samples, DMA_BUFFER_SIZE);

    /* Init PWM */
    pwmStart(&PWMD2, &pwmcfg_proximity);
    pwmEnableChannel(&PWMD2, 1, (pwmcnt_t) 1); // Enable channel 2 to trigger the measures.

    chThdCreateStatic(proximity_thd_wa, sizeof(proximity_thd_wa), NORMALPRIO, proximity_thd, NULL);
	
}

void ircomPause(int status) {
	ircomData.paused = status;
}

void ircomEnableContinuousListening(void) {
	ircomReceiveData.continuousListening = 1;
}

void ircomDisableContinuousListening(void) {
	ircomReceiveData.continuousListening = 0;
}

void ircom_skip_samples(int samples_count) {
    ircom_samples_to_skip = samples_count;
    ircom_last_ir_scan_id = 0;      // reset array index
    array_filled = 0;	        // reset array filled flag
}

uint8_t ircom_is_array_filled(void) {
	uint8_t result = array_filled;
    array_filled = 0;
    return result;
}

void ircom_adc_off(void) {
	pwmDisableChannelI(&PWMD2, 1); // Disable channel 2 to stop the measures.
}

void ircom_adc_on(void) {
	pwmEnableChannelI(&PWMD2, 1, (pwmcnt_t) 1); // Enable channel 2 to trigger the measures.
}

void ircomStop(void) {
	gptStopTimer(&GPTD14);    // disable Timer14
}

void ircomRestart(void) {
	gptStartContinuous(&GPTD14, 100);    // enable Timer14
}

void ircomEnableProximity(void) {
	prox_sampling_enabled = 1;
}

void ircomDisableProximity(void) {
	prox_sampling_enabled = 0;
}

void ircomSetOmnidirectional(void) {
	ircomSendData.type = IRCOM_SEND_OMNI;
}

void ircomSetDirectional(int sensor) {
    ircomSendData.type = IRCOM_SEND_DIRECTIONAL;
    ircomSendData.sensor = sensor % 4; // ir emitters are coupled... -> only 4 pulses
}

void ircom_calibrate_ir(void) {
	calibrationState = 0;
	calibrationInProgress = 1;
	while(calibrationInProgress) {
		chThdSleepMilliseconds(20);
	}
}

int ircom_get_prox(unsigned int sensor_number) {
	if (sensor_number > 7) {
		return 0;
	} else {
		return prox_values.delta[sensor_number];
	}
}

int ircom_get_calibrated_prox(unsigned int sensor_number) {
	int temp;
	if (sensor_number > 7) {
		return 0;
	} else {
		temp = prox_values.delta[sensor_number] - prox_values.initValue[sensor_number];
		if (temp>0) {
			return temp;
		} else {
			return 0;
		}
	}
}

int ircom_get_ambient_light(unsigned int sensor_number) {
	if (sensor_number > 7) {
		return 0;
	} else {
		return prox_values.ambient[sensor_number];
	}
}

/**************************END PUBLIC FUNCTIONS***********************************/
