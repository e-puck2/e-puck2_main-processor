#ifndef PO8030_H
#define PO8030_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "hal.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PO8030_ADDR 0x6E
#define PO8030_MAX_WIDTH 640
#define PO8030_MAX_HEIGHT 480

// Shared registers
#define REG_DEVICE_ID_H 0x00
#define REG_DEVICE_ID_L 0x01
#define REG_BANK 0x3
#define BANK_A 0x0
#define BANK_B 0x1
#define BANK_C 0x2
#define BANK_D 0x3
// Bank A registers
#define PO8030_REG_FRAMEWIDTH_H 0x04
#define PO8030_REG_FRAMEWIDTH_L 0x05
#define PO8030_REG_FRAMEHEIGHT_H 0x06
#define PO8030_REG_FRAMEHEIGHT_L 0x07
#define PO8030_REG_WINDOWX1_H 0x08
#define PO8030_REG_WINDOWX1_L 0x09
#define PO8030_REG_WINDOWY1_H 0x0A
#define PO8030_REG_WINDOWY1_L 0x0B
#define PO8030_REG_WINDOWX2_H 0x0C
#define PO8030_REG_WINDOWX2_L 0x0D
#define PO8030_REG_WINDOWY2_H 0x0E
#define PO8030_REG_WINDOWY2_L 0x0F
#define PO8030_REG_VSYNCSTARTROW_H 0x10
#define PO8030_REG_VSYNCSTARTROW_L 0x11
#define PO8030_REG_VSYNCSTOPROW_H 0x12
#define PO8030_REG_VSYNCSTOPROW_L 0x13
#define PO8030_REG_INTTIME_H 0x17
#define PO8030_REG_INTTIME_M 0x18
#define PO8030_REG_INTTIME_L 0x19
#define PO8030_REG_WB_RGAIN 0x23
#define PO8030_REG_WB_GGAIN 0x24
#define PO8030_REG_WB_BGAIN 0x25
#define PO8030_REG_AUTO_FWX1_H 0x35
#define PO8030_REG_AUTO_FWX1_L 0x36
#define PO8030_REG_AUTO_FWX2_H 0x37
#define PO8030_REG_AUTO_FWX2_L 0x38
#define PO8030_REG_AUTO_FWY1_H 0x39
#define PO8030_REG_AUTO_FWY1_L 0x3A
#define PO8030_REG_AUTO_FWY2_H 0x3B
#define PO8030_REG_AUTO_FWY2_L 0x3C
#define PO8030_REG_AUTO_CWX1_H 0x3D
#define PO8030_REG_AUTO_CWX1_L 0x3E
#define PO8030_REG_AUTO_CWX2_H 0x3F
#define PO8030_REG_AUTO_CWX2_L 0x40
#define PO8030_REG_AUTO_CWY1_H 0x41
#define PO8030_REG_AUTO_CWY1_L 0x42
#define PO8030_REG_AUTO_CWY2_H 0x43
#define PO8030_REG_AUTO_CWY2_L 0x44
#define PO8030_REG_PAD_CONTROL 0x5B
#define PO8030_REG_SOFTRESET 0x69
#define PO8030_REG_CLKDIV 0x6A
#define PO8030_REG_BAYER_CONTROL_01 0x6C // Vertical/horizontal mirror.
// Bank B registers
#define PO8030_REG_ISP_FUNC_2 0x06 // Embossing, sketch, proximity.
#define PO8030_REG_FORMAT 0x4E
#define PO8030_REG_SKETCH_OFFSET 0x8F
#define PO8030_REG_SCALE_X 0x93
#define PO8030_REG_SCALE_Y 0x94
#define PO8030_REG_SCALE_TH_H 0x95
#define PO8030_REG_SCALE_TH_L 0x96
#define PO8030_REG_CONTRAST 0x9D
#define PO8030_REG_BRIGHTNESS 0x9E
#define PO8030_REG_SYNC_CONTROL0 0xB7
// Bank C registers
#define PO8030_REG_AUTO_CONTROL_1 0x04 // AutoWhiteBalance, AutoExposure.
#define PO8030_REG_EXPOSURE_T 0x12
#define PO8030_REG_EXPOSURE_H 0x13
#define PO8030_REG_EXPOSURE_M 0x14
#define PO8030_REG_EXPOSURE_L 0x15
#define PO8030_REG_SATURATION 0x2C

typedef enum {
    FORMAT_CBYCRY = 0x00,
    FORMAT_CRYCBY = 0x01,
    FORMAT_YCBYCR = 0x02,
    FORMAT_YCRYCB = 0x03,
    FORMAT_RGGB = 0x10,
    FORMAT_GBRG = 0x11,
    FORMAT_GRBG = 0x12,
    FORMAT_BGGR = 0x13,
    FORMAT_RGB565 = 0x30,
    FORMAT_RGB565_BYTE_SWAP = 0x31,
    FORMAT_BGR565 = 0x32,
    FORMAT_BGR565_BYTE_SWAP = 0x33,
    FORMAT_RGB444 = 0x36,
    FORMAT_RGB444_BYTE_SWAP = 0x37,
    FORMAT_DPC_BAYER = 0x41,
    FORMAT_YYYY = 0x44
} format_t;

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

struct po8030_configuration {
	uint16_t 		width;
	uint16_t 		height;
	format_t 		curr_format;
	subsampling_t 	curr_subsampling_x;
	subsampling_t 	curr_subsampling_y;
};

void po8030_start(void);
int8_t po8030_read_id(uint16_t *id);
int8_t po8030_set_bank(uint8_t bank);
int8_t po8030_set_format(format_t fmt);
int8_t po8030_set_size(image_size_t imgsize);
int8_t po8030_set_vga(void);
int8_t po8030_set_qvga(void);
int8_t po8030_set_qqvga(void);
int8_t po8030_set_scale_buffer_size(format_t fmt, image_size_t imgsize);
int8_t po8030_config(format_t fmt, image_size_t imgsize);
i2cflags_t get_last_i2c_error(void);
int8_t po8030_set_brightness(uint8_t value);
int8_t po8030_set_contrast(uint8_t value);
int8_t po8030_advanced_config(format_t fmt, unsigned int x1, unsigned int y1, unsigned int width, unsigned int height, subsampling_t subsampling_x, subsampling_t subsampling_y);
int8_t po8030_set_mirror(uint8_t vertical, uint8_t horizontal);
int8_t po8030_set_awb(uint8_t awb);
int8_t po8030_set_rgb_gain(uint8_t r, uint8_t g, uint8_t b);
int8_t po8030_set_ae(uint8_t ae);
int8_t po8030_set_exposure(uint16_t integral, uint8_t fractional);
uint32_t po8030_get_image_size(void);

// Utility functions used with the shell.
void po8030_save_current_format(format_t fmt);
format_t po8030_get_saved_format(void);
void po8030_save_current_subsampling(subsampling_t x, subsampling_t y);
subsampling_t po8030_get_saved_subsampling_x(void);
subsampling_t po8030_get_saved_subsampling_y(void);

#ifdef __cplusplus
}
#endif

#endif
