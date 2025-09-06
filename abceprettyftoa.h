#ifndef _ABCE_PRETTY_FTOA_H_
#define _ABCE_PRETTY_FTOA_H_

#include <stddef.h>

void abce_pretty_ftoa_fix_exponent(char *buf);

void abce_pretty_ftoa(char *buf, size_t bufsiz, double d);

#endif
