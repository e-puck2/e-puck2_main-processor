#include <hal.h>
#include "camera.h"
#include "../i2c_bus.h"
#include "ch.h"
#include "usbcfg.h"
#include "chprintf.h"
#include "po8030.h"
#include "po6030.h"
#include "ov7670.h"
#include "dcmi_camera.h"

#define CAM_PO8030 0
#define CAM_PO6030 1
#define CAM_OV7670 2

int8_t curr_cam = -1;
format_t curr_format = FORMAT_COLOR;

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
    } else if(ov7670_is_connected() == 1) {
    	curr_cam = CAM_OV7670;
    	ov7670_start();
    } else {
//    	uint8_t regValue[2] = {0};
//    	read_reg(0x30, 0x0A, &regValue[0]);
//    	read_reg(0x30, 0x0B, &regValue[1]);
//    	chprintf((BaseSequentialStream *)&SDU1, "H=%x, L=%x\r\n", regValue[0], regValue[1]);
//    	ov2640_start();
    }

}

int8_t cam_config(format_t fmt, image_size_t imgsize) {
	curr_format = fmt;
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
	} else if(curr_cam == CAM_OV7670) {
		if(fmt == FORMAT_GREYSCALE) {
			return ov7670_config(OV7670_FORMAT_GREYSCALE, imgsize);
		} else {
			return ov7670_config(OV7670_FORMAT_RGB565, imgsize);
		}
	}
	return -1;
}

uint32_t cam_get_image_size(void) {
	if(curr_cam == CAM_PO8030) {
		return po8030_get_image_size();
	} else if(curr_cam == CAM_PO6030) {
		return po6030_get_image_size();
	} else if(curr_cam == CAM_OV7670) {
		return ov7670_get_image_size();
	} else {
		return 0;
	}
}

uint32_t cam_get_mem_required(void) {
	if(curr_cam == CAM_PO8030) {
		return po8030_get_image_size();
	} else if(curr_cam == CAM_PO6030) {
		return po6030_get_image_size();
	} else if(curr_cam == CAM_OV7670) {
		if(curr_format == FORMAT_GREYSCALE) {
			return ov7670_get_image_size()*2; 	// The OV7670 doesn't support greyscale images, thus colors format are used instead.
												// This means that the actual memory required to capture one frame is twice as bigger than
												// the final image size.
		} else {
			return ov7670_get_image_size();
		}
	} else {
		return 0;
	}
}

int8_t cam_advanced_config(format_t fmt, unsigned int x1, unsigned int y1,
                                unsigned int width, unsigned int height,
								subsampling_t subsampling_x, subsampling_t subsampling_y) {
	curr_format = fmt;
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
	} else if(curr_cam == CAM_OV7670) {
		if(fmt == FORMAT_GREYSCALE) {
			return ov7670_advanced_config(OV7670_FORMAT_GREYSCALE, x1, y1, width, height, subsampling_x, subsampling_y);
		} else {
			return ov7670_advanced_config(OV7670_FORMAT_RGB565, x1, y1, width, height, subsampling_x, subsampling_y);
		}
	}
	return -1;
}

int8_t cam_set_brightness(int8_t value) {
	uint8_t actual_value = value;
	if(curr_cam == CAM_PO8030) {
		if(value < 0) {
			actual_value = (-value);
			actual_value |= 0x80;
		}
		return po8030_set_brightness(actual_value);
	} else if(curr_cam == CAM_PO6030) {
		return po6030_set_brightness(actual_value);
	} else if(curr_cam == CAM_OV7670) {
		if(value < 0) {
			actual_value = (-value);
			actual_value |= 0x80;
		}
		return ov7670_set_brightness(actual_value);
	} else {
		return -1;
	}
}

int8_t cam_set_contrast(uint8_t value) {
	if(curr_cam == CAM_PO8030) {
		return po8030_set_contrast(value);
	} else if(curr_cam == CAM_PO6030) {
		return po6030_set_contrast(value);
	} else if(curr_cam == CAM_OV7670) {
		return ov7670_set_contrast(value);
	} else {
		return -1;
	}
}

int8_t cam_set_mirror(uint8_t vertical, uint8_t horizontal) {
	if(curr_cam == CAM_PO8030) {
		return po8030_set_mirror(vertical, horizontal);
	} else if(curr_cam == CAM_PO6030) {
		return po6030_set_mirror(vertical, horizontal);
	} else if(curr_cam == CAM_OV7670) {
		return ov7670_set_mirror(vertical, horizontal);
	} else {
		return -1;
	}
}

int8_t cam_set_awb(uint8_t awb) {
	if(curr_cam == CAM_PO8030) {
		return po8030_set_awb(awb);
	} else if(curr_cam == CAM_PO6030) {
		return po6030_set_awb(awb);
	} else if(curr_cam == CAM_OV7670) {
		return ov7670_set_awb(awb);
	} else {
		return -1;
	}
}

int8_t cam_set_rgb_gain(uint8_t r, uint8_t g, uint8_t b) {
	if(curr_cam == CAM_PO8030) {
		return po8030_set_rgb_gain(r, g, b);
	} else if(curr_cam == CAM_PO6030) {
		return po6030_set_rgb_gain(r, g, b);
	} else if(curr_cam == CAM_OV7670) {
		return ov7670_set_rgb_gain(r, g, b);
	} else {
		return -1;
	}
}

int8_t cam_set_ae(uint8_t ae) {
	if(curr_cam == CAM_PO8030) {
		return po8030_set_ae(ae);
	} else if(curr_cam == CAM_PO6030) {
		return po6030_set_ae(ae);
	} else if(curr_cam == CAM_OV7670) {
		return ov7670_set_ae(ae);
	} else {
		return -1;
	}
}

int8_t cam_set_exposure(uint16_t integral, uint8_t fractional) {
	if(curr_cam == CAM_PO8030) {
		return po8030_set_exposure(integral, fractional);
	} else if(curr_cam == CAM_PO6030) {
		return po6030_set_exposure(integral, fractional);
	} else if(curr_cam == CAM_OV7670) {
		return ov7670_set_exposure(integral);
	} else {
		return -1;
	}
}

uint16_t cam_get_id(void) {
	if(curr_cam == CAM_PO8030) {
		return 0x8030;
	} else if(curr_cam == CAM_PO6030) {
		return 0x6030;
	} else if(curr_cam == CAM_OV7670) {
		return 0x7670;
	} else {
		return 0;
	}
}

uint8_t* cam_get_last_image_ptr(void) {
	uint8_t *last_img_ptr = dcmi_get_last_image_ptr();
	uint16_t i = 0, j = 0;
	if((curr_cam == CAM_OV7670) && (curr_format==FORMAT_GREYSCALE)) {
		// Manually perform RGB565 to greyscale conversion because the OV7670 camera doesn't support this format.
		// It is actually kind of a hack, not an actual conversion, it simply takes 1 byte out of 2, that is only R5G3 are used for
		// greyscale representation.
		for(i=0, j=0; i<cam_get_mem_required(); i+=2, j++) {
			last_img_ptr[j] = last_img_ptr[i];
		}
	}
	return last_img_ptr;
}

/**************************END PUBLIC FUNCTIONS***********************************/
