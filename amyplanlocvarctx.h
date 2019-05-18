#ifndef _AMYPLAN_LOCVARCTX_H_
#define _AMYPLAN_LOCVARCTX_H_

#include <stddef.h>
#include "abcerbtree.h"
#include "abcemurmur.h"
#include "abcecontainerof.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// NB: must be a power of 2!!!
#define ABCE_DEFAULT_LOCVARCTX_SIZE 8

struct amyplan_locvar {
  struct abce_rb_tree_node node;
  size_t idx;
  char name[0]; // Can be '\0'-terminated since charset limited
};

struct amyplan_locvarctx {
  struct amyplan_locvarctx *parent;
  size_t startidx;
  size_t sz;
  size_t args;
  size_t capacity;
  size_t jmpaddr_break; // NB: remember to push false before jumping!
  size_t jmpaddr_continue;
  struct abce_rb_tree_nocmp heads[0];
};

static inline size_t amyplan_locvarctx_sz(struct amyplan_locvarctx *ctx)
{
  return ctx->sz;
}

static inline size_t amyplan_locvarctx_arg_sz(struct amyplan_locvarctx *ctx)
{
  while (ctx->parent != NULL)
  {
    ctx = ctx->parent;
  }
  return ctx->args; // argument count
}

static inline size_t amyplan_locvarctx_recursive_sz(struct amyplan_locvarctx *ctx)
{
  size_t result = ctx->sz + ctx->startidx;
  while (ctx->parent != NULL)
  {
    ctx = ctx->parent;
  }
  result -= ctx->startidx; // register count
  result -= ctx->args; // argument count
  return result;
}

static inline int amyplan_locvarctx_is_loop(struct amyplan_locvarctx *ctx)
{
  return ctx->jmpaddr_break != (size_t)-1;
}

struct amyplan_locvarctx *amyplan_locvarctx_breakcontinue(struct amyplan_locvarctx *ctx, size_t levels);

static inline int64_t amyplan_locvarctx_break(struct amyplan_locvarctx *ctx, size_t levels)
{
  ctx = amyplan_locvarctx_breakcontinue(ctx, levels);
  if (ctx == NULL)
  {
    return -ENOENT;
  }
  return ctx->jmpaddr_break;
}

static inline int64_t amyplan_locvarctx_continue(struct amyplan_locvarctx *ctx, size_t levels)
{
  ctx = amyplan_locvarctx_breakcontinue(ctx, levels);
  if (ctx == NULL)
  {
    return -ENOENT;
  }
  return ctx->jmpaddr_continue;
}

struct amyplan_locvarctx *amyplan_locvarctx_alloc(struct amyplan_locvarctx *parent,
                                            size_t init_startidx,
                                            size_t jmpaddr_break,
                                            size_t jmpaddr_continue);

void amyplan_locvarctx_free(struct amyplan_locvarctx *ctx);

int64_t amyplan_locvarctx_search_rec(struct amyplan_locvarctx *ctx, const char *name);

int amyplan_locvarctx_add(struct amyplan_locvarctx *ctx, const char *name);

int amyplan_locvarctx_add_param(struct amyplan_locvarctx *ctx, const char *name);

#endif
