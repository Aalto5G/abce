#ifndef _SAFEMODE_H_
#define _SAFEMODE_H_
#include "abcedatatypes.h"
#include <stdint.h>
int noio_restrict_fn(void **pbaton, uint16_t ins);
#endif
