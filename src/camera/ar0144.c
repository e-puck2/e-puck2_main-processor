#include "ar0144.h"
#include "ch.h"
#include "usbcfg.h"
#include "chprintf.h"
#include "../i2c_bus.h"

#define AR0144_ADDR 0x10	// SADDR being forced to 0 on e-puck2

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

