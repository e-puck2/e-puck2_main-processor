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

/**
 * @brief 		DCMI Driver initialization.
 */
void dcmi_start(void);

/**
 * @brief   Configures the DCMI peripheral.
 * @details This function configures the DCMI peripheral but keeps it disabled; the DMA is configured and enabled.
 * @post    Upon either of the two buffers being filled, the configured callback
 *          (transfer_complete_cb) is invoked.
 *          At the end of each frame the configured callback
 *          (frame_end_cb) is invoked.
 * @note    The buffers are organized as uint8_t arrays for data sizes equal to
 *          8 bits else it is organized as uint16_t arrays.
 *
 * @param[in] dcmip				pointer to the @p DCMIDriver object
 * @param[in] config			pointer to the @p DCMIConfig object
 * @param[in] transactionSize	Size of each receive buffer, in DCMI words.
 * @param[out] rxbuf0			the pointer to the first receive buffer
 * @param[out] rxbuf1			the pointer to the second receive buffer
 *
 */
void dcmi_prepare(DCMIDriver *dcmip, const DCMIConfig *config, uint32_t transactionSize, void* rxbuf0, void* rxbuf1);

/**
 * @brief Deactivates the DCMI peripheral.
 * @details This function disables the DCMI and related interrupts; also the DMA is released.
 *
 * @param[in] dcmip      pointer to the @p DCMIDriver object
 *
 */
void dcmi_unprepare(DCMIDriver *dcmip);

 /**
 * @brief   Captures a single frame from the DCMI.
 * @details This asynchronous function starts a single shot receive operation.
 *
 * @param[in] dcmip     pointer to the @p DCMIDriver object
 *
 */
void dcmi_start_one_shot(DCMIDriver *dcmip);

 /**
 * @brief   Begins reception of frames from the DCMI.
 * @details This asynchronous function starts a continuous receive operation.
 *
 * @param[in] dcmip     pointer to the @p DCMIDriver object
 *
 */
void dcmi_start_stream(DCMIDriver *dcmip);

 /**
 * @brief   Stops reception of frames from the DCMI.
 *
 * @param[in] dcmip     pointer to the @p DCMIDriver object
 *                      
 * 
 * @return              The operation status.
 * @retval MSG_OK       if the function succeeded.
 * @retval MSG_TIMEOUT  if a timeout occurred before operation end.
 *
 */
msg_t dcmi_stop_stream(DCMIDriver *dcmip);

/**
 * @brief 		Returns if an image is ready
 * 
 *@return             	Image ready
 *@retval 1				an image is ready
 *@retval 0				no image ready
 *
 */
uint8_t image_is_ready(void);

#endif /* DCMI_CAMERA_H */
