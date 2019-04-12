#ifndef OV2640_H
#define OV2640_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "hal.h"
#include "camera.h"

#ifdef __cplusplus
extern "C" {
#endif

#define OV2640_MAX_WIDTH 640
#define OV2640_MAX_HEIGHT 480

typedef enum {
	OV2640_FORMAT_RGB565 = 0x08,
	OV2640_FORMAT_JPEG = 0x10,
	OV2640_FORMAT_Y8 = 0x40
} ov2640_format_t;

struct ov2640_configuration {
	uint16_t 		width;
	uint16_t 		height;
	ov2640_format_t curr_format;
	subsampling_t 	curr_subsampling_x;
	subsampling_t 	curr_subsampling_y;
};

/**
 * @brief       Initializes the clock generation for the po6030
 */
void ov2640_start(void);

 /**
 * @brief   Configures some parameters of the camera
 * 
 * @param fmt           format of the image. See format_t
 * @param imgsize       size of the image. See image_size_t
 *
 * @return              The operation status.
 * @retval MSG_OK       if the function succeeded.
 * @retval MSG_TIMEOUT  if a timeout occurred before operation end or if wrong imgsize
 *
 */
//int8_t ov2640_config(po6030_format_t fmt, image_size_t imgsize);

 /**
 * @brief   Configures advanced setting of the camera
 * 
 * @param fmt           format of the image. See format_t
 * @param x1            x coordinate of the upper left corner of the zone to capture from the sensor
 * @param y1            y coordinate of the upper left corner of the zone to capture from the sensor
 * @param width         width of the image to capture
 * @param height        height of the image to capture
 * subsampling_x        subsampling in the x axis. See subsampling_t
 * subsampling_y        subsampling in the y axis. See subsampling_t
 *
 * @return              The operation status.
 * @retval MSG_OK       if the function succeeded.
 * @retval MSG_TIMEOUT  if a timeout occurred before operation end
 *
 */
//int8_t ov2640_advanced_config(  po6030_format_t fmt, unsigned int x1, unsigned int y1,
//                                unsigned int width, unsigned int height,
//								subsampling_t subsampling_x, subsampling_t subsampling_y);

 /**
 * @brief   Sets the brigthness of the camera
 * 
 * @param value         Brightness. [7] = sign (positive if 0) and [6:0] the value. => from -128 to 127
 *
 * @return              The operation status.
 * @retval MSG_OK       if the function succeeded.
 * @retval MSG_TIMEOUT  if a timeout occurred before operation end
 * @retval others        see in the implementation for details
 *
 */
//int8_t ov2640_set_brightness(uint8_t value);

 /**
 * @brief   Sets the contrast of the camera
 * 
 * @param value         Contrast
 *
 * @return              The operation status.
 * @retval MSG_OK       if the function succeeded.
 * @retval MSG_TIMEOUT  if a timeout occurred before operation end
 *
 */
//int8_t ov2640_set_contrast(uint8_t value);

 /**
 * @brief   Sets mirroring for both vertical and horizontal orientations.
 * 
 * @param vertical      1 to enable vertical mirroring. 0 otherwise
 * @param horizontal    1 to enable horizontal mirroring. 0 otherwise
 *
 * @return              The operation status.
 * @retval MSG_OK       if the function succeeded.
 * @retval MSG_TIMEOUT  if a timeout occurred before operation end
 *
 */
//int8_t ov2640_set_mirror(uint8_t vertical, uint8_t horizontal);

 /**
 * @brief   Enables/disables auto white balance.
 * 
 * @param awb      1 to enable auto white balance. 0 otherwise
 *
 * @return              The operation status.
 * @retval MSG_OK       if the function succeeded.
 * @retval MSG_TIMEOUT  if a timeout occurred before operation end
 *
 */
//int8_t ov2640_set_awb(uint8_t awb);

 /**
 * @brief   Sets the white balance for the red, green and blue gains. 
 *          Writes the values to the camera but has no effect if auto white balance is enabled
 * 
 *          The resulting gain is the value divided by 64 (max resulting gain = 4).
 *          
 * @param r             red gain. Default is 0x5E.
 * @param g             green gain. Default is 0x40.
 * @param b             blue gain. Default is 0x5D.
 *
 * @return              The operation status.
 * @retval MSG_OK       if the function succeeded.
 * @retval MSG_TIMEOUT  if a timeout occurred before operation end
 *
 */
//int8_t ov2640_set_rgb_gain(uint8_t r, uint8_t g, uint8_t b);

 /**
 * @brief   Enables/disables auto exposure.
 * 
 * @param ae            1 to enable auto exposure. 0 otherwise
 *
 * @return              The operation status.
 * @retval MSG_OK       if the function succeeded.
 * @retval MSG_TIMEOUT  if a timeout occurred before operation end
 *
 */
//int8_t ov2640_set_ae(uint8_t ae);

 /**
 * @brief   Sets integration time, aka the exposure.
 *          Total integration time is: (integral + fractional/256) line time. 
 * 
 * @param integral      unit is line time. Default is 0x0080 (128).
 * @param fractional    unit is 1/256 line time. Default is 0x00 (0).
 *
 * @return              The operation status.
 * @retval MSG_OK       if the function succeeded.
 * @retval MSG_TIMEOUT  if a timeout occurred before operation end
 *
 */
//int8_t ov2640_set_exposure(uint16_t integral, uint8_t fractional);

 /**
 * @brief   Returns the current image size in bytes.
 *
 * @return              The image size in bytes
 *
 */
uint32_t ov2640_get_image_size(void);

/**
* @brief	Check whether the ov2640 camera is connected.
*
* @return	1 if the camera is present, 0 otherwise.
*
*/
uint8_t ov2640_is_connected(void);

#ifdef __cplusplus
}
#endif

#endif
