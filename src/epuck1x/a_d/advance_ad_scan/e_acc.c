/********************************************************************************

			Accessing the accelerometer data (advance)
			Novembre 7 2005 Borter Jean-Joel


This file is part of the e-puck library license.
See http://www.e-puck.org/index.php?option=com_content&task=view&id=18&Itemid=45

(c) 2005 Borter Jean-Joel

Robotics system laboratory http://lsro.epfl.ch
Laboratory of intelligent systems http://lis.epfl.ch
Swarm intelligent systems group http://swis.epfl.ch
EPFL Ecole polytechnique federale de Lausanne http://www.epfl.ch

**********************************************************************************/

/*! \file
 * \ingroup a_d
 * \brief Accessing the accelerometer data.
 *
 * The functions of this file are made to deal with the accelerometer
 * data. You can know the magnitude, the orientation, the inclination, ...
 * of the acceleration that the e-puck is enduring.
 * \n \n Two structures are used:
 * - TypeAccSpheric to store the acceleration data on sherical coordinates.
 * - TypeAccRaw to store the acceleration data on cartesian coordinates.
 *
 * A little exemple to read the accelerator.
 * \code
 * #include <p30F6014A.h>
 * #include <motor_led/e_epuck_ports.h>
 * #include <motor_led/e_init_port.h>
 * #include <a_d/advance_ad_scan/e_ad_conv.h>
 * #include <a_d/advance_ad_scan/e_acc.h>
 * 
 * int main(void)
 * {
 * 	int z;
 *  	e_init_port();
 *  	e_init_ad_scan();
 *  	while(1)
 *  	{
 *  		long i;
 *			z = e_get_acc(2);
 *  		if(z < 2100)	//LED4 on if e-puck is on the back
 * 		{
 *  			LED0 = 0;
 *  			LED4 = 1;
 * 		}
 *  		else		//LED0 on if e-puck is on his wells
 *  		{
 *  			LED0 = 1;
 *  			LED4 = 0;
 * 		}
 * 		for(i=0; i<100000; i++) { asm("nop"); }
 *		}
 *  }
 * \endcode
 * \author Code: Borter Jean-Joel \n Doc: Jonathan Besuchet
 */

#include "math.h"
#include "e_acc.h"
#include "../sensors/imu.h"
#include <stdlib.h>

/*****************************************************
 * internal function                                 *
 *****************************************************/

/*! \brief Read the last value of a given accelerator axes
 * \param captor		ID of the AD channel to read 
 *						(must be 0 = x, 1 = y or 2 = z)
 * \return value		filtered channel's value
 */
// Return the last axis value without the calibration value, for Z axis remove also the gravity default value.
int e_get_acc(unsigned int captor) {
	// Values adapted to be compatible with e-puck1.x:
    // - shift by 4 the value received from the sensor (16 bits) to get 12 bits value as with e-puck1.x
    // - decrease the resulting value by 1/4 to get about the same value for 1g (in e-puck1.x 1g = about 800)
	int32_t tempValue = 0;
	if(captor == 0) { // X axis => change sign to be compatible with e-puck1.x.
		tempValue = -(get_acc(captor) - get_acc_offset(captor));
	} else if(captor == 1) { // Y axis.
		tempValue = get_acc(captor) - get_acc_offset(captor);
	} else { // Z axis => remove gravity from offset.
		tempValue = get_acc(captor) - (get_acc_offset(captor) - GRAVITY);
	}
	tempValue = (tempValue>>4) - (tempValue>>6);
	return (int)tempValue;
}

/*! \brief Read the value of a channel, filtered by an averaging filter
 * \param captor		ID of the AD channel to read (must be 0 to 2)
 * \param filter_size	size of the filter (must be between 1 and SAMPLE_NUMBER)
 * \return value		filtered channel's value
 */
int e_get_acc_filtered(unsigned int captor, unsigned int filter_size)
{
	// Values adapted to be compatible with e-puck1.x:
    // - shift by 4 the value received from the sensor (16 bits) to get 12 bits value as with e-puck1.x
    // - decrease the resulting value by 1/4 to get about the same value for 1g (in e-puck1.x 1g = about 800)
	// - add an offset of 2048 because it represents the value of 0g with e-puck1.x
	int32_t tempValue = 0;
	if(captor == 0) { // X axis => change sign to be compatible with e-puck1.x.
		tempValue = -get_acc_filtered(captor, filter_size);
	} else if(captor == 1) { // Y axis.
		tempValue = get_acc_filtered(captor, filter_size);
	} else { // Z axis.
		tempValue = get_acc_filtered(captor, filter_size);
	}
	tempValue = ((tempValue>>4) - (tempValue>>6)) + 2048;
	return (int)tempValue;
}

/*****************************************************
 * user called function                               *
 *****************************************************/

/*! \brief initialize de ad converter and calculate the zero
 * values
 *
 * It reads two times the average_size to avoid
 * edge effects then it reads 100 values and average them to initiate
 * the "zero" value of the accelerometer
 */
void e_acc_calibr(void)
{
	calibrate_acc();
}

/*! \brief calculate and return the accel. in spherical coord
 * \return acceleration in spherical coord
 * \sa TypeAccSpheric
 */
TypeAccSpheric e_read_acc_spheric(void)
{
	TypeAccSpheric result;
	int32_t acc_x, acc_y, acc_z;
	acc_x = -(get_acc_filtered(0, FILTER_SIZE) - get_acc_offset(0)); // X axis => change sign to be compatible with e-puck1.x.
	acc_y = get_acc_filtered(1, FILTER_SIZE) - get_acc_offset(1); // Y axis.
	acc_z = get_acc_filtered(2, FILTER_SIZE) - (get_acc_offset(2) - GRAVITY); // Z axis => remove gravity from offset.

	// Calculate the absolute acceleration value.
	result.acceleration = sqrtf((float)((acc_x * acc_x) + (acc_y * acc_y) + (acc_z * acc_z)));
	result.inclination =  90.0 - atan2f((float)(acc_z), sqrtf( (float)((acc_x * acc_x) + (acc_y * acc_y) ))) * CST_RADIAN;
	if (result.inclination<5 || result.inclination>160) {
		result.orientation=0;
	} else {
		result.orientation = (atan2f((float)(acc_x), (float)(acc_y)) * CST_RADIAN) + 180.0;		// 180 is added to have 0 to 360° range
	}
	return result;
}

/*! \brief calculate and return the inclination angle
 * \return inclination angle of the robot
 */
float e_read_inclination(void)
{
	TypeAccSpheric result;
	int16_t acc_x, acc_y, acc_z;
	acc_x = get_acc_filtered(0, FILTER_SIZE) - get_acc_offset(0);	// generates positive
	acc_y = get_acc_filtered(1, FILTER_SIZE) - get_acc_offset(1);	// and negative value
	acc_z = get_acc_filtered(2, FILTER_SIZE) - get_acc_offset(2);	// to make the use easy

	result.inclination =  90.0 - atan2f((float)(acc_z), sqrtf( (float)(((long)acc_x * (long)acc_x) + ((long)acc_y * (long)acc_y) ))) * CST_RADIAN;
	return result.inclination;
}

/*! \brief calculate and return the orientation angle
 * \return orientation of the accel vector
 */
float e_read_orientation(void)
{
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

/*! \brief calculate and return the intensity of the acceleration
 * \return intensity of the acceleration vector
 */
float e_read_acc(void)
{ 
	TypeAccSpheric result;
	int16_t acc_x, acc_y, acc_z;
	acc_x = get_acc_filtered(0, FILTER_SIZE) - get_acc_offset(0);	// generates positive
	acc_y = get_acc_filtered(1, FILTER_SIZE) - get_acc_offset(1);	// and negative value
	acc_z = get_acc_filtered(2, FILTER_SIZE) - get_acc_offset(2);	// to make the use easy

	result.acceleration = sqrtf((float)(((long)acc_x * (long)acc_x) + ((long)acc_y * (long)acc_y) + ((long)acc_z * (long)acc_z)));
	return result.acceleration;
}

/*! \brief calculate and return acceleration on the x,y,z axis
 * \return acceleration on the x,y,z axis
 * \sa TypeAccRaw
 */
TypeAccRaw e_read_acc_xyz(void)
{
	TypeAccRaw result;
	result.acc_x = get_acc_filtered(0, FILTER_SIZE) - get_acc_offset(0);	// generates positive
	result.acc_y = get_acc_filtered(1, FILTER_SIZE) - get_acc_offset(1);	// and negative value
	result.acc_z = get_acc_filtered(2, FILTER_SIZE) - get_acc_offset(2);	// to make the use easy
	return result;
}

/*! \brief calculate and return acceleration on the x axis
 * \return acceleration on the x axis
 */
int e_read_acc_x(void)
{
	return (get_acc_filtered(0, FILTER_SIZE) - get_acc_offset(0));
}

/*! \brief calculate and return acceleration on the y axis
 * \return acceleration on the y axis
 */
int e_read_acc_y(void)
{
	return (get_acc_filtered(1, FILTER_SIZE) - get_acc_offset(1));
}

/*! \brief calculate and return acceleration on the z axis
 * \return acceleration on the z axis
 */
int e_read_acc_z(void)
{
	return (get_acc_filtered(2, FILTER_SIZE) - get_acc_offset(2));
}

/*! \brief light the led according to the orientation angle */
void e_display_angle(void)
{
//	float angle = 0.0;
//
//// 	To avoid oscillation the limite of variation is limited at
////  a fix value	wich is 1/9 of the LED resolution
//	angle = e_read_orientation();
//	if ( abs(angle_mem - angle) > 5.0)
//	{
//		LED0=LED1=LED2=LED3=LED4=LED5=LED6=LED7=0;
//			// table of selection
//		if ( 	  (angle > (360.0 - 22.5)) |  (angle <= (0.0   + 22.5)) && angle != ANGLE_ERROR)
//			LED0 = 1;
//		else if ( angle > (45.0 - 22.5)  && angle <= (45.0  + 22.5))
//			LED7 = 1;
//		else if ( angle > (90.0 - 22.5)  && angle <= (90.0  + 22.5))
//			LED6 = 1;
//		else if ( angle > (135.0 - 22.5) && angle <= (135.0 + 22.5))
//			LED5 = 1;
//		else if ( angle > (180.0 - 22.5) && angle <= (180.0 + 22.5))
//			LED4 = 1;
//		else if ( angle > (225.0 - 22.5) && angle <= (225.0 + 22.5))
//			LED3 = 1;
//		else if ( angle > (270.0 - 22.5) && angle <= (270.0 + 22.5))
//			LED2 = 1;
//		else if ( angle > (315.0 - 22.5) && angle <= (315.0 + 22.5))
//			LED1 = 1;
//		angle_mem = angle;	// value to compare with the next one
//	}
}
