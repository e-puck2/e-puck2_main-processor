#ifndef AR0144_H
#define AR0144_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "hal.h"
#include "camera.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AR0144_MAX_WIDTH 1280
#define AR0144_MAX_HEIGHT 800
#define AR0144_MAX_FRAME_WIDTH 1283
#define AR0144_MAX_FRAME_HEIGHT 803

typedef enum {
	AR0144_FORMAT_BAYER = 0x00,
	AR0144_FORMAT_RGB565 = 0x01,	// Actually this format is not supported, but in case it is requested a conversion will be performed.
	AR0144_FORMAT_GREYSCALE = 0x02	// Actually this format is not supported, but in case it is requested a conversion will be performed.
} ar0144_format_t;

struct ar0144_configuration {
	uint16_t 		width;
	uint16_t 		height;
	ar0144_format_t curr_format;
	subsampling_t 	curr_subsampling_x;
	subsampling_t 	curr_subsampling_y;
};

/**
 * @brief       Configure the ar0144 camera to start streaming.
 *
 * @return              The operation status.
 * @retval MSG_OK       if the function succeeded.
 * @retval MSG_TIMEOUT  if a timeout occurred before operation end or if wrong imgsize
 */
int8_t ar0144_start(void);

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
int8_t ar0144_config(ar0144_format_t fmt, image_size_t imgsize);

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
int8_t ar0144_advanced_config(ar0144_format_t fmt, unsigned int x1, unsigned int y1,
                               unsigned int width, unsigned int height,
								subsampling_t subsampling_x, subsampling_t subsampling_y);

/**
* @brief	Returns the current image size in bytes.
*
* @return	The image size in bytes
*
*/
uint32_t ar0144_get_image_size(void);

/**
* @brief	Check whether the ar0144 camera is connected.
*
* @return	1 if the camera is present, 0 otherwise.
*
*/
uint8_t ar0144_is_connected(void);

#ifdef __cplusplus
}
#endif

#endif
