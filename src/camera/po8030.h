#ifndef PO8030_H
#define PO8030_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "hal.h"
#include "camera.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PO8030_MAX_WIDTH 640
#define PO8030_MAX_HEIGHT 480

typedef enum {
    PO8030_FORMAT_CBYCRY = 0x00,
	PO8030_FORMAT_CRYCBY = 0x01,
	PO8030_FORMAT_YCBYCR = 0x02,
	PO8030_FORMAT_YCRYCB = 0x03,
	PO8030_FORMAT_RGGB = 0x10,
	PO8030_FORMAT_GBRG = 0x11,
	PO8030_FORMAT_GRBG = 0x12,
	PO8030_FORMAT_BGGR = 0x13,
	PO8030_FORMAT_RGB565 = 0x30,
	PO8030_FORMAT_RGB565_BYTE_SWAP = 0x31,
	PO8030_FORMAT_BGR565 = 0x32,
	PO8030_FORMAT_BGR565_BYTE_SWAP = 0x33,
	PO8030_FORMAT_RGB444 = 0x36,
	PO8030_FORMAT_RGB444_BYTE_SWAP = 0x37,
	PO8030_FORMAT_DPC_BAYER = 0x41,
	PO8030_FORMAT_YYYY = 0x44
} po8030_format_t;

struct po8030_configuration {
	uint16_t 		width;
	uint16_t 		height;
	po8030_format_t	curr_format;
	subsampling_t 	curr_subsampling_x;
	subsampling_t 	curr_subsampling_y;
};

/**
 * @brief       Initializes the clock generation for the po8030
 */
void po8030_start(void);

 /**
 * @brief   Configures some parameters of the camera
 * 
 * @param fmt           format of the image. See po8030_format_t
 * @param imgsize       size of the image. See image_size_t
 *
 * @return              The operation status.
 * @retval MSG_OK       if the function succeeded.
 * @retval MSG_TIMEOUT  if a timeout occurred before operation end or if wrong imgsize
 *
 */
int8_t po8030_config(po8030_format_t fmt, image_size_t imgsize);

 /**
 * @brief   Configures advanced setting of the camera
 * 
 * @param fmt           format of the image. See po8030_format_t
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
int8_t po8030_advanced_config(  po8030_format_t fmt, unsigned int x1, unsigned int y1,
                                unsigned int width, unsigned int height, 
                                subsampling_t subsampling_x, subsampling_t subsampling_y);

 /**
 * @brief   Sets the brigthness of the camera
 * 
 * @param value         Brightness. [7] = sign (positive if 0) and [6:0] the magnitude. => from -127 to 127
 * 						luminance = Y*contrast + brightness.
 *
 * @return              The operation status.
 * @retval MSG_OK       if the function succeeded.
 * @retval MSG_TIMEOUT  if a timeout occurred before operation end
 * @retval others       see in the implementation for details
 *
 */
int8_t po8030_set_brightness(uint8_t value);

 /**
 * @brief   Sets the contrast of the camera
 * 
 * @param value         Contrast (0..255)
 * 						luminance = Y*contrast + brightness.
 *
 * @return              The operation status.
 * @retval MSG_OK       if the function succeeded.
 * @retval MSG_TIMEOUT  if a timeout occurred before operation end
 *
 */
int8_t po8030_set_contrast(uint8_t value);

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
int8_t po8030_set_mirror(uint8_t vertical, uint8_t horizontal);

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
int8_t po8030_set_awb(uint8_t awb);

 /**
 * @brief   Sets the white balance for the red, green and blue gains. 
 *          These values are considered only when auto white balance is disabled, so this function also disables auto white balance.
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
int8_t po8030_set_rgb_gain(uint8_t r, uint8_t g, uint8_t b);

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
int8_t po8030_set_ae(uint8_t ae);

 /**
 * @brief   Sets integration time, aka the exposure.
 * 			These values are considered only when auto exposure is disabled, so this function also disables auto exposure.
 *          Total integration time is: (integral + fractional/256) * line time.
 * 
 * @param integral      unit is line time. Default is 0x0080 (128).
 * @param fractional    unit is 1/256 line time. Default is 0x00 (0).
 *
 * @return              The operation status.
 * @retval MSG_OK       if the function succeeded.
 * @retval MSG_TIMEOUT  if a timeout occurred before operation end
 *
 */
int8_t po8030_set_exposure(uint16_t integral, uint8_t fractional);

 /**
 * @brief   Returns the current image size in bytes.
 *
 * @return              The image size in bytes
 *
 */
uint32_t po8030_get_image_size(void);

/**
* @brief	Check whether the po8030 camera is connected.
*
* @return	1 if the camera is present, 0 otherwise.
*
*/
uint8_t po8030_is_connected(void);

#ifdef __cplusplus
}
#endif

#endif
