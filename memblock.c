#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include "rbtree.h"
#include "murmur.h"
#include "containerof.h"
#include "likely.h"
#include "abceopcodes.h"
#include "datatypes.h"

void *abce_std_alloc(void *old, size_t newsz, void *alloc_baton)
{
  if (old == NULL)
  {
    return malloc(newsz == 0 ? 1 : newsz);
  }
  else if (newsz == 0)
  {
    free(old);
    return NULL;
  }
  else
  {
    return realloc(old, newsz);
  }
}

void abce_mb_arearefdn(struct abce *abce, struct abce_mb_area **mba, enum abce_type typ);

int64_t abce_cache_add_str(struct abce *abce, const char *str, size_t len)
{
  struct abce_mb mb;
  uint32_t hashval, hashloc;
  struct abce_const_str_len key = {.str = str, .len = len};
  struct rb_tree_node *n;

  hashval = abce_str_len_hash(&key);
  hashloc = hashval % (sizeof(abce->strcache)/sizeof(*abce->strcache));
  n = RB_TREE_NOCMP_FIND(&abce->strcache[hashloc], abce_str_cache_cmp_asymlen, NULL, &key);
  if (n != NULL)
  {
    return CONTAINER_OF(n, struct abce_mb_string, node)->locidx;
  }
  if (abce->cachesz >= abce->cachecap)
  {
    return -EOVERFLOW;
  }
  mb = abce_mb_create_string(abce, str, len);
  if (mb.typ == ABCE_T_N)
  {
    return -ENOMEM;
  }
  mb.u.area->u.str.locidx = abce->cachesz;
  abce->cachebase[abce->cachesz++] = mb;
  if (rb_tree_nocmp_insert_nonexist(&abce->strcache[hashloc], abce_str_cache_cmp_sym, NULL, &mb.u.area->u.str.node) != 0)
  {
    abort();
  }
  return mb.u.area->u.str.locidx;
}

struct abce_mb abce_mb_create_scope(struct abce *abce, size_t capacity,
                                      const struct abce_mb *parent, int holey)
{
  struct abce_mb_area *mba;
  struct abce_mb mb = {};
  size_t i;

  capacity = abce_next_highest_power_of_2(capacity);

  mba = abce->alloc(NULL, sizeof(*mba) + capacity * sizeof(*mba->u.sc.heads),
                    abce->alloc_baton);
  if (mba == NULL)
  {
    mb.typ = ABCE_T_N;
    return mb;
  }
  mba->u.sc.size = capacity;
  mba->u.sc.holey = holey;
  if (parent)
  {
    if (parent->typ == ABCE_T_N)
    {
      mba->u.sc.parent = NULL;
    }
    else if (parent->typ != ABCE_T_SC)
    {
      abort();
    }
    else
    {
      mba->u.sc.parent = abce_mb_arearefup(abce, parent);
    }
  }
  else
  {
    mba->u.sc.parent = NULL;
  }
  for (i = 0; i < capacity; i++)
  {
    rb_tree_nocmp_init(&mba->u.sc.heads[i]);
  }
  mba->refcnt = 1;
  mb.typ = ABCE_T_SC;
  mb.u.area = mba;
  return mb;
}

void abce_mb_arearefdn(struct abce *abce, struct abce_mb_area **mbap, enum abce_type typ)
{
  size_t i;
  struct abce_mb_area *mba = *mbap;
  if (mba == NULL)
  {
    return;
  }
  switch (typ)
  {
    case ABCE_T_T:
      if (!--mba->refcnt)
      {
        while (mba->u.tree.tree.root != NULL)
        {
          struct abce_mb_rb_entry *mbe =
            CONTAINER_OF(mba->u.tree.tree.root,
                         struct abce_mb_rb_entry, n);
          abce_mb_refdn(abce, &mbe->key);
          abce_mb_refdn(abce, &mbe->val);
          rb_tree_nocmp_delete(&mba->u.tree.tree,
                               mba->u.tree.tree.root);
          abce->alloc(mbe, 0, abce->alloc_baton);
        }
        abce->alloc(mba, 0, abce->alloc_baton);
      }
      break;
    case ABCE_T_IOS:
      if (!--mba->refcnt)
      {
        fclose(mba->u.ios.f);
        abce->alloc(mba, 0, abce->alloc_baton);
      }
      break;
    case ABCE_T_A:
      if (!--mba->refcnt)
      {
        for (i = 0; i < mba->u.ar.size; i++)
        {
          abce_mb_refdn(abce, &mba->u.ar.mbs[i]);
        }
        abce->alloc(mba->u.ar.mbs, 0, abce->alloc_baton);
        abce->alloc(mba, 0, abce->alloc_baton);
      }
      break;
    case ABCE_T_S:
      if (!--mba->refcnt)
      {
        abce->alloc(mba, 0, abce->alloc_baton);
      }
      break;
    case ABCE_T_SC:
      abce_mb_arearefdn(abce, &mba->u.sc.parent, ABCE_T_SC);
      if (!--mba->refcnt)
      {
        for (i = 0; i < mba->u.sc.size; i++)
        {
          while (mba->u.sc.heads[i].root != NULL)
          {
            struct abce_mb_rb_entry *mbe =
              CONTAINER_OF(mba->u.sc.heads[i].root,
                           struct abce_mb_rb_entry, n);
            abce_mb_refdn(abce, &mbe->key);
            abce_mb_refdn(abce, &mbe->val);
            rb_tree_nocmp_delete(&mba->u.sc.heads[i],
                                 mba->u.sc.heads[i].root);
            abce->alloc(mbe, 0, abce->alloc_baton);
          }
        }
        abce->alloc(mba, 0, abce->alloc_baton);
      }
      break;
    default:
      break;
  }
  *mbap = NULL;
}

void abce_mb_dump_impl(const struct abce_mb *mb);

void abce_mb_treedump(const struct rb_tree_node *n, int *first)
{
  struct abce_mb_rb_entry *e = CONTAINER_OF(n, struct abce_mb_rb_entry, n);
  if (n == NULL)
  {
    return;
  }
  if (*first)
  {
    *first = 0;
  }
  else
  {
    printf(", ");
  }
  abce_mb_treedump(n->left, first);
  abce_mb_dump_impl(&e->key);
  printf(": ");
  abce_mb_dump_impl(&e->val);
  abce_mb_treedump(n->right, first);
}

void abce_dump_str(const char *str, size_t sz)
{
  size_t i;
  printf("\"");
  for (i = 0; i < sz; i++)
  {
    unsigned char uch = (unsigned char)str[i];
    if (uch == '\n')
    {
      printf("\\n");
    }
    else if (uch == '\r')
    {
      printf("\\r");
    }
    else if (uch == '\t')
    {
      printf("\\t");
    }
    else if (uch == '\b')
    {
      printf("\\b");
    }
    else if (uch == '\f')
    {
      printf("\\f");
    }
    else if (uch == '\\')
    {
      printf("\\\\");
    }
    else if (uch == '"')
    {
      printf("\"");
    }
    else if (uch < 0x20)
    {
      printf("\\u%.4X", uch);
    }
    else
    {
      putchar(uch);
    }
  }
  printf("\"");
}

void abce_mb_dump_impl(const struct abce_mb *mb)
{
  size_t i;
  int first = 1;
  switch (mb->typ)
  {
    case ABCE_T_PB:
      printf("pb");
      break;
    case ABCE_T_N:
      printf("null");
      break;
    case ABCE_T_D:
      printf("%.20g", mb->u.d);
      break;
    case ABCE_T_B:
      printf("%s", mb->u.d ? "true" : "false");
      break;
    case ABCE_T_F:
      printf("fun(%lld)", (long long)mb->u.d);
      break;
    case ABCE_T_BP:
      printf("bp(%lld)", (long long)mb->u.d);
      break;
    case ABCE_T_IP:
      printf("ip(%lld)", (long long)mb->u.d);
      break;
    case ABCE_T_LP:
      printf("lp(%lld)", (long long)mb->u.d);
      break;
    case ABCE_T_IOS:
      printf("ios(%p)", mb->u.area);
      break;
    case ABCE_T_A:
      printf("[");
      for (i = 0; i < mb->u.area->u.ar.size; i++)
      {
        if (i != 0)
        {
          printf(", ");
        }
        abce_mb_dump_impl(&mb->u.area->u.ar.mbs[i]);
      }
      printf("]");
      break;
    case ABCE_T_SC:
      printf("sc(%zu){", mb->u.area->u.sc.size);
      for (i = 0; i < mb->u.area->u.sc.size; i++)
      {
        abce_mb_treedump(mb->u.area->u.sc.heads[i].root, &first);
      }
      printf("}");
      break;
    case ABCE_T_T:
      printf("{");
      abce_mb_treedump(mb->u.area->u.tree.tree.root, &first);
      printf("}");
      break;
    case ABCE_T_S:
      abce_dump_str(mb->u.area->u.str.buf, mb->u.area->u.str.size);
      break;
  }
}

struct abce_mb *abce_alloc_stack(size_t limit)
{
  return mmap(NULL, abce_topages(limit * sizeof(struct abce_mb)), PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
}

void abce_free_stack(struct abce_mb *stackbase, size_t limit)
{
  if (munmap(stackbase, abce_topages(limit * sizeof(struct abce_mb))) != 0)
  {
    abort();
  }
}

unsigned char *abce_alloc_bcode(size_t limit)
{
  return mmap(NULL, abce_topages(limit), PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
}

void abce_free_bcode(unsigned char *bcodebase, size_t limit)
{
  if (munmap(bcodebase, abce_topages(limit)) != 0)
  {
    abort();
  }
}

void abce_init(struct abce *abce)
{
  memset(abce, 0, sizeof(*abce));
  abce->alloc = abce_std_alloc;
  abce->trap = NULL;
  abce->alloc_baton = NULL;
  abce->userdata = NULL;
  abce->stacklimit = 1024*1024;
  abce->stackbase = abce_alloc_stack(abce->stacklimit);
  abce->sp = 0;
  abce->bp = 0;
  abce->ip = 0;
  abce->bytecodecap = 32*1024*1024;
  abce->bytecode = abce_alloc_bcode(abce->bytecodecap);
  abce->bytecodesz = 0;

  abce->cachecap = 1024*1024;
  abce->cachebase = abce_alloc_stack(abce->cachecap);
  abce->cachesz = 0;

  abce->dynscope = abce_mb_create_scope_noparent(abce, ABCE_DEFAULT_SCOPE_SIZE);
  if (abce->dynscope.typ == ABCE_T_N)
  {
    abort();
  }
}

void abce_free(struct abce *abce)
{
  size_t i;
  abce_mb_refdn(abce, &abce->dynscope);
  for (i = 0; i < sizeof(abce->strcache)/sizeof(*abce->strcache); i++)
  {
    while (abce->strcache[i].root != NULL)
    {
      struct abce_mb_string *mbe =
        CONTAINER_OF(abce->strcache[i].root,
                     struct abce_mb_string, node);
      struct abce_mb_area *area = CONTAINER_OF(mbe, struct abce_mb_area, u.str);
      rb_tree_nocmp_delete(&abce->strcache[i],
                           abce->strcache[i].root);
      abce_mb_arearefdn(abce, &area, ABCE_T_S);
    }
  }
  abce_free_stack(abce->stackbase, abce->stacklimit);
  abce->stackbase = NULL;
  abce->stacklimit = 0;
  abce_free_bcode(abce->bytecode, abce->bytecodecap);
  abce->bytecode = NULL;
  abce->bytecodecap = 0;
  abce_free_stack(abce->cachebase, abce->cachecap);
  abce->cachebase = NULL;
  abce->cachecap = 0;
}
