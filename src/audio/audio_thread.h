#ifndef AUDIO_THREAD_H
#define AUDIO_THREAD_H

#ifdef __cplusplus
extern "C" {
#endif

#include <hal.h>

void audio_start(uint16_t freq);
void dac_start(void);
void dac_play(uint16_t freq);
void dac_stop(void);

#ifdef __cplusplus
}
#endif

#endif
