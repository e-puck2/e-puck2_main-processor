#ifndef MICROPHONE_H
#define MICROPHONE_H


#ifdef __cplusplus
extern "C" {
#endif

#include <hal.h>

void mic_start(void);
uint16_t mic_get_volume(uint8_t mic);
int16_t mic_get_last(uint8_t mic);
int16_t* mic_get_buffer_ptr(void);
bool mic_buffer_is_ready(void);
void mic_buffer_ready_reset(void);
uint16_t mic_buffer_get_size(void);

#ifdef __cplusplus
}
#endif

#endif
