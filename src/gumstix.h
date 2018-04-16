#ifndef GUMSTIX_H
#define GUMSTIX_H

#include "chprintf.h"

#ifdef __cplusplus
extern "C" {
#endif

void start_gumstix_comm(BaseSequentialStream *serport);

#ifdef __cplusplus
}
#endif

#endif
