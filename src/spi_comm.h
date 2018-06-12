#ifndef SPI_COMM_H
#define SPI_COMM_H

#include <stdint.h>
#include <hal.h>

#define SPI_DELAY 5000
#define SPI_PACKET_MAX_SIZE 4092

void spi_comm_start(void);

#endif /* SPI_COMM_H */
