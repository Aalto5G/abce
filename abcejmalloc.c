#include "abcejmalloc.h"
#include "abcelinkedlist.h"
#include <sys/mman.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>


#define CONTAINER_OF(ptr, type, member) \
  ((type*)(((char*)ptr) - (((char*)&(((type*)0)->member)) - ((char*)0))))

struct jmalloc_block {
  union {
    char block[0];
    struct abce_linked_list_node llnode;
  } u;
};

size_t arenaremain;
char *arena;

// 16, 32, 64, 128, 256, 512, 1024, 2048
struct abce_linked_list_head blocks[8] = {
  {.node = {.prev = &blocks[0].node, .next = &blocks[0].node}},
  {.node = {.prev = &blocks[1].node, .next = &blocks[1].node}},
  {.node = {.prev = &blocks[2].node, .next = &blocks[2].node}},
  {.node = {.prev = &blocks[3].node, .next = &blocks[3].node}},
  {.node = {.prev = &blocks[4].node, .next = &blocks[4].node}},
  {.node = {.prev = &blocks[5].node, .next = &blocks[5].node}},
  {.node = {.prev = &blocks[6].node, .next = &blocks[6].node}},
  {.node = {.prev = &blocks[7].node, .next = &blocks[7].node}},
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
  struct abce_linked_list_head *ls = NULL;
  struct jmalloc_block *blk;
  void *ret;
  if (sz > 2048)
  {
    ret = mmap(NULL, abce_jm_topages(sz), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
    if (ret == MAP_FAILED)
    {
      return NULL;
    }
    return ret;
  }
  if (sz <= 16 && 16 >= sizeof(struct jmalloc_block))
  {
    ls = &blocks[0];
    sz = 16;
  }
  else if (sz <= 32 && 32 >= sizeof(struct jmalloc_block))
  {
    ls = &blocks[1];
    sz = 32;
  }
  else if (sz <= 64 && 64 >= sizeof(struct jmalloc_block))
  {
    ls = &blocks[2];
    sz = 64;
  }
  else if (sz <= 128 && 128 >= sizeof(struct jmalloc_block))
  {
    ls = &blocks[3];
    sz = 128;
  }
  else if (sz <= 256 && 256 >= sizeof(struct jmalloc_block))
  {
    ls = &blocks[4];
    sz = 256;
  }
  else if (sz <= 512 && 512 >= sizeof(struct jmalloc_block))
  {
    ls = &blocks[5];
    sz = 512;
  }
  else if (sz <= 1024 && 1024 >= sizeof(struct jmalloc_block))
  {
    ls = &blocks[6];
    sz = 1024;
  }
  else if (sz <= 2048 && 2048 >= sizeof(struct jmalloc_block))
  {
    ls = &blocks[7];
    sz = 2048;
  }
  if (abce_linked_list_is_empty(ls))
  {
    if (arenaremain < sz)
    {
      arenaremain = 32*1024*1024;
      arena = mmap(NULL, arenaremain, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
      if (arena == MAP_FAILED || arena == NULL)
      {
        abort();
      }
    }
    if (arenaremain < sz)
    {
      abort();
    }
    ret = arena;
    arenaremain -= sz;
    arena += sz;
    return ret;
  }
  blk = CONTAINER_OF(ls->node.next, struct jmalloc_block, u.llnode);
  abce_linked_list_delete(&blk->u.llnode);
  return blk;
}

void abce_jmfree(void *ptr, size_t sz)
{
  struct abce_linked_list_head *ls = NULL;
  struct jmalloc_block *blk = ptr;
  if (sz > 2048)
  {
    munmap(ptr, abce_jm_topages(sz));
    return;
  }
  if (sz <= 16 && 16 >= sizeof(struct jmalloc_block))
  {
    ls = &blocks[0];
    sz = 16;
  }
  else if (sz <= 32 && 32 >= sizeof(struct jmalloc_block))
  {
    ls = &blocks[1];
    sz = 32;
  }
  else if (sz <= 64 && 64 >= sizeof(struct jmalloc_block))
  {
    ls = &blocks[2];
    sz = 64;
  }
  else if (sz <= 128 && 128 >= sizeof(struct jmalloc_block))
  {
    ls = &blocks[3];
    sz = 128;
  }
  else if (sz <= 256 && 256 >= sizeof(struct jmalloc_block))
  {
    ls = &blocks[4];
    sz = 256;
  }
  else if (sz <= 512 && 512 >= sizeof(struct jmalloc_block))
  {
    ls = &blocks[5];
    sz = 512;
  }
  else if (sz <= 1024 && 1024 >= sizeof(struct jmalloc_block))
  {
    ls = &blocks[6];
    sz = 1024;
  }
  else if (sz <= 2048 && 2048 >= sizeof(struct jmalloc_block))
  {
    ls = &blocks[7];
    sz = 2048;
  }
  abce_linked_list_add_head(&blk->u.llnode, ls);
}
