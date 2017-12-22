#ifndef MICROPHONE_H
#define MICROPHONE_H


#ifdef __cplusplus
extern "C" {
#endif

#include <hal.h>

void mic_start(void);
uint16_t mic_get_volume(uint8_t mic);
int16_t mic_get_last(uint8_t mic);

#ifdef __cplusplus
}
#endif

#endif
