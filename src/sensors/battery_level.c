#include <ch.h>
#include <hal.h>
#include <stdio.h>
#include "battery_level.h"
#include "main.h"

// Battery level sampled every 2 seconds
// Each sampling total time takes about 46.9 us:
// - ADC clock div = 8 => APB2/8 = 84/8 = 10.5 MHz
// - [sampling (480 cycles) + conversion (12 cycles)] x 1'000'000/10'500'000 = about 46.9 us

#define DMA_BUFFER_SIZE (16)

static uint16_t adc_value;
static adcsample_t adc_samples[DMA_BUFFER_SIZE];
static BSEMAPHORE_DECL(measurement_ready, true);
static battery_msg_t battery_value;

uint16_t get_battery_raw(void) {
	return battery_value.raw_value;
}

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

static const ADCConversionGroup group = {
    .circular = true,
    .num_channels = 1,
    .end_cb = adc_cb,
    .error_cb = NULL,
    .cr1 = 0,
    .cr2 = ADC_CR2_SWSTART, /* manual start */
    .smpr1 = 0,
    .smpr2 = ADC_SMPR2_SMP_AN0(ADC_SAMPLE_480),

    .sqr1 = ADC_SQR1_NUM_CH(1),
    .sqr2 = 0,
    .sqr3 = ADC_SQR3_SQ1_N(1),
};

static THD_FUNCTION(battery_thd, arg)
{
    (void) arg;

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
        // battery_value.voltage = ...;
        messagebus_topic_publish(&battery_topic, &battery_value, sizeof(battery_value));

        /* Sleep for some time. */
        chThdSleepSeconds(2);
    }
}

void battery_level_start(void)
{
	adcStart(&ADCD1, NULL);
    static THD_WORKING_AREA(battery_thd_wa, 1024);
    chThdCreateStatic(battery_thd_wa, sizeof(battery_thd_wa), NORMALPRIO, battery_thd, NULL);
}
