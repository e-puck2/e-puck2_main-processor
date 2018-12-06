#ifndef SPI_COMM_H
#define SPI_COMM_H

#include <stdint.h>
#include <hal.h>

#define SPI_DELAY 5000
#define SPI_PACKET_MAX_SIZE 4092

void spi_comm_start(void);
void spi_image_transfer_enable(void);
void spi_image_transfer_disable(void);
void spi_rgb_setting_enable(void);
void spi_rgb_setting_disable(void);
void spi_comm_suspend(void);
void spi_comm_resume(void);

#endif /* SPI_COMM_H */
