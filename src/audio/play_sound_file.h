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


#define SF_ERROR 1
#define SF_OK    0

#define DEFAULT_VOLUME  10
#define VOLUME_MAX      50  //after this the sound begins to saturate.

typedef enum{
  SF_SIMPLE_PLAY = 0,  //plays the new sound file but if a file is already playing, then this order is ignored
  SF_WAIT_AND_CHANGE,  //waits (put the invocking thread in sleep) the end of the current file playing if any and plays the new one
  SF_FORCE_CHANGE,   //stops the current playing file if any and play the new one
}playSoundFileOption_t;


void playSoundFileStart(void);

void waitSoundFileHasFinished(void);

void stopCurrentSoundFile(void);

void playSoundFile(char* pathToFile, playSoundFileOption_t option);

void setSoundFileVolume(uint8_t volume);

#endif /* PLAY_SOUND_FILE */