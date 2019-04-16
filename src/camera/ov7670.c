#include "ov7670.h"
#include "ch.h"
#include "usbcfg.h"
#include "chprintf.h"
#include "../i2c_bus.h"

#define OV7670_ADDR 0x21

static struct ov7670_configuration ov7670_conf;
static bool cam_configured = false;

/**
  * @brief  OV7670 Registers
  */
#define REG_GAIN	0x00			/* Gain lower 8 bits (rest in vref) */
#define REG_BLUE	0x01			/* blue gain */
#define REG_RED		0x02			/* red gain */
#define REG_GREEN	0x6a			/* green gain */
#define REG_VREF	0x03			/* Pieces of GAIN, VSTART, VSTOP */
#define REG_COM1	0x04			/* Control 1 */
#define  COM1_CCIR656	  0x40 		/* CCIR656 enable */
#define REG_BAVE	0x05			/* U/B Average level */
#define REG_GbAVE	0x06			/* Y/Gb Average level */
#define REG_AECHH	0x07			/* AEC MS 5 bits */
#define REG_RAVE	0x08			/* V/R Average level */
#define REG_COM2	0x09			/* Control 2 */
#define  COM2_SSLEEP	  0x10		/* Soft sleep mode */
#define OV7670_REG_PID		0x0a	/* Product ID MSB */
#define OV7670_REG_VER		0x0b	/* Product ID LSB */
#define REG_COM3	0x0c			/* Control 3 */
#define  COM3_SWAP	  0x40	  		/* Byte swap */
#define  COM3_SCALEEN	  0x08	  	/* Enable scaling */
#define  COM3_DCWEN	  0x04	  		/* Enable downsamp/crop/window */
#define REG_COM4	0x0d			/* Control 4 */
#define REG_COM5	0x0e			/* All "reserved" */
#define REG_COM6	0x0f			/* Control 6 */
#define REG_AECH	0x10			/* More bits of AEC value */
#define REG_CLKRC	0x11			/* Clocl control */
#define   CLK_EXT	  0x40	  		/* Use external clock directly */
#define   CLK_SCALE	  0x3f	  		/* Mask for internal clock scale */
#define REG_COM7	0x12			/* Control 7 */
#define   COM7_RESET	  0x80	  	/* Register reset */
#define   COM7_FMT_MASK	  0x38
#define   COM7_FMT_VGA	  0x00
#define	  COM7_FMT_CIF	  0x20	  	/* CIF format */
#define   COM7_FMT_QVGA	  0x10	  	/* QVGA format */
#define   COM7_FMT_QCIF	  0x08	  	/* QCIF format */
#define	  COM7_RGB	  0x04	  		/* bits 0 and 2 - RGB format */
#define	  COM7_YUV	  0x00	  		/* YUV */
#define	  COM7_BAYER	  0x01	  	/* Bayer format */
#define	  COM7_PBAYER	  0x05	  	/* "Processed bayer" */
#define REG_COM8	0x13			/* Control 8 */
#define   COM8_FASTAEC	  0x80	  	/* Enable fast AGC/AEC */
#define   COM8_AECSTEP	  0x40	  	/* Unlimited AEC step size */
#define   COM8_BFILT	  0x20	  	/* Band filter enable */
#define   COM8_AGC	  0x04	  		/* Auto gain enable */
#define   COM8_AWB	  0x02	  		/* White balance enable */
#define   COM8_AEC	  0x01	  		/* Auto exposure enable */
#define REG_COM9	0x14			/* Control 9  - gain ceiling */
#define REG_COM10	0x15			/* Control 10 */
#define   COM10_HSYNC	  0x40	  	/* HSYNC instead of HREF */
#define   COM10_PCLK_HB	  0x20	  	/* Suppress PCLK on horiz blank */
#define   COM10_HREF_REV  0x08	 	/* Reverse HREF */
#define   COM10_VS_LEAD	  0x04	 	/* VSYNC on clock leading edge */
#define   COM10_VS_NEG	  0x02	  	/* VSYNC negative */
#define   COM10_HS_NEG	  0x01	  	/* HSYNC negative */
#define REG_HSTART	0x17			/* Horiz start high bits */
#define REG_HSTOP	0x18			/* Horiz stop high bits */
#define REG_VSTART	0x19			/* Vert start high bits */
#define REG_VSTOP	0x1a			/* Vert stop high bits */
#define REG_PSHFT	0x1b			/* Pixel delay after HREF */
#define REG_MIDH	0x1c			/* Manuf. ID high */
#define REG_MIDL	0x1d			/* Manuf. ID low */
#define REG_MVFP	0x1e			/* Mirror / vflip */
#define   MVFP_MIRROR	  0x20	  	/* Mirror image */
#define   MVFP_FLIP	  0x10	  		/* Vertical flip */

#define REG_AEW		0x24			/* AGC upper limit */
#define REG_AEB		0x25			/* AGC lower limit */
#define REG_VPT		0x26			/* AGC/AEC fast mode op region */
#define REG_HSYST	0x30			/* HSYNC rising edge delay */
#define REG_HSYEN	0x31			/* HSYNC falling edge delay */
#define REG_HREF	0x32			/* HREF pieces */
#define REG_TSLB	0x3a			/* lots of stuff */
#define   TSLB_YLAST	  0x04	  	/* UYVY or VYUY - see com13 */
#define REG_COM11	0x3b			/* Control 11 */
#define   COM11_NIGHT	  0x80	  	/* NIght mode enable */
#define   COM11_NMFR	  0x60	  	/* Two bit NM frame rate */
#define   COM11_HZAUTO	  0x10	  	/* Auto detect 50/60 Hz */
#define	  COM11_50HZ	  0x08	  	/* Manual 50Hz select */
#define   COM11_EXP	  0x02
#define REG_COM12	0x3c			/* Control 12 */
#define   COM12_HREF	  0x80	  	/* HREF always */
#define REG_COM13	0x3d			/* Control 13 */
#define   COM13_GAMMA	  0x80	  	/* Gamma enable */
#define	  COM13_UVSAT	  0x40	  	/* UV saturation auto adjustment */
#define   COM13_UVSWAP	  0x01	  	/* V before U - w/TSLB */
#define REG_COM14	0x3e			/* Control 14 */
#define   COM14_DCWEN	  0x10	  	/* DCW/PCLK-scale enable */
#define REG_EDGE	0x3f			/* Edge enhancement factor */
#define REG_COM15	0x40			/* Control 15 */
#define   COM15_R10F0	  0x00	  	/* Data range 10 to F0 */
#define	  COM15_R01FE	  0x80	  	/*            01 to FE */
#define   COM15_R00FF	  0xc0	  	/*            00 to FF */
#define   COM15_RGB565	  0x10	  	/* RGB565 output */
#define   COM15_RGB555	  0x30	  	/* RGB555 output */
#define REG_COM16	0x41			/* Control 16 */
#define   COM16_AWBGAIN   0x08	  	/* AWB gain enable */
#define REG_COM17	0x42			/* Control 17 */
#define   COM17_AECWIN	  0xc0	  	/* AEC window - must match COM4 */
#define   COM17_CBAR	  0x08	  	/* DSP Color bar */

/*
 * This matrix defines how the colors are generated, must be
 * tweaked to adjust hue and saturation.
 *
 * Order: v-red, v-green, v-blue, u-red, u-green, u-blue
 *
 * They are nine-bit signed quantities, with the sign bit
 * stored in 0x58.  Sign for v-red is bit 0, and up from there.
 */
#define	REG_CMATRIX_BASE 0x4f
#define   CMATRIX_LEN 6
#define REG_CMATRIX_SIGN 0x58


#define REG_BRIGHT	0x55			/* Brightness */
#define REG_CONTRAS	0x56			/* Contrast control */

#define REG_GFIX	0x69			/* Fix gain control */

#define REG_DBLV	0x6b			/* PLL control an debugging */
#define   DBLV_BYPASS	  0x00	  	/* Bypass PLL */
#define   DBLV_X4	  0x01	  		/* clock x4 */
#define   DBLV_X6	  0x10	  		/* clock x6 */
#define   DBLV_X8	  0x11	  		/* clock x8 */

#define REG_SCALING_XSC	0x70		/* Test pattern and horizontal scale factor */
#define   TEST_PATTTERN_0 0x80
#define REG_SCALING_YSC	0x71		/* Test pattern and vertical scale factor */
#define   TEST_PATTTERN_1 0x80
#define REG_SCALING_DCWCTR 0x72

#define REG_SCALING_PCLK_DIV 0x73
#define REG_SCALING_PCLK_DELAY 0xA2

#define REG_REG76	0x76			/* OV's name */
#define   R76_BLKPCOR	  0x80	  	/* Black pixel correction enable */
#define   R76_WHTPCOR	  0x40	  	/* White pixel correction enable */

#define REG_RGB444	0x8c			/* RGB 444 control */
#define   R444_ENABLE	  0x02	  	/* Turn on RGB444, overrides 5x5 */
#define   R444_RGBX	  0x01	  		/* Empty nibble at end */

#define REG_HAECC1	0x9f			/* Hist AEC/AGC control 1 */
#define REG_HAECC2	0xa0			/* Hist AEC/AGC control 2 */

#define REG_BD50MAX	0xa5			/* 50hz banding step limit */
#define REG_HAECC3	0xa6			/* Hist AEC/AGC control 3 */
#define REG_HAECC4	0xa7			/* Hist AEC/AGC control 4 */
#define REG_HAECC5	0xa8			/* Hist AEC/AGC control 5 */
#define REG_HAECC6	0xa9			/* Hist AEC/AGC control 6 */
#define REG_HAECC7	0xaa			/* Hist AEC/AGC control 7 */
#define REG_BD60MAX	0xab			/* 60hz banding step limit */


/***************************INTERNAL FUNCTIONS************************************/

int16_t ov7670_write_reg(uint8_t addr, uint8_t reg, uint8_t value) {
	uint8_t txbuf[2] = {reg, value};

	i2cAcquireBus(&I2CD1);
	if(I2CD1.state != I2C_STOP) {
		msg_t status = i2cMasterTransmitTimeout(&I2CD1, addr, txbuf, 2, 0, 0, MS2ST(50));
		if (status != MSG_OK){
			i2c_update_last_error();
			if(I2CD1.state == I2C_LOCKED){
				i2c_stop();
				i2c_start();
			}
			i2cReleaseBus(&I2CD1);
			return status;
		}
	}
	i2cReleaseBus(&I2CD1);

    return MSG_OK;
}

int8_t ov7670_read_reg(uint8_t addr, uint8_t reg, uint8_t *value) {

	uint8_t txbuf[1] = {reg};

	i2cAcquireBus(&I2CD1);
	if(I2CD1.state != I2C_STOP) {
		msg_t status = i2cMasterTransmitTimeout(&I2CD1, addr, txbuf, 1, 0, 0, MS2ST(50));
		if (status != MSG_OK){
			i2c_update_last_error();
			if(I2CD1.state == I2C_LOCKED) {
				i2c_stop();
				i2c_start();
			}
			i2cReleaseBus(&I2CD1);
			return status;
		}
		status = i2cMasterReceiveTimeout(&I2CD1, addr, value, sizeof(*value), MS2ST(50));
		if (status != MSG_OK){
			i2c_update_last_error();
			if(I2CD1.state == I2C_LOCKED) {
				i2c_stop();
				i2c_start();
			}
			i2cReleaseBus(&I2CD1);
			return status;
		}
	}
	i2cReleaseBus(&I2CD1);

    return MSG_OK;
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
int8_t ov7670_read_id(uint16_t *id) {
    uint8_t regValue[2] = {0};
    int8_t err = 0;

    if((err = ov7670_read_reg(OV7670_ADDR, OV7670_REG_PID, &regValue[0])) != MSG_OK) {
        return err;
    }
    if((err = ov7670_read_reg(OV7670_ADDR, OV7670_REG_VER, &regValue[1])) != MSG_OK) {
        return err;
    }
    *id = (((uint16_t)regValue[0])<<8)|regValue[1];

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
int8_t ov7670_set_vga(void) {
	return -1;
}

/**
* @brief   Sets the camera to work in QVGA
*
* @return              The operation status.
* @retval MSG_OK       if the function succeeded.
* @retval MSG_TIMEOUT  if a timeout occurred before operation end.
*
*/
int8_t ov7670_set_qvga(void) {
	return -1;
}

int8_t ov7670_set_qqvga(void) {
    int8_t err = 0;

    if((err = ov7670_write_reg(OV7670_ADDR, REG_HSTART, 0x16)) != MSG_OK) {				// start = HSTART<<3 + HREF[2:0] = 22*8 + 4 = 180
    	return err;
    }
    if((err = ov7670_write_reg(OV7670_ADDR, REG_HSTOP, 0x04)) != MSG_OK) { 				// stop = HSTOP<<3 + HREF[5:3] = 4*8 + 4 = 36 (180+640-784)
    	return err;
    }
    if((err = ov7670_write_reg(OV7670_ADDR, REG_HREF, 0x24)) != MSG_OK) { 				// With flag "edge offset" set, then the image is strange (too much clear, not sharp); so clear this bit.
    	return err;
    }
    if((err = ov7670_write_reg(OV7670_ADDR, REG_VSTART, 0x02)) != MSG_OK) {				// start = VSTART<<2 + VREF[1:0] = 2*4 + 2 = 10
    	return err;
    }
    if((err = ov7670_write_reg(OV7670_ADDR, REG_VSTOP, 0x7a)) != MSG_OK) {				// stop = VSTOP<<2 + VREF[3:2] = 122*4 + 2 = 490
    	return err;
    }
    if((err = ov7670_write_reg(OV7670_ADDR, REG_VREF, 0x0a)) != MSG_OK) {
    	return err;
    }
    if((err = ov7670_write_reg(OV7670_ADDR, REG_COM3, COM3_DCWEN)) != MSG_OK) { 		// Enable scaling.
    	return err;
    }
    if((err = ov7670_write_reg(OV7670_ADDR, REG_COM14, COM14_DCWEN|0x0A)) != MSG_OK) {	// PCLK divide by 4 to have the same framerate.
    	return err;
    }
    if((err = ov7670_write_reg(OV7670_ADDR, REG_SCALING_DCWCTR, 0x22)) != MSG_OK) { 	// Vertical and horizontal down sample by 4.
    	return err;
    }
    if((err = ov7670_write_reg(OV7670_ADDR, REG_SCALING_PCLK_DIV, 0xF2)) != MSG_OK) { 	// DSP clock divided by 4.
    	return err;
    }

    ov7670_conf.width = 160;
    ov7670_conf.height = 120;
	ov7670_conf.curr_subsampling_x = SUBSAMPLING_X4;
	ov7670_conf.curr_subsampling_y = SUBSAMPLING_X4;

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
int8_t ov7670_set_size(image_size_t imgsize) {
   if(imgsize == SIZE_VGA) {
       return ov7670_set_vga();
   } else if(imgsize == SIZE_QVGA) {
       return ov7670_set_qvga();
   } else if(imgsize == SIZE_QQVGA) {
       return ov7670_set_qqvga();
   } else {
		return -1;
	}
}

/*************************END INTERNAL FUNCTIONS**********************************/


/****************************PUBLIC FUNCTIONS*************************************/

void ov7670_start(void) {
    // Default camera configuration.
    ov7670_write_reg(OV7670_ADDR, REG_COM7, 0x80);						// Reset to default values.
    chThdSleepMilliseconds(10);
    ov7670_write_reg(OV7670_ADDR, REG_CLKRC, 0x80);						// No internal clock prescaler.
    //ov7670_write_reg(OV7670_ADDR, REG_COM11, 0x0A);					// Disable night mode and others...not needed
    ov7670_write_reg(OV7670_ADDR, REG_TSLB, 0x04);
    ov7670_write_reg(OV7670_ADDR, REG_COM7, COM7_RGB|COM7_FMT_VGA);		// Output format: RGB, VGA.
    //ov7670_write_reg(OV7670_ADDR, REG_RGB444, 0x00);					// Disable RGB444...not needed
    ov7670_write_reg(OV7670_ADDR, REG_COM15, COM15_R00FF|COM15_RGB565);	// Set RGB565.
    ov7670_write_reg(OV7670_ADDR, REG_COM10, 0x02); 					// Negative Vsync otherwise it doesn't work...??
    ov7670_write_reg(OV7670_ADDR, 0xb0, 0x84);							// Color mode?? (Not documented!)


	ov7670_advanced_config(OV7670_FORMAT_RGB565, 240, 180, 160, 120, SUBSAMPLING_X1, SUBSAMPLING_X1);
}

int8_t ov7670_config(ov7670_format_t fmt, image_size_t imgsize) {

    int8_t err = 0;

    ov7670_conf.curr_format = fmt;

    if((err = ov7670_set_size(imgsize)) != MSG_OK) {
        return err;
    }

    return MSG_OK;
}

int8_t ov7670_advanced_config(  ov7670_format_t fmt, unsigned int x1, unsigned int y1,
                                unsigned int width, unsigned int height,
								subsampling_t subsampling_x, subsampling_t subsampling_y) {

	int8_t err = MSG_OK;
	x1 += 183;
	y1 += 10;
	unsigned int x2 = x1 + width;
	unsigned int y2 = y1 + height;
	uint8_t regValue[3]= {0};

	// The frame is supposed to be 784x510 and the HSTOP (that is x2) can be lower than HSTART (x1) if it overflows.
	if(x2 >= 784) {
		x2 -=784;
	}

	if(width>OV7670_MAX_WIDTH || height>OV7670_MAX_HEIGHT) {
		return -8;
	}

	if(x1>OV7670_MAX_WIDTH) {
		return -2;
	}

	if(y1>OV7670_MAX_HEIGHT) {
		return -3;
	}

	if(subsampling_y > subsampling_x) { // It seems it isn't supported by the camera.
		return -4;
	}

	if((err = ov7670_read_reg(OV7670_ADDR, REG_SCALING_DCWCTR, &regValue[0])) != MSG_OK) {	// Read horizontal and vertical down sampling.
		return err;
	}
	regValue[0] &= ~(0x33);	// Clear horizontal and vertical down sampling bits.

	switch(subsampling_x) {
		case SUBSAMPLING_X1:
			ov7670_conf.width = width;
			break;

		case SUBSAMPLING_X2:
			// Check if width is a multiple of the sub-sampling factor.
			if(width%2) {
				return -6;
			} else {
				regValue[0] |= 1; // Horizontal down sampling by 2.
				ov7670_conf.width = width/2;
			}
			break;

		case SUBSAMPLING_X4:
			// Check if width is a multiple of the sub-sampling factor.
			if(width%4) {
				return -6;
			} else {
				regValue[0] |= 2; // Horizontal down sampling by 4.
				ov7670_conf.width = width/4;
			}
			break;
	}

	switch(subsampling_y) {
		case SUBSAMPLING_X1:
			ov7670_conf.height = height;
			break;

		case SUBSAMPLING_X2:
			// Check if width is a multiple of the sub-sampling factor.
			if(height%2) {
				return -7;
			} else {
				regValue[0] |= 0x10; // Vertical down sampling by 2.
				ov7670_conf.height = height/2;
			}
			break;

		case SUBSAMPLING_X4:
			// Check if width is a multiple of the sub-sampling factor.
			if(height%4) {
				return -7;
			} else {
				regValue[0] |= 0x20; // Vertical down sampling by 4.
				ov7670_conf.height = height/4;
			}
			break;
	}

    if((err = ov7670_write_reg(OV7670_ADDR, REG_HSTART, x1>>3)) != MSG_OK) { // start = HSTART<<3 + HREF[2:0]
    	return err;
    }
    if((err = ov7670_write_reg(OV7670_ADDR, REG_HSTOP, x2>>3)) != MSG_OK) { // stop = HSTOP<<3 + HREF[5:3]
    	return err;
    }
    if((err = ov7670_write_reg(OV7670_ADDR, REG_HREF, (x1&0x07) | ((x2&0x07)<<3))) != MSG_OK) { // HREF[5:3] = HSTOP 3 LSBits; HREF[2:0] = HSTART 3 LSBits
    	return err;
    }
    if((err = ov7670_write_reg(OV7670_ADDR, REG_VSTART, y1>>2)) != MSG_OK) { // start = VSTART<<2 + VREF[1:0]
    	return err;
    }
    if((err = ov7670_write_reg(OV7670_ADDR, REG_VSTOP, y2>>2)) != MSG_OK) { // stop = VSTOP<<2 + VREF[3:2]
    	return err;
    }
    if((err = ov7670_write_reg(OV7670_ADDR, REG_VREF, (y1&0x03) | ((y2&0x03)<<2))) != MSG_OK) {
    	return err;
    }

	if((err = ov7670_read_reg(OV7670_ADDR, REG_COM14, &regValue[1])) != MSG_OK) { // Read PCLK divider.
		return err;
	}
	regValue[1] &= ~(0x07);	// Clear PCLK divider bits.
	regValue[1] |= 0x08;	// Enable manual scaling.
	if((err = ov7670_read_reg(OV7670_ADDR, REG_SCALING_PCLK_DIV, &regValue[2])) != MSG_OK) { // Read DSP clock divider.
		return err;
	}
	regValue[2] &= ~(0x07);	// Clear PCLK divider bits.
	regValue[2] |= 0xF0;	// Enable clock divider.

	if(subsampling_x >= subsampling_y) {
		switch(subsampling_x) {
			case SUBSAMPLING_X1:
				break;

			case SUBSAMPLING_X2:
				regValue[1] |= 1;
				regValue[2] |= 1;
				break;

			case SUBSAMPLING_X4:
				regValue[1] |= 2;
				regValue[2] |= 2;
				break;
		}
	} else {
		// This is not working, it seems it isn't supported by the camera.
//		switch(subsampling_y) {
//			case SUBSAMPLING_X1:
//				break;
//
//			case SUBSAMPLING_X2:
//				regValue[1] |= 1;
//				regValue[2] |= 1;
//				break;
//
//			case SUBSAMPLING_X4:
//				regValue[1] |= 2;
//				regValue[2] |= 2;
//				break;
//		}
	}

    if((subsampling_x == SUBSAMPLING_X1) && (subsampling_y == SUBSAMPLING_X1)) {
    	if((err = ov7670_write_reg(OV7670_ADDR, REG_COM3, 0x00)) != MSG_OK) { // Disable scaling.
    		return err;
    	}
    } else {
    	if((err = ov7670_write_reg(OV7670_ADDR, REG_COM3, COM3_DCWEN)) != MSG_OK) { // Enable scaling.
    		return err;
    	}
    }

    if((err = ov7670_write_reg(OV7670_ADDR, REG_COM14, COM14_DCWEN|regValue[1])) != MSG_OK) { // PCLK divider.
    	return err;
    }
    if((err = ov7670_write_reg(OV7670_ADDR, REG_SCALING_DCWCTR, regValue[0])) != MSG_OK) { // Vertical and horizontal down sample.
    	return err;
    }
    if((err = ov7670_write_reg(OV7670_ADDR, REG_SCALING_PCLK_DIV, regValue[2])) != MSG_OK) { // DSP clock divider.
    	return err;
    }

//	// The following code is an initial step to use the YUYV format to get the greyscale image, but unfortunately it doesn't work.
//  // The idea is to have a sequence of YUYV and then keep only 1 byte over 2 to get only the greyscale information.
//	if((err = ov7670_read_reg(OV7670_ADDR, REG_COM7, &regValue[0])) != MSG_OK) { // Read common control 7 register.
//		return err;
//	}
//	regValue[0] &= ~(0x05);	// Clear output format bits.
//	if(fmt == OV7670_FORMAT_RGB565) {
//		regValue[0] |= COM7_RGB;
//	}
//    if((err = ov7670_write_reg(OV7670_ADDR, REG_COM7, regValue[0])) != MSG_OK) { // Set output format: either RGB565 or YUV (used for conversion to greyscale).
//    	return err;
//    }

	ov7670_conf.curr_format = fmt;
	ov7670_conf.curr_subsampling_x = subsampling_x;
	ov7670_conf.curr_subsampling_y = subsampling_y;

    if(err == MSG_OK){
        cam_configured = true;
    }

    return MSG_OK;

}

int8_t ov7670_set_brightness(uint8_t value) {
    return ov7670_write_reg(OV7670_ADDR, REG_BRIGHT, value);
}

int8_t ov7670_set_contrast(uint8_t value) {
    //ov7670_write_reg(OV7670_ADDR, 0x57, ...);	//CONTRAS_CENTER
    //ov7670_write_reg(OV7670_ADDR, 0x58, 0x9e); // MTXS
    return ov7670_write_reg(OV7670_ADDR, 0x56, value);
}

int8_t ov7670_set_mirror(uint8_t vertical, uint8_t horizontal) {
    int8_t err = 0;
    uint8_t value = 0;

	if((err = ov7670_read_reg(OV7670_ADDR, REG_MVFP, &value)) != MSG_OK) {
		return err;
	}

    if(vertical == 1) {
        value |= 0x10;
    } else {
    	value &= ~0x10;
    }

    if(horizontal == 1) {
        value |= 0x20;
    } else {
    	value &= ~0x20;
    }

    return ov7670_write_reg(OV7670_ADDR, REG_MVFP, value);
}

int8_t ov7670_set_awb(uint8_t awb) {
    int8_t err = 0;
    uint8_t value = 0;

	if((err = ov7670_read_reg(OV7670_ADDR, REG_COM8, &value)) != MSG_OK) {
		return err;
	}

    if(awb == 1) {
    	value |= COM8_AWB;
    } else {
    	value &= ~(COM8_AWB);
    }

    if((err = ov7670_write_reg(OV7670_ADDR, REG_COM8, value)) != MSG_OK) {
    	return err;
    }

    return MSG_OK;
}

int8_t ov7670_set_rgb_gain(uint8_t r, uint8_t g, uint8_t b) {
    int8_t err = 0;

    if((err = ov7670_set_awb(0)) != MSG_OK) {
        return err;
    }

    if((err = ov7670_write_reg(OV7670_ADDR, REG_RED, r)) != MSG_OK) {
    	return err;
    }
    if((err = ov7670_write_reg(OV7670_ADDR, REG_GREEN, g)) != MSG_OK) {
    	return err;
    }
    if((err = ov7670_write_reg(OV7670_ADDR, REG_BLUE, b)) != MSG_OK) {
    	return err;
    }

    return MSG_OK;
}

int8_t ov7670_set_ae(uint8_t ae) {
    int8_t err = 0;
    uint8_t value = 0;

	if((err = ov7670_read_reg(OV7670_ADDR, REG_COM8, &value)) != MSG_OK) {
		return err;
	}

    if(ae == 1) {
    	value |= COM8_AEC;
    } else {
    	value &= ~(COM8_AEC);
    }

    if((err = ov7670_write_reg(OV7670_ADDR, REG_COM8, value)) != MSG_OK) {
    	return err;
    }

    return MSG_OK;
}

int8_t ov7670_set_exposure(uint16_t integral) {
    int8_t err = 0;

    if((err = ov7670_set_ae(0)) != MSG_OK) {
        return err;
    }

    if((err = ov7670_write_reg(OV7670_ADDR, REG_AECHH, (integral>>10)&0x001F)) != MSG_OK) {	// AEC[15:10]
    	return err;
    }
    if((err = ov7670_write_reg(OV7670_ADDR, REG_AECH, (integral>>2)&0x00FF)) != MSG_OK) {	// AEC[9:2]
    	return err;
    }
    if((err = ov7670_write_reg(OV7670_ADDR, REG_COM1, (integral&0x0003))) != MSG_OK) {		// AEC[1:0]
    	return err;
    }

    return MSG_OK;
}


/*! Returns the current image size in bytes.
 */
uint32_t ov7670_get_image_size(void) {
    if(ov7670_conf.curr_format == OV7670_FORMAT_GREYSCALE) {
        return (uint32_t)ov7670_conf.width * (uint32_t)ov7670_conf.height;
    } else {
        return (uint32_t)ov7670_conf.width * (uint32_t)ov7670_conf.height * 2;
    }
}

uint8_t ov7670_is_connected(void) {
	uint16_t id = 0;
	int8_t res = ov7670_read_id(&id);
	if((res==MSG_OK) && (id==0X7673)) {
		return 1;
	} else {
		return 0;
	}
}

/**************************END PUBLIC FUNCTIONS***********************************/

