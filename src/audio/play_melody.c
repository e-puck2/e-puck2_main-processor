/*

File    : play_melody.c
Author  : Eliot Ferragni
Date    : 4 january 2018
REV 1.0

Functions and defines to play little melodies on the speaker

Adapted from the code written by Dipto Pratyaksa
taken at https://www.princetronics.com/supermariothemesong/
*/


#include "play_melody.h"
#include "audio_thread.h"

typedef struct{
	uint16_t* notes;
	float* tempo;
	uint16_t length;
}melody_t;

//Mario main theme melody
static uint16_t mario_melody[] = {
  NOTE_E5, NOTE_E5, 0, NOTE_E5,
  0, NOTE_C5, NOTE_E5, 0,
  NOTE_G5, 0, 0, 0,
  NOTE_G4, 0, 0, 0,
 
  NOTE_C5, 0, 0, NOTE_G4,
  0, 0, NOTE_E4, 0,
  0, NOTE_A4, 0, NOTE_B4,
  0, NOTE_AS4, NOTE_A4, 0,
 
  NOTE_G4, NOTE_E5, NOTE_G5,
  NOTE_A5, 0, NOTE_F5, NOTE_G5,
  0, NOTE_E5, 0, NOTE_C5,
  NOTE_D5, NOTE_B4, 0, 0,
 
  NOTE_C5, 0, 0, NOTE_G4,
  0, 0, NOTE_E4, 0,
  0, NOTE_A4, 0, NOTE_B4,
  0, NOTE_AS4, NOTE_A4, 0,
 
  NOTE_G4, NOTE_E5, NOTE_G5,
  NOTE_A5, 0, NOTE_F5, NOTE_G5,
  0, NOTE_E5, 0, NOTE_C5,
  NOTE_D5, NOTE_B4, 0, 0
};
//Mario main them tempo
static float mario_tempo[] = {
  12, 12, 12, 12,
  12, 12, 12, 12,
  12, 12, 12, 12,
  12, 12, 12, 12,
 
  12, 12, 12, 12,
  12, 12, 12, 12,
  12, 12, 12, 12,
  12, 12, 12, 12,
 
  9, 9, 9,
  12, 12, 12, 12,
  12, 12, 12, 12,
  12, 12, 12, 12,
 
  12, 12, 12, 12,
  12, 12, 12, 12,
  12, 12, 12, 12,
  12, 12, 12, 12,
 
  9, 9, 9,
  12, 12, 12, 12,
  12, 12, 12, 12,
  12, 12, 12, 12,
};
//Underworld melody
static uint16_t underworld_melody[] = {
  NOTE_C4, NOTE_C5, NOTE_A3, NOTE_A4,
  NOTE_AS3, NOTE_AS4, 0,
  0,
  NOTE_C4, NOTE_C5, NOTE_A3, NOTE_A4,
  NOTE_AS3, NOTE_AS4, 0,
  0,
  NOTE_F3, NOTE_F4, NOTE_D3, NOTE_D4,
  NOTE_DS3, NOTE_DS4, 0,
  0,
  NOTE_F3, NOTE_F4, NOTE_D3, NOTE_D4,
  NOTE_DS3, NOTE_DS4, 0,
  0, NOTE_DS4, NOTE_CS4, NOTE_D4,
  NOTE_CS4, NOTE_DS4,
  NOTE_D4, NOTE_GS3,
  NOTE_G3, NOTE_CS4,
  NOTE_C4, NOTE_FS4, NOTE_F4, NOTE_E3, NOTE_AS4, NOTE_A4,
  NOTE_GS4, NOTE_DS4, NOTE_B3,
  NOTE_AS3, NOTE_A3, NOTE_GS3,
  0, 0, 0
};
//Underwolrd tempo
static float underworld_tempo[] = {
  12, 12, 12, 12,
  12, 12, 6,
  3,
  12, 12, 12, 12,
  12, 12, 6,
  3,
  12, 12, 12, 12,
  12, 12, 6,
  3,
  12, 12, 12, 12,
  12, 12, 6,
  6, 18, 18, 18,
  6, 6,
  6, 6,
  6, 6,
  18, 18, 18, 18, 18, 18,
  10, 10, 10,
  10, 10, 10,
  3, 3, 3
};

//Star Wars melody
static uint16_t starwars_melody[] = {
	NOTE_A4, NOTE_A4, NOTE_A4,
	NOTE_F4, NOTE_C5, NOTE_A4, NOTE_F4, NOTE_C5, NOTE_A4, 
	0,
	NOTE_E5, NOTE_E5, NOTE_E5,
	NOTE_F5, NOTE_C5, NOTE_GS4, NOTE_F4, NOTE_C5, NOTE_A4,
};

//Star Wars tempo
static float starwars_tempo[] = {
	3.6, 3.6, 3.6,
	4.8, 12, 3.6, 4.8, 12, 3.6,
	2,
	3.6, 3.6, 3.6,
	4.8, 12, 3.6, 4.8, 12, 3.6
};

melody_t melody[NB_SONGS] = {
	//MARIO
	{
		.notes = mario_melody,
		.tempo = mario_tempo,
		.length = sizeof(mario_melody)/sizeof(uint16_t),

	},
	//UNDERWORLD
	{
		.notes = underworld_melody,
		.tempo = underworld_tempo,
		.length = sizeof(underworld_melody)/sizeof(uint16_t),
	},
	//STARWARS
	{
		.notes = starwars_melody,
		.tempo = starwars_tempo,
		.length = sizeof(starwars_melody)/sizeof(uint16_t),
	},
};

void play_note(uint16_t note, uint16_t duration_ms) {

	if(note != 0){
		dac_play(note);
	}
	chThdSleepMilliseconds(duration_ms);
	dac_stop();
}


void play_melody(song_selection_t choice){
	melody_t* song = &melody[choice];

	for (int thisNote = 0; thisNote < song->length; thisNote++) {

		// to calculate the note duration, take one second
		// divided by the note type.
		//e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
		uint16_t noteDuration = (uint16_t)(1000 / song->tempo[thisNote]);

		play_note(song->notes[thisNote], noteDuration);

		// to distinguish the notes, set a minimum time between them.
		// the note's duration + 30% seems to work well:
		uint16_t pauseBetweenNotes = (uint16_t)(noteDuration * 1.30);
		chThdSleepMilliseconds(pauseBetweenNotes);

	}
}


