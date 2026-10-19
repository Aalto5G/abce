#include "abcejmalloc.h"
#include "abce.h"
#include "abcelinkedlist.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#define ALIGN (_Alignof(max_align_t))
#else
static inline size_t get_max_align_compatibility(void)
{
  size_t res = 8;
  if (sizeof(void*) > res) res *= 2;
  if (sizeof(void*) > res) res *= 2;
  if (sizeof(void*) > res) res *= 2;
  if (sizeof(void*) > res) res *= 2;
  if (sizeof(void(*)(void)) > res) res *= 2;
  if (sizeof(void(*)(void)) > res) res *= 2;
  if (sizeof(void(*)(void)) > res) res *= 2;
  if (sizeof(void(*)(void)) > res) res *= 2;
  if (sizeof(ptrdiff_t) > res) res *= 2;
  if (sizeof(ptrdiff_t) > res) res *= 2;
  if (sizeof(ptrdiff_t) > res) res *= 2;
  if (sizeof(ptrdiff_t) > res) res *= 2;
  if (sizeof(size_t) > res) res *= 2;
  if (sizeof(size_t) > res) res *= 2;
  if (sizeof(size_t) > res) res *= 2;
  if (sizeof(size_t) > res) res *= 2;
  if (sizeof(long long) > res) res *= 2;
  if (sizeof(long long) > res) res *= 2;
  if (sizeof(long long) > res) res *= 2;
  if (sizeof(long long) > res) res *= 2;
  if (sizeof(unsigned long long) > res) res *= 2;
  if (sizeof(unsigned long long) > res) res *= 2;
  if (sizeof(unsigned long long) > res) res *= 2;
  if (sizeof(unsigned long long) > res) res *= 2;
  if (sizeof(double) > res) res *= 2;
  if (sizeof(double) > res) res *= 2;
  if (sizeof(double) > res) res *= 2;
  if (sizeof(double) > res) res *= 2;
  if (sizeof(long double) > res) res *= 2;
  if (sizeof(long double) > res) res *= 2;
  if (sizeof(long double) > res) res *= 2;
  if (sizeof(long double) > res) res *= 2;
  return res;
}
#define ALIGN (get_max_align_compatibility())
#endif

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

#define CONTAINER_OF(ptr, type, member) \
  ((type*)(((char*)ptr) - (((char*)&(((type*)0)->member)) - ((char*)0))))

union jmalloc_union {
  //char block[0];
  struct jmalloc_block *next;
};

struct jmalloc_block {
  union jmalloc_union u;
};

static size_t abce_arenaremain;
static char *abce_arena;
static size_t jmarenabytes;

// 16, 32, 64, 128, 256, 512, 1024, 2048
static struct jmalloc_block *abce_blocks[8];
// 4096, 8192, 16384, 32768, 65536, 131072, 262144, 524288, 1048576
//...except now 131072 is the largest that does not use mmap
static struct jmalloc_block *abce_blocks2[9];


static const uint8_t abce_lookup[129] = {
0,0,1,2,2,3,3,3,3,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7
};

static const uint8_t abce_lookup2[256] = {
0,1,2,2,3,3,3,3,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8
};

#if 0 // abce_do_mmap_madvise does this already
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
#endif

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
    if (unlikely(sz > 32*1024))
    {
      jmarenabytes += sz;
      ret = abce_do_mmap_madvise(sz, 1);
      //ret = mmap(NULL, abce_jm_topages(sz), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
      if (ret == NULL)
      {
        return NULL;
      }
      return ret;
    }
    lookupval = abce_lookup2[(sz-1)/4096];
    ls = &abce_blocks2[lookupval];
    sz = 1U<<(12+lookupval);
  }
  else
  {
    lookupval = abce_lookup[(sz+15)/16];
    if (lookupval == 0 && (sizeof(struct jmalloc_block) > 16 || ALIGN > 16))
    {
      lookupval = 1;
    }
    if (lookupval == 1 && (sizeof(struct jmalloc_block) > 32 || ALIGN > 32))
    {
      lookupval = 2;
    }
    if (lookupval == 2 && (sizeof(struct jmalloc_block) > 64 || ALIGN > 64))
    {
      lookupval = 3;
    }
    if (lookupval == 3 && (sizeof(struct jmalloc_block) > 128 || ALIGN > 128))
    {
      lookupval = 4;
    }
    if (lookupval == 4 && (sizeof(struct jmalloc_block) > 256 || ALIGN > 256))
    {
      lookupval = 5;
    }
    if (lookupval == 5 && (sizeof(struct jmalloc_block) > 512 || ALIGN > 512))
    {
      lookupval = 6;
    }
    if (lookupval == 6 && (sizeof(struct jmalloc_block) > 1024 || ALIGN > 1024))
    {
      lookupval = 7;
    }
    if (sizeof(struct jmalloc_block) > 2048 || ALIGN > 2048)
    {
      abort();
    }
    ls = &abce_blocks[lookupval];
    sz = 1U<<(4+lookupval);
  }
  if (unlikely(!*ls))
  {
    if (unlikely(abce_arenaremain < sz))
    {
      abce_arenaremain = 128*1024;
      jmarenabytes += abce_arenaremain;
      abce_arena = abce_do_mmap_madvise(abce_arenaremain, 1);
      //abce_arena = mmap(NULL, abce_arenaremain, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
      if (unlikely(abce_arena == NULL))
      {
        abort();
      }
    }
    if (unlikely(abce_arenaremain < sz))
    {
      abort();
    }
    ret = abce_arena;
    abce_arenaremain -= sz;
    abce_arena += sz;
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
      abce_do_munmap(ptr, sz);
      return;
    }
    lookupval = abce_lookup2[(sz-1)/4096];
    ls = &abce_blocks2[lookupval];
  }
  else
  {
    lookupval = abce_lookup[(sz+15)/16];
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
    ls = &abce_blocks[lookupval];
  }
  blk->u.next = *ls;
  *ls = blk;
}

size_t abce_jmgetarenabytes(void)
{
  return jmarenabytes;
}
