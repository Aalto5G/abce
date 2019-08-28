#ifndef _JMALLOC_H_
#define _JMALLOC_H_

#include <stddef.h>

void *abce_jmalloc(size_t sz);

void abce_jmfree(void *ptr, size_t sz);

void *abce_jmrealloc(void *oldptr, size_t oldsz, size_t newsz);

#endif
