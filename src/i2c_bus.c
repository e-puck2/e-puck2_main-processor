#include <hal.h>
#include <ch.h>
#include "i2c_bus.h"

static i2cflags_t errors = 0;
static systime_t timeout = MS2ST(4); // 4 ms

void i2c_start(void) {
    /*
     * I2C configuration structure for camera, IMU and distance sensor.
     * Set it to 400kHz fast mode
     */
    static const I2CConfig i2c_cfg1 = {
        .op_mode = OPMODE_I2C,
        .clock_speed = 400000,
        .duty_cycle = FAST_DUTY_CYCLE_2
    };

    i2cStart(&I2CD1, &i2c_cfg1);
}

void i2c_stop(void) {
	i2cAcquireBus(&I2CD1);
	i2cStop(&I2CD1);
	i2cReleaseBus(&I2CD1);
}

i2cflags_t get_last_i2c_error(void) {
    return errors;
}

int8_t read_reg(uint8_t addr, uint8_t reg, uint8_t *value) {
	
	uint8_t txbuf[1] = {reg};
	uint8_t rxbuf[1] = {0};

	i2cAcquireBus(&I2CD1);
	if(I2CD1.state != I2C_STOP) {
		msg_t status = i2cMasterTransmitTimeout(&I2CD1, addr, txbuf, 1, rxbuf, 1, timeout);
		if (status != MSG_OK){
			errors = i2cGetErrors(&I2CD1);
			return status;
		}
	}
	i2cReleaseBus(&I2CD1);

	*value = rxbuf[0];

    return MSG_OK;
}


int8_t write_reg(uint8_t addr, uint8_t reg, uint8_t value) {

	uint8_t txbuf[2] = {reg, value};
	uint8_t rxbuf[1] = {0};

	i2cAcquireBus(&I2CD1);
	if(I2CD1.state != I2C_STOP) {
		msg_t status = i2cMasterTransmitTimeout(&I2CD1, addr, txbuf, 2, rxbuf, 0, timeout);
		if (status != MSG_OK){
			errors = i2cGetErrors(&I2CD1);
			return status;
		}
	}
	i2cReleaseBus(&I2CD1);

    return MSG_OK;
}

int8_t read_reg_multi(uint8_t addr, uint8_t reg, uint8_t *buf, int8_t len) {

	i2cAcquireBus(&I2CD1);
	if(I2CD1.state != I2C_STOP) {
		msg_t status = i2cMasterTransmitTimeout(&I2CD1, addr, &reg, 1, buf, len, timeout);
		if (status != MSG_OK){
			errors = i2cGetErrors(&I2CD1);
			return status;
		}
	}
	i2cReleaseBus(&I2CD1);

	return MSG_OK;
}
