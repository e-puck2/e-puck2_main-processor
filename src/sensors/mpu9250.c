#include <stdint.h>
#include <math.h>
#include "mpu9250.h"
#include "../i2c_bus.h"

#define STANDARD_GRAVITY 9.80665f
#define DEG2RAD(deg) (deg / 180 * M_PI)

static uint32_t imuConfig;

/***************************INTERNAL FUNCTIONS************************************/

 /**
 * @brief   reads the id of the sensor
 * 
 * @param id            pointer to store the id of the sensor
 * 
 * @return              The operation status.
 * @retval MSG_OK       if the function succeeded.
 * @retval MSG_TIMEOUT  if a timeout occurred before operation end
 */
int8_t mpu9250_read_id(uint8_t *id) {
    int8_t err = 0;
    if((err = read_reg(MPU9250_ADDRESS, WHO_AM_I_MPU9250, id)) != MSG_OK) {
        return err;
    }
    return MSG_OK;
}

 /**
 * @brief   reads a 16bit word from an 8bit buffer
 * 
 * @param buf           buffer to read
 * 
 * @return              The 16bit word read
 */
static int32_t read_word(const uint8_t *buf) // signed int16
{
    return (int16_t)((int8_t)buf[0]) << 8 | buf[1];
}

/*************************END INTERNAL FUNCTIONS**********************************/


/****************************PUBLIC FUNCTIONS*************************************/

int8_t mpu9250_setup(int config) {
	int8_t err = 0;
	uint8_t regValue = 0;
	
	imuConfig = config;

    // Reset device.
    if((err = write_reg(MPU9250_ADDRESS, PWR_MGMT_1, 0x80)) != MSG_OK) {
        return err;
    }	
    chThdSleepMilliseconds(1);
    while(1) {
		read_reg(MPU9250_ADDRESS, PWR_MGMT_1, &regValue);
		if(!(regValue & 0x80)) {
			break;
		}
        chThdSleepMilliseconds(1);
    }
	
    // Gyro full scale.
    if((err = write_reg(MPU9250_ADDRESS, GYRO_CONFIG, (config << 1) & 0x18)) != MSG_OK) {
        return err;
    }
    chThdSleepMilliseconds(1);

    // Accelerometer full scale.
    if((err = write_reg(MPU9250_ADDRESS, ACCEL_CONFIG, (config << 3) & 0x18)) != MSG_OK) {
        return err;
    }
    chThdSleepMilliseconds(1);

    // Sample rate divisor.
    if((err = write_reg(MPU9250_ADDRESS, SMPLRT_DIV, (config >> 8) & 0xff)) != MSG_OK) {
        return err;
    }
    chThdSleepMilliseconds(1);

    // Enable interrupts: data ready.
    if((err = write_reg(MPU9250_ADDRESS, INT_ENABLE, INTERRUPT_DATA_RDY)) != MSG_OK) {
        return err;
    }
    chThdSleepMilliseconds(1);

//    // low pass filter config, FSYNC disabled
//    mpu60X0_reg_write(dev, MPU60X0_RA_CONFIG, (config >> 16) & 0x07);
//    chThdSleepMilliseconds(1);

    return err;
	
}

bool mpu9250_ping(void) {
	uint8_t id = 0;
	mpu9250_read_id(&id);
	return id == 0x68;
}

void mpu9250_read(float *gyro, float *acc, float *temp, int16_t *gyro_raw, int16_t *acc_raw, uint8_t *status) {
    static const float gyro_res[] = { DEG2RAD(1 / 131.f),
                                      DEG2RAD(1 / 65.5f),
                                      DEG2RAD(1 / 32.8f),
                                      DEG2RAD(1 / 16.4f) }; // rad/s/LSB
    static const float acc_res[] = { STANDARD_GRAVITY / 16384.f,
                                     STANDARD_GRAVITY / 8192.f,
                                     STANDARD_GRAVITY / 4096.f,
                                     STANDARD_GRAVITY / 2048.f }; // m/s^2 / LSB
    uint8_t buf[1 + 6 + 2 + 6]; // interrupt status, accel, temp, gyro
    read_reg_multi(MPU9250_ADDRESS, INT_STATUS, buf, sizeof(buf));
    if(status) {
    	*status = buf[0];
    }
    if (acc) {
    	// Change the sign of all axes to have -1g when the robot is still on the plane and the axis points upwards and is perpendicular to the surface.
    	acc_raw[0] = -read_word(&buf[1]);
    	acc_raw[1] = -read_word(&buf[3]);
    	acc_raw[2] = -read_word(&buf[5]);
        acc[0] = (float)acc_raw[0] * acc_res[imuConfig & 0x3];
        acc[1] = (float)acc_raw[1] * acc_res[imuConfig & 0x3];
        acc[2] = (float)acc_raw[2] * acc_res[imuConfig & 0x3];
    }
    if (temp) {
        *temp = (float)((read_word(&buf[7]) - 21.0f) / 333.87f) + 21.0f; // Degrees.
    }
    if (gyro) {
    	gyro_raw[0] = read_word(&buf[9]);
    	gyro_raw[1] = read_word(&buf[11]);
    	gyro_raw[2] = read_word(&buf[13]);
        gyro[0] = (float)gyro_raw[0] * gyro_res[(imuConfig >> 2) & 0x3];
        gyro[1] = (float)gyro_raw[1] * gyro_res[(imuConfig >> 2) & 0x3];
        gyro[2] = (float)gyro_raw[2] * gyro_res[(imuConfig >> 2) & 0x3];
    }
}

/**************************END PUBLIC FUNCTIONS***********************************/

