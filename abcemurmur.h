#ifndef _ABCE_MURMUR_H_
#define _ABCE_MURMUR_H_

#include <stdint.h>
#include <stddef.h>
#include "abcehdr.h"

#ifdef __cplusplus
extern "C" {
#endif

struct abce_murmurctx {
  uint32_t hash;
  uint32_t len;
};

#define MURMURCTX_INITER(seed) \
  { \
    .hash = (seed), \
    .len = 0, \
  }

static inline void abce_murmurctx_feed32(struct abce_murmurctx *ctx, uint32_t val)
{
  uint32_t c1 = 0xcc9e2d51;
  uint32_t c2 = 0x1b873593;
  uint32_t m = 5;
  uint32_t n = 0xe6546b64;
  uint32_t k = val;
  uint32_t hash = ctx->hash;
  k = k*c1;
  k = (k << 15) | (k >> 17);
  k = k*c2;
  hash = hash ^ k;
  hash = (hash << 13) | (hash >> 19);
  hash = hash*m + n;
  ctx->len += 4;
  ctx->hash = hash;
}

static inline void abce_murmurctx_feed_buf(
  struct abce_murmurctx *ctx, const void *buf, size_t sz)
{
  const char *cbuf = (const char*)buf;
  size_t i = 0;
  struct abce_murmurctx ctx2 = *ctx;
  while (i < (sz/4)*4)
  {
    abce_murmurctx_feed32(&ctx2, abce_hdr_get32h(&cbuf[i]));
    i += 4;
  }
  while (i < sz)
  {
    abce_murmurctx_feed32(&ctx2, abce_hdr_get8h(&cbuf[i]));
    i += 1;
  }
  *ctx = ctx2;
}

static inline uint32_t abce_murmurctx_get(struct abce_murmurctx *ctx)
{
  uint32_t hash;
  hash = ctx->hash;
  hash = hash ^ ctx->len;
  hash = hash ^ (hash >> 16);
  hash = hash * 0x85ebca6b;
  hash = hash ^ (hash >> 13);
  hash = hash * 0xc2b2ae35;
  hash = hash ^ (hash >> 16);
  return hash;
}


static inline uint32_t abce_murmur32(uint32_t seed, uint32_t val)
{
  struct abce_murmurctx ctx = MURMURCTX_INITER(seed);
  abce_murmurctx_feed32(&ctx, val);
  return abce_murmurctx_get(&ctx);
}

static inline uint32_t abce_murmur_buf(uint32_t seed, const void *buf, size_t sz)
{
  struct abce_murmurctx ctx = MURMURCTX_INITER(seed);
  abce_murmurctx_feed_buf(&ctx, buf, sz);
  return abce_murmurctx_get(&ctx);
}

#ifdef __cplusplus
};
#endif

#endif
