#ifndef IMU_H
#define IMU_H

#ifdef __cplusplus
extern "C" {
#endif

#include <hal.h>

/** Message containing one measurement from the IMU. */
typedef struct {
    float acceleration[3]; // m/s^2
    float gyro[3]; // rad/s
    float temperature;
    int16_t acc_raw[3];
    int16_t gyro_raw[3];
    int16_t acc_raw_offset[3];
    int16_t gyro_raw_offset[3];
    int16_t acc_raw_filtered[3];
    int16_t gyro_raw_filtered[3];
    uint8_t status;
} imu_msg_t;

/** Starts the Inertial Motion Unit (IMU) publisher. */
void imu_start(void);

int16_t get_acc(uint8_t axis);
void get_acc_all(int16_t *values);
int16_t get_acc_filtered(uint8_t axis, uint8_t filter_size);
int16_t get_acc_offset(uint8_t axis);
void calibrate_acc(void);

int16_t get_gyro(uint8_t axis);
void get_gyro_all(int16_t *values);
int16_t get_gyro_filtered(uint8_t axis, uint8_t filter_size);
int16_t get_gyro_offset(uint8_t axis);
void calibrate_gyro(void);

float get_temperature(void);

#ifdef __cplusplus
}
#endif
#endif /* IMU_H */
