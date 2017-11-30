#ifndef EPUCK1X_WRAPPER_H
#define EPUCK1X_WRAPPER_H

#ifdef __cplusplus
extern "C" {
#endif

// LEDs handling.
void e_set_led(unsigned int led_number, unsigned int value); // set led_number (0-7) to value (0=off 1=on higher=inverse) 
void e_led_clear(void);
void e_set_body_led(unsigned int value); // value (0=off 1=on higher=inverse) 
void e_set_front_led(unsigned int value); //value (0=off 1=on higher=inverse) 

// Various.
int getselector(void);

// UARTs handling.
// UART1 was used for BT communication.
// UART2 was used for serial communication with the Gumstix extension.
/*! \brief Init uart 1 at 115200bps, 8 data bits, 1 stop bit, Enable ISR for RX and TX */
void e_init_uart1(void);

/*! \brief Check if something is comming on uart 1
 * \return the number of characters available, 0 if none are available */
int  e_ischar_uart1(void);

/*! \brief If available, read 1 char and put it in pointer
 * \param car The pointer where the caracter will be stored if available
 * \return 1 if a char has been readed, 0 if no char is available
 */
int  e_getchar_uart1(char *car);

/*! \brief Send a buffer of char of size length
 * \param buff The top of the array where the data are stored
 * \param length The length of the array to send
 */
void e_send_uart1_char(const char * buff, int length);

/*! \brief  To check if the sending operation is done
 * \return 1 if buffer sending is in progress, return 0 if not
 */
int  e_uart1_sending(void);


/*! \brief Init uart 2 at 115200bps, 8 data bits, 1 stop bit, Enable ISR for RX and TX */
void e_init_uart2(int baud);

/*! \brief Check if something is comming on uart 2
 * \return the number of characters available, 0 if none are available */
int  e_ischar_uart2(void);

/*! \brief If available, read 1 char and put it in pointer
 * \param car The pointer where the caracter will be stored if available
 * \return 1 if a char has been readed, 0 if no char is available
 */
int  e_getchar_uart2(char *car);

/*! \brief Send a buffer of char of size length
 * \param buff The top of the array where the datas are stored
 * \param length The length of the array
 */
void e_send_uart2_char(const char * buff, int length);

/*! \brief  To check if the sending operation is done
 * \return 1 if buffer sending is in progress, return 0 if not
 */
int  e_uart2_sending(void);

// Motors handling.
void e_init_motors(void); 				// init to be done before using the other calls
void e_set_speed_left(int motor_speed);  // motor speed: from -1000 to 1000
void e_set_speed_right(int motor_speed); // motor speed: from -1000 to 1000
void e_set_speed(int linear_speed, int angular_speed);
void e_set_steps_left(int steps_left);
void e_set_steps_right(int steps_right);
int e_get_steps_left(void);
int e_get_steps_right(void);

// Proximity handling.
void e_calibrate_ir(void);
int e_get_prox(unsigned int sensor_number); // to get a prox value
int e_get_calibrated_prox(unsigned int sensor_number);
int e_get_ambient_light(unsigned int sensor_number); // to get ambient light value

// Camera handling.
#define ARRAY_WIDTH 640
#define ARRAY_HEIGHT 480
#define GREY_SCALE_MODE		0
#define RGB_565_MODE		1
#define YUV_MODE			2
int e_poxxxx_init_cam(void);
int e_poxxxx_config_cam(unsigned int sensor_x1,unsigned int sensor_y1,
			 unsigned int sensor_width,unsigned int sensor_height,
			 unsigned int zoom_fact_width,unsigned int zoom_fact_height,
			 int color_mode);
void e_poxxxx_write_cam_registers(void);
void e_poxxxx_launch_capture(char * buf);
int e_poxxxx_is_img_ready(void);

// Accelerometer handling.
#define FILTER_SIZE		5				// define the size of the averaging filter
#define CST_RADIAN		(180.0/3.1415)	// used to convert radian in degrees
/*! \struct TypeAccSpheric
 * \brief struct to store the acceleration vector in spherical coord
 */
typedef struct
{
	float acceleration;		/*!< lenght of the acceleration vector
							 * = intensity of the acceleration */
	float orientation;		/*!< orientation of the acceleration vector
							 * in the horizontal plan, zero facing front
							 * - 0° = inclination to the front
							 * (front part lower than rear part)
							 * - 90° = inclination to the left
							 * (left part lower than right part)
							 * - 180° = inclination to the rear
							 * (rear part lower than front part)
							 * - 270° = inclination to the right
							 * (right part lower than left part) */
	float inclination;		/*!< inclination angle with the horizontal plan
							 * - 0° = e-puck horizontal
							 * - 90° = e-puck vertical
							 * - 180° = e-puck horizontal but up-side-down */
} TypeAccSpheric;

/*! \struct TypeAccRaw
 * \brief struct to store the acceleration raw data
 * in carthesian coord
 */
typedef struct
{
	int acc_x;	/*!< The acceleration on x axis */
	int acc_y;	/*!< The acceleration on y axis */
	int acc_z;	/*!< The acceleration on z axis */
} TypeAccRaw;

int e_get_acc(unsigned int captor);
int e_get_acc_filtered(unsigned int captor, unsigned int filter_size);
TypeAccSpheric e_read_acc_spheric(void);
float e_read_orientation(void);
float e_read_inclination(void);
float e_read_acc(void);
TypeAccRaw e_read_acc_xyz(void);
int e_read_acc_x(void);
int e_read_acc_y(void);
int e_read_acc_z(void);
void e_acc_calibr(void);
void e_display_angle(void);


// Microphone handling.
#define MIC0_BUFFER	0
#define MIC1_BUFFER	1
#define MIC2_BUFFER	2

/*! \brief Get the last value of a given micro
 * \param micro_id		micro's ID (0, 1, or 2)
 *							(use \ref MIC0_BUFFER, \ref MIC1_BUFFER , \ref MIC2_BUFFER)
 * \return result		last value of the micro
 */
int e_get_micro(unsigned int micro_id);

int e_get_micro_average(unsigned int micro_id, unsigned int filter_size);

/*! \brief Get the difference between the highest and lowest sample.
 * \param micro_id		micro's ID (0, 1, or 2)
 *							(use \ref MIC0_BUFFER, \ref MIC1_BUFFER , \ref MIC2_BUFFER)
 * \return result		volume
 */
int e_get_micro_volume (unsigned int micro_id);

#ifdef __cplusplus
}
#endif

#endif
