#ifndef AR0144_H
#define AR0144_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "hal.h"
#include "camera.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief       Initializes the clock generation for the po6030
 */
void ar0144_start(void);


#ifdef __cplusplus
}
#endif

#endif
