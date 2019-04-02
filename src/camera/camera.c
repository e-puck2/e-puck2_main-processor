#include <hal.h>
#include "camera.h"
#include "../i2c_bus.h"
#include "ch.h"
#include "usbcfg.h"
#include "chprintf.h"
#include "po8030.h"
#include "po6030.h"

#define CAM_PO8030 0
#define CAM_PO6030 1

int8_t curr_cam = -1;


/***************************INTERNAL FUNCTIONS************************************/


/*************************END INTERNAL FUNCTIONS**********************************/


/****************************PUBLIC FUNCTIONS*************************************/

void cam_start(void) {
	i2c_start();

    // Timer initialization to clock the camera.
	// Need to be configured here in order to read the camera registers.
    static const PWMConfig pwmcfg_cam = {
        .frequency = 42000000,  //42MHz
        .period = 0x02,         //PWM period = 42MHz/2 => 21MHz
        .cr2 = 0,
        .callback = NULL,
        .channels = {
            // Channel 1 is used as master clock for the camera.
            {.mode = PWM_OUTPUT_ACTIVE_HIGH, .callback = NULL},
            {.mode = PWM_OUTPUT_DISABLED, .callback = NULL},
            {.mode = PWM_OUTPUT_DISABLED, .callback = NULL},
            {.mode = PWM_OUTPUT_DISABLED, .callback = NULL},
        },
    };
    pwmStart(&PWMD5, &pwmcfg_cam);
    // Enables channel 1 to clock the camera.
    pwmEnableChannel(&PWMD5, 0, 1); //1 is half the period set => duty cycle = 50%
    chThdSleepMilliseconds(1000); // Give time for the clock to be stable and the camera to wake-up.

    if(po8030_is_connected() == 1) {
    	curr_cam = CAM_PO8030;
    	po8030_start();
    	//chprintf((BaseSequentialStream *)&SDU1, "po8030 detected\r\n");
    } else if(po6030_is_connected() == 1) {
    	curr_cam = CAM_PO6030;
    	po6030_start();
    	po6030_set_mirror(1, 1);
    	//chprintf((BaseSequentialStream *)&SDU1, "po6030 detected\r\n");
    } else {
//    	write_reg(0x30, 0xFF, 0x01);
//    	uint8_t regValue[2] = {0};
//    	read_reg(0x30, 0x0A, &regValue[0]);
//    	read_reg(0x30, 0x0B, &regValue[1]);
//    	chprintf((BaseSequentialStream *)&SDU1, "H=%x, L=%x\r\n", regValue[0], regValue[1]);
    }

}

int8_t cam_config(format_t fmt, image_size_t imgsize) {
	if(curr_cam == CAM_PO8030) {
		if(fmt == FORMAT_GREYSCALE) {
			return po8030_config(PO8030_FORMAT_YYYY, imgsize);
		} else {
			return po8030_config(PO8030_FORMAT_RGB565, imgsize);
		}
	} else if(curr_cam == CAM_PO6030) {
		if(fmt == FORMAT_GREYSCALE) {
			return po6030_config(PO6030_FORMAT_YYYY, imgsize);
		} else {
			return po6030_config(PO6030_FORMAT_RGB565, imgsize);
		}
	}
	return -1;
}

uint32_t cam_get_image_size(void) {
	if(curr_cam == CAM_PO8030) {
		return po8030_get_image_size();
	} else if(curr_cam == CAM_PO6030) {
		return po6030_get_image_size();
	} else {
		return 0;
	}
}

int8_t cam_advanced_config(format_t fmt, unsigned int x1, unsigned int y1,
                                unsigned int width, unsigned int height,
								subsampling_t subsampling_x, subsampling_t subsampling_y) {
	if(curr_cam == CAM_PO8030) {
		if(fmt == FORMAT_GREYSCALE) {
			return po8030_advanced_config(PO8030_FORMAT_YYYY, x1, y1, width, height, subsampling_x, subsampling_y);
		} else {
			return po8030_advanced_config(PO8030_FORMAT_RGB565, x1, y1, width, height, subsampling_x, subsampling_y);
		}
	} else if(curr_cam == CAM_PO6030) {
		if(fmt == FORMAT_GREYSCALE) {
			return po6030_advanced_config(PO6030_FORMAT_YYYY, x1, y1, width, height, subsampling_x, subsampling_y);
		} else {
			return po6030_advanced_config(PO6030_FORMAT_RGB565, x1, y1, width, height, subsampling_x, subsampling_y);
		}
	}
	return -1;
}

int8_t cam_set_brightness(uint8_t value) {
	if(curr_cam == CAM_PO8030) {
		return po8030_set_brightness(value);
	} else if(curr_cam == CAM_PO6030) {
		return po6030_set_brightness(value);
	} else {
		return -1;
	}
}

int8_t cam_set_contrast(uint8_t value) {
	if(curr_cam == CAM_PO8030) {
		return po8030_set_contrast(value);
	} else if(curr_cam == CAM_PO6030) {
		return po6030_set_contrast(value);
	} else {
		return -1;
	}
}

int8_t cam_set_mirror(uint8_t vertical, uint8_t horizontal) {
	if(curr_cam == CAM_PO8030) {
		return po8030_set_mirror(vertical, horizontal);
	} else if(curr_cam == CAM_PO6030) {
		return po6030_set_mirror(vertical, horizontal);
	} else {
		return -1;
	}
}

int8_t cam_set_awb(uint8_t awb) {
	if(curr_cam == CAM_PO8030) {
		return po8030_set_awb(awb);
	} else if(curr_cam == CAM_PO6030) {
		return po6030_set_awb(awb);
	} else {
		return -1;
	}
}

int8_t cam_set_rgb_gain(uint8_t r, uint8_t g, uint8_t b) {
	if(curr_cam == CAM_PO8030) {
		return po8030_set_rgb_gain(r, g, b);
	} else if(curr_cam == CAM_PO6030) {
		return po6030_set_rgb_gain(r, g, b);
	} else {
		return -1;
	}
}

int8_t cam_set_ae(uint8_t ae) {
	if(curr_cam == CAM_PO8030) {
		return po8030_set_ae(ae);
	} else if(curr_cam == CAM_PO6030) {
		return po6030_set_ae(ae);
	} else {
		return -1;
	}
}

int8_t cam_set_exposure(uint16_t integral, uint8_t fractional) {
	if(curr_cam == CAM_PO8030) {
		return po8030_set_exposure(integral, fractional);
	} else if(curr_cam == CAM_PO6030) {
		return po6030_set_exposure(integral, fractional);
	} else {
		return -1;
	}
}

uint16_t cam_get_id(void) {
	if(curr_cam == CAM_PO8030) {
		return 0x8030;
	} else if(curr_cam == CAM_PO6030) {
		return 0x6030;
	} else {
		return 0;
	}
}

/**************************END PUBLIC FUNCTIONS***********************************/
