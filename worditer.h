#ifndef _ABCE_WORD_ITER_H_
#define _ABCE_WORD_ITER_H_

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

struct abce_word_iter {
  const char *string;
  size_t stringsz;
  uint64_t bm[4];
  size_t start;
  size_t end;
};

static inline size_t abce_memspn_impl(const char *s, size_t ssz, uint64_t bm[4])
{
  size_t i;
  for (i = 0; i < ssz; i++)
  {
    uint8_t u8 = (unsigned char)s[i];
    if (!(bm[u8/64] & (1ULL<<(u8%64))))
    {
      break;
    }
  }
  return i;
}
static inline size_t abce_memcspn_impl(const char *s, size_t ssz, uint64_t bm[4])
{
  size_t i;
  for (i = 0; i < ssz; i++)
  {
    uint8_t u8 = (unsigned char)s[i];
    if (bm[u8/64] & (1ULL<<(u8%64)))
    {
      break;
    }
  }
  return i;
}

static inline size_t abce_memspn(const char *s, size_t ssz, const char *accept, size_t asz)
{
  uint64_t bm[4] = {};
  size_t i;
  for (i = 0; i < asz; i++)
  {
    uint8_t u8 = (unsigned char)accept[i];
    bm[u8/64] |= 1ULL<<(u8%64);
  }
  return abce_memspn_impl(s, ssz, bm);
}

static inline size_t abce_memcspn(const char *s, size_t ssz, const char *reject, size_t rsz)
{
  uint64_t bm[4] = {};
  size_t i;
  memset(bm, 0xff, sizeof(bm));
  for (i = 0; i < rsz; i++)
  {
    uint8_t u8 = (unsigned char)reject[i];
    bm[u8/64] &= ~(1ULL<<(u8%64));
  }
  return abce_memspn_impl(s, ssz, bm);
}

static inline void
abce_word_iter_init(struct abce_word_iter *it, const char *str, size_t strsz,
                    const char *sep, size_t sepsz)
{
  size_t i;
  memset(it, 0, sizeof(*it));
  for (i = 0; i < sepsz; i++)
  {
    uint8_t u8 = (unsigned char)sep[i];
    it->bm[u8/64] |= 1ULL<<(u8%64);
  }
  it->string = str;
  it->stringsz = strsz;
  it->start = abce_memspn_impl(it->string, it->stringsz, it->bm);
  it->end = it->start + abce_memcspn_impl(it->string + it->start, it->stringsz - it->start, it->bm);
}

static inline void
abce_word_iter_next(struct abce_word_iter *it)
{
  it->start = it->end + abce_memspn_impl(it->string + it->end, it->stringsz - it->end, it->bm);
  it->end = it->start + abce_memcspn_impl(it->string + it->start, it->stringsz - it->start, it->bm);
}

static inline int
abce_word_iter_at_end(struct abce_word_iter *it)
{
  return it->start == it->stringsz;
}

#endif
