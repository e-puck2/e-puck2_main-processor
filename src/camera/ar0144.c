#include "ar0144.h"
#include "ch.h"
#include "usbcfg.h"
#include "chprintf.h"
#include "../i2c_bus.h"

#define AR0144_ADDR 0x10

static bool cam_configured = false;

#define AR0144_REG_CHIP_VERSION_REG				0x3000
#define AR0144_REG_Y_ADDR_START					0x3002
#define AR0144_REG_X_ADDR_START					0x3004
#define AR0144_REG_Y_ADDR_END					0x3006
#define AR0144_REG_X_ADDR_END					0x3008
#define AR0144_REG_FRAME_LENGTH_LINES			0x300A
#define AR0144_REG_LINE_LENGTH_PCK				0x300C
#define AR0144_REG_REVISION_NUMBER				0x300E
#define AR0144_REG_LOCK_CONTROL					0x3010
#define AR0144_REG_COARSE_INTEGRATION_TIME		0x3012
#define AR0144_REG_FINE_INTEGRATION_TIME		0x3014
#define AR0144_REG_COARSE_INTEGRATION_TIME_CB	0x3016
#define AR0144_REG_FINE_INTEGRATION_TIME_CB		0x3018
#define AR0144_REG_RESET_REGISTER				0x301A
#define AR0144_REG_MODE_SELECT_					0x301C
#define AR0144_REG_IMAGE_ORIENTATION_			0x301D
#define AR0144_REG_VT_PIX_CLK_DIV				0x302A
#define AR0144_REG_VT_SYS_CLK_DIV				0x302C
#define AR0144_REG_PRE_PLL_CLK_DIV				0x302E
#define AR0144_REG_PLL_MULTIPLIER				0x3030
#define AR0144_REG_OP_PIX_CLK_DIV				0x3036
#define AR0144_REG_OP_SYS_CLK_DIV				0x3038
#define AR0144_REG_LINE_LENGTH_PCK_CB			0x303E
#define AR0144_REG_READ_MODE					0x3040
#define AR0144_REG_EXTRA_DELAY					0x3042
#define AR0144_REG_DATAPATH_SELECT				0x306E
#define AR0144_REG_OPERATION_MODE_CTRL			0x3082
#define AR0144_REG_OPERATION_MODE_CTRL_CB		0x3084
#define AR0144_REG_X_ADDR_START_CB				0x308A
#define AR0144_REG_Y_ADDR_START_CB				0x308C
#define AR0144_REG_X_ADDR_END_CB				0x308E
#define AR0144_REG_Y_ADDR_END_CB				0x3090
#define AR0144_REG_X_ODD_INC					0x30A2
#define AR0144_REG_Y_ODD_INC					0x30A6
#define AR0144_REG_Y_ODD_INC_CB					0x30A8
#define AR0144_REG_FRAME_LENGTH_LINES_CB		0x30AA
#define AR0144_REG_X_ODD_INC_CB					0x30AE
#define AR0144_REG_DIGITAL_TEST					0x30B0
#define AR0144_REG_DATA_FORMAT_BITS				0x31AC
#define AR0144_REG_SERIAL_FORMAT				0x31AE
#define AR0144_REG_COMPANDING					0x31D0
#define AR0144_REG_MIPI_CNTRL					0x3354



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
int8_t ar0144_read_id(uint16_t *id) {
    uint8_t regValue[2] = {0};
    int8_t err = 0;

    if((err = read_reg_16(AR0144_ADDR, AR0144_REG_CHIP_VERSION_REG, &regValue[0], 2)) != MSG_OK) {
        return err;
    }
    *id = (((uint16_t)regValue[0])<<8)|regValue[1];

    return MSG_OK;
}


/****************************PUBLIC FUNCTIONS*************************************/

void ar0144_start(void) {
    uint8_t regValue[2] = {0};
    int8_t err = 0;
	uint16_t id = 0;
	int8_t res = ar0144_read_id(&id);
	if(res==MSG_OK) {
		chprintf((BaseSequentialStream *)&SDU1, "ID=%d\r\n", id);
	} else {
		chprintf((BaseSequentialStream *)&SDU1, "Cannot read id\r\n");
	}

/*
	if((err = read_reg_16(AR0144_ADDR, AR0144_REG_Y_ADDR_START, &regValue[0], 2)) != MSG_OK) {
    	chprintf((BaseSequentialStream *)&SDU1, "Cannot read y_addr\r\n");
    } else {
    	id = (((uint16_t)regValue[0])<<8)|regValue[1];
    	chprintf((BaseSequentialStream *)&SDU1, "y_addr=%d\r\n", id);
    }

	regValue[0] = 1;
    regValue[1] = 10;
    if((err = write_reg_16(AR0144_ADDR, AR0144_REG_Y_ADDR_START, &regValue[0], 2)) != MSG_OK) {
    	chprintf((BaseSequentialStream *)&SDU1, "Cannot write y_addr\r\n");
    } else {
    	chprintf((BaseSequentialStream *)&SDU1, "y_addr written correctly\r\n");
    }

    if((err = read_reg_16(AR0144_ADDR, AR0144_REG_Y_ADDR_START, &regValue[0], 2)) != MSG_OK) {
    	chprintf((BaseSequentialStream *)&SDU1, "Cannot read y_addr\r\n");
    } else {
    	id = (((uint16_t)regValue[0])<<8)|regValue[1];
    	chprintf((BaseSequentialStream *)&SDU1, "y_addr=%d\r\n", id);
    }
*/

	// PIXCLK = (ext. clk * pll multiplier)/(pre_pll_clk_div * vt_sys_clk_div * vt_pix_clk_div)
	// (21 * 134)/(7*1*6)=67 MHz
	// (6 * 54)/(1*1*6)=54 MHz
	regValue[0] = 0x0;
    regValue[1] = 10; //6;
    // No transaction end interrupt: 7 (54 MHz); 8 (48 MHz); 11 (34.90 MHz); 12 (32 MHz)
    // Transaction interrupt: 6 (64 MHz); 9 (42.6 MHz) => end int every 1,1,2 images; 10 (38.4 MHz) => end int every 6 images
    if((err = write_reg_16(AR0144_ADDR, AR0144_REG_VT_PIX_CLK_DIV, &regValue[0], 2)) != MSG_OK) {
    	chprintf((BaseSequentialStream *)&SDU1, "Cannot write AR0144_REG_VT_PIX_CLK_DIV\r\n");
    } else {
    	chprintf((BaseSequentialStream *)&SDU1, "AR0144_REG_VT_PIX_CLK_DIV written correctly\r\n");
    }

    regValue[0] = 0x0;
    regValue[1] = 1;
    if((err = write_reg_16(AR0144_ADDR, AR0144_REG_VT_SYS_CLK_DIV, &regValue[0], 2)) != MSG_OK) {
    	chprintf((BaseSequentialStream *)&SDU1, "Cannot write AR0144_REG_VT_SYS_CLK_DIV\r\n");
    } else {
    	chprintf((BaseSequentialStream *)&SDU1, "AR0144_REG_VT_SYS_CLK_DIV written correctly\r\n");
    }

    regValue[0] = 0x0;
    regValue[1] = 7; //1; //7;
    if((err = write_reg_16(AR0144_ADDR, AR0144_REG_PRE_PLL_CLK_DIV, &regValue[0], 2)) != MSG_OK) {
    	chprintf((BaseSequentialStream *)&SDU1, "Cannot write AR0144_REG_PRE_PLL_CLK_DIV\r\n");
    } else {
    	chprintf((BaseSequentialStream *)&SDU1, "AR0144_REG_PRE_PLL_CLK_DIV written correctly\r\n");
    }

    regValue[0] = 0x0;
    regValue[1] = 128; //128 => ok; //108 => no; //134;
    if((err = write_reg_16(AR0144_ADDR, AR0144_REG_PLL_MULTIPLIER, &regValue[0], 2)) != MSG_OK) {
    	chprintf((BaseSequentialStream *)&SDU1, "Cannot write AR0144_REG_PLL_MULTIPLIER\r\n");
    } else {
    	chprintf((BaseSequentialStream *)&SDU1, "AR0144_REG_PLL_MULTIPLIER written correctly\r\n");
    }

    // Output clock (OP_CLK) set to 38.4 MHz, the same as PIXCLK
    regValue[0] = 0x0;
    regValue[1] = 10;
    if((err = write_reg_16(AR0144_ADDR, AR0144_REG_OP_PIX_CLK_DIV, &regValue[0], 2)) != MSG_OK) {
    	chprintf((BaseSequentialStream *)&SDU1, "Cannot write AR0144_REG_OP_PIX_CLK_DIV\r\n");
    } else {
    	chprintf((BaseSequentialStream *)&SDU1, "AR0144_REG_OP_PIX_CLK_DIV written correctly\r\n");
    }

    regValue[0] = 0x0;
    regValue[1] = 1;
    if((err = write_reg_16(AR0144_ADDR, AR0144_REG_OP_SYS_CLK_DIV, &regValue[0], 2)) != MSG_OK) {
    	chprintf((BaseSequentialStream *)&SDU1, "Cannot write AR0144_REG_OP_SYS_CLK_DIV\r\n");
    } else {
    	chprintf((BaseSequentialStream *)&SDU1, "AR0144_REG_OP_SYS_CLK_DIV written correctly\r\n");
    }

    regValue[0] = 0x0;
    regValue[1] = 0;
    if((err = write_reg_16(AR0144_ADDR, AR0144_REG_DIGITAL_TEST, &regValue[0], 2)) != MSG_OK) {
    	chprintf((BaseSequentialStream *)&SDU1, "Cannot write AR0144_REG_DIGITAL_TEST\r\n");
    } else {
    	chprintf((BaseSequentialStream *)&SDU1, "AR0144_REG_DIGITAL_TEST written correctly\r\n");
    }

    // y start = 240 with skip=8 (40x40)
    regValue[0] = 0x0;
    regValue[1] = 0xF0;
    if((err = write_reg_16(AR0144_ADDR, AR0144_REG_Y_ADDR_START, &regValue[0], 2)) != MSG_OK) {
    	chprintf((BaseSequentialStream *)&SDU1, "Cannot write AR0144_REG_Y_ADDR_START\r\n");
    } else {
    	chprintf((BaseSequentialStream *)&SDU1, "AR0144_REG_Y_ADDR_START written correctly\r\n");
    }

    // x start = 484 with skip=8 (40x40)
    regValue[0] = 0x1;
    regValue[1] = 0xE4;
    if((err = write_reg_16(AR0144_ADDR, AR0144_REG_X_ADDR_START, &regValue[0], 2)) != MSG_OK) {
    	chprintf((BaseSequentialStream *)&SDU1, "Cannot write AR0144_REG_X_ADDR_START\r\n");
    } else {
    	chprintf((BaseSequentialStream *)&SDU1, "AR0144_REG_X_ADDR_START written correctly\r\n");
    }

    // y end = 559 with skip=8 (40x40)
    regValue[0] = 0x02;
    regValue[1] = 0x2F;
    if((err = write_reg_16(AR0144_ADDR, AR0144_REG_Y_ADDR_END, &regValue[0], 2)) != MSG_OK) {
    	chprintf((BaseSequentialStream *)&SDU1, "Cannot write AR0144_REG_Y_ADDR_END\r\n");
    } else {
    	chprintf((BaseSequentialStream *)&SDU1, "AR0144_REG_Y_ADDR_END written correctly\r\n");
    }

    // x end = 803 with skip=8 (40x40)
    regValue[0] = 0x03;
    regValue[1] = 0x23;
    if((err = write_reg_16(AR0144_ADDR, AR0144_REG_X_ADDR_END, &regValue[0], 2)) != MSG_OK) {
    	chprintf((BaseSequentialStream *)&SDU1, "Cannot write AR0144_REG_X_ADDR_END\r\n");
    } else {
    	chprintf((BaseSequentialStream *)&SDU1, "AR0144_REG_X_ADDR_END written correctly\r\n");
    }

    // 1070
    //regValue[0] = 0x04;
    //regValue[1] = 0x2E;
    // 855
    regValue[0] = 0x03;
    regValue[1] = 0x57;
    if((err = write_reg_16(AR0144_ADDR, AR0144_REG_FRAME_LENGTH_LINES, &regValue[0], 2)) != MSG_OK) {
    	chprintf((BaseSequentialStream *)&SDU1, "Cannot write AR0144_REG_FRAME_LENGTH_LINES\r\n");
    } else {
    	chprintf((BaseSequentialStream *)&SDU1, "AR0144_REG_FRAME_LENGTH_LINES written correctly\r\n");
    }

    // 1488
    regValue[0] = 0x05;
    regValue[1] = 0xD0;
    if((err = write_reg_16(AR0144_ADDR, AR0144_REG_LINE_LENGTH_PCK, &regValue[0], 2)) != MSG_OK) {
    	chprintf((BaseSequentialStream *)&SDU1, "Cannot write AR0144_REG_LINE_LENGTH_PCK\r\n");
    } else {
    	chprintf((BaseSequentialStream *)&SDU1, "AR0144_REG_LINE_LENGTH_PCK written correctly\r\n");
    }

    // 666 clocks
    //regValue[0] = 0x02;
    //regValue[1] = 0x9A;
    // 398 clocks
    //regValue[0] = 0x01;
    //regValue[1] = 0x8E;
    // 320 clocks 0x140
    // 318 clocks 0x13E
    regValue[0] = 0x01;
    regValue[1] = 0x3E;
    if((err = write_reg_16(AR0144_ADDR, AR0144_REG_EXTRA_DELAY, &regValue[0], 2)) != MSG_OK) {
    	chprintf((BaseSequentialStream *)&SDU1, "Cannot write AR0144_REG_EXTRA_DELAY\r\n");
    } else {
    	chprintf((BaseSequentialStream *)&SDU1, "AR0144_REG_EXTRA_DELAY written correctly\r\n");
    }

    // 676 => 15 ms
    regValue[0] = 0x02;
    regValue[1] = 0xA4;
    // 1291 => 30 ms
    //regValue[0] = 0x05;
    //regValue[1] = 0x0B;
    if((err = write_reg_16(AR0144_ADDR, AR0144_REG_COARSE_INTEGRATION_TIME, &regValue[0], 2)) != MSG_OK) {
    	chprintf((BaseSequentialStream *)&SDU1, "Cannot write AR0144_REG_COARSE_INTEGRATION_TIME\r\n");
    } else {
    	chprintf((BaseSequentialStream *)&SDU1, "AR0144_REG_COARSE_INTEGRATION_TIME written correctly\r\n");
    }

    // Output 12 bits
    regValue[0] = 0x0C;
    regValue[1] = 0x0C;
    if((err = write_reg_16(AR0144_ADDR, AR0144_REG_DATA_FORMAT_BITS, &regValue[0], 2)) != MSG_OK) {
    	chprintf((BaseSequentialStream *)&SDU1, "Cannot write AR0144_REG_DATA_FORMAT_BITS\r\n");
    } else {
    	chprintf((BaseSequentialStream *)&SDU1, "AR0144_REG_DATA_FORMAT_BITS written correctly\r\n");
    }

    // Line Valid (LV) normal behavior
    regValue[0] = 0x48;
    regValue[1] = 0x10;
    if((err = write_reg_16(AR0144_ADDR, AR0144_REG_DATAPATH_SELECT, &regValue[0], 2)) != MSG_OK) {
    	chprintf((BaseSequentialStream *)&SDU1, "Cannot write AR0144_REG_DATAPATH_SELECT\r\n");
    } else {
    	chprintf((BaseSequentialStream *)&SDU1, "AR0144_REG_DATAPATH_SELECT written correctly\r\n");
    }

    // No horizontal skip
    //regValue[0] = 0;
    //regValue[1] = 1;
    // Skip 16
    //regValue[0] = 0;
    //regValue[1] = 31;
    // Skip 8
    regValue[0] = 0;
    regValue[1] = 15;
    if((err = write_reg_16(AR0144_ADDR, AR0144_REG_X_ODD_INC, &regValue[0], 2)) != MSG_OK) {
    	chprintf((BaseSequentialStream *)&SDU1, "Cannot write AR0144_REG_X_ODD_INC\r\n");
    } else {
    	chprintf((BaseSequentialStream *)&SDU1, "AR0144_REG_X_ODD_INC written correctly\r\n");
    }

    // No vertical skip
    //regValue[0] = 0;
    //regValue[1] = 1;
    // Skip 16
    //regValue[0] = 0;
    //regValue[1] = 31;
    // Skip 8
    regValue[0] = 0;
    regValue[1] = 15;
    if((err = write_reg_16(AR0144_ADDR, AR0144_REG_Y_ODD_INC, &regValue[0], 2)) != MSG_OK) {
    	chprintf((BaseSequentialStream *)&SDU1, "Cannot write AR0144_REG_Y_ODD_INC\r\n");
    } else {
    	chprintf((BaseSequentialStream *)&SDU1, "AR0144_REG_Y_ODD_INC written correctly\r\n");
    }

    // Linear mode (no other options)
    regValue[0] = 0;
    regValue[1] = 3;
    if((err = write_reg_16(AR0144_ADDR, AR0144_REG_OPERATION_MODE_CTRL, &regValue[0], 2)) != MSG_OK) {
    	chprintf((BaseSequentialStream *)&SDU1, "Cannot write AR0144_REG_OPERATION_MODE_CTRL\r\n");
    } else {
    	chprintf((BaseSequentialStream *)&SDU1, "AR0144_REG_OPERATION_MODE_CTRL written correctly\r\n");
    }

    // No flip/mirroring, no binning
    regValue[0] = 0;
    regValue[1] = 0;
    if((err = write_reg_16(AR0144_ADDR, AR0144_REG_READ_MODE, &regValue[0], 2)) != MSG_OK) {
    	chprintf((BaseSequentialStream *)&SDU1, "Cannot write AR0144_REG_READ_MODE\r\n");
    } else {
    	chprintf((BaseSequentialStream *)&SDU1, "AR0144_REG_READ_MODE written correctly\r\n");
    }

    // No compression
    regValue[0] = 0x0;
    regValue[1] = 0x0;
    if((err = write_reg_16(AR0144_ADDR, AR0144_REG_COMPANDING, &regValue[0], 2)) != MSG_OK) {
    	chprintf((BaseSequentialStream *)&SDU1, "Cannot write AR0144_REG_COMPANDING\r\n");
    } else {
    	chprintf((BaseSequentialStream *)&SDU1, "AR0144_REG_COMPANDING written correctly\r\n");
    }


    // Not needed??
    regValue[0] = 0x02;
    regValue[1] = 0x00;
    if((err = write_reg_16(AR0144_ADDR, AR0144_REG_SERIAL_FORMAT, &regValue[0], 2)) != MSG_OK) {
    	chprintf((BaseSequentialStream *)&SDU1, "Cannot write AR0144_REG_SERIAL_FORMAT\r\n");
    } else {
    	chprintf((BaseSequentialStream *)&SDU1, "AR0144_REG_SERIAL_FORMAT written correctly\r\n");
    }

    regValue[0] = 0x90;
    regValue[1] = 0x10;
    if((err = write_reg_16(AR0144_ADDR, AR0144_REG_MIPI_CNTRL, &regValue[0], 2)) != MSG_OK) {
    	chprintf((BaseSequentialStream *)&SDU1, "Cannot write AR0144_REG_MIPI_CNTRL\r\n");
    } else {
    	chprintf((BaseSequentialStream *)&SDU1, "AR0144_REG_MIPI_CNTRL written correctly\r\n");
    }


    // Serial interface disabled, parallel interface enabled, stream on
    regValue[0] = 0x30;
    regValue[1] = 0xDC; //0xDC;
    if((err = write_reg_16(AR0144_ADDR, AR0144_REG_RESET_REGISTER, &regValue[0], 2)) != MSG_OK) {
    	chprintf((BaseSequentialStream *)&SDU1, "Cannot write AR0144_REG_RESET_REGISTER\r\n");
    } else {
    	chprintf((BaseSequentialStream *)&SDU1, "AR0144_REG_RESET_REGISTER written correctly\r\n");
    }

}


/**************************END PUBLIC FUNCTIONS***********************************/

