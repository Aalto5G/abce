#ifndef _LOCVARCTX_H_
#define _LOCVARCTX_H_

#include <stddef.h>
#include "rbtree.h"
#include "murmur.h"
#include "containerof.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// NB: must be a power of 2!!!
#define ABCE_DEFAULT_LOCVARCTX_SIZE 8

struct abce_locvar {
  struct rb_tree_node node;
  size_t idx;
  char name[0]; // Can be '\0'-terminated since charset limited
};

struct abce_locvarctx {
  struct abce_locvarctx *parent;
  size_t startidx;
  size_t sz;
  size_t capacity;
  size_t jmpaddr_break; // NB: remember to push false before jumping!
  size_t jmpaddr_continue;
  struct rb_tree_nocmp heads[0];
};

static inline size_t abce_locvarctx_sz(struct abce_locvarctx *ctx)
{
  return ctx->sz;
}

static inline size_t abce_locvarctx_recursive_sz(struct abce_locvarctx *ctx)
{
  size_t result = ctx->sz + ctx->startidx;
  while (ctx->parent != NULL)
  {
    ctx = ctx->parent;
  }
  result -= ctx->startidx; // argument and register cunt
  return result;
}

static inline int abce_locvarctx_is_loop(struct abce_locvarctx *ctx)
{
  return ctx->jmpaddr_break != (size_t)-1;
}

struct abce_locvarctx *abce_locvarctx_breakcontinue(struct abce_locvarctx *ctx, size_t levels);

static inline int64_t abce_locvarctx_break(struct abce_locvarctx *ctx, size_t levels)
{
  ctx = abce_locvarctx_breakcontinue(ctx, levels);
  if (ctx == NULL)
  {
    return -ENOENT;
  }
  return ctx->jmpaddr_break;
}

static inline int64_t abce_locvarctx_continue(struct abce_locvarctx *ctx, size_t levels)
{
  ctx = abce_locvarctx_breakcontinue(ctx, levels);
  if (ctx == NULL)
  {
    return -ENOENT;
  }
  return ctx->jmpaddr_continue;
}

struct abce_locvarctx *abce_locvarctx_alloc(struct abce_locvarctx *parent,
                                            size_t init_startidx,
                                            size_t jmpaddr_break,
                                            size_t jmpaddr_continue);

void abce_locvarctx_free(struct abce_locvarctx *ctx);

int64_t abce_locvarctx_search_rec(struct abce_locvarctx *ctx, const char *name);

int abce_locvarctx_add(struct abce_locvarctx *ctx, const char *name);

#endif
