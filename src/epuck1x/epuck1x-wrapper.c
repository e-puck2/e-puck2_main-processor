#include <hal.h>
#include <stdlib.h>
#include <math.h>
#include "epuck1x-wrapper.h"
#include "..\audio\microphone.h"
#include "..\camera\dcmi_camera.h"
#include "..\camera\po8030.h"
#include "..\sensors\imu.h"
#include "..\sensors\proximity.h"
#include "..\leds.h"
#include "..\main.h"
#include "..\motors.h"
#include "..\usbcfg.h"
#include "..\utility.h"


// LEDs handling.
void e_set_led(unsigned int led_number, unsigned int value) {
	set_led(led_number, value);
}

void e_led_clear(void) {
	clear_leds();
}

void e_set_body_led(unsigned int value) {
	set_body_led(value);
}

void e_set_front_led(unsigned int value) {
	set_front_led(value);
}

// Various.
int getselector(void) {
	return get_selector();
}

// UARTs handling.
void e_init_uart1(void) {
	return;
}

int  e_ischar_uart1(void) {
	return 0;
}

int  e_getchar_uart1(char *car) {
	(void)car;
	return 0;
}

void e_send_uart1_char(const char * buff, int length) {
	(void)buff;
	(void)length;
	return;
}

int  e_uart1_sending(void) {
	return 0;
}

void e_init_uart2(int baud) {
	(void)baud;
	return;
}

int  e_ischar_uart2(void) {
	return 0;
}

int  e_getchar_uart2(char *car) {
	if (SDU1.config->usbp->state == USB_ACTIVE) {
		//return chnReadTimeout(&SDU1, car, 1, TIME_IMMEDIATE);
		//return chnReadTimeout(&SDU1, car, 1, MS2ST(10));
		return chSequentialStreamRead(&SDU1, (uint8_t*)car, 1);
	}
	return 0;
}

void e_send_uart2_char(const char * buff, int length) {
	if (SDU1.config->usbp->state == USB_ACTIVE) {
		//chnWriteTimeout(&SDU1, (uint8_t*)buff, length, TIME_INFINITE);
		chSequentialStreamWrite(&SDU1, (uint8_t*)buff, length);
	}
}

int  e_uart2_sending(void) {
	return 0;
}

// Motors handling.
void e_init_motors(void) {
	return;
}

void e_set_speed_left(int motor_speed) {
	left_motor_set_speed(motor_speed*2);	// Multiplied by 2 because on e-puck2 the motors are handled with 8 steps per cycle instead of 4 as done with e-puck1.x.
											// Thus we need twice the number of steps to get the same speed as with the e-puck1.x.
}

void e_set_speed_right(int motor_speed) {
	right_motor_set_speed(motor_speed*2);	// Multiplied by 2 because on e-puck2 the motors are handled with 8 steps per cycle instead of 4 as done with e-puck1.x.
											// Thus we need twice the number of steps to get the same speed as with the e-puck1.x.
}

void e_set_speed(int linear_speed, int angular_speed) {
	if(abs(linear_speed) + abs(angular_speed) > MOTOR_SPEED_LIMIT) {
		return;
	} else {
		// Multiplied by 2 because on e-puck2 the motors are handled with 8 steps per cycle instead of 4 as done with e-puck1.x.
		// Thus we need twice the number of steps to get the same speed as with the e-puck1.x.
		left_motor_set_speed ((linear_speed - angular_speed)*2);
		right_motor_set_speed((linear_speed + angular_speed)*2);
	}
}

void e_set_steps_left(int steps_left) {
	(void)steps_left;
	return;
}

void e_set_steps_right(int steps_right) {
	(void)steps_right;
	return;
}

int e_get_steps_left(void) {
	return left_motor_get_pos();
}

int e_get_steps_right(void) {
	return right_motor_get_pos();
}

// Proximity handling.
void e_calibrate_ir(void) {
	calibrate_ir();
}

int e_get_prox(unsigned int sensor_number) {
	return get_prox(sensor_number);
}

int e_get_calibrated_prox(unsigned int sensor_number) {
	return get_calibrated_prox(sensor_number);
}

int e_get_ambient_light(unsigned int sensor_number) {
	return get_ambient_light(sensor_number);
}

// Camera handling.
/**
 * Initalize the camera, return the version in hex, 0x3030, 0x6030 or 0x8030.
 */
int e_poxxxx_init_cam(void) {

	capture_mode = CAPTURE_ONE_SHOT;
	double_buffering = 0;
	po8030_save_current_format(FORMAT_RGB565);
	po8030_save_current_subsampling(SUBSAMPLING_X4, SUBSAMPLING_X4);
	po8030_advanced_config(FORMAT_RGB565, 240, 160, 160, 160, SUBSAMPLING_X4, SUBSAMPLING_X4);
	sample_buffer = (uint8_t*)malloc(po8030_get_image_size());
	dcmi_prepare(&DCMID, &dcmicfg, po8030_get_image_size(), (uint32_t*)sample_buffer, NULL);

	return 0x8030;
}

int e_poxxxx_config_cam(unsigned int sensor_x1,unsigned int sensor_y1,
			 unsigned int sensor_width,unsigned int sensor_height,
			 unsigned int zoom_fact_width,unsigned int zoom_fact_height,
			 int color_mode) {
	(void)sensor_x1;
	(void)sensor_y1;
	(void)sensor_width;
	(void)sensor_height;
	(void)zoom_fact_width;
	(void)zoom_fact_height;
	(void)color_mode;
	return 0;
}

void e_poxxxx_write_cam_registers(void) {
	return;
}

void e_poxxxx_launch_capture(char * buf) {
	(void)buf;
	dcmi_start_one_shot(&DCMID);
}

/*! Check if the current capture is finished
 * \return Zero if the current capture is in progress, non-zero if the capture is done.
 * \sa e_poxxxx_launch_capture
 */
int e_poxxxx_is_img_ready(void) {
	return image_is_ready();
}

// Accelerometer handling.
/*! \brief Read the last value of a given accelerator axes
 * \param captor		ID of the AD channel to read
 *						(must be 0 = x, 1 = y or 2 = z)
 * \return value		filtered channel's value
 */
int e_get_acc(unsigned int captor) {
	return get_acc(captor);
}

/*! \brief Read the value of a channel, filtered by an averaging filter
 * \param captor		ID of the AD channel to read (must be 0 to 2)
 * \param filter_size	size of the filter (must be between 1 and SAMPLE_NUMBER)
 * \return value		filtered channel's value
 */
int e_get_acc_filtered(unsigned int captor, unsigned int filter_size) {
	return get_acc_filtered(captor, filter_size);
}

TypeAccSpheric e_read_acc_spheric(void) {
	TypeAccSpheric result;
	int16_t acc_x, acc_y, acc_z;
	acc_x = get_acc_filtered(0, FILTER_SIZE) - get_acc_offset(0);	// generates positive
	acc_y = get_acc_filtered(1, FILTER_SIZE) - get_acc_offset(1);	// and negative value
	acc_z = get_acc_filtered(2, FILTER_SIZE) - get_acc_offset(2);	// to make the use easy

	// Calculate the absolute acceleration value.
	result.acceleration = sqrtf((float)(((long)acc_x * (long)acc_x) + ((long)acc_y * (long)acc_y) + ((long)acc_z * (long)acc_z)));
	result.inclination =  90.0 - atan2f((float)(acc_z), sqrtf( (float)(((long)acc_x * (long)acc_x) + ((long)acc_y * (long)acc_y) ))) * CST_RADIAN;
	if (result.inclination<5 || result.inclination>160) {
		result.orientation=0;
	} else {
		result.orientation = (atan2f((float)(acc_x), (float)(acc_y)) * CST_RADIAN) + 180.0;		// 180 is added to have 0 to 360° range
	}
	return result;
}

float e_read_orientation(void) {
	TypeAccSpheric result;
	int16_t acc_x, acc_y, acc_z;
	acc_x = get_acc_filtered(0, FILTER_SIZE) - get_acc_offset(0);	// generates positive
	acc_y = get_acc_filtered(1, FILTER_SIZE) - get_acc_offset(1);	// and negative value
	acc_z = get_acc_filtered(2, FILTER_SIZE) - get_acc_offset(2);	// to make the use easy

	result.inclination =  90.0 - atan2f((float)(acc_z), sqrtf( (float)(((long)acc_x * (long)acc_x) + ((long)acc_y * (long)acc_y) ))) * CST_RADIAN;
	if (result.inclination<5 || result.inclination>160) {
		result.orientation=0;
	} else {
		result.orientation = (atan2f((float)(acc_x), (float)(acc_y)) * CST_RADIAN) + 180.0;		// 180 is added to have 0 to 360° range
	}
	return result.orientation;
}

float e_read_inclination(void) {
	TypeAccSpheric result;
	int16_t acc_x, acc_y, acc_z;
	acc_x = get_acc_filtered(0, FILTER_SIZE) - get_acc_offset(0);	// generates positive
	acc_y = get_acc_filtered(1, FILTER_SIZE) - get_acc_offset(1);	// and negative value
	acc_z = get_acc_filtered(2, FILTER_SIZE) - get_acc_offset(2);	// to make the use easy

	result.inclination =  90.0 - atan2f((float)(acc_z), sqrtf( (float)(((long)acc_x * (long)acc_x) + ((long)acc_y * (long)acc_y) ))) * CST_RADIAN;
	return result.inclination;
}

float e_read_acc(void) {
	TypeAccSpheric result;
	int16_t acc_x, acc_y, acc_z;
	acc_x = get_acc_filtered(0, FILTER_SIZE) - get_acc_offset(0);	// generates positive
	acc_y = get_acc_filtered(1, FILTER_SIZE) - get_acc_offset(1);	// and negative value
	acc_z = get_acc_filtered(2, FILTER_SIZE) - get_acc_offset(2);	// to make the use easy

	result.acceleration = sqrtf((float)(((long)acc_x * (long)acc_x) + ((long)acc_y * (long)acc_y) + ((long)acc_z * (long)acc_z)));
	return result.acceleration;
}

TypeAccRaw e_read_acc_xyz(void) {
	TypeAccRaw result;
	result.acc_x = get_acc_filtered(0, FILTER_SIZE) - get_acc_offset(0);	// generates positive
	result.acc_y = get_acc_filtered(1, FILTER_SIZE) - get_acc_offset(1);	// and negative value
	result.acc_z = get_acc_filtered(2, FILTER_SIZE) - get_acc_offset(2);	// to make the use easy
	return result;
}

int e_read_acc_x(void) {
	return (get_acc_filtered(0, FILTER_SIZE) - get_acc_offset(0));
}

int e_read_acc_y(void) {
	return (get_acc_filtered(1, FILTER_SIZE) - get_acc_offset(1));
}

int e_read_acc_z(void) {
	return (get_acc_filtered(2, FILTER_SIZE) - get_acc_offset(2));
}

void e_acc_calibr(void) {
	calibrate_acc();
}

void e_display_angle(void) {

}

int e_get_micro(unsigned int micro_id) {
	int16_t value = 0;
	if(micro_id == 2) {
		value = mic_get_last(3); // Front and back microphones are swapped in the e-puck2 with respect to e-puck1.x.
	} else {
		value = mic_get_last(micro_id);
	}
	return (value>>4); // Adapt the values to be compatible with e-puck1.x (16 bits => 12 bits).
}

int e_get_micro_average(unsigned int micro_id, unsigned int filter_size) {
	(void)micro_id;
	(void)filter_size;
	return 0;
}

int e_get_micro_volume (unsigned int micro_id) {
	int32_t value = 0;
	if(micro_id == 2) {
		value = mic_get_volume(3); // Front and back microphones are swapped in the e-puck2 with respect to e-puck1.x.
	} else {
		value = mic_get_volume(micro_id);
	}
	return (value>>4); // Adapt the values to be compatible with e-puck1.x (16 bits => 12 bits).
}

