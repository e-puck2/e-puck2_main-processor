#ifndef I2C_BUS_H
#define I2C_BUS_H

#ifdef __cplusplus
extern "C" {
#endif

void i2c_start(void);
void i2c_stop(void);
i2cflags_t get_last_i2c_error(void);
int8_t read_reg(uint8_t addr, uint8_t reg, uint8_t *value);
int8_t write_reg(uint8_t addr, uint8_t reg, uint8_t value);
int8_t read_reg_multi(uint8_t addr, uint8_t reg, uint8_t *buf, int8_t len);

#ifdef __cplusplus
}
#endif

#endif
