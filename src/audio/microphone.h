#ifndef MICROPHONE_H
#define MICROPHONE_H


#ifdef __cplusplus
extern "C" {
#endif

void mic_start(void);
int32_t mic_get_volume(uint8_t mic);
int16_t mic_get_last(uint8_t mic);

#ifdef __cplusplus
}
#endif

#endif
