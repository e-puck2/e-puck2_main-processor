#include <ch.h>
#include <hal.h>
#include <stdio.h>
#include "battery_level.h"
#include "main.h"

// Battery level sampled every 2 seconds
// Each sampling total time takes about 46.9 us:
// - ADC clock div = 8 => APB2/8 = 84/8 = 10.5 MHz
// - [sampling (480 cycles) + conversion (12 cycles)] x 1'000'000/10'500'000 = about 46.9 us

#define DMA_BUFFER_SIZE (32)

#define VOLTAGE_DIVIDER         (1.0f * RESISTOR_R2 / (RESISTOR_R1 + RESISTOR_R2))

#define VREF                    3.0f //volt correspond to the voltage on the VREF+ pin
#define ADC_RESOLUTION          4096

#define COEFF_ADC_TO_VOLT       ((1.0f * ADC_RESOLUTION * VOLTAGE_DIVIDER) / VREF) //convertion from adc value to voltage

static uint16_t adc_value;
static adcsample_t adc_samples[DMA_BUFFER_SIZE];
static BSEMAPHORE_DECL(measurement_ready, true);
static battery_msg_t battery_value;

/***************************INTERNAL FUNCTIONS************************************/

 /**
 * @brief   Callback called after an ADC measure which 
 *          averages and copies the measure
 */
static void adc_cb(ADCDriver *adcp, adcsample_t *samples, size_t n)
{
	(void) adcp;

    /* Calculate the average over all samples. */
	adc_value = 0;
    for (size_t i = 0; i < n; i++) {
    	adc_value += samples[i];
    }
    adc_value /= n;

    chSysLockFromISR();
    adcStopConversionI(adcp);
    chBSemSignalI(&measurement_ready);
    chSysUnlockFromISR();
}

//ADC configuration
static const ADCConversionGroup group = {
    .circular = true,
    .num_channels = 1,
    .end_cb = adc_cb,
    .error_cb = NULL,
    .cr1 = 0,
    .cr2 = ADC_CR2_SWSTART, /* manual start */
    .smpr1 = 0,
    .smpr2 = ADC_SMPR2_SMP_AN1(ADC_SAMPLE_480),

    .sqr1 = ADC_SQR1_NUM_CH(1),
    .sqr2 = 0,
    .sqr3 = ADC_SQR3_SQ1_N(1),
};

 /**
 * @brief   Thread which updates the measures and publishes them
 */
static THD_FUNCTION(battery_thd, arg)
{
    (void) arg;
    chRegSetThreadName("Battery Thd");

    messagebus_topic_t battery_topic;
    MUTEX_DECL(battery_topic_lock);
    CONDVAR_DECL(battery_topic_condvar);
    messagebus_topic_init(&battery_topic, &battery_topic_lock, &battery_topic_condvar, &battery_value, sizeof(battery_value));
    messagebus_advertise_topic(&bus, &battery_topic, "/battery_level");

    while (true) {
        adcAcquireBus(&ADCD1);

        /* Starts a measurement and waits for it to complete. */
        adcStartConversion(&ADCD1, &group, adc_samples, DMA_BUFFER_SIZE);
        chBSemWait(&measurement_ready);
        adcReleaseBus(&ADCD1);

        /* Converts the measurement to volts and publish the measurement on the
         * bus. */
        battery_value.raw_value = adc_value;
        battery_value.voltage = adc_value / COEFF_ADC_TO_VOLT;
        battery_value.percentage =  (battery_value.voltage - MIN_VOLTAGE) * 
                                    (MAX_PERCENTAGE - MIN_PERCENTAGE) / 
                                    (MAX_VOLTAGE - MIN_VOLTAGE) + MIN_PERCENTAGE;
        messagebus_topic_publish(&battery_topic, &battery_value, sizeof(battery_value));

        /* Sleep for some time. */
        chThdSleepSeconds(2);
    }
}

/*************************END INTERNAL FUNCTIONS**********************************/


/****************************PUBLIC FUNCTIONS*************************************/

void battery_level_start(void)
{
	adcStart(&ADCD1, NULL);
    static THD_WORKING_AREA(battery_thd_wa, 1024);
    chThdCreateStatic(battery_thd_wa, sizeof(battery_thd_wa), NORMALPRIO, battery_thd, NULL);
}

uint16_t get_battery_raw(void) {
    return battery_value.raw_value;
}

float get_battery_voltage(void) {
    return battery_value.voltage;
}

float get_battery_percentage(void){
    return battery_value.percentage;
}

/**************************END PUBLIC FUNCTIONS***********************************/
