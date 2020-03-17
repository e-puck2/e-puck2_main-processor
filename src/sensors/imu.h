#ifndef IMU_H
#define IMU_H

#ifdef __cplusplus
extern "C" {
#endif

#include <hal.h>
#include "sensors/mpu9250.h"

/** Message containing one measurement from the IMU. */
typedef struct {
    float acceleration[3]; // m/s^2
    float gyro_rate[3]; // rad/s
    float temperature;
    float magnetometer[3]; //uT (1 microtesla [uT] = 10 milligauss [mG])
    int16_t acc_raw[3];
    int16_t gyro_raw[3];
    int16_t acc_offset[3];
    int16_t gyro_offset[3];
    int16_t acc_filtered[3];
    int16_t gyro_filtered[3];
    uint8_t status;
    float mag_sens_adjust[3]; // Axial sensitivity adjustment factors.
    float mag_offset[3]; // Hard iron calibration factors.
    float mag_scale[3]; // Soft iron calibration factors.
} imu_msg_t;


 /**
 * @brief   Starts the Inertial Motion Unit (IMU) publisher.
 *          Broadcasts a imu_msg_t message on the /imu topic
 *
 * @return              The operation status.
 * @retval MSG_OK       if the function succeeded.
 * @retval MSG_TIMEOUT  if a timeout occurred before operation end
 */
int8_t imu_start(void);

/**
* @brief   Stop the Inertial Motion Unit (IMU) publisher.
*
*/
void imu_stop(void);

 /**
 * @brief   Returns the last accelerometer value measured
 *          for the axis given
 * 
 * @param axis      0-2, respectively x,y or z
 * 
 * @return          Last accelerometer value measured
 */
int16_t get_acc(uint8_t axis);

 /**
 * @brief   Returns the last accelerometer value measured
 *          for the three axis
 * 
 * @param value     pointer to a buffer (of at least a size of 3 * int16_t) 
 *                  to which store the measures   
 */
void get_acc_all(int16_t *values);

 /**
 * @brief   Returns an average of the last "filter_size" axis values 
 *          read from the accelerometer.
 *          
 * 
 * @param axis          0-2, respectively x,y or z
 * @param filter_size   number of samples to take for the averaging process  
 * 
 * @return              Averaged accelerometer measure  
 */
int16_t get_acc_filtered(uint8_t axis, uint8_t filter_size);

 /**
 * @brief   Returns the calibration value of the accelerometer
 *          for the axis given
 * 
 * @param axis      0-2, respectively x,y or z
 * 
 * @return          Calibration value of the accelerometer
 */
int16_t get_acc_offset(uint8_t axis);

 /**
 * @brief   Launches a calibration process of the accelerometer
 */
void calibrate_acc(void);

/**
* @brief   Returns the last acceleration (m/s^2) for the given axis
*
* @param axis      0-2, respectively x,y or z
*
* @return          Last measured acceleration
*/
float get_acceleration(uint8_t axis);

 /**
 * @brief   Returns the last gyroscope value measured
 *          for the axis given
 * 
 * @param axis      0-2, respectively x,y or z
 * 
 * @return          Last gyroscope value measured
 */
int16_t get_gyro(uint8_t axis);

 /**
 * @brief   Returns the last gyroscope value measured
 *          for the three axis
 * 
 * @param value     pointer to a buffer (of at least a size of 3 * int16_t) 
 *                  to which store the measures   
 */
void get_gyro_all(int16_t *values);

 /**
 * @brief   Returns an average of the last "filter_size" axis values 
 *          read from the gyroscope.
 *          
 * 
 * @param axis          0-2, respectively x,y or z
 * @param filter_size   number of samples to take for the averaging process  
 * 
 * @return              Averaged accelerometer measure  
 */
int16_t get_gyro_filtered(uint8_t axis, uint8_t filter_size);

 /**
 * @brief   Returns the calibration value of the gyroscope
 *          for the axis given
 * 
 * @param axis      0-2, respectively x,y or z
 * 
 * @return          Calibration value of the gyroscope
 */
int16_t get_gyro_offset(uint8_t axis);

 /**
 * @brief   Launches a calibration process of the gyroscope
 */
void calibrate_gyro(void);

/**
* @brief   Returns the last rate (rad/2) for the given axis
*
* @param axis      0-2, respectively x,y or z
*
* @return          Last measured rate
*/
float get_gyro_rate(uint8_t axis);

 /**
 * @brief   Returns the last temperature value measured
 * 
 * @return          Last temperature value measured in Celsius degrees.
 */
float get_temperature(void);

/**
* @brief   Returns the last magnetic field (micro Tesla) for the given axis.
*
* @param axis      0-2, respectively x,y or z
*
* @return          Last Last measured magnetic field.
*/
float get_magnetic_field(uint8_t axis);

/**
* @brief   Launches a calibration process of the magnetometer.
* The body LED of the robot will turn on during the calibration.
* The user should move the robot following an "8 shape" motion, more times if possible, until the body LED turn off.
*/
void calibrate_magnetometer(void);

/**
* @brief	Returns the last magnetometer values measured for the three axes, corrected for the hard and soft iron
*			distortions and adjusted with factory calibration parameters.
*
* @param value     pointer to a buffer (of at least a size of 3 * float)
*                  to which store the corrected measures
*/
void get_mag_filtered(float *values);

#ifdef __cplusplus
}
#endif
#endif /* IMU_H */
