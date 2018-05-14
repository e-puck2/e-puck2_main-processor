/*

File    : play_sound_file.c
Author  : Eliot Ferragni
Date    : 9 may 2018
REV 1.0

Functions and defines to play sounds from the sd card in the uncompressed WAV format in signed 16bits 44,1kHz

*/


#include <ff.h>
#include <audio/play_sound_file.h>
#include <audio/audio_thread.h>

#include <chprintf.h>

#define SIZE_SOUND_BUFFER_16bits    512
#define SIZE_WAV_HEADER_BYTES       44

#define SAMPLING_44100HZ            44100

static FATFS SDC_FS;

static uint16_t sound_buffer1[SIZE_SOUND_BUFFER_16bits];
static uint16_t sound_buffer2[SIZE_SOUND_BUFFER_16bits];

static BSEMAPHORE_DECL(change_buffer, true);

static uint16_t* buffer_to_read = NULL;
static uint32_t size_buffer_to_read = 0;
static uint8_t volume_dac = 1;

inline uint16_t Rev16(uint16_t a)
{
  asm ("rev16 %1,%0"
          : "=r" (a)
          : "r" (a));
  return a;
}

/*
 * DAC end callback.
 */
static void end_cb(DACDriver *dacp, const dacsample_t *buffer, size_t n) {

    (void)dacp;
    (void)buffer;
    (void)n;

    //to quit the callback and do nothing if we are here because of the half transfert interrupt
    //we only want the transfert finished
    if(buffer == sound_buffer1 || buffer == sound_buffer2){
        return;
    }

    //swaps the buffers
    if(buffer_to_read == sound_buffer1){
        buffer_to_read = sound_buffer2;
    }else{
        buffer_to_read = sound_buffer1;
    }

    chSysLockFromISR();
    if(size_buffer_to_read){
        dac_change_bufferI(buffer_to_read, size_buffer_to_read, SAMPLING_44100HZ);
        if(size_buffer_to_read<SIZE_SOUND_BUFFER_16bits){
            size_buffer_to_read = 0;
        }
        chBSemSignalI(&change_buffer);
    }else{
        dac_stopI();
    }
    chSysUnlockFromISR();
}

uint8_t play_sound_file(char *pathToFile){
    FIL file;   /* file object */
    FRESULT err;
    UINT bytesRead = 0;

    uint8_t first_time = 1;

    uint16_t* buffer_to_fill = NULL;

    uint16_t i = 0;
    /*
    * Attempt to mount the drive.
    */
    err = f_mount(&SDC_FS,"",0);
    if (err != FR_OK) {
        return ERROR;
    }
    sdcConnect(&SDCD1);

    /*
    * Open the sound file
    */
    err = f_open(&file, pathToFile, FA_READ);
    if (err != FR_OK) {
        return ERROR;
    }

    buffer_to_read = sound_buffer1;
    buffer_to_fill = sound_buffer1;

    err = f_read(&file, buffer_to_fill, SIZE_WAV_HEADER_BYTES, &bytesRead);    //flush the header

    // uint8_t* buffer = (uint8_t*) buffer_to_fill;

    // for(uint16_t i = 0 ; i < 22 ; i++){
    //     chprintf((BaseSequentialStream *)&SD3,"%1X ", buffer_to_fill[i]);
    // }
    // chprintf((BaseSequentialStream *)&SD3,"fin header\n");

    volatile systime_t time = 0;
    do{
        time = chVTGetSystemTime();
        err = f_read(&file, buffer_to_fill, SIZE_SOUND_BUFFER_16bits*2, &bytesRead);
        if (err != FR_OK) {
            f_close(&file);
            return ERROR;
        }

        // for(uint16_t i = 0 ; i < bytesRead ; i++){
        //     buffer_to_fill[i] = sound_buffer_read[i];
        // }
        // for(uint16_t i = 0 ; i < 20 ; i++){
        //     chprintf((BaseSequentialStream *)&SD3,"%1X ", buffer_to_fill[i]);
        // }

        size_buffer_to_read = bytesRead/2;  //conversion to uint16_t

        for(i = 0 ; i < size_buffer_to_read ; i++){
            //buffer_to_fill[i] = Rev16(buffer_to_fill[i]);
            buffer_to_fill[i] = (uint16_t)(((int16_t)buffer_to_fill[i] + 32768) >> 4);   //convert a signed 16bits to unsigned 12 bits
            buffer_to_fill[i] = (buffer_to_fill[i] * volume_dac)/100;  
        }

        // if(bytesRead % 2){
        //     size_buffer_to_read += 1;   //in case we have an odd number of bytes read
        //     uint8_t temp_number = buffer_to_fill[size_buffer_to_read-1];
        //     ((temp_number & 0xF0) + 128) >> 8;  //converts the 8bits
        //     buffer_to_fill[size_buffer_to_read-1] = temp_number;
        // }

        if(first_time){
            dac_play_buffer(buffer_to_read, size_buffer_to_read, SAMPLING_44100HZ, end_cb);
            first_time = 0;
        }else{
            chBSemWait(&change_buffer);
        }

        if(buffer_to_fill == sound_buffer1){
            buffer_to_fill = sound_buffer2;
        }else{
            buffer_to_fill = sound_buffer1;
        }

        time = chVTGetSystemTime() - time;

    }while(size_buffer_to_read >= SIZE_SOUND_BUFFER_16bits);

    /*
     * Close the file.
     */
    f_close(&file);

    /*
    * Attempt to unmount the drive.
    */
    sdcDisconnect(&SDCD1);
    err = f_mount(NULL,"",0);
    if (err != FR_OK) {
        return ERROR;
    }

    return OK;

}

void set_music_volume(uint8_t volume){
    volume_dac = volume;
}






