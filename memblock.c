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
#include "abce.h"

void *abce_std_alloc(void *old, size_t oldsz, size_t newsz, struct abce *abce)
{
  void *result;
  if (old == NULL)
  {
    if (newsz == 0)
    {
      newsz = 1;
    }
    if (oldsz != 0)
    {
      abort();
    }
    if ((newsz > abce->bytes_cap) ||
        (abce->bytes_alloced > (abce->bytes_cap - newsz)))
    {
      return NULL;
    }
    result = malloc(newsz);
    if (result)
    {
      abce->bytes_alloced += newsz;
    }
    return result;
  }
  else if (newsz == 0)
  {
    if (oldsz > abce->bytes_alloced)
    {
      abort();
    }
    free(old);
    abce->bytes_alloced -= oldsz;
    return NULL;
  }
  else
  {
    if (oldsz > abce->bytes_alloced)
    {
      abort();
    }
    if ((newsz > abce->bytes_cap) ||
        ((abce->bytes_alloced - oldsz) > (abce->bytes_cap - newsz)))
    {
      return NULL;
    }
    result = realloc(old, newsz);
    if (result)
    {
      abce->bytes_alloced -= oldsz;
      abce->bytes_alloced += newsz;
    }
    return result;
  }
}

void abce_mb_do_arearefdn(struct abce *abce, struct abce_mb_area **mba, enum abce_type typ);

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

  mba = abce->alloc(NULL, 0, sizeof(*mba) + capacity * sizeof(*mba->u.sc.heads),
                    abce);
  if (mba == NULL)
  {
    abce->err.code = ABCE_E_NO_MEM;
    abce->err.val2 = sizeof(*mba) + capacity * sizeof(*mba->u.sc.heads);
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

  // Add it to cache
  mb.u.area->u.sc.locidx = abce->cachesz;
  abce->cachebase[abce->cachesz++] = abce_mb_refup(abce, &mb);

  return mb;
}

void abce_mb_do_arearefdn(struct abce *abce, struct abce_mb_area **mbap, enum abce_type typ)
{
  size_t i;
  struct abce_mb_area *mba = *mbap;
  if (mba == NULL)
  {
    return;
  }
  if (mba->refcnt != 0)
  {
    abort();
  }
  switch (typ)
  {
    case ABCE_T_T:
      if (1)
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
          abce->alloc(mbe, sizeof(*mbe), 0, abce);
        }
        abce->alloc(mba, sizeof(*mba), 0, abce);
      }
      break;
    case ABCE_T_IOS:
      if (1)
      {
        fclose(mba->u.ios.f);
        abce->alloc(mba, sizeof(*mba), 0, abce);
      }
      break;
    case ABCE_T_A:
      if (1)
      {
        for (i = 0; i < mba->u.ar.size; i++)
        {
          abce_mb_refdn(abce, &mba->u.ar.mbs[i]);
        }
        abce->alloc(mba->u.ar.mbs, mba->u.ar.capacity*sizeof(*mba->u.ar.mbs), 0, abce);
        abce->alloc(mba, sizeof(*mba), 0, abce);
      }
      break;
    case ABCE_T_S:
      if (1)
      {
        abce->alloc(mba, sizeof(*mba) + mba->u.str.size + 1, 0, abce);
      }
      break;
    case ABCE_T_PB:
      if (1)
      {
        abce->alloc(mba->u.pb.buf, mba->u.pb.capacity, 0, abce);
        abce->alloc(mba, sizeof(*mba), 0, abce);
      }
      break;
    case ABCE_T_SC:
      if (1)
      {
        abce_mb_arearefdn(abce, &mba->u.sc.parent, ABCE_T_SC);
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
            abce->alloc(mbe, sizeof(*mbe), 0, abce);
          }
        }
        abce->alloc(mba, sizeof(*mba) + mba->u.sc.size * sizeof(*mba->u.sc.heads), 0, abce);
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
