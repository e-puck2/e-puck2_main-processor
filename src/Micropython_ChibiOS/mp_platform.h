/**
 * @file	mp_platform.h
 * @brief  	Used to make the link between the micropython files and the ChibiOS project.
 * 
 * @written by  	Eliot Ferragni
 * @creation date	20.03.2020
 */
 
#ifndef MP_PLATFORM_H
#define MP_PLATFORM_H


#define MICROPYTHON_HEAP_SIZE	120000
#define MICROPYTHON_PORT		USB_SERIAL

#define MICROPYTHON_FLASH_CODE_SECTOR	7	//must be coherent with the address set in the .ld file


/**
 * @brief Starts the micropython thread
 */
void micropythonStart(void);

#endif /* MP_PLATFORM_H */