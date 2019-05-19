#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include "abcerbtree.h"
#include "abcemurmur.h"
#include "abcecontainerof.h"
#include "abcelikely.h"
#include "abceopcodes.h"
#include "abcedatatypes.h"
#include "abcestring.h"

struct abce_strbuf {
  char *str;
  size_t sz;
  size_t cap;
  int taint;
};

static void abce_strbuf_bump(struct abce *abce, struct abce_strbuf *buf, size_t bump)
{
  char *newbuf;
  size_t newcap;
  if (buf->taint)
  {
    return;
  }
  newcap = buf->sz*2 + 1;
  if (newcap < buf->sz + bump)
  {
    newcap = buf->sz + bump;
  }
  newbuf = abce->alloc(buf->str, buf->cap, newcap, &abce->alloc_baton);
  if (newbuf == NULL)
  {
    buf->taint = 1;
    return;
  }
  buf->str = newbuf;
  buf->cap = newcap;
}

static inline int abce_strbuf_add_nul(struct abce *abce, struct abce_strbuf *buf)
{
  if (buf->sz >= buf->cap)
  {
    abce_strbuf_bump(abce, buf, 1);
    if (buf->taint)
    {
      return -ENOMEM;
    }
  }
  buf->str[buf->sz] = '\0';
  return 0;
}

static inline int abce_strbuf_add(struct abce *abce, struct abce_strbuf *buf, char ch)
{
  if (buf->sz >= buf->cap)
  {
    abce_strbuf_bump(abce, buf, 1);
    if (buf->taint)
    {
      return -ENOMEM;
    }
  }
  buf->str[buf->sz++] = ch;
  return 0;
}

static inline int abce_strbuf_add_many(struct abce *abce, struct abce_strbuf *buf, const char *st, size_t sz)
{
  if (buf->sz + sz > buf->cap)
  {
    abce_strbuf_bump(abce, buf, sz);
    if (buf->taint)
    {
      return -ENOMEM;
    }
  }
  memcpy(&buf->str[buf->sz], st, sz);
  buf->sz += sz;
  return 0;
}

int abce_strgsub(struct abce *abce,
                 char **res, size_t *ressz, size_t *rescap,
                 const char *haystack, size_t haystacksz,
                 const char *needle, size_t needlesz,
                 const char *sub, size_t subsz)
{
  struct abce_strbuf buf = {};
  size_t haystackpos = 0;
  if (needlesz == 0)
  {
    *res = NULL;
    *ressz = 0;
    return -EINVAL;
  }
  while (haystackpos + needlesz <= haystacksz)
  {
    if (memcmp(&haystack[haystackpos], needle, needlesz) == 0)
    {
      abce_strbuf_add_many(abce, &buf, sub, subsz);
      haystackpos += needlesz;
      continue;
    }
    abce_strbuf_add(abce, &buf, haystack[haystackpos++]);
  }
  abce_strbuf_add_many(abce, &buf, &haystack[haystackpos], haystacksz - haystackpos);
  abce_strbuf_add_nul(abce, &buf);
  if (buf.taint)
  {
    abce->alloc(buf.str, buf.cap, 0, &abce->alloc_baton);
    *res = NULL;
    *ressz = 0;
    *rescap = 0;
    return -ENOMEM;
  }
  *res = buf.str;
  *ressz = buf.sz;
  *rescap = buf.cap;
  return 0;
}
