#include "ov2640.h"
#include "ch.h"
#include "usbcfg.h"
#include "chprintf.h"
#include "../i2c_bus.h"

#define OV2640_ADDR 0x30

/**
  * @brief  OV2640 Registers
  */
/* OV2640 Registers definition when DSP bank selected (0xFF = 0x00) */
#define OV2640_DSP_R_BYPASS             0x05
#define OV2640_DSP_Qs                   0x44
#define OV2640_DSP_CTRL                 0x50
#define OV2640_DSP_HSIZE1               0x51
#define OV2640_DSP_VSIZE1               0x52
#define OV2640_DSP_XOFFL                0x53
#define OV2640_DSP_YOFFL                0x54
#define OV2640_DSP_VHYX                 0x55
#define OV2640_DSP_DPRP                 0x56
#define OV2640_DSP_TEST                 0x57
#define OV2640_DSP_ZMOW                 0x5A
#define OV2640_DSP_ZMOH                 0x5B
#define OV2640_DSP_ZMHH                 0x5C
#define OV2640_DSP_BPADDR               0x7C
#define OV2640_DSP_BPDATA               0x7D
#define OV2640_DSP_CTRL2                0x86
#define OV2640_DSP_CTRL3                0x87
#define OV2640_DSP_SIZEL                0x8C
#define OV2640_DSP_HSIZE2               0xC0
#define OV2640_DSP_VSIZE2               0xC1
#define OV2640_DSP_CTRL0                0xC2
#define OV2640_DSP_CTRL1                0xC3
#define OV2640_DSP_R_DVP_SP             0xD3
#define OV2640_DSP_IMAGE_MODE           0xDA
#define OV2640_DSP_RESET                0xE0
#define OV2640_DSP_MS_SP                0xF0
#define OV2640_DSP_SS_ID                0x7F
#define OV2640_DSP_SS_CTRL              0xF8
#define OV2640_DSP_MC_BIST              0xF9
#define OV2640_DSP_MC_AL                0xFA
#define OV2640_DSP_MC_AH                0xFB
#define OV2640_DSP_MC_D                 0xFC
#define OV2640_DSP_P_STATUS             0xFE
#define OV2640_DSP_RA_DLMT              0xFF

/* OV2640 Registers definition when sensor bank selected (0xFF = 0x01) */
#define OV2640_SENSOR_GAIN              0x00
#define OV2640_SENSOR_COM1              0x03
#define OV2640_SENSOR_REG04             0x04
#define OV2640_SENSOR_REG08             0x08
#define OV2640_SENSOR_COM2              0x09
#define OV2640_SENSOR_PIDH              0x0A
#define OV2640_SENSOR_PIDL              0x0B
#define OV2640_SENSOR_COM3              0x0C
#define OV2640_SENSOR_COM4              0x0D
#define OV2640_SENSOR_AEC               0x10
#define OV2640_SENSOR_CLKRC             0x11
#define OV2640_SENSOR_COM7              0x12
#define OV2640_SENSOR_COM8              0x13
#define OV2640_SENSOR_COM9              0x14
#define OV2640_SENSOR_COM10             0x15
#define OV2640_SENSOR_HREFST            0x17
#define OV2640_SENSOR_HREFEND           0x18
#define OV2640_SENSOR_VSTART            0x19
#define OV2640_SENSOR_VEND              0x1A
#define OV2640_SENSOR_MIDH              0x1C
#define OV2640_SENSOR_MIDL              0x1D
#define OV2640_SENSOR_AEW               0x24
#define OV2640_SENSOR_AEB               0x25
#define OV2640_SENSOR_W                 0x26
#define OV2640_SENSOR_REG2A             0x2A
#define OV2640_SENSOR_FRARL             0x2B
#define OV2640_SENSOR_ADDVSL            0x2D
#define OV2640_SENSOR_ADDVHS            0x2E
#define OV2640_SENSOR_YAVG              0x2F
#define OV2640_SENSOR_REG32             0x32
#define OV2640_SENSOR_ARCOM2            0x34
#define OV2640_SENSOR_REG45             0x45
#define OV2640_SENSOR_FLL               0x46
#define OV2640_SENSOR_FLH               0x47
#define OV2640_SENSOR_COM19             0x48
#define OV2640_SENSOR_ZOOMS             0x49
#define OV2640_SENSOR_COM22             0x4B
#define OV2640_SENSOR_COM25             0x4E
#define OV2640_SENSOR_BD50              0x4F
#define OV2640_SENSOR_BD60              0x50
#define OV2640_SENSOR_REG5D             0x5D
#define OV2640_SENSOR_REG5E             0x5E
#define OV2640_SENSOR_REG5F             0x5F
#define OV2640_SENSOR_REG60             0x60
#define OV2640_SENSOR_HISTO_LOW         0x61
#define OV2640_SENSOR_HISTO_HIGH        0x62

/**
 * @brief  OV2640 Features Parameters
 */
#define OV2640_BRIGHTNESS_LEVEL0        0x40   /* Brightness level -2         */
#define OV2640_BRIGHTNESS_LEVEL1        0x30   /* Brightness level -1         */
#define OV2640_BRIGHTNESS_LEVEL2        0x20   /* Brightness level 0          */
#define OV2640_BRIGHTNESS_LEVEL3        0x10   /* Brightness level +1         */
#define OV2640_BRIGHTNESS_LEVEL4        0x00   /* Brightness level +2         */

#define OV2640_BLACK_WHITE_BW           0x18   /* Black and white effect      */
#define OV2640_BLACK_WHITE_NEGATIVE     0x40   /* Negative effect             */
#define OV2640_BLACK_WHITE_BW_NEGATIVE  0x58   /* BW and Negative effect      */
#define OV2640_BLACK_WHITE_NORMAL       0x00   /* Normal effect               */

#define OV2640_CONTRAST_LEVEL0          0x3418 /* Contrast level -2           */
#define OV2640_CONTRAST_LEVEL1          0x2A1C /* Contrast level -2           */
#define OV2640_CONTRAST_LEVEL2          0x2020 /* Contrast level -2           */
#define OV2640_CONTRAST_LEVEL3          0x1624 /* Contrast level -2           */
#define OV2640_CONTRAST_LEVEL4          0x0C28 /* Contrast level -2           */

#define OV2640_COLOR_EFFECT_ANTIQUE     0xA640 /* Antique effect              */
#define OV2640_COLOR_EFFECT_BLUE        0x40A0 /* Blue effect                 */
#define OV2640_COLOR_EFFECT_GREEN       0x4040 /* Green effect                */
#define OV2640_COLOR_EFFECT_RED         0xC040 /* Red effect                  */
/**
  * @}
  */

#define OV2640_BANK_DSP 0x0
#define OV2640_BANK_SENSOR 0x1


static struct ov2640_configuration ov2640_conf;
static bool cam_configured = false;

/* Initialization sequence for QQVGA resolution (160x120) */
const char OV2640_QQVGA[][2]=
{
  {0xff, 0x00},	// DSP bank (register list Table 12)
  {0x2c, 0xff},	// Reserved
  {0x2e, 0xdf},	// Reserved
  {0xff, 0x01},	// Sensor bank (register list Table 13)
  {0x3c, 0x32},	// Reserved
  {0x11, 0x00},	// Clock Rate Control => CLK = XVCLK/(0x00 + 1) = XVCLK
  {0x09, 0x02}, // Common control 2 => 2x capability...?
  {0x04, 0xA8},	// Mirror => horizontal mirroring...?
  {0x13, 0xe5},	// Common control 8 => Banding filter on, auto AGC, auto exposure...? Should be 0xe7 since bit1 is reserved
  {0x14, 0x48}, // Common control 9 => AGC gain = 8x...should be 0x40 since bit[4:0] are reserved
  {0x2c, 0x0c},
  {0x33, 0x78},
  {0x3a, 0x33},
  {0x3b, 0xfB},
  {0x3e, 0x00},
  {0x43, 0x11},
  {0x16, 0x10},
  {0x4a, 0x81},
  {0x21, 0x99},
  {0x24, 0x40},
  {0x25, 0x38},
  {0x26, 0x82},
  {0x5c, 0x00},
  {0x63, 0x00},
  {0x46, 0x3f},
  {0x0c, 0x3c},
  {0x61, 0x70},
  {0x62, 0x80},
  {0x7c, 0x05},
  {0x20, 0x80},
  {0x28, 0x30},
  {0x6c, 0x00},
  {0x6d, 0x80},
  {0x6e, 0x00},
  {0x70, 0x02},
  {0x71, 0x94},
  {0x73, 0xc1},
  {0x3d, 0x34},
  {0x5a, 0x57},
  {0x12, 0x00},
  {0x11, 0x00},
  {0x17, 0x11},
  {0x18, 0x75},
  {0x19, 0x01},
  {0x1a, 0x97},
  {0x32, 0x36},
  {0x03, 0x0f},
  {0x37, 0x40},
  {0x4f, 0xbb},
  {0x50, 0x9c},
  {0x5a, 0x57},
  {0x6d, 0x80},
  {0x6d, 0x38},
  {0x39, 0x02},
  {0x35, 0x88},
  {0x22, 0x0a},
  {0x37, 0x40},
  {0x23, 0x00},
  {0x34, 0xa0},
  {0x36, 0x1a},
  {0x06, 0x02},
  {0x07, 0xc0},
  {0x0d, 0xb7},
  {0x0e, 0x01},
  {0x4c, 0x00},
  {0xff, 0x00},
  {0xe5, 0x7f},
  {0xf9, 0xc0},
  {0x41, 0x24},
  {0xe0, 0x14},
  {0x76, 0xff},
  {0x33, 0xa0},
  {0x42, 0x20},
  {0x43, 0x18},
  {0x4c, 0x00},
  {0x87, 0xd0},
  {0x88, 0x3f},
  {0xd7, 0x03},
  {0xd9, 0x10},
  {0xd3, 0x82},
  {0xc8, 0x08},
  {0xc9, 0x80},
  {0x7d, 0x00},
  {0x7c, 0x03},
  {0x7d, 0x48},
  {0x7c, 0x08},
  {0x7d, 0x20},
  {0x7d, 0x10},
  {0x7d, 0x0e},
  {0x90, 0x00},
  {0x91, 0x0e},
  {0x91, 0x1a},
  {0x91, 0x31},
  {0x91, 0x5a},
  {0x91, 0x69},
  {0x91, 0x75},
  {0x91, 0x7e},
  {0x91, 0x88},
  {0x91, 0x8f},
  {0x91, 0x96},
  {0x91, 0xa3},
  {0x91, 0xaf},
  {0x91, 0xc4},
  {0x91, 0xd7},
  {0x91, 0xe8},
  {0x91, 0x20},
  {0x92, 0x00},
  {0x93, 0x06},
  {0x93, 0xe3},
  {0x93, 0x02},
  {0x93, 0x02},
  {0x93, 0x00},
  {0x93, 0x04},
  {0x93, 0x00},
  {0x93, 0x03},
  {0x93, 0x00},
  {0x93, 0x00},
  {0x93, 0x00},
  {0x93, 0x00},
  {0x93, 0x00},
  {0x93, 0x00},
  {0x93, 0x00},
  {0x96, 0x00},
  {0x97, 0x08},
  {0x97, 0x19},
  {0x97, 0x02},
  {0x97, 0x0c},
  {0x97, 0x24},
  {0x97, 0x30},
  {0x97, 0x28},
  {0x97, 0x26},
  {0x97, 0x02},
  {0x97, 0x98},
  {0x97, 0x80},
  {0x97, 0x00},
  {0x97, 0x00},
  {0xc3, 0xef},
  {0xff, 0x00},
  {0xba, 0xdc},
  {0xbb, 0x08},
  {0xb6, 0x24},
  {0xb8, 0x33},
  {0xb7, 0x20},
  {0xb9, 0x30},
  {0xb3, 0xb4},
  {0xb4, 0xca},
  {0xb5, 0x43},
  {0xb0, 0x5c},
  {0xb1, 0x4f},
  {0xb2, 0x06},
  {0xc7, 0x00},
  {0xc6, 0x51},
  {0xc5, 0x11},
  {0xc4, 0x9c},
  {0xbf, 0x00},
  {0xbc, 0x64},
  {0xa6, 0x00},
  {0xa7, 0x1e},
  {0xa7, 0x6b},
  {0xa7, 0x47},
  {0xa7, 0x33},
  {0xa7, 0x00},
  {0xa7, 0x23},
  {0xa7, 0x2e},
  {0xa7, 0x85},
  {0xa7, 0x42},
  {0xa7, 0x33},
  {0xa7, 0x00},
  {0xa7, 0x23},
  {0xa7, 0x1b},
  {0xa7, 0x74},
  {0xa7, 0x42},
  {0xa7, 0x33},
  {0xa7, 0x00},
  {0xa7, 0x23},
  {0xc0, 0xc8},
  {0xc1, 0x96},
  {0x8c, 0x00},
  {0x86, 0x3d},
  {0x50, 0x92},
  {0x51, 0x90},
  {0x52, 0x2c},
  {0x53, 0x00},
  {0x54, 0x00},
  {0x55, 0x88},
  {0x5a, 0x50},
  {0x5b, 0x3c},
  {0x5c, 0x00},
  {0xd3, 0x04},
  {0x7f, 0x00},
  {0xda, 0x00},
  {0xe5, 0x1f},
  {0xe1, 0x67},
  {0xe0, 0x00},
  {0xdd, 0x7f},
  {0x05, 0x00},
  {0xff, 0x00},
  {0xe0, 0x04},
  {0xc0, 0xc8},
  {0xc1, 0x96},
  {0x86, 0x3d},
  {0x50, 0x92},
  {0x51, 0x90},
  {0x52, 0x2c},
  {0x53, 0x00},
  {0x54, 0x00},
  {0x55, 0x88},
  {0x57, 0x00},
  {0x5a, 0x28},
  {0x5b, 0x1E},
  {0x5c, 0x00},
  {0xd3, 0x08},
  {0xe0, 0x00},
  {0xFF, 0x00},
  {0x05, 0x00},
  {0xDA, 0x08},
  {0xda, 0x09},
  {0x98, 0x00},
  {0x99, 0x00},
  {0x00, 0x00},
};


/***************************INTERNAL FUNCTIONS************************************/
/**
* @brief   Sets the bank of the camera.
*
* @param[in] id     bank
*
* @return              The operation status.
* @retval MSG_OK       if the function succeeded.
* @retval MSG_TIMEOUT  if a timeout occurred before operation end.
*
*/
int8_t ov2640_set_bank(uint8_t bank) {
	return write_reg(OV2640_ADDR, OV2640_DSP_RA_DLMT, bank);
}

 /**
 * @brief   Reads the id of the camera
 *
 * @param[out] id     pointer to store the value
 * 
 * @return              The operation status.
 * @retval MSG_OK       if the function succeeded.
 * @retval MSG_TIMEOUT  if a timeout occurred before operation end.
 *
 */
int8_t ov2640_read_id(uint16_t *id) {
    uint8_t regValue[2] = {0};
    int8_t err = 0;

    ov2640_set_bank(OV2640_BANK_SENSOR);

    if((err = read_reg(OV2640_ADDR, OV2640_SENSOR_PIDH, &regValue[0])) != MSG_OK) {
        return err;
    }
    if((err = read_reg(OV2640_ADDR, OV2640_SENSOR_PIDL, &regValue[1])) != MSG_OK) {
        return err;
    }
    *id = (((uint16_t)regValue[0])<<8)|regValue[1];

    return MSG_OK;
}

  /**
 * @brief   Sets the camera to work in QQVGA
 *
 * @return              The operation status.
 * @retval MSG_OK       if the function succeeded.
 * @retval MSG_TIMEOUT  if a timeout occurred before operation end.
 *
 */
int8_t ov2640_set_qqvga(void) {
    int8_t err = 0;
    uint32_t i=0;
	for(i=0; i<(sizeof(OV2640_QQVGA)/2); i++) {
		write_reg(OV2640_ADDR, OV2640_QQVGA[i][0], OV2640_QQVGA[i][1]);
	}

    ov2640_conf.width = 160;
    ov2640_conf.height = 120;
	ov2640_conf.curr_subsampling_x = SUBSAMPLING_X4;
	ov2640_conf.curr_subsampling_y = SUBSAMPLING_X4;
	
    return MSG_OK;
}

/*************************END INTERNAL FUNCTIONS**********************************/


/****************************PUBLIC FUNCTIONS*************************************/

void ov2640_start(void) {
    // Default camera configuration.

//	write_reg(0x30, 0xFF, 0x01);
//	write_reg(0x30, 0x12, 0x80); // Reset the camera registers.
//	chThdSleepMilliseconds(1000);

	ov2640_conf.curr_format = OV2640_FORMAT_RGB565;
	ov2640_set_qqvga();
}

/*! Returns the current image size in bytes.
 */
uint32_t ov2640_get_image_size(void) {
    if(ov2640_conf.curr_format == OV2640_FORMAT_Y8) {
        return (uint32_t)ov2640_conf.width * (uint32_t)ov2640_conf.height;
    } else {
        return (uint32_t)ov2640_conf.width * (uint32_t)ov2640_conf.height * 2;
    }
}

uint8_t ov2640_is_connected(void) {
	uint16_t id = 0;
	int8_t res = ov2640_read_id(&id);
	if((res==MSG_OK) && (id==0x2641)) {
		return 1;
	} else {
		return 0;
	}
}

/**************************END PUBLIC FUNCTIONS***********************************/

