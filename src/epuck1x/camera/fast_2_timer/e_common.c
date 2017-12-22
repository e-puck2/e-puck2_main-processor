#include "e_poxxxx.h"
#include "..\camera\dcmi_camera.h"
#include "..\camera\po8030.h"
#include <stdlib.h>

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


void e_poxxxx_set_mirror(int vertical, int horizontal) {
	(void)vertical;
	(void)horizontal;
	return;
}

void e_poxxxx_write_cam_registers(void) {
	return;
}

/**
 * Initalize the camera, return the version in hexa, 0x3030 or 0x6030
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

/**
 * Return the camera orientation
 * \return 0: vertical 480x640, 1: horizontal 640x480, horizontal inverted, -1: unknown
 */
int e_poxxxx_get_orientation(void) {
	return 1;
}

/*! Enable/Disable AWB and AE 
 * \param awb 1 means AWB enabled, 0 means disabled
 * \param ae 1 means AE enabled, 0 means disabled
 */
void e_poxxxx_set_awb_ae(int awb, int ae) {
	(void)awb;
	(void)ae;
	return;
}

/*! Set the gains of the camera
 * \param red The red pixels' gain (0..255)
 * \param green The green pixels' gain (0..255)
 * \param blue The blue pixels' gain (0..255)
 * \warning Only meaningful if AWB is disabled 
 */
void e_poxxxx_set_rgb_gain(unsigned char r, unsigned char g, unsigned char b) {
	(void)r;
	(void)g;
	(void)b;
	return;
}

/*! Set exposure time 
 * \param t Exposure time, LSB is in 1/64 line time
 * \warning Only meaningful if AE is disabled 
 */
void e_poxxxx_set_exposure(unsigned long exp) {
	(void)exp;
	return;
}
unsigned int getCameraVersion(void) {
	return 0x8030;
}

/*! Launch a capture in the \a buf buffer
 * \param buf The buffer to write to
 * \sa e_poxxxx_config_cam and e_poxxxx_is_img_ready
 */
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
