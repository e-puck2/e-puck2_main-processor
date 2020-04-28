#include "ar0144.h"
#include "ch.h"
#include "usbcfg.h"
#include "chprintf.h"
#include "../i2c_bus.h"

#define AR0144_ADDR 0x10	// SADDR being forced to 0 on e-puck2

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
#define AR0144_REG_DATA_PEDESTAL				0x301E
#define AR0144_REG_SOFTWARE_RESET_				0x3021
#define AR0144_REG_GROUPED_PARAMETER_HOLD_		0x3022
#define AR0144_REG_MASK_CORRUPTED_FRAMES_		0x3023
#define AR0144_REG_PIXEL_ORDER_					0x3024
#define AR0144_REG_GPI_STATUS					0x3026
#define AR0144_REG_ROW_SPEED					0x3028
#define AR0144_REG_VT_PIX_CLK_DIV				0x302A
#define AR0144_REG_VT_SYS_CLK_DIV				0x302C
#define AR0144_REG_PRE_PLL_CLK_DIV				0x302E
#define AR0144_REG_PLL_MULTIPLIER				0x3030
#define AR0144_REG_CTX_CONTROL_REG				0x3034
#define AR0144_REG_OP_PIX_CLK_DIV				0x3036
#define AR0144_REG_OP_SYS_CLK_DIV				0x3038
#define AR0144_REG_FRAME_COUNT					0x303A
#define AR0144_REG_FRAME_STATUS					0x303C
#define AR0144_REG_LINE_LENGTH_PCK_CB			0x303E
#define AR0144_REG_READ_MODE					0x3040
#define AR0144_REG_EXTRA_DELAY					0x3042
#define AR0144_REG_GREEN1_GAIN					0x3056
#define AR0144_REG_BLUE_GAIN					0x3058
#define AR0144_REG_RED_GAIN						0x305A
#define AR0144_REG_GREEN2_GAIN					0x305C
#define AR0144_REG_GLOBAL_GAIN					0x305E
#define AR0144_REG_ANALOG_GAIN					0x3060
#define AR0144_REG_SMIA_TEST					0x3064
#define AR0144_REG_CTX_WR_DATA_REG				0x3066
#define AR0144_REG_CTX_RD_DATA_REG				0x3068
#define AR0144_REG_DATAPATH_SELECT				0x306E
#define AR0144_REG_TEST_PATTERN_MODE			0x3070
#define AR0144_REG_TEST_DATA_RED				0x3072
#define AR0144_REG_TEST_DATA_GREENR				0x3074
#define AR0144_REG_TEST_DATA_BLUE				0x3076
#define AR0144_REG_TEST_DATA_GREENB				0x3078
#define AR0144_REG_OPERATION_MODE_CTRL			0x3082
#define AR0144_REG_OPERATION_MODE_CTRL_CB		0x3084
#define AR0144_REG_SEQ_DATA_PORT				0x3086
#define AR0144_REG_SEQ_CTRL_PORT				0x3088
#define AR0144_REG_X_ADDR_START_CB				0x308A
#define AR0144_REG_Y_ADDR_START_CB				0x308C
#define AR0144_REG_X_ADDR_END_CB				0x308E
#define AR0144_REG_Y_ADDR_END_CB				0x3090
#define AR0144_REG_X_EVEN_INC					0x30A0
#define AR0144_REG_X_ODD_INC					0x30A2
#define AR0144_REG_Y_EVEN_INC					0x30A4
#define AR0144_REG_Y_ODD_INC					0x30A6
#define AR0144_REG_Y_ODD_INC_CB					0x30A8
#define AR0144_REG_FRAME_LENGTH_LINES_CB		0x30AA
#define AR0144_REG_X_ODD_INC_CB					0x30AE
#define AR0144_REG_DIGITAL_TEST					0x30B0
#define AR0144_REG_TEMPSENS_DATA_REG			0x30B2
#define AR0144_REG_TEMPSENS_CTRL_REG			0x30B4
#define AR0144_REG_GREEN1_GAIN_CB				0x30BC
#define AR0144_REG_BLUE_GAIN_CB					0x30BE
#define AR0144_REG_RED_GAIN_CB					0x30C0
#define AR0144_REG_GREEN2_GAIN_CB				0x30C2
#define AR0144_REG_GLOBAL_GAIN_CB				0x30C4
#define AR0144_REG_TEMPSENS_CALIB1				0x30C6
#define AR0144_REG_TEMPSENS_CALIB2				0x30C8
#define AR0144_REG_GRR_CONTROL1					0x30CE
#define AR0144_REG_NOISE_PEDESTAL				0x30FE
#define AR0144_REG_AECTRLREG					0x3100
#define AR0144_REG_AE_LUMA_TARGET_REG			0x3102
#define AR0144_REG_AE_MIN_EV_STEP_REG			0x3108
#define AR0144_REG_AE_MAX_EV_STEP_REG			0x310A
#define AR0144_REG_AE_DAMP_OFFSET_REG			0x310C
#define AR0144_REG_AE_DAMP_GAIN_REG				0x310E
#define AR0144_REG_AE_DAMP_MAX_REG				0x3110
#define AR0144_REG_AE_MAX_EXPOSURE_REG			0x311C
#define AR0144_REG_AE_MIN_EXPOSURE_REG			0x311E
#define AR0144_REG_AE_DARK_CUR_THRESH_REG		0x3124
#define AR0144_REG_AE_CURRENT_GAINS				0x312A
#define AR0144_REG_ACTUAL_FRAME_LENGTH_LINES	0x312E
#define AR0144_REG_AE_ROI_X_START_OFFSET		0x3140
#define AR0144_REG_AE_ROI_Y_START_OFFSET		0x3142
#define AR0144_REG_AE_ROI_X_SIZE				0x3144
#define AR0144_REG_AE_ROI_Y_SIZE				0x3146
#define AR0144_REG_AE_HIST_BEGIN_PERC			0x3148
#define AR0144_REG_AE_HIST_END_PERC				0x314A
#define AR0144_REG_AE_HIST_DIV					0x314C
#define AR0144_REG_AE_NORM_WIDTH_MIN			0x314E
#define AR0144_REG_AE_MEAN_H					0x3150
#define AR0144_REG_AE_MEAN_L					0x3152
#define AR0144_REG_AE_HIST_BEGIN_H				0x3154
#define AR0144_REG_AE_HIST_BEGIN_L				0x3156
#define AR0144_REG_AE_HIST_END_H				0x3158
#define AR0144_REG_AE_HIST_END_L				0x315A
#define AR0144_REG_AE_LOW_END_MEAN_H			0x315C
#define AR0144_REG_AE_LOW_END_MEAN_L			0x315E
#define AR0144_REG_AE_PERC_LOW_END				0x3160
#define AR0144_REG_AE_NORM_ABS_DEV				0x3162
#define AR0144_REG_AE_COARSE_INTEGRATION_TIME	0x3164
#define AR0144_REG_AE_AG_EXPOSURE_HI			0x3166
#define AR0144_REG_AE_AG_EXPOSURE_LO			0x3168
#define AR0144_REG_DELTA_DK_CONTROL				0x3180
#define AR0144_REG_DATA_FORMAT_BITS				0x31AC
#define AR0144_REG_SERIAL_FORMAT				0x31AE
#define AR0144_REG_FRAME_PREAMBLE				0x31B0
#define AR0144_REG_LINE_PREAMBLE				0x31B2
#define AR0144_REG_MIPI_TIMING_0				0x31B4
#define AR0144_REG_MIPI_TIMING_1				0x31B6
#define AR0144_REG_MIPI_TIMING_2				0x31B8
#define AR0144_REG_MIPI_TIMING_3				0x31BA
#define AR0144_REG_MIPI_TIMING_4				0x31BC
#define AR0144_REG_SERIAL_CONFIG_STATUS			0x31BE
#define AR0144_REG_SERIAL_CONTROL_STATUS		0x31C6
#define AR0144_REG_SERIAL_CRC_0					0x31C8
#define AR0144_REG_COMPANDING					0x31D0
#define AR0144_REG_STAT_FRAME_ID				0x31D2
#define AR0144_REG_I2C_WRT_CHECKSUM				0x31D6
#define AR0144_REG_SERIAL_TEST					0x31D8
#define AR0144_REG_PIX_DEF_ID					0x31E0
#define AR0144_REG_HORIZONTAL_CURSOR_POSITION	0x31E8
#define AR0144_REG_VERTICAL_CURSOR_POSITION		0x31EA
#define AR0144_REG_HORIZONTAL_CURSOR_WIDTH		0x31EC
#define AR0144_REG_VERTICAL_CURSOR_WIDTH		0x31EE
#define AR0144_REG_AE_STATS_CONTROL				0x31F0
#define AR0144_REG_CCI_IDS						0x31FC
#define AR0144_REG_CUSTOMER_REV					0x31FE
#define AR0144_REG_AE_X1_START_OFFSET			0x3240
#define AR0144_REG_AE_X2_START_OFFSET			0x3242
#define AR0144_REG_AE_X3_START_OFFSET			0x3244
#define AR0144_REG_AE_X4_START_OFFSET			0x3246
#define AR0144_REG_AE_Y1_START_OFFSET			0x3248
#define AR0144_REG_AE_Y2_START_OFFSET			0x324A
#define AR0144_REG_AE_Y3_START_OFFSET			0x324C
#define AR0144_REG_AE_Y4_START_OFFSET			0x324E
#define AR0144_REG_AE_GRID_SEL_LO				0x3250
#define AR0144_REG_AE_GRID_SEL_HI				0x3252
#define AR0144_REG_LED_FLASH_CONTROL			0x3270
#define AR0144_REG_MIPI_TEST_CNTRL				0x3338
#define AR0144_REG_MIPI_COMPRESS_8_DATA_TYPE	0x333A
#define AR0144_REG_MIPI_COMPRESS_7_DATA_TYPE	0x333C
#define AR0144_REG_MIPI_COMPRESS_6_DATA_TYPE	0x333E
#define AR0144_REG_MIPI_JPEG_PN9_DATA_TYPE		0x3340
#define AR0144_REG_MIPI_CNTRL					0x3354
#define AR0144_REG_MIPI_TEST_PATTERN_CNTRL		0x3356
#define AR0144_REG_MIPI_TEST_PATTERN_STATUS		0x3358
#define AR0144_REG_P_GR_P0Q0					0x3600
#define AR0144_REG_P_GR_P0Q1					0x3602
#define AR0144_REG_P_GR_P0Q2					0x3604
#define AR0144_REG_P_GR_P0Q3					0x3606
#define AR0144_REG_P_GR_P0Q4					0x3608
#define AR0144_REG_P_RD_P0Q0					0x360A
#define AR0144_REG_P_RD_P0Q1					0x360C
#define AR0144_REG_P_RD_P0Q2					0x360E
#define AR0144_REG_P_RD_P0Q3					0x3610
#define AR0144_REG_P_RD_P0Q4					0x3612
#define AR0144_REG_P_BL_P0Q0					0x3614
#define AR0144_REG_P_BL_P0Q1					0x3616
#define AR0144_REG_P_BL_P0Q2					0x3618
#define AR0144_REG_P_BL_P0Q3					0x361A
#define AR0144_REG_P_BL_P0Q4					0x361C
#define AR0144_REG_P_GB_P0Q0					0x361E
#define AR0144_REG_P_GB_P0Q1					0x3620
#define AR0144_REG_P_GB_P0Q2					0x3622
#define AR0144_REG_P_GB_P0Q3					0x3624
#define AR0144_REG_P_GB_P0Q4					0x3626
#define AR0144_REG_P_GR_P1Q0					0x3640
#define AR0144_REG_P_GR_P1Q1					0x3642
#define AR0144_REG_P_GR_P1Q2					0x3644
#define AR0144_REG_P_GR_P1Q3					0x3646
#define AR0144_REG_P_GR_P1Q4					0x3648
#define AR0144_REG_P_RD_P1Q0					0x364A
#define AR0144_REG_P_RD_P1Q1					0x364C
#define AR0144_REG_P_RD_P1Q2					0x364E
#define AR0144_REG_P_RD_P1Q3					0x3650
#define AR0144_REG_P_RD_P1Q4					0x3652
#define AR0144_REG_P_BL_P1Q0					0x3654
#define AR0144_REG_P_BL_P1Q1					0x3656
#define AR0144_REG_P_BL_P1Q2					0x3658
#define AR0144_REG_P_BL_P1Q3					0x365A
#define AR0144_REG_P_BL_P1Q4					0x365C
#define AR0144_REG_P_GB_P1Q0					0x365E
#define AR0144_REG_P_GB_P1Q1					0x3660
#define AR0144_REG_P_GB_P1Q2					0x3662
#define AR0144_REG_P_GB_P1Q3					0x3664
#define AR0144_REG_P_GB_P1Q4					0x3666
#define AR0144_REG_P_GR_P2Q0					0x3680
#define AR0144_REG_P_GR_P2Q1					0x3682
#define AR0144_REG_P_GR_P2Q2					0x3684
#define AR0144_REG_P_GR_P2Q3					0x3686
#define AR0144_REG_P_GR_P2Q4					0x3688
#define AR0144_REG_P_RD_P2Q0					0x368A
#define AR0144_REG_P_RD_P2Q1					0x368C
#define AR0144_REG_P_RD_P2Q2					0x368E
#define AR0144_REG_P_RD_P2Q3					0x3690
#define AR0144_REG_P_RD_P2Q4					0x3692
#define AR0144_REG_P_BL_P2Q0					0x3694
#define AR0144_REG_P_BL_P2Q1					0x3696
#define AR0144_REG_P_BL_P2Q2					0x3698
#define AR0144_REG_P_BL_P2Q3					0x369A
#define AR0144_REG_P_BL_P2Q4					0x369C
#define AR0144_REG_P_GB_P2Q0					0x369E
#define AR0144_REG_P_GB_P2Q1					0x36A0
#define AR0144_REG_P_GB_P2Q2					0x36A2
#define AR0144_REG_P_GB_P2Q3					0x36A4
#define AR0144_REG_P_GB_P2Q4					0x36A6
#define AR0144_REG_P_GR_P3Q0					0x36C0
#define AR0144_REG_P_GR_P3Q1					0x36C2
#define AR0144_REG_P_GR_P3Q2					0x36C4
#define AR0144_REG_P_GR_P3Q3					0x36C6
#define AR0144_REG_P_GR_P3Q4					0x36C8
#define AR0144_REG_P_RD_P3Q0					0x36CA
#define AR0144_REG_P_RD_P3Q1					0x36CC
#define AR0144_REG_P_RD_P3Q2					0x36CE
#define AR0144_REG_P_RD_P3Q3					0x36D0
#define AR0144_REG_P_RD_P3Q4					0x36D2
#define AR0144_REG_P_BL_P3Q0					0x36D4
#define AR0144_REG_P_BL_P3Q1					0x36D6
#define AR0144_REG_P_BL_P3Q2					0x36D8
#define AR0144_REG_P_BL_P3Q3					0x36DA
#define AR0144_REG_P_BL_P3Q4					0x36DC
#define AR0144_REG_P_GB_P3Q0					0x36DE
#define AR0144_REG_P_GB_P3Q1					0x36E0
#define AR0144_REG_P_GB_P3Q2					0x36E2
#define AR0144_REG_P_GB_P3Q3					0x36E4
#define AR0144_REG_P_GB_P3Q4					0x36E6
#define AR0144_REG_P_GR_P4Q0					0x3700
#define AR0144_REG_P_GR_P4Q1					0x3702
#define AR0144_REG_P_GR_P4Q2					0x3704
#define AR0144_REG_P_GR_P4Q3					0x3706
#define AR0144_REG_P_GR_P4Q4					0x3708
#define AR0144_REG_P_RD_P4Q0					0x370A
#define AR0144_REG_P_RD_P4Q1					0x370C
#define AR0144_REG_P_RD_P4Q2					0x370E
#define AR0144_REG_P_RD_P4Q3					0x3710
#define AR0144_REG_P_RD_P4Q4					0x3712
#define AR0144_REG_P_BL_P4Q0					0x3714
#define AR0144_REG_P_BL_P4Q1					0x3716
#define AR0144_REG_P_BL_P4Q2					0x3718
#define AR0144_REG_P_BL_P4Q3					0x371A
#define AR0144_REG_P_BL_P4Q4					0x371C
#define AR0144_REG_P_GB_P4Q0					0x371E
#define AR0144_REG_P_GB_P4Q1					0x3720
#define AR0144_REG_P_GB_P4Q2					0x3722
#define AR0144_REG_P_GB_P4Q3					0x3724
#define AR0144_REG_P_GB_P4Q4					0x3726
#define AR0144_REG_POLY_SC_ENABLE				0x3780
#define AR0144_REG_POLY_ORIGIN_C				0x3782
#define AR0144_REG_POLY_ORIGIN_R				0x3784
#define AR0144_REG_DIGITAL_CTRL_1				0x3786
#define AR0144_REG_P_GR_Q5						0x37C0
#define AR0144_REG_P_RD_Q5						0x37C2
#define AR0144_REG_P_BL_Q5						0x37C4
#define AR0144_REG_P_GB_Q5						0x37C6

static struct ar0144_configuration ar0144_conf;

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
int8_t ar0144_read_id(uint16_t *id) {
    uint8_t regValue[2] = {0};
    int8_t err = 0;

    if((err = read_reg_16(AR0144_ADDR, AR0144_REG_CHIP_VERSION_REG, &regValue[0], 2)) != MSG_OK) {
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
int8_t ar0144_set_vga(void) {
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
int8_t ar0144_set_qvga(void) {
	return -1;
}

/**
* @brief   Sets the camera to work in QQVGA
*
* @return              The operation status.
* @retval MSG_OK       if the function succeeded.
* @retval MSG_TIMEOUT  if a timeout occurred before operation end.
*
*/
int8_t ar0144_set_qqvga(void) {
    return -1;
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
int8_t ar0144_set_size(image_size_t imgsize) {
   if(imgsize == SIZE_VGA) {
       return ar0144_set_vga();
   } else if(imgsize == SIZE_QVGA) {
       return ar0144_set_qvga();
   } else if(imgsize == SIZE_QQVGA) {
       return ar0144_set_qqvga();
   } else {
		return -1;
	}
}

/*************************END INTERNAL FUNCTIONS**********************************/


/****************************PUBLIC FUNCTIONS*************************************/

int8_t ar0144_start(void) {
    uint8_t regValue[2] = {0};
    int8_t err = 0;

    // Reset camera
    regValue[0] = 0x00;
    regValue[1] = 0x01;
    if((err = write_reg_16(AR0144_ADDR, AR0144_REG_RESET_REGISTER, &regValue[0], 2)) != MSG_OK) {
    	return err;
    }
    chThdSleepMilliseconds(10);

    // There are 3 clocks generated by the camera:
    // - VCO = cam_clk * pll multiplier / pre_pll_clk_div; must be in the range [384..768] MHz
    // - PIXCLK = (cam_clk * pll multiplier)/(pre_pll_clk_div * vt_sys_clk_div * vt_pix_clk_div); must be in the range [6..54] MHz
    //   54 MHz is the maximum pixel clock acceptable by the DCMI interface
    // - OP_CLK = (cam_clk * pll multiplier)/(pre_pll_clk_div * op_sys_clk_div * op_pix_clk_div)
    //
    // When using the parallel interface, PIXCLK and OP_CLK must have the same operating frequency.
    // PIXCLK drives the FV (VSYNC) and LV (HSYNC) signals.
    // OP_CLK drives the pixel clock (PIXCLK pin in the electrical schema, not to be confused with above PIXCLK clock).
    //
    // The multiplier and divisors are setup to have a pixel clock of 48 MHz:
    // pll multiplier = 128 (range is [32..384])
    // pre_pll_clk_div = 7 (range is [1..64])
    // vt_pix_clk_div = 8 (range is [4..16])
    // vt_sys_clk_div = 1 (available values are 1,2,4,6,8,10,12,14,16)
    // op_pix_clk_div = 8 (available vaules are 8,10,12)
    // op_sys_clk_div = 1 (available values are 1,2,4,6,8,10,12,14,16)

    // pll multiplier
    regValue[0] = 0x0;
    regValue[1] = 128;
    if((err = write_reg_16(AR0144_ADDR, AR0144_REG_PLL_MULTIPLIER, &regValue[0], 2)) != MSG_OK) {
    	return err;
    }
    // pre_pll_clk_div
    regValue[0] = 0x0;
    regValue[1] = 7;
    if((err = write_reg_16(AR0144_ADDR, AR0144_REG_PRE_PLL_CLK_DIV, &regValue[0], 2)) != MSG_OK) {
    	return err;
    }
	// vt_pix_clk_div
	regValue[0] = 0x0;
    regValue[1] = 8;
    if((err = write_reg_16(AR0144_ADDR, AR0144_REG_VT_PIX_CLK_DIV, &regValue[0], 2)) != MSG_OK) {
    	return err;
    }
    // vt_sys_clk_div
    regValue[0] = 0x0;
    regValue[1] = 1;
    if((err = write_reg_16(AR0144_ADDR, AR0144_REG_VT_SYS_CLK_DIV, &regValue[0], 2)) != MSG_OK) {
    	return err;
    }
    // op_pix_clk_div
    regValue[0] = 0x0;
    regValue[1] = 8; //10;
    if((err = write_reg_16(AR0144_ADDR, AR0144_REG_OP_PIX_CLK_DIV, &regValue[0], 2)) != MSG_OK) {
    	return err;
    }
    // op_sys_clk_div
    regValue[0] = 0x0;
    regValue[1] = 1;
    if((err = write_reg_16(AR0144_ADDR, AR0144_REG_OP_SYS_CLK_DIV, &regValue[0], 2)) != MSG_OK) {
    	return err;
    }

    // digital_test: enable pll
    regValue[0] = 0x0;
    regValue[1] = 0;
    if((err = write_reg_16(AR0144_ADDR, AR0144_REG_DIGITAL_TEST, &regValue[0], 2)) != MSG_OK) {
    	return err;
    }

    // Line length pck = 1488 => minimum line length.
    regValue[0] = 0x05;
    regValue[1] = 0xD0;
    if((err = write_reg_16(AR0144_ADDR, AR0144_REG_LINE_LENGTH_PCK, &regValue[0], 2)) != MSG_OK) {
    	return err;
    }

    // Given we want a framerate of 30 Hz and a pixel clock of 48 MHz then:
    // - Max frame clocks = 48000000/30 = 1600000 clocks
    // - Frame length lines = Max frame clocks/Line length pck - frame end blanking= 1600000/1488 - 5 = 1070
    // - Extra clocks delay = Max frame clocks - Frame length line*Line length pck - frame end blanking*Line length pck = 1600000 - 1070*1488 - 5*1488 = 400
    // Frame length lines
    regValue[0] = 0x04;
    regValue[1] = 0x2E;
    if((err = write_reg_16(AR0144_ADDR, AR0144_REG_FRAME_LENGTH_LINES, &regValue[0], 2)) != MSG_OK) {
    	return err;
    }
    // Extra clocks delay
    regValue[0] = 0x01;
    regValue[1] = 0x8F;
    if((err = write_reg_16(AR0144_ADDR, AR0144_REG_EXTRA_DELAY, &regValue[0], 2)) != MSG_OK) {
    	return err;
    }

    // Integration time = 33 ms (that is maximum given fps=30)
    // Integration reg value = (33*1000)/(1/48MHz*1488) = 1064
    regValue[0] = 0x04;
    regValue[1] = 0x28;
    if((err = write_reg_16(AR0144_ADDR, AR0144_REG_COARSE_INTEGRATION_TIME, &regValue[0], 2)) != MSG_OK) {
    	return err;
    }

    // Output 12 bits: we use only 8 MSBits.
    regValue[0] = 0x0C;
    regValue[1] = 0x0C;
    if((err = write_reg_16(AR0144_ADDR, AR0144_REG_DATA_FORMAT_BITS, &regValue[0], 2)) != MSG_OK) {
    	return err;
    }

    // Line Valid (LV) normal behavior
    regValue[0] = 0x48;
    regValue[1] = 0x10;
    if((err = write_reg_16(AR0144_ADDR, AR0144_REG_DATAPATH_SELECT, &regValue[0], 2)) != MSG_OK) {
    	return err;
    }

    // Linear mode (no other options)
    regValue[0] = 0;
    regValue[1] = 3;
    if((err = write_reg_16(AR0144_ADDR, AR0144_REG_OPERATION_MODE_CTRL, &regValue[0], 2)) != MSG_OK) {
    	return err;
    }

    // No flip/mirroring, no binning
    regValue[0] = 0;
    regValue[1] = 0;
    if((err = write_reg_16(AR0144_ADDR, AR0144_REG_READ_MODE, &regValue[0], 2)) != MSG_OK) {
    	return err;
    }

    // No compression
    regValue[0] = 0x0;
    regValue[1] = 0x0;
    if((err = write_reg_16(AR0144_ADDR, AR0144_REG_COMPANDING, &regValue[0], 2)) != MSG_OK) {
    	return err;
    }

    // Serial format: from the register reference, this register should not be related to parallel interface
    // but instead when using the parallel interface the lower byte must 0 otherwise the parallel interface
    // is disabled (FV and LV are stopped).
    regValue[0] = 0x02;
    regValue[1] = 0x00;
    if((err = write_reg_16(AR0144_ADDR, AR0144_REG_SERIAL_FORMAT, &regValue[0], 2)) != MSG_OK) {
    	return err;
    }

    ar0144_advanced_config(AR0144_FORMAT_BAYER, 560, 340, 160, 120, SUBSAMPLING_X1, SUBSAMPLING_X1);

    // Serial interface disabled, parallel interface enabled, stream on
    regValue[0] = 0x30;
    regValue[1] = 0xDC; //0xDC;
    if((err = write_reg_16(AR0144_ADDR, AR0144_REG_RESET_REGISTER, &regValue[0], 2)) != MSG_OK) {
    	return err;
    }

    return MSG_OK;
}

int8_t ar0144_config(ar0144_format_t fmt, image_size_t imgsize) {

    int8_t err = 0;

    ar0144_conf.curr_format = fmt;

    if((err = ar0144_set_size(imgsize)) != MSG_OK) {
        return err;
    }

    return MSG_OK;
}

int8_t ar0144_advanced_config(ar0144_format_t fmt, unsigned int x1, unsigned int y1,
                               unsigned int width, unsigned int height,
								subsampling_t subsampling_x, subsampling_t subsampling_y) {

	int8_t err = MSG_OK;
	x1 += 6; // Frame start blanking
	unsigned int x2 = x1 + width - 1;
	unsigned int y2 = y1 + height - 1;
	uint8_t regValue[2]= {0};
	uint8_t sub_x = 1, sub_y = 1;

	if(width>AR0144_MAX_WIDTH || height>AR0144_MAX_HEIGHT) {
		return -8;
	}

	if(x1>AR0144_MAX_WIDTH) {
		return -2;
	}

	if(y1>AR0144_MAX_HEIGHT) {
		return -3;
	}

	if(x2<=x1 || x2>AR0144_MAX_FRAME_WIDTH) {
		return -4;
	}

	if(y2<=y1 || y2>AR0144_MAX_FRAME_HEIGHT) {
		return -5;
	}

	switch(subsampling_x) {
		case SUBSAMPLING_X1:
			ar0144_conf.width = width;
			sub_x = 1;
			break;

		case SUBSAMPLING_X2:
			// Check if width is a multiple of the sub-sampling factor.
			if(width%2) {
				return -6;
			} else {
				ar0144_conf.width = width/2;
				sub_x = 3;
			}
			break;

		case SUBSAMPLING_X4:
			// Check if width is a multiple of the sub-sampling factor.
			if(width%4) {
				return -6;
			} else {
				ar0144_conf.width = width/4;
				sub_x = 7;
			}
			break;
	}

	switch(subsampling_y) {
		case SUBSAMPLING_X1:
			ar0144_conf.height = height;
			sub_y = 1;
			break;

		case SUBSAMPLING_X2:
			// Check if width is a multiple of the sub-sampling factor.
			if(height%2) {
				return -7;
			} else {
				ar0144_conf.height = height/2;
				sub_y = 3;
			}
			break;

		case SUBSAMPLING_X4:
			// Check if width is a multiple of the sub-sampling factor.
			if(height%4) {
				return -7;
			} else {
				ar0144_conf.height = height/4;
				sub_y = 7;
			}
			break;
	}

    // y start
    regValue[0] = y1>>8;
    regValue[1] = y1&0xFF;
    if((err = write_reg_16(AR0144_ADDR, AR0144_REG_Y_ADDR_START, &regValue[0], 2)) != MSG_OK) {
    	return err;
    }
    // y end
    regValue[0] = y2>>8;
    regValue[1] = y2&0xFF;
    if((err = write_reg_16(AR0144_ADDR, AR0144_REG_Y_ADDR_END, &regValue[0], 2)) != MSG_OK) {
    	return err;
    }
    // x start
    regValue[0] = x1>>8;
    regValue[1] = x1&0xFF;
    if((err = write_reg_16(AR0144_ADDR, AR0144_REG_X_ADDR_START, &regValue[0], 2)) != MSG_OK) {
    	return err;
    }
    // x end
    regValue[0] = x2>>8;
    regValue[1] = x2&0xFF;
    if((err = write_reg_16(AR0144_ADDR, AR0144_REG_X_ADDR_END, &regValue[0], 2)) != MSG_OK) {
    	return err;
    }

    // Horizontal skip
    regValue[0] = 0;
    regValue[1] = sub_x;
    if((err = write_reg_16(AR0144_ADDR, AR0144_REG_X_ODD_INC, &regValue[0], 2)) != MSG_OK) {
    	return err;
    }

    // Vertical skip
    regValue[0] = 0;
    regValue[1] = sub_y;
    if((err = write_reg_16(AR0144_ADDR, AR0144_REG_Y_ODD_INC, &regValue[0], 2)) != MSG_OK) {
    	return err;
    }

	ar0144_conf.curr_format = fmt;
	ar0144_conf.curr_subsampling_x = subsampling_x;
	ar0144_conf.curr_subsampling_y = subsampling_y;

    return MSG_OK;
}

uint32_t ar0144_get_image_size(void) {
    if(ar0144_conf.curr_format == AR0144_FORMAT_RGB565) {
        //return (uint32_t)ar0144_conf.width * (uint32_t)ar0144_conf.height * 2;
        return (uint32_t)ar0144_conf.width * (uint32_t)ar0144_conf.height; // At the moment RGB565 is not supported
    } else {
        return (uint32_t)ar0144_conf.width * (uint32_t)ar0144_conf.height;
    }
}

uint8_t ar0144_is_connected(void) {
	uint16_t id = 0;
	int8_t res = ar0144_read_id(&id);
	if((res==MSG_OK) && (id==0x0356)) {
		//chprintf((BaseSequentialStream *)&SDU1, "ID=%d\r\n", id);
		return 1;
	} else {
		return 0;
	}
}


/**************************END PUBLIC FUNCTIONS***********************************/

