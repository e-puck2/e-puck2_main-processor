#include <ch.h>
#include <hal.h>
#include <main.h>
#include "usbcfg.h"
#include "chprintf.h"
#include "i2c_bus.h"
#include "imu.h"
#include "../leds.h"
//#include "exti.h"
#include "icm20948/ICM_20948_REGISTERS.h"
#include "icm20948/AK09916_REGISTERS.h"
#include <math.h>

#define IMU_NOT_FOUND -1
#define IMU_MPU9250 0
#define IMU_ICM20948 1

static imu_msg_t imu_values;

static uint8_t accAxisFilteringInProgress = 0;
static uint8_t accAxisFilteringState = 0;
static uint8_t accAxisSelected = 0;
static uint8_t accFilterSize = 0;
static uint8_t accCalibrationInProgress = 0;

static uint8_t gyroAxisFilteringInProgress = 0;
static uint8_t gyroAxisFilteringState = 0;
static uint8_t gyroAxisSelected = 0;
static uint8_t gyroFilterSize = 0;
static uint8_t gyroCalibrationInProgress = 0;

static uint8_t magCalibrationInProgress = 0;
static uint8_t magCalibrationState = 0;

static thread_t *imuThd;
static bool imu_configured = false;

static int8_t imuModel = IMU_NOT_FOUND;

/***************************INTERNAL FUNCTIONS************************************/

ICM_20948_Status_e startup_magnetometer(void)
{
	ICM_20948_Status_e status = ICM_20948_Stat_Ok;
	//status = ICM_20948_i2c_master_passthrough(&icmDevice, false); //Do not connect the SDA/SCL pins to AUX_DA/AUX_CL
	status = ICM_20948_i2c_master_enable(&icmDevice, false);

	uint8_t temp = 0x01;
	ICM_20948_execute_w(&icmDevice, AGB3_REG_I2C_MST_DELAY_CTRL, &temp, 1);


	// Read magnetometer ID
	ICM_20948_set_bank(&icmDevice, 3);
	temp = (0x80 | MAG_AK09916_I2C_ADDR);
	ICM_20948_execute_w(&icmDevice, AGB3_REG_I2C_PERIPH0_ADDR, &temp, 1);
	temp = AK09916_REG_WIA2;
	ICM_20948_execute_w(&icmDevice, AGB3_REG_I2C_PERIPH0_REG, &temp, 1);
	temp = 0x81;
	ICM_20948_execute_w(&icmDevice, AGB3_REG_I2C_PERIPH0_CTRL, &temp, 1);

	ICM_20948_set_bank(&icmDevice, 0);
	ICM_20948_execute_r(&icmDevice, AGB0_REG_USER_CTRL, &temp, 1); // Enable master i2c to actually perform the read operation
	uint8_t temp2 = temp|0x20;
	ICM_20948_execute_w(&icmDevice, AGB0_REG_USER_CTRL, &temp2, 1);
	chThdSleepMilliseconds(10);
	ICM_20948_execute_w(&icmDevice, AGB0_REG_USER_CTRL, &temp, 1);

	uint8_t ak09916_id_ = 0;
	ICM_20948_execute_r(&icmDevice, AGB0_REG_EXT_PERIPH_SENS_DATA_00, &ak09916_id_, 1);
	//chprintf((BaseSequentialStream *)&SDU1, "id=%d\r\n", ak09916_id_);
	if(ak09916_id_ != 0x09)
	{
		set_led(LED7, 1);
	}

	// Reset magnetometer through ICM20948 (not direct communication)
	ICM_20948_set_bank(&icmDevice, 3);
	temp = MAG_AK09916_I2C_ADDR;
	ICM_20948_execute_w(&icmDevice, AGB3_REG_I2C_PERIPH0_ADDR, &temp, 1);
	temp = AK09916_REG_CNTL3;
	ICM_20948_execute_w(&icmDevice, AGB3_REG_I2C_PERIPH0_REG, &temp, 1);
	temp = 1;
	ICM_20948_execute_w(&icmDevice, AGB3_REG_I2C_PERIPH0_DO, &temp, 1);
	ICM_20948_set_bank(&icmDevice, 0);
	ICM_20948_execute_r(&icmDevice, AGB0_REG_USER_CTRL, &temp, 1); // Enable master i2c to actually perform the i2c operation
	temp2 = temp|0x20;
	ICM_20948_execute_w(&icmDevice, AGB0_REG_USER_CTRL, &temp2, 1);
	chThdSleepMilliseconds(10);
	ICM_20948_execute_w(&icmDevice, AGB0_REG_USER_CTRL, &temp, 1);

	chThdSleepMilliseconds(10); // Needed?

	// Configure magnetometer to continuous mode, 100 Hz
	ICM_20948_set_bank(&icmDevice, 3);
	temp = MAG_AK09916_I2C_ADDR;
	ICM_20948_execute_w(&icmDevice, AGB3_REG_I2C_PERIPH0_ADDR, &temp, 1);
	temp = AK09916_REG_CNTL2;
	ICM_20948_execute_w(&icmDevice, AGB3_REG_I2C_PERIPH0_REG, &temp, 1);
	temp = 8;
	ICM_20948_execute_w(&icmDevice, AGB3_REG_I2C_PERIPH0_DO, &temp, 1);
	ICM_20948_set_bank(&icmDevice, 0);
	ICM_20948_execute_r(&icmDevice, AGB0_REG_USER_CTRL, &temp, 1); // Enable master i2c to actually perform the i2c operation
	temp2 = temp|0x20;
	ICM_20948_execute_w(&icmDevice, AGB0_REG_USER_CTRL, &temp2, 1);
	chThdSleepMilliseconds(10);
	ICM_20948_execute_w(&icmDevice, AGB0_REG_USER_CTRL, &temp, 1);

	// Configure ICM20948 to automatically read from magnetometer 9 bytes starting from register ST1, basically from ST1 to ST2
	ICM_20948_set_bank(&icmDevice, 3);
	temp = MAG_AK09916_I2C_ADDR | 0x80;
	ICM_20948_execute_w(&icmDevice, AGB3_REG_I2C_PERIPH0_ADDR, &temp, 1);
	temp = AK09916_REG_ST1;
	ICM_20948_execute_w(&icmDevice, AGB3_REG_I2C_PERIPH0_REG, &temp, 1);
	temp = 0x89;
	ICM_20948_execute_w(&icmDevice, AGB3_REG_I2C_PERIPH0_CTRL, &temp, 1);

	return status;

}

 /**
 * @brief   Thread which updates the measures and publishes them
 */
static THD_WORKING_AREA(imu_reader_thd_wa, 1024);
static THD_FUNCTION(imu_reader_thd, arg) {
     (void) arg;
     chRegSetThreadName(__FUNCTION__);

     //event_listener_t imu_int;

     /* Starts waiting for the external interrupts. */
     //chEvtRegisterMaskWithFlags(&exti_events, &imu_int,
     //                           (eventmask_t)EXTI_EVENT_IMU_INT,
     //                           (eventflags_t)EXTI_EVENT_IMU_INT);

     // Declares the topic on the bus.
     messagebus_topic_t imu_topic;
     MUTEX_DECL(imu_topic_lock);
     CONDVAR_DECL(imu_topic_condvar);
     messagebus_topic_init(&imu_topic, &imu_topic_lock, &imu_topic_condvar, &imu_values, sizeof(imu_values));
     messagebus_advertise_topic(&bus, &imu_topic, "/imu");

     uint8_t accCalibrationNumSamples = 0;
     int32_t accCalibrationSum = 0;
     uint8_t gyroCalibrationNumSamples = 0;
     int32_t gyroCalibrationSum = 0;
     systime_t time;
     uint8_t mag_samples = 0;
     float mag_min[3] = {0.0};
     float mag_max[3] = {0.0};
     uint8_t mag_update_cycles = 0;

     // Set the magnetometer calibration values that will be used if no other calibrations will be accomplished.
	imu_values.mag_sens_adjust[0] = 1.148;
 	imu_values.mag_sens_adjust[1] = 1.097;
 	imu_values.mag_sens_adjust[2] = 1.445;
 	imu_values.mag_offset[0] = 37.0;
 	imu_values.mag_offset[1] = -3.5;
 	imu_values.mag_offset[2] = 187.0;
 	imu_values.mag_scale[0] = 1.43;
 	imu_values.mag_scale[1] = 0.99;
 	imu_values.mag_scale[2] = 0.77;

     while (chThdShouldTerminateX() == false) {
    	 time = chVTGetSystemTime();

      //    /* Waits for a measurement to come. */
      //    chEvtWaitAny(EXTI_EVENT_IMU_INT);
      //    //Clears the flag. Otherwise the event is always true
    	 // chEvtGetAndClearFlags(&imu_int);

    	if(imu_configured == true){
	 		/* Reads the incoming measurement. */
    		if(imuModel == IMU_MPU9250)
    		{
				mpu9250_read(	imu_values.gyro_rate, imu_values.acceleration, &imu_values.temperature,
								imu_values.magnetometer, imu_values.gyro_raw, imu_values.acc_raw,
								imu_values.gyro_offset, imu_values.acc_offset, &imu_values.status);
    		}
    		else
    		{
				icm20948_read(	&icmDevice, imu_values.gyro_rate, imu_values.acceleration, &imu_values.temperature,
								imu_values.magnetometer, imu_values.gyro_raw, imu_values.acc_raw,
								imu_values.gyro_offset, imu_values.acc_offset, &imu_values.status);
    		}
    	}


         /* Publishes it on the bus. */
         messagebus_topic_publish(&imu_topic, &imu_values, sizeof(imu_values));

         if(accAxisFilteringInProgress) {
         	switch(accAxisFilteringState) {
 				case 0:
 					imu_values.acc_offset[accAxisSelected] = 0;
 					accCalibrationSum = 0;
 					accCalibrationNumSamples = 0;
 					accAxisFilteringState = 1;
 					break;

 				case 1:
 					accCalibrationSum += imu_values.acc_raw[accAxisSelected];
 					accCalibrationNumSamples++;
 					if(accCalibrationNumSamples == accFilterSize) {
 						imu_values.acc_filtered[accAxisSelected] = accCalibrationSum/accFilterSize;
 						accAxisFilteringInProgress = 0;
 						if(accCalibrationInProgress == 1) {
 							imu_values.acc_offset[accAxisSelected] = imu_values.acc_filtered[accAxisSelected];
 							accCalibrationInProgress = 0;
 						}
 					}
 					break;
         	}
         }

         if(gyroAxisFilteringInProgress) {
         	switch(gyroAxisFilteringState) {
 				case 0:
 					imu_values.gyro_offset[gyroAxisSelected] = 0;
 					gyroCalibrationSum = 0;
 					gyroCalibrationNumSamples = 0;
 					gyroAxisFilteringState = 1;
 					break;

 				case 1:
 					gyroCalibrationSum += imu_values.gyro_raw[gyroAxisSelected];
 					gyroCalibrationNumSamples++;
 					if(gyroCalibrationNumSamples == gyroFilterSize) {
 						imu_values.gyro_filtered[gyroAxisSelected] = gyroCalibrationSum/gyroFilterSize;
 						gyroAxisFilteringInProgress = 0;
 						if(gyroCalibrationInProgress == 1) {
 							imu_values.gyro_offset[gyroAxisSelected] = imu_values.gyro_filtered[gyroAxisSelected];
 							gyroCalibrationInProgress = 0;
 						}
 					}
 					break;
         	}
         }

         if(magCalibrationInProgress) {
        	 switch(magCalibrationState) {
        	 	 case 0: // Reset calibration values.
        	 		 imu_values.mag_offset[0] = imu_values.mag_offset[1] = imu_values.mag_offset[2] = 0.0;
        	 		 imu_values.mag_scale[0] = imu_values.mag_scale[1] = imu_values.mag_scale[2] = 0.0;
        	 		 mag_samples = 0;
        	 		 mag_min[0] = mag_min[1] = mag_min[2] = 4912.0;
        	 		 mag_max[0] = mag_max[1] = mag_max[2] = -4912.0;
        	 		 mag_update_cycles = 35; // Magnetometer is updated @ 8 Hz, the thread run @ 250 Hz, plus a bit of safety (256/8+4=35).
        	 		 magCalibrationState = 1;
        	 		 break;

        	 	 case 1: // Compute new calibration values.
        	 		 mag_update_cycles++;
        	 		 if(mag_update_cycles >= 35) { // Magnetometer values are for sure updated after 35 thread cycles (about 140 ms).
        	 			 mag_update_cycles = 0;
        	 			 if(imu_values.magnetometer[0] < mag_min[0]) {
        	 				mag_min[0] = imu_values.magnetometer[0];
        	 			 }
        	 			 if(imu_values.magnetometer[0] > mag_max[0]) {
        	 				 mag_max[0] = imu_values.magnetometer[0];
        	 			 }
        	 			 if(imu_values.magnetometer[1] < mag_min[1]) {
        	 				mag_min[1] = imu_values.magnetometer[1];
        	 			 }
        	 			 if(imu_values.magnetometer[1] > mag_max[1]) {
        	 				 mag_max[1] = imu_values.magnetometer[1];
        	 			 }
        	 			 if(imu_values.magnetometer[2] < mag_min[2]) {
        	 				mag_min[2] = imu_values.magnetometer[2];
        	 			 }
        	 			 if(imu_values.magnetometer[2] > mag_max[2]) {
        	 				 mag_max[2] = imu_values.magnetometer[2];
        	 			 }

        	 			//chprintf((BaseSequentialStream *)&SDU1, "x=%f, y=%f, z=%f\r\n", imu_values.magnetometer[0], imu_values.magnetometer[1], imu_values.magnetometer[2]);

						 mag_samples++;
						 if(mag_samples >= 50) { // The calibration lasts about 35*4*50 = 7000 ms.

							 //chprintf((BaseSequentialStream *)&SDU1, "x=[%f;%f], y=[%f;%f], z=[%f;%f]\r\n", mag_min[0], mag_max[0], mag_min[1], mag_max[1], mag_min[2], mag_max[2]);

							 // Hard iron correction.
							 imu_values.mag_offset[0] = (mag_max[0] + mag_min[0]) / 2.0;
							 imu_values.mag_offset[1] = (mag_max[1] + mag_min[1]) / 2.0;
							 imu_values.mag_offset[2] = (mag_max[2] + mag_min[2]) / 2.0;

							 //chprintf((BaseSequentialStream *)&SDU1, "off_x=%f, off_y=%f, off_z=%f\r\n", imu_values.mag_offset[0], imu_values.mag_offset[1], imu_values.mag_offset[2]);

							 // Soft iron correction.
							 imu_values.mag_scale[0] = (mag_max[0] - mag_min[0]) / 2.0;
							 imu_values.mag_scale[1] = (mag_max[1] - mag_min[1]) / 2.0;
							 imu_values.mag_scale[2] = (mag_max[2] - mag_min[2]) / 2.0;
							 float avg = (imu_values.mag_scale[0] + imu_values.mag_scale[1] + imu_values.mag_scale[2])/3.0;
							 imu_values.mag_scale[0] = avg / imu_values.mag_scale[0];
							 imu_values.mag_scale[1] = avg / imu_values.mag_scale[1];
							 imu_values.mag_scale[2] = avg / imu_values.mag_scale[2];

							 //chprintf((BaseSequentialStream *)&SDU1, "scale_x=%f, scale_y=%f, scale_z=%f\r\n", imu_values.mag_scale[0], imu_values.mag_scale[1], imu_values.mag_scale[2]);

							 magCalibrationInProgress = 0;
						 }
        	 		 }
        	 		 break;
        	 }
         }

         chThdSleepUntilWindowed(time, time + MS2ST(4)); //reduced the sample rate to 250Hz

     }
}

/*************************END INTERNAL FUNCTIONS**********************************/


/****************************PUBLIC FUNCTIONS*************************************/

int8_t imu_start(void)
{
	int8_t status = MSG_OK;

	if(imu_configured) {
		return status;
	}

	chThdSleepMilliseconds(100); // IMU startup time.

	i2c_start();

	if(mpu9250_ping()) // MPU9250
	{
		imuModel = IMU_MPU9250;
	    status = mpu9250_setup(MPU9250_ACC_FULL_RANGE_2G
			                  | MPU9250_GYRO_FULL_RANGE_250DPS
			                  | MPU9250_SAMPLE_RATE_DIV(100));
			                  //| MPU60X0_LOW_PASS_FILTER_6)
	}
	else // ICM20948
	{
		imuModel = IMU_ICM20948;

		//status = icm20948_init();
		icmDevice._dmp_firmware_available = false;
		ICM_20948_link_serif(&icmDevice, &icmSerif);
		//chThdSleepMilliseconds(100); // Power-on time
		if(ICM_20948_check_id(&icmDevice) == ICM_20948_Stat_Ok)
		{
			status = MSG_OK;
		}
		else
		{
			status = -1;
			set_led(LED1, 1);
		}

		// Here we are doing a SW reset to make sure the device starts in a known state
		ICM_20948_sw_reset(&icmDevice);
		chThdSleepMilliseconds(250);
		ICM_20948_set_clock_source(&icmDevice, ICM_20948_Clock_Auto);

		uint8_t temp = 0x00;
		ICM_20948_set_bank(&icmDevice, 0);

		temp = 0x80;
		ICM_20948_execute_w(&icmDevice, AGB0_REG_PWR_MGMT_1, &temp, 1);
		chThdSleepMilliseconds(10);
        temp = 0x01;
        ICM_20948_execute_w(&icmDevice, AGB0_REG_PWR_MGMT_1, &temp, 1);

		temp = 0x00;
		ICM_20948_execute_w(&icmDevice, AGB0_REG_PWR_MGMT_2, &temp, 1); // Turn on gyro and acc.

		// Set Gyro and Accelerometer to continuous sample mode
		//ICM_20948_set_sample_mode(&icmDevice, (ICM_20948_InternalSensorID_bm)(ICM_20948_Internal_Acc | ICM_20948_Internal_Gyr | ICM_20948_Internal_Mst), ICM_20948_Sample_Mode_Continuous); // options: ICM_20948_Sample_Mode_Continuous. ICM_20948_Sample_Mode_Cycled

		// Set full scale ranges for both acc (+-2g) and gyr (+-250dps)
		ICM_20948_fss_t myfss;
		myfss.a = gpm2;   // (ICM_20948_ACCEL_CONFIG_FS_SEL_e)
		myfss.g = dps250; // (ICM_20948_GYRO_CONFIG_1_FS_SEL_e)
		ICM_20948_set_full_scale(&icmDevice, (ICM_20948_InternalSensorID_bm)(ICM_20948_Internal_Acc | ICM_20948_Internal_Gyr), myfss);

		// Set gyro (100 hz) and acc (~100 hz) sample rates
		ICM_20948_smplrt_t mySmplrt;
		mySmplrt.g = 10; // ODR is computed as follows: 1.1 kHz/(1+GYRO_SMPLRT_DIV[7:0]) => 10 = 100Hz.
		mySmplrt.a = 10; // ODR is computed as follows: 1.125 kHz/(1+ACCEL_SMPLRT_DIV[11:0]). 10 = 102Hz.
		ICM_20948_set_sample_rate(&icmDevice, (ICM_20948_InternalSensorID_bm)(ICM_20948_Internal_Acc | ICM_20948_Internal_Gyr), mySmplrt);

		// Do not to use DLPF => this inhibit the ODR configuration and thus the fifo fill rate!!!
		//ICM_20948_enable_dlpf(&icmDevice, ICM_20948_Internal_Acc, false);
		//ICM_20948_enable_dlpf(&icmDevice, ICM_20948_Internal_Gyr, false);

		// Now wake the sensor up
		//ICM_20948_sleep(&icmDevice, false);
		//ICM_20948_low_power(&icmDevice, false);

		temp = 0x30;
		ICM_20948_set_bank(&icmDevice, 0);
		ICM_20948_execute_w(&icmDevice, AGB0_REG_INT_PIN_CONFIG, &temp, 1);

		if(startup_magnetometer() != ICM_20948_Stat_Ok)
		{
			set_led(LED3, 1);
		}

		ICM_20948_set_bank(&icmDevice, 0);
		temp = 1;
		ICM_20948_execute_w(&icmDevice, AGB0_REG_FIFO_EN_1, &temp, 1); // Enable fifo from slave0
		temp = 0x1E;
		ICM_20948_execute_w(&icmDevice, AGB0_REG_FIFO_EN_2, &temp, 1); // Enable fifo from accel and gyro
		temp = 0x60;
		ICM_20948_execute_w(&icmDevice, AGB0_REG_USER_CTRL, &temp, 1); // Enable master i2c and fifo

	}

    //not tested yet because the auxilliary I2C of the MPU-9250 is condamned due
    //to PCB correction on the e-puck2-F4, so the magnetometer cannot be read...
    // if(status == MSG_OK){
    // 	status = mpu9250_magnetometer_setup();
    // }

    if(status == MSG_OK){
    	imu_configured = true;
    	imuThd = chThdCreateStatic(imu_reader_thd_wa, sizeof(imu_reader_thd_wa), NORMALPRIO, imu_reader_thd, NULL);
    }

    return status;
}

void imu_stop(void) {
    chThdTerminate(imuThd);
    chThdWait(imuThd);
    imuThd = NULL;
    imu_configured = false;
}

// Gets last axis value read from the sensor.
int16_t get_acc(uint8_t axis) {
	if(axis < 3) {
		return imu_values.acc_raw[axis];
	}
	return 0;
}

void get_acc_all(int16_t *values) {
	values[0] = imu_values.acc_raw[0];
	values[1] = imu_values.acc_raw[1];
	values[2] = imu_values.acc_raw[2];
}

// Returns an average of the last "filter_size" axis values read from the sensor.
int16_t get_acc_filtered(uint8_t axis, uint8_t filter_size) {
	if(axis < 3) {
		if(imu_configured == true){
			accAxisFilteringState = 0;
			accAxisFilteringInProgress = 1;
			accAxisSelected = axis;
			accFilterSize = filter_size;
			while(accAxisFilteringInProgress) {
				chThdSleepMilliseconds(20);
			}
		}
		return imu_values.acc_filtered[axis];
	}
	return 0;
}

// Saves an average of the last 50 samples for each axis, these values are the calibration/offset values.
void calibrate_acc(void) {
	if(imu_configured == true){
		accCalibrationInProgress = 1;
		get_acc_filtered(0, 50);
		accCalibrationInProgress = 1;
		get_acc_filtered(1, 50);
		accCalibrationInProgress = 1;
		get_acc_filtered(2, 50);
		accCalibrationInProgress = 0;
	}
}

// Returns the calibration value of the axis.
int16_t get_acc_offset(uint8_t axis) {
	if(axis < 3) {
		return imu_values.acc_offset[axis];
	}
	return 0;
}

float get_acceleration(uint8_t axis) {
	if(axis < 3) {
		return imu_values.acceleration[axis];
	}
	return 0;
}

int16_t get_gyro(uint8_t axis) {
	if(axis < 3) {
		return imu_values.gyro_raw[axis];
	}
	return 0;
}

void get_gyro_all(int16_t *values) {
	values[0] = imu_values.gyro_raw[0];
	values[1] = imu_values.gyro_raw[1];
	values[2] = imu_values.gyro_raw[2];
}

int16_t get_gyro_filtered(uint8_t axis, uint8_t filter_size) {
	if(axis < 3) {
		if(imu_configured == true){
			gyroAxisFilteringState = 0;
			gyroAxisFilteringInProgress = 1;
			gyroAxisSelected = axis;
			gyroFilterSize = filter_size;
			while(gyroAxisFilteringInProgress) {
				chThdSleepMilliseconds(20);
			}
		}
		return imu_values.gyro_filtered[axis];
	}
	return 0;
}

int16_t get_gyro_offset(uint8_t axis) {
	if(axis < 3) {
		return imu_values.gyro_offset[axis];
	}
	return 0;
}

// Saves an average of the last 50 samples for each axis, these values are the calibration/offset values.
void calibrate_gyro(void) {
	if(imu_configured == true){
		gyroCalibrationInProgress = 1;
		get_gyro_filtered(0, 50);
		gyroCalibrationInProgress = 1;
		get_gyro_filtered(1, 50);
		gyroCalibrationInProgress = 1;
		get_gyro_filtered(2, 50);
		gyroCalibrationInProgress = 0;
	}
	
}

float get_gyro_rate(uint8_t axis) {
	if(axis < 3) {
		return imu_values.gyro_rate[axis];
	}
	return 0;
}

float get_temperature(void) {
	return imu_values.temperature;
}

void calibrate_magnetometer(void) {
	if(imu_configured == true) {
		mpu9250_magnetometer_read_sens_adj(imu_values.mag_sens_adjust);
		magCalibrationState = 0;
		magCalibrationInProgress = 1;
		while(magCalibrationInProgress) {
			chThdSleepMilliseconds(20);
		}
	}
}

void get_mag_filtered(float *values) {
	// Get last measure.
	values[0] = imu_values.magnetometer[0];
	values[1] = imu_values.magnetometer[1];
	values[2] = imu_values.magnetometer[2];

	// Apply factory axial sensitivity adjustments.
	values[0] *= imu_values.mag_sens_adjust[0];
	values[1] *= imu_values.mag_sens_adjust[1];
	values[2] *= imu_values.mag_sens_adjust[2];

	// Apply hard iron ie. offset bias from calibration.
	values[0] -= imu_values.mag_offset[0];
	values[1] -= imu_values.mag_offset[1];
	values[2] -= imu_values.mag_offset[2];

	// Apply soft iron ie. scale bias from calibration.
	values[0] *= imu_values.mag_scale[0];
	values[1] *= imu_values.mag_scale[1];
	values[2] *= imu_values.mag_scale[2];
}

float get_magnetic_field(uint8_t axis) {
	if(axis < 3) {
		return imu_values.magnetometer[axis];
	}
	return 0;
}

/**************************END PUBLIC FUNCTIONS***********************************/

