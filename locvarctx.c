#include "locvarctx.h"

struct abce_locvarctx *abce_locvarctx_breakcontinue(struct abce_locvarctx *ctx, size_t levels)
{
  if (levels == 0 || ctx == NULL)
  {
    abort();
  }
  while (ctx != NULL)
  {
    if (abce_locvarctx_is_loop(ctx))
    {
      levels--;
    }
    if (levels == 0)
    {
      break;
    }
    ctx = ctx->parent;
  }
  if (ctx == NULL || !abce_locvarctx_is_loop(ctx))
  {
    return NULL;
  }
  return ctx;
}

struct abce_locvarctx *abce_locvarctx_alloc(struct abce_locvarctx *parent,
                                            size_t init_startidx,
                                            size_t jmpaddr_break,
                                            size_t jmpaddr_continue)
{
  struct abce_locvarctx *ctx0 =
    malloc(sizeof(*ctx0) + ABCE_DEFAULT_LOCVARCTX_SIZE*sizeof(*ctx0->heads));
  size_t i;
  if ((jmpaddr_break == (size_t)-1) != (jmpaddr_continue == (size_t)-1))
  {
    abort();
  }
  if (parent)
  {
    if (init_startidx != 0)
    {
      abort();
    }
  }
  ctx0->jmpaddr_break = jmpaddr_break;
  ctx0->jmpaddr_continue = jmpaddr_continue;
  ctx0->parent = parent;
  ctx0->sz = 0;
  ctx0->args = 0;
  ctx0->capacity = ABCE_DEFAULT_LOCVARCTX_SIZE;
  ctx0->startidx = init_startidx;
  if (parent)
  {
    ctx0->startidx = parent->startidx + parent->sz;
  }
  for (i = 0; i < ctx0->capacity; i++)
  {
    abce_rb_tree_nocmp_init(&ctx0->heads[i]);
  }
  return ctx0;
}

void abce_locvarctx_free(struct abce_locvarctx *ctx)
{
  size_t i;
  for (i = 0; i < ctx->capacity; i++)
  {
    while (ctx->heads[i].root != NULL)
    {
      struct abce_locvar *locvar =
        CONTAINER_OF(ctx->heads[i].root, struct abce_locvar, node);
      abce_rb_tree_nocmp_delete(&ctx->heads[i], ctx->heads[i].root);
      free(locvar);
    }
  }
  free(ctx);
}

static inline uint32_t abce_str_hash(const char *str)
{
  size_t len = strlen(str);
  return murmur_buf(0x12345678U, str, len);
}

static inline int abce_locvar_str_cmp_asym(const char *str, struct abce_rb_tree_node *n2, void *ud)
{
  struct abce_locvar *e = CONTAINER_OF(n2, struct abce_locvar, node);
  size_t len1 = strlen(str);
  size_t len2, lenmin;
  int ret;
  char *str2;
  len2 = strlen(e->name);
  str2 = e->name;
  lenmin = (len1 < len2) ? len1 : len2;
  ret = memcmp(str, str2, lenmin);
  if (ret != 0)
  {
    return ret;
  }
  if (len1 > len2)
  {
    return 1;
  }
  if (len1 < len2)
  {
    return -1;
  }
  return 0;
}

static inline int abce_locvar_str_cmp_sym(
  struct abce_rb_tree_node *n1, struct abce_rb_tree_node *n2, void *ud)
{
  struct abce_locvar *e1 = CONTAINER_OF(n1, struct abce_locvar, node);
  struct abce_locvar *e2 = CONTAINER_OF(n2, struct abce_locvar, node);
  size_t len1, len2, lenmin;
  int ret;
  char *str1, *str2;
  str1 = e1->name;
  len1 = strlen(str1);
  str2 = e2->name;
  len2 = strlen(str2);
  lenmin = (len1 < len2) ? len1 : len2;
  ret = memcmp(str1, str2, lenmin);
  if (ret != 0)
  {
    return ret;
  }
  if (len1 > len2)
  {
    return 1;
  }
  if (len1 < len2)
  {
    return -1;
  }
  return 0;
}

int64_t abce_locvarctx_search_rec(struct abce_locvarctx *ctx, const char *name)
{
  uint32_t hashval;
  size_t hashloc;
  struct abce_rb_tree_node *n;
  hashval = abce_str_hash(name);
  hashloc = hashval & (ctx->capacity - 1);
  n = ABCE_RB_TREE_NOCMP_FIND(&ctx->heads[hashloc], abce_locvar_str_cmp_asym, NULL, name);
  if (n != NULL)
  {
    return CONTAINER_OF(n, struct abce_locvar, node)->idx;
  }
  if (ctx->parent == NULL)
  {
    return -ENOENT;
  }
  return abce_locvarctx_search_rec(ctx->parent, name);
}

int abce_locvarctx_add_param(struct abce_locvarctx *ctx, const char *name)
{
  int64_t loc;
  size_t namelen = strlen(name);
  struct abce_locvar *locvar;
  uint32_t hashval;
  size_t hashloc;
  if (ctx->parent != NULL)
  {
    abort();
  }
  hashval = abce_str_hash(name);
  hashloc = hashval & (ctx->capacity - 1);
  loc = abce_locvarctx_search_rec(ctx, name);
  if (loc >= 0)
  {
    return -EEXIST;
  }
  locvar = malloc(sizeof(*locvar) + namelen + 1);
  if (locvar == NULL)
  {
    return -ENOMEM;
  }
  locvar->idx = ctx->sz++;
  ctx->args++;
  memcpy(locvar->name, name, namelen + 1);
  if (abce_rb_tree_nocmp_insert_nonexist(&ctx->heads[hashloc], abce_locvar_str_cmp_sym, NULL, &locvar->node) != 0)
  {
    abort();
  }
  return 0;
}

int abce_locvarctx_add(struct abce_locvarctx *ctx, const char *name)
{
  int64_t loc;
  size_t namelen = strlen(name);
  struct abce_locvar *locvar;
  uint32_t hashval;
  size_t hashloc;
  hashval = abce_str_hash(name);
  hashloc = hashval & (ctx->capacity - 1);
  loc = abce_locvarctx_search_rec(ctx, name);
  if (loc >= 0)
  {
    return -EEXIST;
  }
  locvar = malloc(sizeof(*locvar) + namelen + 1);
  if (locvar == NULL)
  {
    return -ENOMEM;
  }
  locvar->idx = ctx->startidx + ctx->sz++;
  memcpy(locvar->name, name, namelen + 1);
  if (abce_rb_tree_nocmp_insert_nonexist(&ctx->heads[hashloc], abce_locvar_str_cmp_sym, NULL, &locvar->node) != 0)
  {
    abort();
  }
  return 0;
}
