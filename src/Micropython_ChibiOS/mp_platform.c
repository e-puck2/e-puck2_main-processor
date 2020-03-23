/**
 * @file	mp_platform.c
 * @brief  	Used to make the link between the micropython files and the ChibiOS project.
 * 
 * @written by  	Eliot Ferragni
 * @creation date	20.03.2020
 */

#include "mp_platform.h"

#include <ch.h>
#include <hal.h>

#include <usbcfg.h>
#include <shell.h>
#include <chprintf.h>

#include "py/compile.h"
#include "py/runtime.h"
#include "py/repl.h"
#include "py/gc.h"
#include "py/mperrno.h"
#include "py/mphal.h"
#include "lib/utils/pyexec.h"
#include "mpconfigport.h"
#include "mpport.h"
#include "py_flash.h" 
#include "lib/mp-readline/readline.h"
#include "flash/mp_flash.h"

/////////////////////////////////////////PRIVATE FUNCTIONS/////////////////////////////////////////

static char *stack_top;
#if MICROPY_ENABLE_GC
static char heap[MICROPYTHON_HEAP_SIZE];
#endif

extern uint32_t _py_flash_rw_start;
extern uint32_t _py_flash_rw_end;

static uint32_t py_flash_rw_start = (uint32_t)&_py_flash_rw_start;
static uint32_t py_flash_rw_end = (uint32_t)&_py_flash_rw_end;
static uint32_t py_flash_sector_size = 0;
static uint32_t flash_code_len = 0;

void mpFlashWrite(uint8_t c){
	//write c to flash

	//writes the choice and the pattern on the flash 
	mp_flash_program_byte(py_flash_rw_start + flash_code_len, c);

	//increment for the next write
	flash_code_len += sizeof(uint8_t);
}

void mpFlashBegin(void){
	mp_flash_unlock();
	flash_code_len = 0;
	mp_flash_erase_sector(MICROPYTHON_FLASH_CODE_SECTOR);
}

void mpFlashFinish(bool valid){

	if(!valid){
		//we mark the flash_code_length as a zero length str
		flash_code_len = 0;
	}

	mpFlashWrite('\0');
	mp_flash_lock();
}

void mpStoreCodeToFlash(void){

	static char c = 0;


	mp_hal_stdout_tx_str("Do you want to send a python script to the flash ?\r\n");
	mp_hal_stdout_tx_str("CTRL-E yes, CTRL-D no\r\n");
	mp_hal_stdout_tx_str("=== ");

	while(true){
		c = mp_hal_stdin_rx_chr();	//wait infinitely for an answer
		if(c == CHAR_CTRL_E){
			//paste mode to write to the flash
			mpFlashBegin();
			chprintf((BaseSequentialStream *)&MICROPYTHON_PORT,"\r\npaste mode; Ctrl-C to cancel, Ctrl-D to finish\r\n=== ");
			while(true){
				c = mp_hal_stdin_rx_chr();	//wait infinitely for an answer
				if(c == CHAR_CTRL_C){
					mpFlashFinish(false);
					mp_hal_stdout_tx_str("\r\n");
					break;
				}else if(c == CHAR_CTRL_D){
					mpFlashFinish(true);
					mp_hal_stdout_tx_str("\r\n");
					break;
				}else{
					if(flash_code_len == py_flash_sector_size){
                    	mp_hal_stdout_tx_str("\r\n Flash full. Stopped copying. Code not saved\r\n");
                    	mpFlashFinish(false);
                    	mp_hal_stdin_rx_flush();
                    	break;
                    }
					mpFlashWrite(c);
					if (c == '\r') {
                        mp_hal_stdout_tx_str("\r\n=== ");
                    } else {
                        mp_hal_stdout_tx_strn(&c, 1);
                    }
				}
			}
			break;
		}else if(c == CHAR_CTRL_D){
			mp_hal_stdout_tx_str("\r\n");
			break;
		}
	}
}

static THD_WORKING_AREA(waMicropythonThd,1024);
static THD_FUNCTION(MicropythonThd,arg) {
  	(void)arg;
  	chRegSetThreadName("Micropython Thd");

  	int stack_dummy;
	stack_top = (char*)&stack_dummy;

	py_flash_sector_size = ((py_flash_rw_end - py_flash_rw_start) / sizeof(uint8_t)) - 1;

soft_reset:
#if MICROPY_ENABLE_GC
	gc_init(heap, heap + sizeof(heap));
#endif
	mp_init();
#if MICROPY_ENABLE_COMPILER

	//Waits to be connected to the terminal
	while(!mp_is_terminal_connected()){
		chThdSleepMilliseconds(500);
	}

	//compiles and eecutes the python script stored in flash
	micropython_parse_compile_execute_from_str(py_flash_code);
	// Main script is finished, so now go into REPL mode.
	// The REPL mode can change, or it can request a soft reset.

	for (;;) {

	    if (pyexec_mode_kind == PYEXEC_MODE_RAW_REPL) {
	        if (pyexec_raw_repl() != 0) {
	            break;
	        }
	    } else {
	        if (pyexec_friendly_repl() != 0) {
	            break;
	        }
	    }
	}
	//receives python code to store to the flash if wanted
	mpStoreCodeToFlash();

	chprintf((BaseSequentialStream *)&MICROPYTHON_PORT,"MPY: soft reboot\r\n");
	//resets the VM
	gc_sweep_all();

	goto soft_reset;
#else
	pyexec_frozen_module("frozentest.py");
#endif
	mp_deinit();
}

/////////////////////////////////////////PUBLIC FUNCTIONS/////////////////////////////////////////

void gc_collect(void) {
    // WARNING: This gc_collect implementation doesn't try to get root
    // pointers from CPU registers, and thus may function incorrectly.
    void *dummy;
    gc_collect_start();
    gc_collect_root(&dummy, ((mp_uint_t)stack_top - (mp_uint_t)&dummy) / sizeof(mp_uint_t));
    gc_collect_end();
    gc_dump_info();
}

int mp_is_terminal_connected(void){
	return isUSBConfigured();
}

//flushes the input buffer
void mp_hal_stdin_rx_flush(void){
	static uint8_t c[1] = {0};
	while(chnReadTimeout((BaseChannel*)&MICROPYTHON_PORT, c, 1,TIME_MS2I(100)) > 0);
}

// Receive single character
int mp_hal_stdin_rx_chr(void) {
    static uint8_t c[1] = {0};

	chnRead((BaseChannel*)&MICROPYTHON_PORT, c, 1);

	return (unsigned char)c[0];

}

// Send string of given length
void mp_hal_stdout_tx_strn(const char *str, mp_uint_t len) {

	if(len > 0){
		chnWrite((BaseChannel*)&MICROPYTHON_PORT, (uint8_t*)str, len);
	}
}

void micropythonStart(void){
	chThdCreateStatic(waMicropythonThd, sizeof(waMicropythonThd), NORMALPRIO, MicropythonThd, NULL);
}
