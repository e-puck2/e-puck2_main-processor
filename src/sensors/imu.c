#include <ch.h>
#include <hal.h>
#include "main.h"
#include "usbcfg.h"
#include "chprintf.h"
#include "imu.h"
#include "sensors/mpu9250.h"
#include "exti.h"

imu_msg_t imu_values;
uint8_t accAxisFilteringInProgress = 0;
uint8_t accAxisFilteringState = 0;
uint8_t accAxisSelected = 0;
uint8_t filterSize = 0;
uint8_t accCalibrationInProgress = 0;

static THD_FUNCTION(imu_reader_thd, arg) {
     (void) arg;
     chRegSetThreadName(__FUNCTION__);

     event_listener_t imu_int;

     /* Starts waiting for the external interrupts. */
     chEvtRegisterMaskWithFlags(&exti_events, &imu_int,
                                (eventmask_t)EXTI_EVENT_IMU_INT,
                                (eventflags_t)EXTI_EVENT_IMU_INT);

     // Declares the topic on the bus.
     messagebus_topic_t imu_topic;
     MUTEX_DECL(imu_topic_lock);
     CONDVAR_DECL(imu_topic_condvar);
     messagebus_topic_init(&imu_topic, &imu_topic_lock, &imu_topic_condvar, &imu_values, sizeof(imu_values));
     messagebus_advertise_topic(&bus, &imu_topic, "/imu");

     uint8_t calibrationNumSamples = 0;
     int32_t calibrationSum = 0;

     while (true) {

         /* Wait for a measurement to come. */
         chEvtWaitAny(EXTI_EVENT_IMU_INT);

//         /* Read the incoming measurement. */
    	 mpu9250_read(imu_values.gyro, imu_values.acceleration, &imu_values.temperature, imu_values.gyro_raw, imu_values.acc_raw, &imu_values.status);

         /* Publish it on the bus. */
         messagebus_topic_publish(&imu_topic, &imu_values, sizeof(imu_values));

         if(accAxisFilteringInProgress) {
         	switch(accAxisFilteringState) {
 				case 0:
 					imu_values.acc_raw_offset[accAxisSelected] = 0;
 					calibrationSum = 0;
 					calibrationNumSamples = 0;
 					accAxisFilteringState = 1;
 					break;

 				case 1:
 					calibrationSum += imu_values.acc_raw[accAxisSelected];
 					calibrationNumSamples++;
 					if(calibrationNumSamples == filterSize) {
 						imu_values.acc_raw_filtered[accAxisSelected] = calibrationSum/filterSize;
 						accAxisFilteringInProgress = 0;
 						if(accCalibrationInProgress == 1) {
 							imu_values.acc_raw_offset[accAxisSelected] = imu_values.acc_raw_filtered[accAxisSelected];
 							accCalibrationInProgress = 0;
 						}
 					}
 					break;
         	}
         }

     }
}

void imu_start(void)
{
    mpu9250_setup(MPU9250_ACC_FULL_RANGE_2G
                  | MPU9250_GYRO_FULL_RANGE_250DPS
                  | MPU9250_SAMPLE_RATE_DIV(100));
                  //| MPU60X0_LOW_PASS_FILTER_6);

    static THD_WORKING_AREA(imu_reader_thd_wa, 1024);
    chThdCreateStatic(imu_reader_thd_wa, sizeof(imu_reader_thd_wa), NORMALPRIO, imu_reader_thd, NULL);
}

// Get last axis value read from the sensor.
int16_t get_acc(uint8_t axis) {
	if(axis < 3) {
		return imu_values.acc_raw[axis];
	}
	return 0;
}

// Return an average of the last "filter_size" axis values read from the sensor.
int16_t get_acc_filtered(uint8_t axis, uint8_t filter_size) {
	if(axis < 3) {
		accAxisFilteringState = 0;
		accAxisFilteringInProgress = 1;
		accAxisSelected = axis;
		filterSize = filter_size;
		while(accAxisFilteringInProgress) {
			chThdSleepMilliseconds(20);
		}
		return imu_values.acc_raw_filtered[axis];
	}
	return 0;
}

// Save an average of the last 50 samples for each axis, these values are the calibration/offset values.
void calibrate_acc(void) {
	accCalibrationInProgress = 1;
	get_acc_filtered(0, 50);
	get_acc_filtered(1, 50);
	get_acc_filtered(2, 50);
	accCalibrationInProgress = 0;
}

// Return the calibration value of the axis.
int16_t get_acc_offset(uint8_t axis) {
	return imu_values.acc_raw_offset[axis];
}


