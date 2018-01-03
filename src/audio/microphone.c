#include <string.h>
#include <ch.h>
#include <hal.h>
#include "microphone.h"
#include "mp45dt02_processing.h"

/**
 * Microphones position:
 *
 *      FRONT
 *       ###
 *    #   2   #
 *  #           #
 * # 1         0 #
 * #             #
 *  #           #
 *    #   3   #
 *       ###
 *
 */
static uint16_t mic_volume[4];
static int16_t mic_last[4];

int16_t mic_get_last(uint8_t mic) {
	if(mic < 4) {
		return mic_last[mic];
	} else {
		return 0;
	}
}

uint16_t mic_get_volume(uint8_t mic) {
	if(mic < 4) {
		return mic_volume[mic];
	} else {
		return 0;
	}
}

static void handlePCMdata(int16_t *data, uint16_t samples, uint8_t peripheral) {
	(void)data;
	(void)samples;
	int16_t max_value[2]={INT16_MIN}, min_value[2]={INT16_MAX};

	for(uint16_t i=0; i<MIC_BUFFER_LEN; i+=2) {
		if(data[i] > max_value[0]) {
			max_value[0] = data[i];
		}
		if(data[i] < min_value[0]) {
			min_value[0] = data[i];
		}
		if(data[i+1] > max_value[1]) {
			max_value[1] = data[i+1];
		}
		if(data[i+1] < min_value[1]) {
			min_value[1] = data[i+1];
		}
	}

	if(peripheral == I2S_PERIPHERAL) {
		mic_volume[0] = max_value[0] - min_value[0];
		mic_volume[1] = max_value[1] - min_value[1];
		mic_last[0] = data[MIC_BUFFER_LEN-2];
		mic_last[1] = data[MIC_BUFFER_LEN-1];
	} else if(peripheral == SPI_PERIPHERAL) {
		mic_volume[2] = max_value[0] - min_value[0];
		mic_volume[3] = max_value[1] - min_value[1];
		mic_last[2] = data[MIC_BUFFER_LEN-2];
		mic_last[3] = data[MIC_BUFFER_LEN-1];
	}
	return;
}

void mic_start(void) {

	// *******************
	// TIMER CONFIGURATION
	// *******************
	// TIM9CH1 => input, the source is the I2S2 clock.
	// TIM9CH2 => output, this is the clock for microphones, 1/2 of input clock.

    rccEnableTIM9(FALSE);
    rccResetTIM9();

    STM32_TIM9->SR   = 0; // Clear eventual pending IRQs.
    STM32_TIM9->DIER = 0; // DMA-related DIER settings => DMA disabled.

    // Input channel configuration.
    STM32_TIM9->CCER &= ~STM32_TIM_CCER_CC1E; // Channel 1 capture disabled.
    STM32_TIM9->CCMR1 &= ~STM32_TIM_CCMR1_CC1S_MASK; // Reset channel selection bits.
    STM32_TIM9->CCMR1 |= STM32_TIM_CCMR1_CC1S(1); // CH1 Input on TI1.
    STM32_TIM9->CCMR1 &= ~STM32_TIM_CCMR1_IC1F_MASK; // No filter.
    STM32_TIM9->CCER &= ~(STM32_TIM_CCER_CC1P | STM32_TIM_CCER_CC1NP); // Rising edge, non-inverted.
    STM32_TIM9->CCMR1 &= ~STM32_TIM_CCMR1_IC1PSC_MASK; // No prescaler

    // Trigger configuration.
    STM32_TIM9->SMCR &= ~STM32_TIM_SMCR_TS_MASK; // Reset trigger selection bits.
    STM32_TIM9->SMCR |= STM32_TIM_SMCR_TS(5); // Input is TI1FP1.
    STM32_TIM9->SMCR &= ~STM32_TIM_SMCR_SMS_MASK; // Reset the slave mode bits.
    STM32_TIM9->SMCR |= STM32_TIM_SMCR_SMS(7); // External clock mode 1 => clock is TI1FP1.

    // Output channel configuration (pwm mode).
    STM32_TIM9->CR1 &= ~STM32_TIM_CR1_CKD_MASK; // No clock division.
    STM32_TIM9->ARR = 1; // Output clock halved.
    STM32_TIM9->PSC = 0; // No prescaler, counter clock frequency = fCK_PSC / (PSC[15:0] + 1).
    STM32_TIM9->EGR = STM32_TIM_EGR_UG; // Enable update event to reload preload register value immediately.
    STM32_TIM9->CCER &= ~STM32_TIM_CCER_CC2E; // Channel 2 output disabled.
    STM32_TIM9->CCMR1 &= ~STM32_TIM_CCMR1_CC2S_MASK; // Reset channel selection bits => channel configured as output.
    STM32_TIM9->CCMR1 &= ~STM32_TIM_CCMR1_OC2M_MASK; // Reset channel mode bits.
    STM32_TIM9->CCMR1 |= STM32_TIM_CCMR1_OC2M(6); // PWM1 mode.
    STM32_TIM9->CCER &= ~(STM32_TIM_CCER_CC2P | STM32_TIM_CCER_CC2NP); // Active high.
    STM32_TIM9->CCR[1] = 1; // Output clock halved.
    STM32_TIM9->CCMR1 |= STM32_TIM_CCMR1_OC2PE; // Enable preload at each update event for channel 2.
    STM32_TIM9->CCER &= ~STM32_TIM_CCMR1_OC2FE; // Disable fast mode.

    // Enable channels.
    STM32_TIM9->CCER |= STM32_TIM_CCER_CC1E | STM32_TIM_CCER_CC2E;
    STM32_TIM9->CR1 |= STM32_TIM_CR1_CEN;

    // ***************************
	// I2S2 AND I2S3 CONFIGURATION
    // ***************************
    mp45dt02Config micConfig;
    memset(&micConfig, 0, sizeof(micConfig));
    micConfig.fullbufferCb = handlePCMdata; // Callback called when the buffer is filled with 1 ms of PCM data.
    mp45dt02Init(&micConfig);

}



