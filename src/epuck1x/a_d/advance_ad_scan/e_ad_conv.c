/********************************************************************************

			Advance Analogic/Digital conversion
			december 2005: first version Francesco Mondada							  
			april 2006: debug and optimisation Michael Bonani             
			Borter Jean-Joel


This file is part of the e-puck library license.
See http://www.e-puck.org/index.php?option=com_content&task=view&id=18&Itemid=45

(c) 2005 Francesco Mondada
(c) 2006-2007 Michael-Bonani & Borter Jean-Joel

Robotics system laboratory http://lsro.epfl.ch
Laboratory of intelligent systems http://lis.epfl.ch
Swarm intelligent systems group http://swis.epfl.ch
EPFL Ecole polytechnique federale de Lausanne http://www.epfl.ch

**********************************************************************************/

/*! \file
 * \ingroup a_d
 * \brief Module for the advance Analogic/Digital conversion.
 * \author Code: Francesco Mondada, Michael-Bonani & Borter Jean-Joel \n Doc: Jonathan Besuchet
 */

#include "e_ad_conv.h"

//int e_mic_scan[3][MIC_SAMP_NB];			/*!< Array to store the mic values */
//int e_acc_scan[3][ACC_SAMP_NB];			/*!< Array to store the acc values */
//unsigned int e_last_mic_scan_id = 0;	//ID of the last scan in the mic array (must be int else probleme of overchange)
//unsigned int e_last_acc_scan_id = 0;	//ID of the last scan in the acc array

//int e_ambient_ir[10];					/*!< Array to store the ambient light measurement */
//int e_ambient_and_reflected_ir[10];		/*!< Array to store the light when IR led is on */

//static unsigned char is_ad_acquisition_completed = 0;
//static unsigned char is_ad_array_filled = 0;
//static unsigned char micro_only = 0;
//int selector;
//
//unsigned char updateAccI2CCounter = 0;
//extern int centre_z;
//unsigned int tickAdcIsr = 0;    // tick resolution = ADC_ISR_PERIOD = 1/16384 = about 61 us

/*! \brief Initialize all the A/D register needed
 *
 * Set up the different ADC register to process the AD conversion
 * by scanning the used AD channels. Each value of the channels will
 * be stored in a different AD buffer register and an inturrupt will
 * occure at the end of the scan.
 * \param only_micro Put MICRO_ONLY to use only the three microphones
 * at 33kHz. Put ALL_ADC to use all the stuff using the ADC.
 */
void e_init_ad_scan(unsigned char only_micro)
{
	(void)only_micro;
}

/*! \brief To know if the ADC acquisitionn is completed
 * \return 0 if the new acquisition is not made, 1 if completed.
 */
unsigned char e_ad_is_acquisition_completed(void)
{
	return 1;
}

/*! \brief To know if the ADC acquisitionn of microphone only is completed
 * \return 0 if the new acquisition is not made, 1 if completed.
 */
unsigned char e_ad_is_array_filled(void)
{
	return 1;
}

/*! \brief Enable the ADC conversion
 */
void e_ad_scan_on(void)
{
	return;
}

/*! \brief Disable the ADC conversion
 */
void e_ad_scan_off(void)
{
	return;
}
