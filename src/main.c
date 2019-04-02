#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "ch.h"
#include "chprintf.h"
#include "hal.h"
#include "shell.h"

#include "aseba_vm/aseba_node.h"
#include "aseba_vm/skel_user.h"
#include "aseba_vm/aseba_can_interface.h"
#include "aseba_vm/aseba_bridge.h"
#include "audio/audio_thread.h"
#include "audio/play_melody.h"
#include "audio/play_sound_file.h"
#include "audio/microphone.h"
#include "camera/camera.h"
#include "epuck1x/Asercom.h"
#include "epuck1x/Asercom2.h"
#include "epuck1x/a_d/advance_ad_scan/e_acc.h"
#include "epuck1x/motor_led/advance_one_timer/e_led.h"
#include "epuck1x/utility/utility.h"
#include "sensors/battery_level.h"
#include "sensors/ground.h"
#include "sensors/imu.h"
#include "sensors/mpu9250.h"
#include "sensors/proximity.h"
#include "sensors/VL53L0X/VL53L0X.h"
#include "button.h"
#include "cmd.h"
#include "config_flash_storage.h"
#include "exti.h"
#include "fat.h"
#include "i2c_bus.h"
#include "ir_remote.h"
#include "leds.h"
#include <main.h>
#include "memory_protection.h"
#include "motors.h"
#include "sdio.h"
#include "selector.h"
#include "serial_comm.h"
#include "spi_comm.h"
#include "usbcfg.h"
#include "communication.h"
#include "uc_usage.h"

#define SHELL_WA_SIZE   THD_WORKING_AREA_SIZE(2048)

messagebus_t bus;
MUTEX_DECL(bus_lock);
CONDVAR_DECL(bus_condvar);

parameter_namespace_t parameter_root, aseba_ns;

static THD_WORKING_AREA(selector_thd_wa, 2048);

static bool load_config(void)
{
    extern uint32_t _config_start;

    return config_load(&parameter_root, &_config_start);
}

static THD_FUNCTION(selector_thd, arg)
{
    (void) arg;
    chRegSetThreadName(__FUNCTION__);

    uint8_t stop_loop = 0;
    systime_t time;

    messagebus_topic_t *prox_topic = messagebus_find_topic_blocking(&bus, "/proximity");
    proximity_msg_t prox_values;
    int16_t leftSpeed = 0, rightSpeed = 0;
    int16_t prox_values_temp[8];

    messagebus_topic_t *imu_topic = messagebus_find_topic_blocking(&bus, "/imu");
    imu_msg_t imu_values;

    uint16_t prox_thr = 1000;

    uint8_t hw_test_state = 0;
    uint8_t *img_buff_ptr;
    uint16_t r = 0, g = 0, b = 0;
    uint8_t rgb_state = 0, rgb_counter = 0;
    uint16_t melody_state = 0, melody_counter = 0;

    uint8_t magneto_state = 0;
    
    uint8_t demo15_state = 0;
    uint8_t temp_rx = 0;

	uint8_t rab_addr = 0x20;
	uint8_t rab_state = 0;
	int8_t i2c_err = 0;
	uint8_t regValue[2] = {0};
	uint16_t rab_data = 0;
	double rab_bearing = 0.0;
	uint16_t rab_range = 0;
	uint16_t rab_sensor = 0;

	uint8_t back_and_forth_state = 0;
	float turn_angle_rad = 0.0;
	uint8_t led_animation_state = 0;
	uint32_t led_animation_count = 0;

	uint8_t wav_volume = 20;
	uint8_t wav_play_state = 0;

	double heading = 0.0;
	float mag_values[3];

	calibrate_acc();
	calibrate_gyro();
	calibrate_ir();

    while(stop_loop == 0) {
    	time = chVTGetSystemTime();

		switch(get_selector()) {
			case 0: // Aseba.
				aseba_vm_start();
				stop_loop = 1;
				break;

			case 1: // Shell.
				shell_start();
				stop_loop = 1;
				break;

			case 2: // Turn on the LEDs for the proximities that have an obstacle in front of them.
				messagebus_topic_wait(prox_topic, &prox_values, sizeof(prox_values));

				if((get_calibrated_prox(0) > prox_thr) || (get_calibrated_prox(7) > prox_thr)) {
					e_set_led(0, 1);
				} else {
					e_set_led(0, 0);
				}

				if(get_calibrated_prox(1) > prox_thr) {
					e_set_led(1, 1);
				} else {
					e_set_led(1, 0);
				}

				if(get_calibrated_prox(2) > prox_thr) {
					e_set_led(2, 1);
				} else {
					e_set_led(2, 0);
				}

				if(get_calibrated_prox(3) > prox_thr) {
					e_set_led(3, 1);
				} else {
					e_set_led(3, 0);
				}

				if((get_calibrated_prox(3) > prox_thr) || (get_calibrated_prox(4) > prox_thr)) {
					e_set_led(4, 1);
				} else {
					e_set_led(4, 0);
				}

				if(get_calibrated_prox(4) > prox_thr) {
					e_set_led(5, 1);
				} else {
					e_set_led(5, 0);
				}

				if(get_calibrated_prox(5) > prox_thr) {
					e_set_led(6, 1);
				} else {
					e_set_led(6, 0);
				}

				if(get_calibrated_prox(6) > prox_thr) {
					e_set_led(7, 1);
				} else {
					e_set_led(7, 0);
				}

				if (SDU1.config->usbp->state == USB_ACTIVE) { // Skip printing if port not opened.
					// Sensors info print: each line contains data related to a single sensor.
			        for (uint8_t i = 0; i < sizeof(prox_values.ambient)/sizeof(prox_values.ambient[0]); i++) {
			        	chprintf((BaseSequentialStream *)&SDU1, "%4d,", prox_values.ambient[i]);
			        	chprintf((BaseSequentialStream *)&SDU1, "%4d,", prox_values.reflected[i]);
			        	chprintf((BaseSequentialStream *)&SDU1, "%4d", prox_values.delta[i]);
			        	chprintf((BaseSequentialStream *)&SDU1, "\r\n");
			        }
			        chprintf((BaseSequentialStream *)&SDU1, "\r\n");
				}

				chThdSleepUntilWindowed(time, time + MS2ST(100)); // Refresh @ 10 Hz.
				break;

			case 3: // Asercom protocol v2 (BT).
				spi_image_transfer_disable(); // Image is transferred via uart channel.
				run_asercom2();
				stop_loop = 1;
				break;

			case 4: // Range and bearing - receiver.
				switch(rab_state) {
					case 0:
						write_reg(rab_addr, 12, 150);	// Set range.
						write_reg(rab_addr, 17, 0);		// Onboard calculation.
						write_reg(rab_addr, 16, 0);		// Store light conditions.
						rab_state = 1;
						break;

					case 1:
					    if((i2c_err = read_reg(rab_addr, 0, &regValue[0])) != MSG_OK) {
					    	chprintf((BaseSequentialStream *)&SDU1, "err\r\n");
					        break;
					    }
					    if(regValue[0] != 0) {
					    	read_reg(rab_addr, 1, &regValue[0]);
							read_reg(rab_addr, 2, &regValue[1]);
							rab_data = (((uint16_t)regValue[0])<<8) + regValue[1];

					    	read_reg(rab_addr, 3, &regValue[0]);
							read_reg(rab_addr, 4, &regValue[1]);
							rab_bearing = ((double)((((uint16_t)regValue[0])<<8) + regValue[1])) * 0.0001;

					    	read_reg(rab_addr, 5, &regValue[0]);
							read_reg(rab_addr, 6, &regValue[1]);
							rab_range = (((uint16_t)regValue[0])<<8) + regValue[1];

							read_reg(rab_addr, 9, &regValue[0]);
							rab_sensor = regValue[0];

		    	if (SDU1.config->usbp->state != USB_ACTIVE) { // Skip printing if port not opened.
								break;
							}

							chprintf((BaseSequentialStream *)&SDU1, "%d %3.2f %d %d\r\n", rab_data, (rab_bearing*180.0/M_PI), rab_range, rab_sensor);
		    	}
						break;
				}
				chThdSleepUntilWindowed(time, time + MS2ST(20)); // Refresh @ 50 Hz.
				break;

			case 5: // Range and bearing - transmitter.
				switch(rab_state) {
					case 0:
						write_reg(rab_addr, 12, 150); // Set range.
						if((i2c_err = read_reg(rab_addr, 12, &regValue[0])) == MSG_OK) {
							chprintf((BaseSequentialStream *)&SDU1, "set range to %d\r\n", regValue[0]);
						}
						write_reg(rab_addr, 17, 0); // Onboard calculation.
						if((i2c_err = read_reg(rab_addr, 12, &regValue[0])) == MSG_OK) {
							chprintf((BaseSequentialStream *)&SDU1, "onboard calculation enabled = %d\r\n", regValue[0]);
						}
						write_reg(rab_addr, 16, 0); // Store light conditions.
						rab_state = 1;
						break;

					case 1:
						write_reg(rab_addr, 13, 0xAA);
						write_reg(rab_addr, 14, 0xFF);
						break;
				}
				chThdSleepUntilWindowed(time, time + MS2ST(20)); // Refresh @ 50 Hz.
				break;

			case 6: // Move the robot back and forth exploiting the gyroscope to turn 180 degrees + LEDs animation.
				while(1) {
					switch(back_and_forth_state) {
						case 0: // Set speed to go forward.
							right_motor_set_speed(300);
							left_motor_set_speed(300);
							left_motor_set_pos(0);
							back_and_forth_state = 1;
							break;

						case 1: // Go forward for a while.
							if(left_motor_get_pos() >= 800) {
								right_motor_set_speed(150);
								left_motor_set_speed(-150);
								turn_angle_rad = 0.0;
								resetTime();
								clear_leds();
								set_body_led(1);
								back_and_forth_state = 2;
							}

							led_animation_count++;
							if(led_animation_count >= 200000) {
								led_animation_count = 0;
								switch(led_animation_state) {
									case 0:
										e_set_led(0, 1);
										led_animation_state = 1;
										break;
									case 1:
										e_set_led(1, 1);
										led_animation_state = 2;
										break;
									case 2:
										e_set_led(0, 0);
										e_set_led(2, 1);
										led_animation_state = 3;
										break;
									case 3:
										e_set_led(1, 0);
										e_set_led(3, 1);
										led_animation_state = 4;
										break;
									case 4:
										e_set_led(2, 0);
										e_set_led(4, 1);
										led_animation_state = 5;
										break;
									case 5:
										e_set_led(3, 0);
										e_set_led(5, 1);
										led_animation_state = 6;
										break;
									case 6:
										e_set_led(4, 0);
										e_set_led(6, 1);
										led_animation_state = 7;
										break;
									case 7:
										e_set_led(5, 0);
										e_set_led(7, 1);
										led_animation_state = 8;
										break;
									case 8:
										e_set_led(6, 0);
										e_set_led(0, 1);
										led_animation_state = 9;
										break;
									case 9:
										e_set_led(7, 0);
										e_set_led(1, 1);
										led_animation_state = 2;
										break;
								}
							}
							break;

						case 2:
							messagebus_topic_wait(imu_topic, &imu_values, sizeof(imu_values));
//							if (SDU1.config->usbp->state == USB_ACTIVE) { // Skip printing if port not opened.
//								chprintf((BaseSequentialStream *)&SDU1, "rate=%f, angle=%f\r\n", get_gyro_rate(2), turn_angle_rad);
//							}
							turn_angle_rad += get_gyro_rate(2)*getDiffTimeMsAndReset()*0.001;
							if(turn_angle_rad >= M_PI) {
								right_motor_set_speed(300);
								left_motor_set_speed(300);
								left_motor_set_pos(0);
								set_body_led(0);
								back_and_forth_state = 1;
							}
							break;
					}

				}
				break;

			case 7: // Play a wav (mono, 16 KHz) named "example.wav" from the micro sd when pressing the button. At each press the playback volume is also increased by 10%.
				switch(wav_play_state) {
					case 0:
						if(mountSDCard()) {
							wav_play_state = 1;
						}
						chThdSleepMilliseconds(1000);
						break;

					case 1:
						if(button_is_pressed()) {
							wav_play_state = 2;
						}
						break;

					case 2:
						if(!button_is_pressed()) {
							wav_play_state = 3;
						}
						break;

					case 3:
						playSoundFile("example.wav", SF_FORCE_CHANGE, 16000);
						waitSoundFileHasFinished();
						if(wav_volume == 100) {
							wav_volume = 0;
						} else {
							wav_volume += 10;
						}
						setSoundFileVolume(wav_volume);
						if (SDU1.config->usbp->state == USB_ACTIVE) { // Skip printing if port not opened.
							chprintf((BaseSequentialStream *)&SDU1, "volume=%d\r\n", wav_volume);
						}
						wav_play_state = 1;
						break;
				}

				chThdSleepUntilWindowed(time, time + MS2ST(20)); // Refresh @ 50 Hz.
				break;

			case 8: // Asercom protocol v2 (USB).
				spi_image_transfer_disable(); // Image is transferred via uart channel.
				run_asercom2();
				stop_loop = 1;
				break;

			case 9: // Asercom protocol.
				spi_image_transfer_disable(); // Image is transferred via uart channel.
				run_asercom();
				stop_loop = 1;
				break;

			case 10: // Gumstix extension.
				imu_stop();
				VL53L0X_stop();
				i2c_stop();
				stop_loop = 1;
				break;

			case 11: // Simple obstacle avoidance + some animation.
				messagebus_topic_wait(prox_topic, &prox_values, sizeof(prox_values));

				prox_values_temp[0] = get_calibrated_prox(0);
				prox_values_temp[1] = get_calibrated_prox(1);
				prox_values_temp[2] = get_calibrated_prox(2);
				prox_values_temp[5] = get_calibrated_prox(5);
				prox_values_temp[6] = get_calibrated_prox(6);
				prox_values_temp[7] = get_calibrated_prox(7);

				leftSpeed = MOTOR_SPEED_LIMIT/2 - prox_values_temp[0]*8 - prox_values_temp[1]*4 - prox_values_temp[2]*2;
				rightSpeed = MOTOR_SPEED_LIMIT/2 - prox_values_temp[7]*8 - prox_values_temp[6]*4 - prox_values_temp[5]*2;
				right_motor_set_speed(rightSpeed);
				left_motor_set_speed(leftSpeed);

	            switch(rgb_state) {
					case 0: // Red.
						set_rgb_led(0, 10, 0, 0);
						set_rgb_led(1, 10, 0, 0);
						set_rgb_led(2, 10, 0, 0);
						set_rgb_led(3, 10, 0, 0);
						break;
					case 1: // Green.
						set_rgb_led(0, 0, 10, 0);
						set_rgb_led(1, 0, 10, 0);
						set_rgb_led(2, 0, 10, 0);
						set_rgb_led(3, 0, 10, 0);
						break;
					case 2: // Blue.
						set_rgb_led(0, 0, 0, 10);
						set_rgb_led(1, 0, 0, 10);
						set_rgb_led(2, 0, 0, 10);
						set_rgb_led(3, 0, 0, 10);
						break;
	            }
				rgb_counter++;
				if(rgb_counter == 100) {
					rgb_counter = 0;
					rgb_state = (rgb_state+1)%3;
					set_body_led(2);
					set_front_led(2);
				}

				melody_counter++;
				if(melody_counter == 2000) {
					melody_counter = 0;
					melody_state = (melody_state+1)%NB_SONGS;
					playMelody(melody_state, ML_SIMPLE_PLAY, NULL);
				}

				chThdSleepUntilWindowed(time, time + MS2ST(10)); // Refresh @ 100 Hz.
				break;

			case 12: // Hardware test.
				switch(hw_test_state) {
					case 0: // Init hardware.
						// Calibrate proximity.
						calibrate_ir();

						// Test audio.
						playMelody(MARIO, ML_SIMPLE_PLAY, NULL);

						// Test motors at low speed.
						left_motor_set_speed(150);
						right_motor_set_speed(150);

						// Init camera.
						cam_advanced_config(FORMAT_COLOR, 240, 160, 160, 120, SUBSAMPLING_X4, SUBSAMPLING_X4);
						dcmi_disable_double_buffering();
						dcmi_set_capture_mode(CAPTURE_ONE_SHOT);
						dcmi_prepare();

						// Calibrate IMU.
						calibrate_acc();
						calibrate_gyro();

						// Test all leds.
						set_body_led(1);
						set_front_led(1);
						set_led(4,1);
						set_rgb_led(0, 10, 0, 0);
						set_rgb_led(1, 10, 0, 0);
						set_rgb_led(2, 10, 0, 0);
						set_rgb_led(3, 10, 0, 0);

						hw_test_state = 1;
						break;

					case 1: // Test.
						chThdSleepUntilWindowed(time, time + MS2ST(50)); // Refresh @ 20 Hz.

			            switch(rgb_state) {
							case 0: // Red.
								set_rgb_led(0, 10, 0, 0);
								set_rgb_led(1, 10, 0, 0);
								set_rgb_led(2, 10, 0, 0);
								set_rgb_led(3, 10, 0, 0);
								break;
							case 1: // Green.
								set_rgb_led(0, 0, 10, 0);
								set_rgb_led(1, 0, 10, 0);
								set_rgb_led(2, 0, 10, 0);
								set_rgb_led(3, 0, 10, 0);
								break;
							case 2: // Blue.
								set_rgb_led(0, 0, 0, 10);
								set_rgb_led(1, 0, 0, 10);
								set_rgb_led(2, 0, 0, 10);
								set_rgb_led(3, 0, 0, 10);
								break;
			            }
						rgb_counter++;
						if(rgb_counter == 20) {
							rgb_counter = 0;
							rgb_state = (rgb_state+1)%3;
						}

						if (SDU1.config->usbp->state != USB_ACTIVE) { // Skip printing if port not opened.
							continue;
						}

						messagebus_topic_wait(prox_topic, &prox_values, sizeof(prox_values));
						messagebus_topic_wait(imu_topic, &imu_values, sizeof(imu_values));

						// Read IMU.
						chprintf((BaseSequentialStream *)&SDU1, "IMU\r\n");
				    	chprintf((BaseSequentialStream *)&SDU1, "Ax=%-7d Ay=%-7d Az=%-7d Gx=%-7d Gy=%-7d Gz=%-7d\r\n\n", imu_values.acc_raw[0], imu_values.acc_raw[1], imu_values.acc_raw[2], imu_values.gyro_raw[0], imu_values.gyro_raw[1], imu_values.gyro_raw[2]);

						// Read selector position.
				    	chprintf((BaseSequentialStream *)&SDU1, "SELECTOR\r\n");
				    	chprintf((BaseSequentialStream *)&SDU1, "%d\r\n\n", get_selector());

						// Read IR receiver.
				    	chprintf((BaseSequentialStream *)&SDU1, "IR RECEIVER\r\n");
				    	chprintf((BaseSequentialStream *)&SDU1, "check : 0x%x, address : 0x%x, data : 0x%x\r\n\n", ir_remote_get_toggle(), ir_remote_get_address(), ir_remote_get_data());

						// Read proximity sensors.
				    	chprintf((BaseSequentialStream *)&SDU1, "PROXIMITY\r\n");
				    	chprintf((BaseSequentialStream *)&SDU1, "%4d,%4d,%4d,%4d,%4d,%4d,%4d,%4d\r\n\n", prox_values.delta[0], prox_values.delta[1], prox_values.delta[2], prox_values.delta[3], prox_values.delta[4], prox_values.delta[5], prox_values.delta[6], prox_values.delta[7]);
				    	chprintf((BaseSequentialStream *)&SDU1, "AMBIENT\r\n");
				    	chprintf((BaseSequentialStream *)&SDU1, "%4d,%4d,%4d,%4d,%4d,%4d,%4d,%4d\r\n\n", prox_values.ambient[0], prox_values.ambient[1], prox_values.ambient[2], prox_values.ambient[3], prox_values.ambient[4], prox_values.ambient[5], prox_values.ambient[6], prox_values.ambient[7]);

						// Read microphones.
				    	chprintf((BaseSequentialStream *)&SDU1, "MICROPHONES\r\n");
				    	chprintf((BaseSequentialStream *)&SDU1, "%4d,%4d,%4d,%4d\r\n\n", mic_get_volume(0), mic_get_volume(1), mic_get_volume(2), mic_get_volume(3));

				    	// Read distance sensor.
				    	chprintf((BaseSequentialStream *)&SDU1, "DISTANCE SENSOR\r\n");
				    	chprintf((BaseSequentialStream *)&SDU1, "%d\r\n\n", VL53L0X_get_dist_mm());

						// Read camera.
				    	spi_comm_suspend();
				    	dcmi_capture_start();
						wait_image_ready();
						spi_comm_resume();
						img_buff_ptr = dcmi_get_last_image_ptr();
						r = (int)img_buff_ptr[0]&0xF8;
			            g = (int)(img_buff_ptr[0]&0x07)<<5 | (img_buff_ptr[1]&0xE0)>>3;
			            b = (int)(img_buff_ptr[1]&0x1F)<<3;
			            chprintf((BaseSequentialStream *)&SDU1, "CAMERA\r\n");
			            chprintf((BaseSequentialStream *)&SDU1, "R=%3d, G=%3d, B=%3d\r\n\n", r, g, b);

			            printUcUsage((BaseSequentialStream *)&SDU1);

			            chThdSleepMilliseconds(100);
						break;
				}
				break;

			case 13: // Reflect the orientation on the LEDs around the robot.
				e_display_angle();
				chThdSleepMilliseconds(50);
				break;

			case 14: // Read magnetometer sensors values and compute heading.
				switch(magneto_state) {
					case 0: // Setup and calibrate the magnetometer.
						mpu9250_magnetometer_setup();
						//set_body_led(1);
						//calibrate_magnetometer();
						//set_body_led(0);
						messagebus_topic_wait(imu_topic, &imu_values, sizeof(imu_values));
						if (SDU1.config->usbp->state == USB_ACTIVE) { // Skip printing if port not opened.
							chprintf((BaseSequentialStream *)&SDU1, "adj_x=%f adj_y=%f adj_z=%f\r\n", imu_values.mag_sens_adjust[0], imu_values.mag_sens_adjust[1], imu_values.mag_sens_adjust[2]);
							chprintf((BaseSequentialStream *)&SDU1, "offset_x=%f offset_y=%f offset_z=%f\r\n", imu_values.mag_offset[0], imu_values.mag_offset[1], imu_values.mag_offset[2]);
							chprintf((BaseSequentialStream *)&SDU1, "scale_x=%f scale_y=%f scale_z=%f\r\n", imu_values.mag_scale[0], imu_values.mag_scale[1], imu_values.mag_scale[2]);
						}
						magneto_state = 1;
						break;

					case 1: // Compute the heading.
				    	messagebus_topic_wait(imu_topic, &imu_values, sizeof(imu_values)); // Wait for the next measurement.
						get_mag_filtered(mag_values);
						heading = (atan2(mag_values[1], mag_values[0]) * 180.0)/M_PI;
						heading += 180.0; // 0..360

						// Turn on a LED based on the current heading.
						e_led_clear();
						if ((heading > 332.5) || (heading <= 27.5)) // 332.5 .. 27.5
							e_set_led(0, 1);
						else if ((heading > 27.5) && (heading <= 72.5)) // 27.5 .. 72.5
							e_set_led(1, 1);
						else if ((heading > 72.5)  && (heading <= 107.5)) // 72.5 .. 107.5
							e_set_led(2, 1);
						else if (( heading > 107.5) && (heading <= 152.5)) // 112.5 .. 152.5
							e_set_led(3, 1);
						else if (( heading > 152.5) && (heading <= 207.5)) // 152.5 .. 207.5
							e_set_led(4, 1);
						else if ( (heading > 207.5) && (heading <= 252.5)) // 207.5 .. 252.5
							e_set_led(5, 1);
						else if ( (heading > 252.5) && (heading <= 287.5)) // 252.5 .. 287.5
							e_set_led(6, 1);
						else if ( (heading > 287.5) && (heading <= 332.5)) // 287.5 .. 332.5
							e_set_led(7, 1);

				    	if (SDU1.config->usbp->state == USB_ACTIVE) { // Skip printing if port not opened.
				    		chprintf((BaseSequentialStream *)&SDU1, "Mx=%f My=%f Mz=%f\r\n", imu_values.magnetometer[0], imu_values.magnetometer[1], imu_values.magnetometer[2]);
				    		chprintf((BaseSequentialStream *)&SDU1, "Mx=%f My=%f Mz=%f\r\n", mag_values[0], mag_values[1], mag_values[2]);
				    		chprintf((BaseSequentialStream *)&SDU1, "heading = %f\r\n", heading);
				    	}

				    	chThdSleepUntilWindowed(time, time + MS2ST(125)); // Refresh @ 8 Hz, that is the magnetometer update frequency.
						break;
				}
				break;

			case 15:
				switch(demo15_state) {
					case 0:
						if(cam_advanced_config(FORMAT_COLOR, 0, 0, 640, 480, SUBSAMPLING_X4, SUBSAMPLING_X4) != MSG_OK) {
							set_led(LED1, 1);
						}
						cam_set_exposure(512, 0); // Fix the exposure to have a stable framerate.

						dcmi_set_capture_mode(CAPTURE_ONE_SHOT);

						if(dcmi_prepare() < 0) {
							set_led(LED5, 1);
						}

						spi_image_transfer_enable();

						//mpu9250_magnetometer_setup();

						// Flush the uart input to avoid interpreting garbage as real commands.
						while(chnReadTimeout(&SD3, (uint8_t*)&temp_rx, 1, MS2ST(1)>0)) {
							chThdSleepMilliseconds(1);
						}

						demo15_state = 1;
						break;

					case 1:
						run_asercom2();
						stop_loop = 1;
						break;
				}
				break;
		}
    }
}

int main(void)
{

    halInit();
    chSysInit();
    mpu_init();

    /** Inits the Inter Process Communication bus. */
    messagebus_init(&bus, &bus_lock, &bus_condvar);

    parameter_namespace_declare(&parameter_root, NULL, NULL);

    // Init the peripherals.
	clear_leds();
	set_body_led(0);
	set_front_led(0);
	usb_start();
	dcmi_start();
	cam_start();
	motors_init();
	proximity_start();
	battery_level_start();
	dac_start();
	exti_start();
	imu_start();
	ir_remote_start();
	spi_comm_start();
	VL53L0X_start();
	serial_start();
	mic_start(NULL);
	sdio_start();
	playMelodyStart();
	playSoundFileStart();
	ground_start();

	// Initialise Aseba system, declaring parameters
    parameter_namespace_declare(&aseba_ns, &parameter_root, "aseba");
    aseba_declare_parameters(&aseba_ns);

    /* Load parameter tree from flash. */
    load_config();

    /* Start AsebaCAN. Must be after config was loaded because the CAN id
     * cannot be changed at runtime. */
    aseba_vm_init();
    aseba_can_start(&vmState);

    chThdCreateStatic(selector_thd_wa, sizeof(selector_thd_wa), NORMALPRIO, selector_thd, NULL);

    /* Infinite loop. */
    while (1) {
        chThdSleepMilliseconds(1000);
    }
}

#define STACK_CHK_GUARD 0xe2dee396
uintptr_t __stack_chk_guard = STACK_CHK_GUARD;

void __stack_chk_fail(void)
{
    chSysHalt("Stack smashing detected");
}
