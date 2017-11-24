#include <ch.h>
#include <hal.h>
#include "dcmi_camera.h"

void frameEndCb(DCMIDriver* dcmip);
void dmaTransferEndCb(DCMIDriver* dcmip);
void dcmiErrorCb(DCMIDriver* dcmip, dcmierror_t err);

const DCMIConfig dcmicfg = {
    frameEndCb,
    dmaTransferEndCb,
	dcmiErrorCb,
    DCMI_CR_PCKPOL
};

uint8_t capture_mode = CAPTURE_ONE_SHOT;
uint8_t *sample_buffer = NULL;
uint8_t *sample_buffer2 = NULL;
uint8_t double_buffering = 0;
uint8_t image_ready = 0;
uint8_t dcmiErrorFlag = 0;

void frameEndCb(DCMIDriver* dcmip) {
    (void) dcmip;
    //palTogglePad(GPIOD, 13) ; // Orange.
}

void dmaTransferEndCb(DCMIDriver* dcmip) {
   (void) dcmip;
    //palTogglePad(GPIOD, 15); // Blue.
	//osalEventBroadcastFlagsI(&ss_event, 0);
   image_ready = 1;
}

void dcmiErrorCb(DCMIDriver* dcmip, dcmierror_t err) {
   (void) dcmip;
   (void) err;
    dcmiErrorFlag = 1;
	//chSysHalt("DCMI error");
}

void dcmi_start(void) {
	dcmiInit();
}

void dcmi_prepare(DCMIDriver *dcmip, const DCMIConfig *config, uint32_t transactionSize, void* rxbuf0, void* rxbuf1) {
	dcmiPrepare(dcmip, config, transactionSize, rxbuf0, rxbuf1);
}

void dcmi_unprepare(DCMIDriver *dcmip) {
	dcmiUnprepare(dcmip);
}

void dcmi_start_one_shot(DCMIDriver *dcmip) {
	image_ready = 0;
	dcmiStartOneShot(dcmip);
}

void dcmi_start_stream(DCMIDriver *dcmip) {
	dcmiStartStream(dcmip);
}

msg_t dcmi_stop_stream(DCMIDriver *dcmip) {
	return dcmiStopStream(dcmip);
}

uint8_t image_is_ready(void) {
	return image_ready;
}
