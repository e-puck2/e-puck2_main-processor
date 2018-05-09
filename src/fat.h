/*

File    : fat.h
Author  : Eliot Ferragni
Date    : 9 may 2018
REV 1.0

Some functions to use fatFS library

Adapted from the code written by Jed Frey
taken at https://github.com/jed-frey/ARMCM4-STM32F407-STF4BB-FATFS
*/


#ifndef FAT_H
#define FAT_H

/**
 * Provides functions to use the fatfs library
 */

#include <ch.h>
#include <hal.h>
//fatfs include
#include <ff.h>
#include <chprintf.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Prints the directory tree to the given stream
 * 
 * @param chp 	Output stream to use for the chprintf
 * @param path 	path to use to begin the scan (0 means the root). Also used as wotking area (bigger is better)
 * @return 		result. See FRESULT
 */
FRESULT scan_files(BaseSequentialStream *chp, char *path);

/**
 * @brief Prints a complete error string depending on the error given
 * 
 * @param chp	Output stream to use for the chprintf
 */
void fverbose_error(BaseSequentialStream *chp, FRESULT err);

#ifdef __cplusplus
}
#endif

#endif /* FAT_H */
