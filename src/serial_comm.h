#ifndef SERIAL_COMM_H
#define SERIAL_COMM_H

#include <stdint.h>
#include <hal.h>

void serial_start(void);
uint32_t serial_get_last_errors(void);
void serial_clear_last_errors(void);

#endif /* SERIAL_COMM_H */
