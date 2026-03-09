#ifndef _ABCE_CONTAINEROF_H_
#define _ABCE_CONTAINEROF_H_

#include <stddef.h>

#define ABCE_CONTAINER_OF(ptr, type, member) \
  ((type*)(((char*)ptr) - offsetof(type, member)))

#endif
