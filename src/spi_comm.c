#include <stdlib.h>
#include <string.h>
#include <ch.h>
#include <hal.h>
#include "camera/dcmi_camera.h"
#include "button.h"
#include "leds.h"
#include "spi_comm.h"
#include "sensors/proximity.h"

uint8_t spi_rx_buff[SPI_PACKET_MAX_SIZE];
uint8_t spi_tx_buff[SPI_PACKET_MAX_SIZE]; //12]; //SPI_PACKET_MAX_SIZE];
uint8_t *last_img_ptr = NULL;
uint8_t image_transfer_enabled = 0;
uint8_t rgb_setting_enabled = 0;
uint8_t spi_suspend_flag = 0;
static thread_t *spi_thd_ptr;

/*
 * SPI communication thread.
 */
static THD_WORKING_AREA(spi_thread_wa, 1024);
static THD_FUNCTION(spi_thread, p) {
	(void)p;
	chRegSetThreadName("SPI thread");

	uint8_t rx_err = 0;
	volatile uint32_t delay = 0;
	uint16_t packetId = 0;
	uint16_t numPackets = 0;
	uint32_t remainingBytes = 0;

	chThdSleepMilliseconds(50); // Wait for the camera to be configured.
	//wait_dcmi_ready(); // To be implemented...

#if 0
// Test single SPI transaction with ESP32. Trigger the transmission by "touching" the prox0.
	uint8_t btn_state = 0;
	uint8_t pack_id = 0;
	last_img_ptr = dcmi_get_last_image_ptr();
	while (true) {
		set_led(LED1, 2);

		btn_state = 0;
		while(btn_state < 2) {
			switch(btn_state) {
				case 0:
					if(get_prox(0) >= 3000) {
						btn_state = 2;
					}
					break;
				case 1:
					if(get_prox(0) <= 300) {
						btn_state = 2;
					}
					break;
			}
		}

		last_img_ptr[0] = pack_id;
		if(pack_id == 255) {
			pack_id = 0;
		} else {
			pack_id++;
		}

		spiSelect(&SPID1);
//		for(delay=0; delay<SPI_DELAY; delay++) {
//			__NOP();
//		}
		//chThdSleepMilliseconds(5);
		//spiExchange(&SPID1, 12, spi_tx_buff, spi_rx_buff);
		spiExchange(&SPID1, SPI_PACKET_MAX_SIZE, &last_img_ptr[0], spi_rx_buff);
//		for(delay=0; delay<SPI_DELAY; delay++) {
//			__NOP();
//		}
		//chThdSleepMilliseconds(5);
		spiUnselect(&SPID1);

		set_led(LED7, 1);
		for(delay=0; delay<SPI_DELAY; delay++) {
			__NOP();
		}
		set_led(LED7, 0);

		//chThdSleepMilliseconds(50);

	}
#endif

	while (true) {

		chSysLock();
		if(spi_suspend_flag == 1) {
			chSchGoSleepS(CH_STATE_SUSPENDED);
		}
	    chSysUnlock();

		if(image_transfer_enabled == 1) {
			// Avoid using DCMI and SPI at the same time because there is an hardware bug with DMA.
			// Therefore we need to first grab an image and then send it through SPI while the DCMI is stopped.
			dcmi_capture_start();
			wait_image_ready();
		}

		memset(spi_rx_buff, 0xFF, SPI_PACKET_MAX_SIZE);
		if(rgb_setting_enabled == 1) {
			get_all_rgb_state(&spi_tx_buff[0]);
		} else {
			memset(spi_tx_buff, 101, 12); // When the intensity value of the RGB LEDs is out of range, then their state isn't changed.
		}

		spiSelect(&SPID1);
		spiExchange(&SPID1, 12, spi_tx_buff, spi_rx_buff);
		spiUnselect(&SPID1);

		// A little pause between transactions is needed for the communication to work, 5000 NOP loops last about 350-500 us.
		for(delay=0; delay<SPI_DELAY; delay++) {
			__NOP();
		}

		button_set_state(spi_rx_buff[0]);

		if(image_transfer_enabled == 1) {
			if(spi_rx_buff[1] == 0xB7) { // -'I' => camera image

				numPackets = po8030_get_image_size()/SPI_PACKET_MAX_SIZE;
				remainingBytes = po8030_get_image_size()%SPI_PACKET_MAX_SIZE;
				rx_err = 0;

				last_img_ptr = dcmi_get_last_image_ptr();

	//			memset(spi_tx_buff, 0xFF, SPI_PACKET_MAX_SIZE);
	//			memset(last_img_ptr, 0xFF, SPI_PACKET_MAX_SIZE);

				for(packetId=0; packetId<numPackets; packetId++) {
					spiSelect(&SPID1);
					spiExchange(&SPID1, SPI_PACKET_MAX_SIZE, &last_img_ptr[packetId*SPI_PACKET_MAX_SIZE], spi_rx_buff);
					spiUnselect(&SPID1);

					// A little pause between transactions is needed for the communication to work, 5000 NOP loops last about 350-500 us.
					for(delay=0; delay<SPI_DELAY; delay++) {
						__NOP();
					}

					if(spi_rx_buff[1] == 0) { // No answer from ESP32...
						rx_err = 1;
						break;
					}
				}

				if(remainingBytes>0 && rx_err==0) {
					spiSelect(&SPID1);
					spiExchange(&SPID1, remainingBytes, &last_img_ptr[packetId*SPI_PACKET_MAX_SIZE], spi_rx_buff);
					spiUnselect(&SPID1);

					// A little pause between transactions is needed for the communication to work, 5000 NOP loops last about 350-500 us.
					for(delay=0; delay<SPI_DELAY; delay++) {
						__NOP();
					}
				}

				dcmi_release_last_image_ptr();

			} else if(spi_rx_buff[1] == 0xB6) { // -'J' => camera settings

			}
		}

	} // Infinite loop.

}

void spi_comm_start(void) {
	// SPI1 maximum speed is 42 MHz, ESP32 supports at most 10MHz, so use a prescaler of 1/8 (84 MHz / 8 = 10.5 MHz).
	// SPI1 configuration (10.5 MHz, CPHA=0, CPOL=0, MSb first).
	static const SPIConfig hs_spicfg = {
		NULL,
		GPIOA,
		15,
		SPI_CR1_BR_1
		//SPI_CR1_BR_1 | SPI_CR1_BR_0 // 5.25 MHz
	};
	spiStart(&SPID1, &hs_spicfg);	// Setup transfer parameters.
	spi_thd_ptr = chThdCreateStatic(spi_thread_wa, sizeof(spi_thread_wa), NORMALPRIO, spi_thread, NULL);
}

void spi_image_transfer_enable() {
	image_transfer_enabled = 1;
}

void spi_image_transfer_disable() {
	image_transfer_enabled = 0;
}

void spi_rgb_setting_enable(void) {
	rgb_setting_enabled = 1;
}

void spi_rgb_setting_disable(void) {
	rgb_setting_enabled = 0;
}

void spi_comm_suspend(void) {
	if(spi_suspend_flag == 0) {
		spi_suspend_flag = 1;
		while(spi_thd_ptr->p_state != CH_STATE_SUSPENDED) { // Wait for the thread to be suspended.
			chThdSleepMilliseconds(2);
		}
	}
}

void spi_comm_resume(void) {
	if(spi_suspend_flag == 1) {
		chSysLock();
		chSchWakeupS(spi_thd_ptr, MSG_OK);
		spi_suspend_flag = 0;
		chSysUnlock();
	}
}


