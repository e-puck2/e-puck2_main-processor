#ifndef AUDIO_THREAD_H
#define AUDIO_THREAD_H

#ifdef __cplusplus
extern "C" {
#endif

#include <hal.h>

 /**
 * @brief   Starts the DAC module. Power of the audio amplifier and DAC peripheral
 */
void dac_start(void);

 /**
 * @brief   Plays the specified frequence on the speaker
 */
void dac_play(uint16_t freq);

void dac_change_bufferI(uint16_t* buf, uint32_t size, uint32_t sampling_frequency);

void dac_play_buffer(uint16_t * buf, uint32_t size, uint32_t sampling_frequency, daccallback_t end_cb);

void dac_stopI(void);

/**
 * @brief   Stops the sound being played on the speaker (if any)
 */
void dac_stop(void);

#ifdef __cplusplus
}
#endif

#endif
