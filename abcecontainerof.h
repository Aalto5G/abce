#ifndef _ABCE_CONTAINEROF_H_
#define _ABCE_CONTAINEROF_H_

#define ABCE_CONTAINER_OF(ptr, type, member) \
  ((type*)(((char*)ptr) - (((char*)&(((type*)0)->member)) - ((char*)0))))

#endif
