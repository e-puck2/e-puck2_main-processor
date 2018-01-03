
/*! \file
 * \brief Manage LSM330 (accelerometer + gyro) registers
 * \author Stefano Morgani
 *
 * x, y axes representation on e-puck rev < 1.3 (analog accelerometer MMA7260):
 *      FORWARD
 *          ^
 *          | (-y)
 *          * - > (+x) RIGHT
 *
 * x, y axes representation on e-puck rev 1.3 (i2c device LSM330):
 *      FORWARD
 *          ^
 *          | (-x)
 *          * - > (+y) RIGHT
 *
 * x, y axes representation on e-puck 2 (i2c device LSM330):
 *      FORWARD
 *          ^
 *          | (-y)
 *          * - > (-x) RIGHT
 * Thus the x and y axes are exchanged to maintain compatibility.
 */

#include "e_lsm330.h"
#include "../sensors/imu.h"


/*!\brief Configure and turn on the device.
 */
void initAccAndGyro(void) {
	return;
}

void getAllAxesAccRaw(unsigned char *arr) {
	get_acc_all((int16_t*) arr);
}

void getAllAxesAcc(signed int *x, signed int *y, signed int *z) {
    int16_t arr[3];
    get_acc_all(arr);
    *x = arr[0];
    *y = arr[1];
    *z = arr[2];
}

int getXAxisAcc(void) {
	return get_acc(0);
}

int getYAxisAcc(void) {
	return get_acc(1);
}

int getZAxisAcc(void) {
	return get_acc(2);
}

void getAllAxesGyroRaw(unsigned char *arr) {
	get_gyro_all((int16_t*) arr);
}

void getAllAxesGyro(signed int *x, signed int *y, signed int *z) {
    int16_t arr[3];
    get_gyro_all(arr);
    *x = arr[0] - get_gyro_offset(0);
    *y = arr[1] - get_gyro_offset(1);
    *z = arr[2] - get_gyro_offset(2);
}

int getXAxisGyro(void) {
	return (get_gyro(0) - get_gyro_offset(0));
}

int getYAxisGyro(void) {
	return (get_gyro(1) - get_gyro_offset(1));
}

int getZAxisGyro(void) {
	return (get_gyro(2) - get_gyro_offset(2));
}

signed char getTemperature(void) {
	return (signed char)get_temperature();
}

void calibrateGyroscope(int numSamples) {
	(void)numSamples;
	calibrate_gyro();
}

/*! \brief Raw value to degrees per second.
 * Take as input the 2's complement value received by the device for a certain axis
 * and return the degrees per second (knowing the gyroscope is configured with sensitivity
 * of +- 250 dps).
 */
float rawToDps(int value) {
    // The gyroscope returns 16 bits 2's complement values with a full-scale of +-250 dps;
    // from the datasheet with full scale = +- 250 dps, then angular rate sensitivity is 8.75 mdps/digit
//    return (float)value*0.00875;
	(void)value;
	return 0;
}

/*! \brief Raw value to degrees per millisecond.
 * Take as input the 2's complement value received by the device for a certain axis
 * and return the degrees per millisecond (knowing the gyroscope is configured with sensitivity
 * of +- 250 dps).
 */
float rawToDpms(int value) {
//    return (float)value*0.00000875;
	(void)value;
	return 0;
}
