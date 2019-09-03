#include "abcejmalloc.h"
#include "abcelinkedlist.h"
#include <sys/mman.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

#define CONTAINER_OF(ptr, type, member) \
  ((type*)(((char*)ptr) - (((char*)&(((type*)0)->member)) - ((char*)0))))

struct jmalloc_block {
  union {
    char block[0];
    struct jmalloc_block *next;
  } u;
};

size_t arenaremain;
char *arena;

// 16, 32, 64, 128, 256, 512, 1024, 2048
struct jmalloc_block *blocks[8];
// 4096, 8192, 16384, 32768, 65536, 131072, 262144, 524288, 1048576
struct jmalloc_block *blocks2[9] = {};


const uint8_t lookup[129] = {
0,0,1,2,2,3,3,3,3,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7
};

const uint8_t lookup2[256] = {
0,1,2,2,3,3,3,3,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8
};

static inline size_t abce_jm_topages(size_t limit)
{
  long pagesz = sysconf(_SC_PAGE_SIZE);
  size_t pages, actlimit;
  if (pagesz <= 0)
  {
    abort();
  }
  pages = (limit + (pagesz-1)) / pagesz;
  actlimit = pages * pagesz;
  return actlimit;
}

void *abce_jmrealloc(void *oldptr, size_t oldsz, size_t newsz)
{
  void *newptr = abce_jmalloc(newsz);
  size_t tocopy = (newsz < oldsz) ? newsz : oldsz;
  memcpy(newptr, oldptr, tocopy);
  if (oldptr != NULL || oldsz != 0)
  {
    abce_jmfree(oldptr, oldsz);
  }
  return newptr;
}

void *abce_jmalloc(size_t sz)
{
  struct jmalloc_block **ls = NULL;
  struct jmalloc_block *blk;
  uint8_t lookupval;
  void *ret;
  if (unlikely(sz > 2048))
  {
    if (unlikely(sz > 1048576))
    {
      ret = mmap(NULL, abce_jm_topages(sz), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
      if (ret == MAP_FAILED)
      {
        return NULL;
      }
      return ret;
    }
    lookupval = lookup2[(sz-1)/4096];
    ls = &blocks2[lookupval];
    sz = 1<<(12+lookupval);
  }
  else
  {
    lookupval = lookup[(sz+15)/16];
    if (lookupval == 0 && sizeof(struct jmalloc_block) > 16)
    {
      lookupval = 1;
    }
    if (lookupval == 1 && sizeof(struct jmalloc_block) > 32)
    {
      lookupval = 2;
    }
    if (lookupval == 2 && sizeof(struct jmalloc_block) > 64)
    {
      lookupval = 3;
    }
    if (lookupval == 3 && sizeof(struct jmalloc_block) > 128)
    {
      lookupval = 4;
    }
    if (lookupval == 4 && sizeof(struct jmalloc_block) > 256)
    {
      lookupval = 5;
    }
    if (lookupval == 5 && sizeof(struct jmalloc_block) > 512)
    {
      lookupval = 6;
    }
    if (lookupval == 6 && sizeof(struct jmalloc_block) > 1024)
    {
      lookupval = 7;
    }
    if (sizeof(struct jmalloc_block) > 2048)
    {
      abort();
    }
    ls = &blocks[lookupval];
    sz = 1<<(4+lookupval);
  }
  if (unlikely(!*ls))
  {
    if (unlikely(arenaremain < sz))
    {
      arenaremain = 32*1024*1024;
      arena = mmap(NULL, arenaremain, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
      if (unlikely(arena == MAP_FAILED || arena == NULL))
      {
        abort();
      }
    }
    if (unlikely(arenaremain < sz))
    {
      abort();
    }
    ret = arena;
    arenaremain -= sz;
    arena += sz;
    return ret;
  }
  blk = *ls;
  *ls = blk->u.next;
  return blk;
}

void abce_jmfree(void *ptr, size_t sz)
{
  struct jmalloc_block **ls = NULL;
  struct jmalloc_block *blk = ptr;
  uint8_t lookupval;
  if (unlikely(sz > 2048))
  {
    if (unlikely(sz > 1048576))
    {
      munmap(ptr, abce_jm_topages(sz));
      return;
    }
    lookupval = lookup2[(sz-1)/4096];
    ls = &blocks2[lookupval];
  }
  else
  {
    lookupval = lookup[(sz+15)/16];
    if (lookupval == 0 && sizeof(struct jmalloc_block) > 16)
    {
      lookupval = 1;
    }
    if (lookupval == 1 && sizeof(struct jmalloc_block) > 32)
    {
      lookupval = 2;
    }
    if (lookupval == 2 && sizeof(struct jmalloc_block) > 64)
    {
      lookupval = 3;
    }
    if (lookupval == 3 && sizeof(struct jmalloc_block) > 128)
    {
      lookupval = 4;
    }
    if (lookupval == 4 && sizeof(struct jmalloc_block) > 256)
    {
      lookupval = 5;
    }
    if (lookupval == 5 && sizeof(struct jmalloc_block) > 512)
    {
      lookupval = 6;
    }
    if (lookupval == 6 && sizeof(struct jmalloc_block) > 1024)
    {
      lookupval = 7;
    }
    if (sizeof(struct jmalloc_block) > 2048)
    {
      abort();
    }
    ls = &blocks[lookupval];
  }
  blk->u.next = *ls;
  *ls = blk;
}
