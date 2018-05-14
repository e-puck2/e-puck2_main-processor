/*

File    : play_sound_file.h
Author  : Eliot Ferragni
Date    : 9 may 2018
REV 1.0

Functions and defines to play sounds from the sd card in the uncompressed WAV format in unsigned 8bits

*/


#ifndef PLAY_SOUND_FILE
#define PLAY_SOUND_FILE

#include <ch.h>
#include <hal.h>


#define ERROR 1
#define OK    0


uint8_t play_sound_file(char *pathToFile);

void set_music_volume(uint8_t volume);

#endif /* PLAY_SOUND_FILE */