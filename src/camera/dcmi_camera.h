#ifndef DCMI_CAMERA_H
#define DCMI_CAMERA_H

#include <stdint.h>
#include <hal.h>
#include "dcmi.h"

#define MAX_BUFF_SIZE 76800 // Bytes.
#define CAPTURE_ONE_SHOT 0
#define CAPTURE_CONTINUOUS 1
extern const DCMIConfig dcmicfg;
extern uint8_t capture_mode;
extern uint8_t *sample_buffer;
extern uint8_t *sample_buffer2;
extern uint8_t double_buffering;

void dcmi_start(void);
void dcmi_prepare(DCMIDriver *dcmip, const DCMIConfig *config, uint32_t transactionSize, void* rxbuf0, void* rxbuf1);
void dcmi_unprepare(DCMIDriver *dcmip);
void dcmi_start_one_shot(DCMIDriver *dcmip);
void dcmi_start_stream(DCMIDriver *dcmip);
msg_t dcmi_stop_stream(DCMIDriver *dcmip);
uint8_t image_is_ready(void);

#endif /* DCMI_CAMERA_H */
