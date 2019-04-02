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
int8_t cam_advanced_config(format_t fmt, unsigned int x1, unsigned int y1,
                                unsigned int width, unsigned int height,
								subsampling_t subsampling_x, subsampling_t subsampling_y);

int8_t cam_set_brightness(uint8_t value);
int8_t cam_set_contrast(uint8_t value);
int8_t cam_set_mirror(uint8_t vertical, uint8_t horizontal);
int8_t cam_set_awb(uint8_t awb);
int8_t cam_set_rgb_gain(uint8_t r, uint8_t g, uint8_t b);
int8_t cam_set_ae(uint8_t ae);
int8_t cam_set_exposure(uint16_t integral, uint8_t fractional);
uint16_t cam_get_id(void);

#ifdef __cplusplus
}
#endif

#endif
