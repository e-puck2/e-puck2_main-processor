#ifndef CAMERA_H
#define CAMERA_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	SIZE_VGA = 0x00,
	SIZE_QVGA = 0x01,
	SIZE_QQVGA = 0x02
} image_size_t;

typedef enum {
    SUBSAMPLING_X1 = 0x20,
    SUBSAMPLING_X2 = 0x40,
    SUBSAMPLING_X4 = 0x80
} subsampling_t;

typedef enum {
    FORMAT_GREYSCALE = 0x00,
	FORMAT_COLOR = 0x01
} format_t;

void cam_start(void);
int8_t cam_config(format_t fmt, image_size_t imgsize);
uint32_t cam_get_image_size(void);
uint32_t cam_get_mem_required(void);
int8_t cam_advanced_config(format_t fmt, unsigned int x1, unsigned int y1,
                                unsigned int width, unsigned int height,
								subsampling_t subsampling_x, subsampling_t subsampling_y);

/**
* @brief   Sets the brigthness of the camera
*
* @param value         2's complement => from -128 to 127
*
* @return              The operation status.
* @retval MSG_OK       if the function succeeded.
* @retval MSG_TIMEOUT  if a timeout occurred before operation end
*
*/
int8_t cam_set_brightness(int8_t value);

/**
* @brief   Sets the contrast of the camera
*
* @param value         Contrast (0..255)
*
* @return              The operation status.
* @retval MSG_OK       if the function succeeded.
* @retval MSG_TIMEOUT  if a timeout occurred before operation end
*
*/
int8_t cam_set_contrast(uint8_t value);

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
int8_t cam_set_mirror(uint8_t vertical, uint8_t horizontal);

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
int8_t cam_set_awb(uint8_t awb);

/**
* @brief   Sets the white balance for the red, green and blue gains.
*
* @param r             red gain.
* @param g             green gain.
* @param b             blue gain.
*
* @return              The operation status.
* @retval MSG_OK       if the function succeeded.
* @retval MSG_TIMEOUT  if a timeout occurred before operation end
*
*/
int8_t cam_set_rgb_gain(uint8_t r, uint8_t g, uint8_t b);

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
int8_t cam_set_ae(uint8_t ae);

/**
* @brief   Sets integration time, aka the exposure.
*          Total integration time is: (integral + fractional/256) * line time.
*
* @param integral      unit is line time.
* @param fractional    unit is 1/256 line time.
*
* @return              The operation status.
* @retval MSG_OK       if the function succeeded.
* @retval MSG_TIMEOUT  if a timeout occurred before operation end
*
*/
int8_t cam_set_exposure(uint16_t integral, uint8_t fractional);

/**
* @brief   Returns the ID of the current camera detected.
*
* @return              Camera ID.
*
*/
uint16_t cam_get_id(void);

/**
* @brief   Get the pointer to the last filled image buffer.
*
* @return	the buffer pointer.
*
*/
uint8_t* cam_get_last_image_ptr(void);

#ifdef __cplusplus
}
#endif

#endif
