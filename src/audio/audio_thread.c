#include <ch.h>
#include <hal.h>
#include "audio_thread.h"

#define STATE_STOPPED     0
#define STATE_PLAYING     1
#define SINUS_BUFFER_SIZE 360

#define DAC_USED          DACD2
#define TIMER_DAC         GPTD6

/*
 * The DAC of the uC is configured to output a buffer given in a circular manner, 
 * in our case a sine wave to produce sound. A GPT (general purpose timer) is used to trigger
 * the DAC. Each time the DAC is triggered, it output the following value of the buffer.
 * 
 * By changing the interval of the GPT, we change the frequency of the sound.
 * 
 */

/*
 * DAC test buffer (sine wave).
 * Values range from 1792 to 2304 (signal amplitude is 512, around 2048 that is the analog zero) to avoid distortion when playing from the speaker.
 * The audio will be multiplied by 5 in hardware before going to the speaker.
 */
static const dacsample_t sinus_buffer[SINUS_BUFFER_SIZE] = {
  2047, 2052, 2056, 2061, 2065, 2070, 2074, 2079, 2083, 2087, 2092, 2096,
  2101, 2105, 2109, 2114, 2118, 2122, 2126, 2131, 2135, 2139, 2143, 2147,
  2151, 2156, 2160, 2164, 2168, 2171, 2175, 2179, 2183, 2187, 2190, 2194,
  2198, 2201, 2205, 2208, 2212, 2215, 2219, 2222, 2225, 2228, 2231, 2235,
  2238, 2240, 2243, 2246, 2249, 2252, 2254, 2257, 2260, 2262, 2264, 2267,
  2269, 2271, 2273, 2275, 2277, 2279, 2281, 2283, 2285, 2286, 2288, 2289,
  2291, 2292, 2293, 2295, 2296, 2297, 2298, 2299, 2299, 2300, 2301, 2301,
  2302, 2302, 2303, 2303, 2303, 2303, 2303, 2303, 2303, 2303, 2303, 2302,
  2302, 2301, 2301, 2300, 2299, 2299, 2298, 2297, 2296, 2295, 2293, 2292,
  2291, 2289, 2288, 2286, 2285, 2283, 2281, 2279, 2277, 2275, 2273, 2271,
  2269, 2267, 2264, 2262, 2260, 2257, 2254, 2252, 2249, 2246, 2243, 2240,
  2238, 2235, 2231, 2228, 2225, 2222, 2219, 2215, 2212, 2208, 2205, 2201,
  2198, 2194, 2190, 2187, 2183, 2179, 2175, 2171, 2168, 2164, 2160, 2156,
  2151, 2147, 2143, 2139, 2135, 2131, 2126, 2122, 2118, 2114, 2109, 2105,
  2101, 2096, 2092, 2087, 2083, 2079, 2074, 2070, 2065, 2061, 2056, 2052,
  2047, 2043, 2039, 2034, 2030, 2025, 2021, 2016, 2012, 2007, 2003, 1999,
  1994, 1990, 1986, 1981, 1977, 1973, 1968, 1964, 1960, 1956, 1952, 1948,
  1943, 1939, 1935, 1931, 1927, 1923, 1920, 1916, 1912, 1908, 1904, 1901,
  1897, 1894, 1890, 1886, 1883, 1880, 1876, 1873, 1870, 1867, 1863, 1860,
  1857, 1854, 1851, 1849, 1846, 1843, 1840, 1838, 1835, 1833, 1831, 1828,
  1826, 1824, 1822, 1820, 1818, 1816, 1814, 1812, 1810, 1809, 1807, 1806,
  1804, 1803, 1802, 1800, 1799, 1798, 1797, 1796, 1796, 1795, 1794, 1794,
  1793, 1793, 1792, 1792, 1792, 1792, 1792, 1792, 1792, 1792, 1792, 1793,
  1793, 1794, 1794, 1795, 1796, 1796, 1797, 1798, 1799, 1800, 1802, 1803,
  1804, 1806, 1807, 1809, 1810, 1812, 1814, 1816, 1818, 1820, 1822, 1824,
  1826, 1828, 1831, 1833, 1835, 1838, 1840, 1843, 1846, 1849, 1851, 1854,
  1857, 1860, 1863, 1867, 1870, 1873, 1876, 1880, 1883, 1886, 1890, 1894,
  1897, 1901, 1904, 1908, 1912, 1916, 1920, 1923, 1927, 1931, 1935, 1939,
  1943, 1948, 1952, 1956, 1960, 1964, 1968, 1973, 1977, 1981, 1986, 1990,
  1994, 1999, 2003, 2007, 2012, 2016, 2021, 2025, 2030, 2034, 2039, 2043
};

static DACConversionGroup dac_conversion;

static uint8_t dac_state = STATE_STOPPED;

/***************************INTERNAL FUNCTIONS************************************/

/*
 * DAC error callback.
 */
static void error_cb(DACDriver *dacp, dacerror_t err) {

  (void)dacp;
  (void)err;

  chSysHalt("DAC failure");
}

/*************************END INTERNAL FUNCTIONS**********************************/


/****************************PUBLIC FUNCTIONS*************************************/

void dac_start(void)  {
    dac_conversion.num_channels = 1U;
    dac_conversion.end_cb = NULL;
    dac_conversion.error_cb = error_cb;
    dac_conversion.trigger = DAC_TRG(0); // Timer 6 TRGO event.

    static DACConfig dac_config;
    dac_config.init = 2048; // Set start value to half of the range.
    dac_config.datamode = DAC_DHRM_12BIT_RIGHT;

    dacStart(&DAC_USED, &dac_config);

    /* start timer for DAC trigger */
	static GPTConfig config;
    config.frequency = STM32_TIMCLK1; /* run timer at full frequency */
    config.callback = NULL;
    config.cr2 = TIM_CR2_MMS_1; /* trigger output on timer update */
    config.dier = 0U;
	
    gptStart(&TIMER_DAC, &config);

    //because when we do nothing, the speaker produces noise due to noise on the alimentation
    dac_stop();

}

void dac_play(uint16_t freq) {
	if(dac_state == STATE_STOPPED) {
		dac_state = STATE_PLAYING;
		dac_conversion.end_cb = NULL;
		dac_power_speaker(true); // Turn on audio.
		dacStartConversion(&DAC_USED, &dac_conversion, sinus_buffer, SINUS_BUFFER_SIZE);
		gptStartContinuous(&TIMER_DAC, STM32_TIMCLK1 / (freq*SINUS_BUFFER_SIZE));
	} else {
		gptChangeInterval(&TIMER_DAC, STM32_TIMCLK1 / (freq*SINUS_BUFFER_SIZE));
	}
}

void dac_play_buffer(uint16_t * buf, uint32_t size, uint32_t sampling_frequency, daccallback_t end_cb) {
  if(dac_state == STATE_STOPPED) {
    dac_state = STATE_PLAYING;
    dac_conversion.end_cb = end_cb;
    dac_power_speaker(true); // Turn on audio.
    dacStartConversion(&DAC_USED, &dac_conversion, buf, size);
    gptStartContinuous(&TIMER_DAC, STM32_TIMCLK1 / sampling_frequency);
  } else {
    gptChangeInterval(&TIMER_DAC, STM32_TIMCLK1 / sampling_frequency);
  }
}

void dac_change_bufferI(uint16_t* buf, uint32_t size, uint32_t sampling_frequency){
  gptStopTimerI(&TIMER_DAC);
  dacStopConversionI(&DAC_USED);

  dacStartConversionI(&DAC_USED, &dac_conversion, buf, size);
  gptStartContinuousI(&TIMER_DAC, STM32_TIMCLK1 / sampling_frequency);

}

void dac_stopI(void) {
  dac_power_speaker(false); // Turn off audio.
  gptStopTimerI(&TIMER_DAC);
  dacStopConversionI(&DAC_USED);
  dac_state = STATE_STOPPED;
}

void dac_stop(void) {
	dac_power_speaker(false); // Turn off audio.
  gptStopTimer(&TIMER_DAC);
  dacStopConversion(&DAC_USED);
	dac_state = STATE_STOPPED;
}

void dac_power_speaker(bool on_off){
  if(on_off){
    palClearPad(GPIOD, GPIOD_AUDIO_PWR);
  }else{
    palSetPad(GPIOD, GPIOD_AUDIO_PWR);
  }
}

/**************************END PUBLIC FUNCTIONS***********************************/

