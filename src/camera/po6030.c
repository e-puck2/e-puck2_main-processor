#include "po6030.h"
#include "ch.h"
#include "usbcfg.h"
#include "chprintf.h"
#include "../i2c_bus.h"

// These offsets are added to the x and y coordinates to avoid problems with the grabbing.
// These values were found by experimenting with the camera and are valid also with different sub-sampling settings.
// These values should not be modified.
#define X_OFFSET 8
#define Y_OFFSET 4

#define PO6030_ADDR 0x6E

// Shared registers
#define REG_DEVICE_ID_H 0x00
#define REG_DEVICE_ID_L 0x01
#define REG_BANK 0x3
#define BANK_A 0x0
#define BANK_B 0x1
#define BANK_C 0x2
#define BANK_D 0x3

// Bank A registers
#define PO6030_REG_BAYER_CONTROL_01 0x90 // Vertical/horizontal mirror.
#define PO6030_REG_BAYER_CONTROL_02 0x91

// Bank B registers
#define PO6030_REG_FRAMEHEIGHT_H 0x29
#define PO6030_REG_FRAMEHEIGHT_L 0x2A
#define PO6030_REG_ISP_FUNC_2 0x32 // Embossing, sketch, proximity.
#define PO6030_REG_I2C_CONTROL_1 0x34
#define PO6030_REG_FORMAT 0x38
#define PO6030_REG_FRAMEWIDTH_H 0x48
#define PO6030_REG_FRAMEWIDTH_L 0x49
#define PO6030_REG_WINDOWX1_H 0x50
#define PO6030_REG_WINDOWX1_L 0x51
#define PO6030_REG_WINDOWY1_H 0x52
#define PO6030_REG_WINDOWY1_L 0x53
#define PO6030_REG_WINDOWX2_H 0x54
#define PO6030_REG_WINDOWX2_L 0x55
#define PO6030_REG_WINDOWY2_H 0x56
#define PO6030_REG_WINDOWY2_L 0x57
#define PO6030_REG_VSYNCSTARTROW_H 0x60
#define PO6030_REG_VSYNCSTARTROW_L 0x61
#define PO6030_REG_VSYNCSTOPROW_H 0x62
#define PO6030_REG_VSYNCSTOPROW_L 0x63
#define PO6030_REG_SYNC_CONTROL_0 0x68
#define PO6030_REG_SCALE_X 0x80
#define PO6030_REG_SCALE_Y 0x81
#define PO6030_REG_SKETCH_OFFSET_1 0x88
#define PO6030_REG_SKETCH_OFFSET_2 0x89
#define PO6030_REG_SKETCH_OFFSET_3 0x8A
#define PO6030_REG_SKETCH_OFFSET_4 0x8B
#define PO6030_REG_CONTRAST 0x91
#define PO6030_REG_BRIGHTNESS 0x92

// Bank C registers
#define PO6030_REG_AUTO_CONTROL_1 0x04 // AutoWhiteBalance, AutoExposure.
#define PO6030_REG_INTTIME_H 0x4C
#define PO6030_REG_INTTIME_M 0x4D
#define PO6030_REG_INTTIME_L 0x4E
#define PO6030_REG_WB_RGAIN 0xA4
#define PO6030_REG_WB_GGAIN 0xA5
#define PO6030_REG_WB_BGAIN 0xA6
#define PO6030_REG_AE_WINDOW_X_H 0x10
#define PO6030_REG_AE_WINDOW_X_L 0x11
#define PO6030_REG_AE_WINDOW_Y_H 0x12
#define PO6030_REG_AE_WINDOW_Y_L 0x13
#define PO6030_REG_AE_WINDOW_WIDTH_H 0x14
#define PO6030_REG_AE_WINDOW_WIDTH_L 0x15
#define PO6030_REG_AE_WINDOW_HEIGHT_H 0x16
#define PO6030_REG_AE_WINDOW_HEIGHT_L 0x17
#define PO6030_REG_AE_CENTER_WINDOW_X_H 0x18
#define PO6030_REG_AE_CENTER_WINDOW_X_L 0x19
#define PO6030_REG_AE_CENTER_WINDOW_Y_H 0x1A
#define PO6030_REG_AE_CENTER_WINDOW_Y_L 0x1B
#define PO6030_REG_AE_CENTER_WINDOW_WIDTH_H 0x1C
#define PO6030_REG_AE_CENTER_WINDOW_WIDTH_L 0x1D
#define PO6030_REG_AE_CENTER_WINDOW_HEIGHT_H 0x1E
#define PO6030_REG_AE_CENTER_WINDOW_HEIGHT_L 0x1F
#define PO6030_REG_EXPOSURE_T 0x2C
#define PO6030_REG_EXPOSURE_H 0x2D
#define PO6030_REG_EXPOSURE_M 0x2E
#define PO6030_REG_EXPOSURE_L 0x2F
#define PO6030_REG_GLOBALGAIN 0x50
#define PO6030_REG_SATURATION 0xB4


static struct po6030_configuration po6030_conf;
static bool cam_configured = false;

/***************************INTERNAL FUNCTIONS************************************/
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
int8_t po6030_read_id(uint16_t *id) {
    uint8_t regValue[2] = {0};
    int8_t err = 0;

    if((err = read_reg(PO6030_ADDR, REG_DEVICE_ID_H, &regValue[0])) != MSG_OK) {
        return err;
    }
    if((err = read_reg(PO6030_ADDR, REG_DEVICE_ID_L, &regValue[1])) != MSG_OK) {
        return err;
    }
    *id = (((uint16_t)regValue[0])<<8)|regValue[1];

    return MSG_OK;
}

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
int8_t po6030_set_bank(uint8_t bank) {
    return write_reg(PO6030_ADDR, REG_BANK, bank);
}

 /**
 * @brief   Sets the format of the camera
 *
 * @param[in] fmt     format chosen. See format_t
 * 
 * @return              The operation status.
 * @retval MSG_OK       if the function succeeded.
 * @retval MSG_TIMEOUT  if a timeout occurred before operation end.
 *
 */
int8_t po6030_set_format(po6030_format_t fmt) {
    int8_t err = 0;

    if((err = po6030_set_bank(BANK_B)) != MSG_OK) {
        return err;
    }

    if((err = write_reg(PO6030_ADDR, PO6030_REG_FORMAT, fmt)) != MSG_OK) {
        return err;
    }
	
	po6030_conf.curr_format = fmt;

    return MSG_OK;
}

 /**
 * @brief   Sets the camera to work in VGA
 *
 * @return              The operation status.
 * @retval MSG_OK       if the function succeeded.
 * @retval MSG_TIMEOUT  if a timeout occurred before operation end.
 *
 */
int8_t po6030_set_vga(void) {
    int8_t err = 0;

    if((err = po6030_set_bank(BANK_B)) != MSG_OK) {
        return err;
    }
    // Window settings.
    if((err = write_reg(PO6030_ADDR, PO6030_REG_WINDOWX1_H, 0x00)) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_WINDOWX1_L, 0x07)) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_WINDOWY1_H, 0x00)) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_WINDOWY1_L, 0x07)) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_WINDOWX2_H, 0x02)) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_WINDOWX2_L, 0x86)) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_WINDOWY2_H, 0x01)) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_WINDOWY2_L, 0xE6)) != MSG_OK) {
        return err;
    }
    // Scale settings.
    if((err = write_reg(PO6030_ADDR, PO6030_REG_SCALE_X, 0x20)) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_SCALE_Y, 0x20)) != MSG_OK) {
        return err;
    }
    // Vsync start and stop row.
    if((err = write_reg(PO6030_ADDR, PO6030_REG_VSYNCSTARTROW_H, 0x00)) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_VSYNCSTARTROW_L, 0x0C)) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_VSYNCSTOPROW_H, 0x01)) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_VSYNCSTOPROW_L, 0xEC)) != MSG_OK) {
        return err;
    }
    // PCLK rate.
    if(po6030_conf.curr_format == PO6030_FORMAT_YYYY) {
    	if((err = write_reg(PO6030_ADDR, PO6030_REG_SYNC_CONTROL_0, 0x01)) != MSG_OK) {
    		return err;
    	}
    } else {
    	if((err = write_reg(PO6030_ADDR, PO6030_REG_SYNC_CONTROL_0, 0x00)) != MSG_OK) {
    		return err;
    	}
    }
    // Reserved register, set as specified in the datasheet.
    if((err = write_reg(PO6030_ADDR, 0x82, 0x01)) != MSG_OK) {
    	return err;
    }

    if((err = po6030_set_bank(BANK_C)) != MSG_OK) {
        return err;
    }
    // AE window selection.
    if((err = write_reg(PO6030_ADDR, PO6030_REG_AE_WINDOW_X_H, 0x00)) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_AE_WINDOW_X_L, 0x25)) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_AE_WINDOW_Y_H, 0x00)) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_AE_WINDOW_Y_L, 0x1C)) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_AE_WINDOW_WIDTH_H, 0x02)) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_AE_WINDOW_WIDTH_L, 0x60)) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_AE_WINDOW_HEIGHT_H, 0x01)) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_AE_WINDOW_HEIGHT_L, 0xBE)) != MSG_OK) {
        return err;
    }
    // AE center window selection.
    if((err = write_reg(PO6030_ADDR, PO6030_REG_AE_CENTER_WINDOW_X_H, 0x00)) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_AE_CENTER_WINDOW_X_L, 0xE5)) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_AE_CENTER_WINDOW_Y_H, 0x00)) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_AE_CENTER_WINDOW_Y_L, 0x87)) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_AE_CENTER_WINDOW_WIDTH_H, 0x00)) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_AE_CENTER_WINDOW_WIDTH_L, 0xA0)) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_AE_CENTER_WINDOW_HEIGHT_H, 0x00)) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_AE_CENTER_WINDOW_HEIGHT_L, 0xA0)) != MSG_OK) {
        return err;
    }

    if((err = po6030_set_bank(BANK_A)) != MSG_OK) {
        return err;
    }

	po6030_conf.width = 640;
	po6030_conf.height = 480;
	po6030_conf.curr_subsampling_x = SUBSAMPLING_X1;
	po6030_conf.curr_subsampling_y = SUBSAMPLING_X1;
	
    return MSG_OK;
}

 /**
 * @brief   Sets the camera to work in QVGA
 *
 * @return              The operation status.
 * @retval MSG_OK       if the function succeeded.
 * @retval MSG_TIMEOUT  if a timeout occurred before operation end.
 *
 */
int8_t po6030_set_qvga(void) {
    int8_t err = 0;

    if((err = po6030_set_bank(BANK_B)) != MSG_OK) {
        return err;
    }
    // Window settings.
    if((err = write_reg(PO6030_ADDR, PO6030_REG_WINDOWX1_H, 0x00)) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_WINDOWX1_L, 0x04)) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_WINDOWY1_H, 0x00)) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_WINDOWY1_L, 0x04)) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_WINDOWX2_H, 0x01)) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_WINDOWX2_L, 0x43)) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_WINDOWY2_H, 0x00)) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_WINDOWY2_L, 0xF3)) != MSG_OK) {
        return err;
    }
    // Scale settings.
    if((err = write_reg(PO6030_ADDR, PO6030_REG_SCALE_X, 0x40)) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_SCALE_Y, 0x40)) != MSG_OK) {
        return err;
    }
    // Vsync start and stop row.
    if((err = write_reg(PO6030_ADDR, PO6030_REG_VSYNCSTARTROW_H, 0x00)) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_VSYNCSTARTROW_L, 0x0C)) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_VSYNCSTOPROW_H, 0x01)) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_VSYNCSTOPROW_L, 0xEC)) != MSG_OK) {
        return err;
    }
    // PCLK rate.
    if(po6030_conf.curr_format == PO6030_FORMAT_YYYY) {
    	if((err = write_reg(PO6030_ADDR, PO6030_REG_SYNC_CONTROL_0, 0x03)) != MSG_OK) {
    		return err;
    	}
    } else {
    	if((err = write_reg(PO6030_ADDR, PO6030_REG_SYNC_CONTROL_0, 0x01)) != MSG_OK) {
    		return err;
    	}
    }
    // Reserved register, set as specified in the datasheet.
    if((err = write_reg(PO6030_ADDR, 0x82, 0x01)) != MSG_OK) {
    	return err;
    }

    if((err = po6030_set_bank(BANK_C)) != MSG_OK) {
        return err;
    }
    // AE window selection.
    if((err = write_reg(PO6030_ADDR, PO6030_REG_AE_WINDOW_X_H, 0x00)) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_AE_WINDOW_X_L, 0x12)) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_AE_WINDOW_Y_H, 0x00)) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_AE_WINDOW_Y_L, 0x0E)) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_AE_WINDOW_WIDTH_H, 0x01)) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_AE_WINDOW_WIDTH_L, 0x30)) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_AE_WINDOW_HEIGHT_H, 0x00)) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_AE_WINDOW_HEIGHT_L, 0xDF)) != MSG_OK) {
        return err;
    }
    // AE center window selection.
    if((err = write_reg(PO6030_ADDR, PO6030_REG_AE_CENTER_WINDOW_X_H, 0x00)) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_AE_CENTER_WINDOW_X_L, 0x72)) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_AE_CENTER_WINDOW_Y_H, 0x00)) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_AE_CENTER_WINDOW_Y_L, 0x43)) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_AE_CENTER_WINDOW_WIDTH_H, 0x00)) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_AE_CENTER_WINDOW_WIDTH_L, 0x50)) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_AE_CENTER_WINDOW_HEIGHT_H, 0x00)) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_AE_CENTER_WINDOW_HEIGHT_L, 0x50)) != MSG_OK) {
        return err;
    }

    if((err = po6030_set_bank(BANK_A)) != MSG_OK) {
        return err;
    }

	po6030_conf.width = 320;
	po6030_conf.height = 240;
	po6030_conf.curr_subsampling_x = SUBSAMPLING_X2;
	po6030_conf.curr_subsampling_y = SUBSAMPLING_X2;

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
int8_t po6030_set_qqvga(void) {
    int8_t err = 0;

    if((err = po6030_set_bank(BANK_B)) != MSG_OK) {
        return err;
    }
    // Window settings.
    if((err = write_reg(PO6030_ADDR, PO6030_REG_WINDOWX1_H, 0x00)) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_WINDOWX1_L, 0x03)) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_WINDOWY1_H, 0x00)) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_WINDOWY1_L, 0x02)) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_WINDOWX2_H, 0x00)) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_WINDOWX2_L, 0xA2)) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_WINDOWY2_H, 0x00)) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_WINDOWY2_L, 0x79)) != MSG_OK) {
        return err;
    }
    // Scale settings.
    if((err = write_reg(PO6030_ADDR, PO6030_REG_SCALE_X, 0x80)) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_SCALE_Y, 0x80)) != MSG_OK) {
        return err;
    }
    // Vsync start and stop row.
    if((err = write_reg(PO6030_ADDR, PO6030_REG_VSYNCSTARTROW_H, 0x00)) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_VSYNCSTARTROW_L, 0x0C)) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_VSYNCSTOPROW_H, 0x01)) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_VSYNCSTOPROW_L, 0xEC)) != MSG_OK) {
        return err;
    }
    // PCLK rate.
    if(po6030_conf.curr_format == PO6030_FORMAT_YYYY) {
    	if((err = write_reg(PO6030_ADDR, PO6030_REG_SYNC_CONTROL_0, 0x07)) != MSG_OK) {
    		return err;
    	}
    } else {
    	if((err = write_reg(PO6030_ADDR, PO6030_REG_SYNC_CONTROL_0, 0x03)) != MSG_OK) {
    		return err;
    	}
    }
    // Reserved register, set as specified in the datasheet.
    if((err = write_reg(PO6030_ADDR, 0x82, 0x01)) != MSG_OK) {
    	return err;
    }

    if((err = po6030_set_bank(BANK_C)) != MSG_OK) {
        return err;
    }
    // AE window selection.
    if((err = write_reg(PO6030_ADDR, PO6030_REG_AE_WINDOW_X_H, 0x00)) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_AE_WINDOW_X_L, 0x09)) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_AE_WINDOW_Y_H, 0x00)) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_AE_WINDOW_Y_L, 0x07)) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_AE_WINDOW_WIDTH_H, 0x00)) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_AE_WINDOW_WIDTH_L, 0x98)) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_AE_WINDOW_HEIGHT_H, 0x00)) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_AE_WINDOW_HEIGHT_L, 0x6F)) != MSG_OK) {
        return err;
    }
    // AE center window selection.
    if((err = write_reg(PO6030_ADDR, PO6030_REG_AE_CENTER_WINDOW_X_H, 0x00)) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_AE_CENTER_WINDOW_X_L, 0x39)) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_AE_CENTER_WINDOW_Y_H, 0x00)) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_AE_CENTER_WINDOW_Y_L, 0x21)) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_AE_CENTER_WINDOW_WIDTH_H, 0x00)) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_AE_CENTER_WINDOW_WIDTH_L, 0x28)) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_AE_CENTER_WINDOW_HEIGHT_H, 0x00)) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_AE_CENTER_WINDOW_HEIGHT_L, 0x28)) != MSG_OK) {
        return err;
    }

	po6030_conf.width = 160;
	po6030_conf.height = 120;
	po6030_conf.curr_subsampling_x = SUBSAMPLING_X4;
	po6030_conf.curr_subsampling_y = SUBSAMPLING_X4;
	
    return MSG_OK;
}

 /**
 * @brief   Sets the size of the image wanted
 * 
 * @param imgsize       size wanted. See image_size_t
 *
 * @return              The operation status.
 * @retval MSG_OK       if the function succeeded.
 * @retval MSG_TIMEOUT  if a timeout occurred before operation end or if wrong imgsize
 *
 */
int8_t po6030_set_size(image_size_t imgsize) {
    if(imgsize == SIZE_VGA) {
        return po6030_set_vga();
    } else if(imgsize == SIZE_QVGA) {
        return po6030_set_qvga();
    } else if(imgsize == SIZE_QQVGA) {
        return po6030_set_qqvga();
    } else {
		return -1;
	}
}

/*************************END INTERNAL FUNCTIONS**********************************/


/****************************PUBLIC FUNCTIONS*************************************/

void po6030_start(void) {
    // Default camera configuration.
	po6030_advanced_config(PO6030_FORMAT_YCBYCR, 240, 180, 160, 120, SUBSAMPLING_X1, SUBSAMPLING_X1);
}

int8_t po6030_config(po6030_format_t fmt, image_size_t imgsize) {

    int8_t err = 0;

    if((err = po6030_set_format(fmt)) != MSG_OK) {
        return err;
    }

    if((err = po6030_set_size(imgsize)) != MSG_OK) {
        return err;
    }

    return MSG_OK;
}

int8_t po6030_advanced_config(  po6030_format_t fmt, unsigned int x1, unsigned int y1,
                                unsigned int width, unsigned int height, 
								subsampling_t subsampling_x, subsampling_t subsampling_y) {
    int8_t err = MSG_OK;
    x1 += X_OFFSET;
    y1 += Y_OFFSET;
	unsigned int x2 = x1 + width - 1;
	unsigned int y2 = y1 + height - 1;
	unsigned int auto_cw_x1 = 0, auto_cw_x2 = 0;
	unsigned int auto_cw_y1 = 0, auto_cw_y2 = 0;
	
	if(width>PO6030_MAX_WIDTH || height>PO6030_MAX_HEIGHT) {
		return -8;
	}

	if(x1>(PO6030_MAX_WIDTH+X_OFFSET)) {
		return -2;
	}
	
	if(y1>(PO6030_MAX_HEIGHT+Y_OFFSET)) {
		return -3;
	}
	
	if(x2<=x1 || x2>(PO6030_MAX_WIDTH+X_OFFSET)) {
		return -4;
	}
	
	if(y2<=y1 || y2>(PO6030_MAX_HEIGHT+Y_OFFSET)) {
		return -5;
	}	
	
	switch(subsampling_x) {
		case SUBSAMPLING_X1:
			auto_cw_x1 = x1 + (width/3);
			auto_cw_x2 = x1 + (width*2/3);
			break;
			
		case SUBSAMPLING_X2:
			// Check if width is a multiple of the sub-sampling factor.
			if(width%2) {
				return -6;
			} else {
				x1 = x1/2 + 1;
				x2 = x1 + (width/2) - 1;
				auto_cw_x1 = x1 + (width/6);
				auto_cw_x2 = x1 + (width*2/6);
			}
			break;
			
		case SUBSAMPLING_X4:
			// Check if width is a multiple of the sub-sampling factor.
			if(width%4) {
				return -6;
			} else {
				x1 = x1/4 + 1;
				x2 = x1 + (width/4) - 1;
				auto_cw_x1 = x1 + (width/12);
				auto_cw_x2 = x1 + (width*2/12);
			}
			break;
	}
	
	switch(subsampling_y) {
		case SUBSAMPLING_X1:
			auto_cw_y1 = y1 + (height/3);
			auto_cw_y2 = y1 + (height*2/3);
			break;
			
		case SUBSAMPLING_X2:
			// Check if width is a multiple of the sub-sampling factor.
			if(height%2) {
				return -7;
			} else {
				y1 = y1/2 + 1;
				y2 = y1 + (height/2) - 1;
				auto_cw_y1 = y1 + (height/6);
				auto_cw_y2 = y1 + (height*2/6);
			}
			break;
			
		case SUBSAMPLING_X4:
			// Check if width is a multiple of the sub-sampling factor.
			if(height%4) {
				return -7;
			} else {
				y1 = y1/4 + 1;
				y2 = y1 + (height/4) - 1;
				auto_cw_y1 = y1 + (height/12);
				auto_cw_y2 = y1 + (height*2/12);
			}
			break;
	}

    if((err = po6030_set_format(fmt)) != MSG_OK) {
        return err;
    }
	
    if((err = po6030_set_bank(BANK_B)) != MSG_OK) {
        return err;
    }

//    chprintf((BaseSequentialStream *)&SDU1, "x1=%d, x2=%d, y1=%d, y2=%d\r\n", x1, x2, y1, y2);

    // Window settings.
    if((err = write_reg(PO6030_ADDR, PO6030_REG_WINDOWX1_H, (uint8_t)(x1>>8))) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_WINDOWX1_L, (uint8_t)(x1&0xFF))) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_WINDOWY1_H, (uint8_t)(y1>>8))) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_WINDOWY1_L, (uint8_t)(y1&0xFF))) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_WINDOWX2_H, (uint8_t)(x2>>8))) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_WINDOWX2_L, (uint8_t)(x2&0xFF))) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_WINDOWY2_H, (uint8_t)(y2>>8))) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_WINDOWY2_L, (uint8_t)(y2&0xFF))) != MSG_OK) {
        return err;
    }
    // Scale settings.
    if((err = write_reg(PO6030_ADDR, PO6030_REG_SCALE_X, subsampling_x)) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_SCALE_Y, subsampling_y)) != MSG_OK) {
        return err;
    }

//    uint8_t temp_value = 0;
//    read_reg(PO6030_ADDR, PO6030_REG_WINDOWX1_H, &temp_value);
//    chprintf((BaseSequentialStream *)&SDU1, "x1_h=%d\r\n", temp_value);
//    read_reg(PO6030_ADDR, PO6030_REG_WINDOWX1_L, &temp_value);
//    chprintf((BaseSequentialStream *)&SDU1, "x1_l=%d\r\n", temp_value);
//    read_reg(PO6030_ADDR, PO6030_REG_WINDOWY1_H, &temp_value);
//    chprintf((BaseSequentialStream *)&SDU1, "y1_h=%d\r\n", temp_value);
//    read_reg(PO6030_ADDR, PO6030_REG_WINDOWY1_L, &temp_value);
//    chprintf((BaseSequentialStream *)&SDU1, "y1_l=%d\r\n", temp_value);
//    read_reg(PO6030_ADDR, PO6030_REG_WINDOWX2_H, &temp_value);
//    chprintf((BaseSequentialStream *)&SDU1, "x2_h=%d\r\n", temp_value);
//    read_reg(PO6030_ADDR, PO6030_REG_WINDOWX2_L, &temp_value);
//    chprintf((BaseSequentialStream *)&SDU1, "x2_l=%d\r\n", temp_value);
//    read_reg(PO6030_ADDR, PO6030_REG_WINDOWY2_H, &temp_value);
//    chprintf((BaseSequentialStream *)&SDU1, "y2_h=%d\r\n", temp_value);
//    read_reg(PO6030_ADDR, PO6030_REG_WINDOWY2_L, &temp_value);
//    chprintf((BaseSequentialStream *)&SDU1, "y2_l=%d\r\n", temp_value);

    // Vsync start and stop row.
    if((err = write_reg(PO6030_ADDR, PO6030_REG_VSYNCSTARTROW_H, 0x00)) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_VSYNCSTARTROW_L, 0x0C)) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_VSYNCSTOPROW_H, 0x01)) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_VSYNCSTOPROW_L, 0xEC)) != MSG_OK) {
        return err;
    }

    // PCLK rate.
    if(fmt == PO6030_FORMAT_YYYY) {
		switch(subsampling_x) {
			case SUBSAMPLING_X1:
				if((err = write_reg(PO6030_ADDR, PO6030_REG_SYNC_CONTROL_0, 0x01)) != MSG_OK) {
					return err;
				}
				break;

			case SUBSAMPLING_X2:
				if((err = write_reg(PO6030_ADDR, PO6030_REG_SYNC_CONTROL_0, 0x03)) != MSG_OK) {
					return err;
				}
				break;

			case SUBSAMPLING_X4:
				if((err = write_reg(PO6030_ADDR, PO6030_REG_SYNC_CONTROL_0, 0x07)) != MSG_OK) {
					return err;
				}
				break;
		}
    } else {
		switch(subsampling_x) {
			case SUBSAMPLING_X1:
				if((err = write_reg(PO6030_ADDR, PO6030_REG_SYNC_CONTROL_0, 0x00)) != MSG_OK) {
					return err;
				}
				break;

			case SUBSAMPLING_X2:
				if((err = write_reg(PO6030_ADDR, PO6030_REG_SYNC_CONTROL_0, 0x01)) != MSG_OK) {
					return err;
				}
				break;

			case SUBSAMPLING_X4:
				if((err = write_reg(PO6030_ADDR, PO6030_REG_SYNC_CONTROL_0, 0x03)) != MSG_OK) {
					return err;
				}
				break;
		}
    }
    // Reserved register, set as specified in the datasheet.
    if((err = write_reg(PO6030_ADDR, 0x82, 0x01)) != MSG_OK) {
    	return err;
    }

    if((err = po6030_set_bank(BANK_C)) != MSG_OK) {
        return err;
    }
    // AE window selection.
    if((err = write_reg(PO6030_ADDR, PO6030_REG_AE_WINDOW_X_H, (x1>>8))) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_AE_WINDOW_X_L, (x1&0xFF))) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_AE_WINDOW_Y_H, (y1>>8))) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_AE_WINDOW_Y_L, (y1&0xFF))) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_AE_WINDOW_WIDTH_H, ((x2-x1)>>8))) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_AE_WINDOW_WIDTH_L, ((x2-x1)&0xFF))) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_AE_WINDOW_HEIGHT_H, ((y2-y1)>>8))) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_AE_WINDOW_HEIGHT_L, ((y2-y1)&0xFF))) != MSG_OK) {
        return err;
    }
    // AE center window selection.
    if((err = write_reg(PO6030_ADDR, PO6030_REG_AE_CENTER_WINDOW_X_H, (auto_cw_x1>>8))) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_AE_CENTER_WINDOW_X_L, (auto_cw_x1&0xFF))) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_AE_CENTER_WINDOW_Y_H, (auto_cw_y1>>8))) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_AE_CENTER_WINDOW_Y_L, (auto_cw_y1&0xFF))) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_AE_CENTER_WINDOW_WIDTH_H, ((auto_cw_x2-auto_cw_x1)>>8))) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_AE_CENTER_WINDOW_WIDTH_L, ((auto_cw_x2-auto_cw_x1)&0xFF))) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_AE_CENTER_WINDOW_HEIGHT_H, ((auto_cw_y2-auto_cw_y1)>>8))) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_AE_CENTER_WINDOW_HEIGHT_L, ((auto_cw_y2-auto_cw_y1)&0xFF))) != MSG_OK) {
        return err;
    }
	
	po6030_conf.width = x2 - x1 + 1;
	po6030_conf.height = y2 - y1 + 1;
	po6030_conf.curr_format = fmt;
	po6030_conf.curr_subsampling_x = subsampling_x;
	po6030_conf.curr_subsampling_y = subsampling_y;
	
    if(err == MSG_OK){
        cam_configured = true;
    }

//    chprintf((BaseSequentialStream *)&SDU1, "size=%d\r\n", po6030_get_image_size());

    return MSG_OK;
}

/*! Sets brightness.
 * \param value Brightness => [7]:[6:0] = Sign:Magnitude: luminance = Y*contrast + brightness. Default=1, max=127, min=-128.
 */
int8_t po6030_set_brightness(uint8_t value) {
    int8_t err = 0;
    if((err = po6030_set_bank(BANK_B)) != MSG_OK) {
        return err;
    }
    return write_reg(PO6030_ADDR, PO6030_REG_BRIGHTNESS, value);
}

/*! Sets contrast.
 * \param value Contrast => [7:0] = Magnitude: luminance = Y*contrast + brightness. Default=64, max=255, min=0.
 */
int8_t po6030_set_contrast(uint8_t value) {
    int8_t err = 0;
    if((err = po6030_set_bank(BANK_B)) != MSG_OK) {
        return err;
    }
    return write_reg(PO6030_ADDR, PO6030_REG_CONTRAST, value);
}

/*! Sets mirroring for both vertical and horizontal orientations.
 * \param vertical: 1 to enable vertical mirroring
 * \param horizontal: 1 to enable horizontal mirroring
 */
int8_t po6030_set_mirror(uint8_t vertical, uint8_t horizontal) {
    int8_t err = 0;
    uint8_t value = 0x35;
    
    if((err = po6030_set_bank(BANK_A)) != MSG_OK) {
        return err;
    }
    
    if(vertical == 1) {
        value |= 0x80;
    }
    if(horizontal == 1) {
        value |= 0x40;
    }
    
    return write_reg(PO6030_ADDR, PO6030_REG_BAYER_CONTROL_01, value);
}

/*! Enable/disable auto white balance.
 * \param awb: 1 to enable auto white balance.
 */
int8_t po6030_set_awb(uint8_t awb) {
    int8_t err = 0;
    uint8_t value = 0;
    
    if((err = po6030_set_bank(BANK_C)) != MSG_OK) {
        return err;
    }
    
    if((err = read_reg(PO6030_ADDR, PO6030_REG_AUTO_CONTROL_1, &value)) != MSG_OK) {
        return err;
    }
    
    if(awb == 1) {
        value &= ~0x04;
    } else {
        value |= 0x04;
    }
    
    return write_reg(PO6030_ADDR, PO6030_REG_AUTO_CONTROL_1, value);
}

/*! Sets white balance red, green, blue gain. 
 *  These values are considered only when auto white balance is disabled, so this function also disables auto white balance.
 * \param r: red gain: value/64 (max=4, min=0). Default is 0x5E.
 * \param g: green gain: value/64 (max=4, min=0). Default is 0x40.
 * \param b: blue gain: value/64 (max=4, min=0). Default is 0x5D.
 */
int8_t po6030_set_rgb_gain(uint8_t r, uint8_t g, uint8_t b) {
    int8_t err = 0;
    
    if((err = po6030_set_awb(0)) != MSG_OK) {
        return err;
    }
    
    if((err = po6030_set_bank(BANK_C)) != MSG_OK) {
        return err;
    }
    
    if((err = write_reg(PO6030_ADDR, PO6030_REG_WB_RGAIN, r)) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_WB_GGAIN, g)) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_WB_BGAIN, b)) != MSG_OK) {
        return err;
    }   

    return MSG_OK;
}

/*! Enables/disables auto exposure.
 * \param ae: 1 to enable auto exposure.
 */
int8_t po6030_set_ae(uint8_t ae) {
    int8_t err = 0;
    uint8_t value = 0;
    
    if((err = po6030_set_bank(BANK_C)) != MSG_OK) {
        return err;
    }
    
    if((err = read_reg(PO6030_ADDR, PO6030_REG_AUTO_CONTROL_1, &value)) != MSG_OK) {
        return err;
    }
    
    if(ae == 1) {
        value &= ~0x03;
    } else {
        value |= 0x03;
    }
    
    return write_reg(PO6030_ADDR, PO6030_REG_AUTO_CONTROL_1, value);
}

/*! Sets integration time. Total integration time is: (integral + fractional/256) line time. 
 *  These values are considered only when auto exposure is disabled, so this function also disables auto exposure.
 * \param integral: unit is line time. Default is 0x0080 (128).
 * \param fractional: unit is 1/256 line time. Default is 0x00 (0).
 */
int8_t po6030_set_exposure(uint16_t integral, uint8_t fractional) {
    int8_t err = 0;
    
    if((err = po6030_set_ae(0)) != MSG_OK) {
        return err;
    }
    
    if((err = po6030_set_bank(BANK_C)) != MSG_OK) {
        return err;
    }
    
    if((err = write_reg(PO6030_ADDR, PO6030_REG_GLOBALGAIN, 0x27)) != MSG_OK) {	// Max value for globalgain is 0x4F.
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_INTTIME_H, (integral>>8))) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_INTTIME_M, (integral&0xFF))) != MSG_OK) {
        return err;
    }
    if((err = write_reg(PO6030_ADDR, PO6030_REG_INTTIME_L, fractional)) != MSG_OK) {
        return err;
    }

//    if((err = write_reg(PO6030_ADDR, PO6030_REG_EXPOSURE_T, (integral>>8))) != MSG_OK) {
//        return err;
//    }
//    if((err = write_reg(PO6030_ADDR, PO6030_REG_EXPOSURE_H, (integral&0xFF))) != MSG_OK) {
//        return err;
//    }
//    if((err = write_reg(PO6030_ADDR, PO6030_REG_EXPOSURE_M, (fractional>>8))) != MSG_OK) {
//        return err;
//    }
//    if((err = write_reg(PO6030_ADDR, PO6030_REG_EXPOSURE_L, (fractional&0xFF))) != MSG_OK) {
//        return err;
//    }

    return MSG_OK;
}

/*! Returns the current image size in bytes.
 */
uint32_t po6030_get_image_size(void) {
    if(po6030_conf.curr_format == PO6030_FORMAT_YYYY) {
        return (uint32_t)po6030_conf.width * (uint32_t)po6030_conf.height;
    } else {
        return (uint32_t)po6030_conf.width * (uint32_t)po6030_conf.height * 2;
    }
}

uint8_t po6030_is_connected(void) {
	uint16_t id = 0;
	int8_t res = po6030_read_id(&id);
	if((res==MSG_OK) && (id==0x6030)) {
		return 1;
	} else {
		return 0;
	}
}

/**************************END PUBLIC FUNCTIONS***********************************/

