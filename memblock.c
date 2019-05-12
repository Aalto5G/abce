#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include "rbtree.h"
#include "murmur.h"
#include "containerof.h"
#include "likely.h"
#include "abceopcodes.h"

void *std_alloc(void *old, size_t newsz, void *alloc_baton)
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

struct abce {
  void *(*alloc)(void *old, size_t newsz, void *alloc_baton);
  void *alloc_baton;
  void *userdata;
  struct memblock *stackbase;
  size_t sp;
  size_t bp;
  int64_t ip;
  size_t stacklimit;
  unsigned char *bytecode;
  size_t bytecodesz;
  size_t bytecodecap;
};

struct memblock;

struct memblock_tree {
  struct rb_tree_nocmp tree;
};
struct memblock_scope {
  int holey;
  struct memblockarea *parent;
  size_t size;
  struct rb_tree_nocmp heads[0];
};
struct memblock_array {
  struct memblock *mbs;
  size_t capacity;
  size_t size;
};
struct memblock_string {
  size_t size;
  char buf[0];
};
struct memblock_ios {
  FILE *f;
};
struct memblockarea {
  size_t refcnt;
  union {
    struct memblock_array ar;
    struct memblock_ios ios;
    struct memblock_scope sc;
    struct memblock_tree tree;
    struct memblock_string str;
  } u;
};
enum type {
  T_T,
  T_D,
  T_B,
  T_F,
  T_S,
  T_PB, // packet buffer
  T_IOS,
  T_BP,
  T_IP,
  T_LP,
  T_A,
  T_SC,
  T_N,
};
struct memblock {
  enum type typ;
  union {
    double d;
    struct memblockarea *area;
  } u;
};
struct memblock_rb_entry {
  struct rb_tree_node n;
  struct memblock key; // must be a string!
  struct memblock val;
};

static inline int abce_add_double(struct abce *abce, double dbl)
{
  if (abce->bytecodesz + 8 > abce->bytecodecap)
  {
    return -EFAULT;
  }
  memcpy(&abce->bytecode[abce->bytecodesz], &dbl, 8);
  abce->bytecodesz += 8;
  return 0;
}

void memblock_refdn(struct abce *abce, struct memblock *mb);

static inline struct memblock
memblock_refup(struct abce *abce, const struct memblock *mb)
{
  switch (mb->typ)
  {
    case T_T: case T_S: case T_IOS: case T_A: case T_SC:
      mb->u.area->refcnt++;
      break;
    default:
      break;
  }
  return *mb;
}


static inline int abce_pop(struct abce *abce)
{
  struct memblock *mb;
  if (abce->sp == 0 || abce->sp <= abce->bp)
  {
    return -EOVERFLOW;
  }
  mb = &abce->stackbase[--abce->sp];
  memblock_refdn(abce, mb);
  return 0;
}

static inline int abce_calc_addr(size_t *paddr, struct abce *abce, int64_t idx)
{
  size_t addr;
  if (idx < 0)
  {
    addr = abce->sp + idx;
    if (addr >= abce->sp || addr < abce->bp)
    {
      return -EOVERFLOW;
    }
  }
  else
  {
    addr = abce->bp + idx;
    if (addr >= abce->sp || addr < abce->bp)
    {
      return -EOVERFLOW;
    }
  }
  *paddr = addr;
  return 0;
}

static inline int abce_getboolean(int *b, struct abce *abce, int64_t idx)
{
  const struct memblock *mb;
  size_t addr;
  if (abce_calc_addr(&addr, abce, idx) != 0)
  {
    return -EOVERFLOW;
  }
  mb = &abce->stackbase[addr];
  if (mb->typ != T_D || mb->typ != T_B)
  {
    return -EINVAL;
  }
  *b = !!mb->u.d;
  return 0;
}

static inline int abce_getfunaddr(int64_t *paddr, struct abce *abce, int64_t idx)
{
  const struct memblock *mb;
  size_t addr;
  if (abce_calc_addr(&addr, abce, idx) != 0)
  {
    return -EOVERFLOW;
  }
  mb = &abce->stackbase[addr];
  if (mb->typ != T_F)
  {
    return -EINVAL;
  }
  *paddr = mb->u.d;
  return 0;
}

static inline int abce_getbp(struct abce *abce, int64_t idx)
{
  const struct memblock *mb;
  size_t addr;
  size_t trial;
  if (abce_calc_addr(&addr, abce, idx) != 0)
  {
    return -EOVERFLOW;
  }
  mb = &abce->stackbase[addr];
  if (mb->typ != T_D || mb->typ != T_B)
  {
    return -EINVAL;
  }
  trial = mb->u.d;
  if (trial != mb->u.d)
  {
    return -EINVAL;
  }
  abce->bp = trial;
  return 0;
}

static inline int abce_getip(struct abce *abce, int64_t idx)
{
  const struct memblock *mb;
  size_t addr;
  size_t trial;
  if (abce_calc_addr(&addr, abce, idx) != 0)
  {
    return -EOVERFLOW;
  }
  mb = &abce->stackbase[addr];
  if (mb->typ != T_D || mb->typ != T_B)
  {
    return -EINVAL;
  }
  trial = mb->u.d;
  if (trial != mb->u.d)
  {
    return -EINVAL;
  }
  abce->ip = trial;
  return 0;
}

static inline int abce_getmb(struct memblock *mb, struct abce *abce, int64_t idx)
{
  const struct memblock *mbptr;
  size_t addr;
  if (abce_calc_addr(&addr, abce, idx) != 0)
  {
    return -EOVERFLOW;
  }
  mbptr = &abce->stackbase[addr];
  *mb = memblock_refup(abce, mbptr);
  return 0;
}

static inline int abce_verifyaddr(struct abce *abce, int64_t idx)
{
  size_t addr;
  if (abce_calc_addr(&addr, abce, idx) != 0)
  {
    return -EOVERFLOW;
  }
  return 0;
}

static inline int abce_getdbl(double *d, struct abce *abce, int64_t idx)
{
  const struct memblock *mb;
  size_t addr;
  if (abce_calc_addr(&addr, abce, idx) != 0)
  {
    return -EOVERFLOW;
  }
  mb = &abce->stackbase[addr];
  if (mb->typ != T_D || mb->typ != T_B)
  {
    return -EINVAL;
  }
  *d = mb->u.d;
  return 0;
}

static inline int abce_push_mb(struct abce *abce, const struct memblock *mb)
{
  if (abce->sp >= abce->stacklimit)
  {
    return -EOVERFLOW;
  }
  abce->stackbase[abce->sp] = memblock_refup(abce, mb);
  abce->sp++;
  return 0;
}

static inline int abce_push_boolean(struct abce *abce, int boolean)
{
  if (abce->sp >= abce->stacklimit)
  {
    return -EOVERFLOW;
  }
  abce->stackbase[abce->sp].typ = T_D;
  abce->stackbase[abce->sp].u.d = boolean ? 1.0 : 0.0;
  abce->sp++;
  return 0;
}

static inline int abce_push_nil(struct abce *abce)
{
  if (abce->sp >= abce->stacklimit)
  {
    return -EOVERFLOW;
  }
  abce->stackbase[abce->sp].typ = T_N;
  abce->sp++;
  return 0;
}

static inline int abce_push_ip(struct abce *abce)
{
  if (abce->sp >= abce->stacklimit)
  {
    return -EOVERFLOW;
  }
  abce->stackbase[abce->sp].typ = T_IP;
  abce->stackbase[abce->sp].u.d = abce->ip;
  abce->sp++;
  return 0;
}
static inline int abce_push_bp(struct abce *abce)
{
  if (abce->sp >= abce->stacklimit)
  {
    return -EOVERFLOW;
  }
  abce->stackbase[abce->sp].typ = T_BP;
  abce->stackbase[abce->sp].u.d = abce->bp;
  abce->sp++;
  return 0;
}
static inline int abce_push_double(struct abce *abce, double dbl)
{
  if (abce->sp >= abce->stacklimit)
  {
    return -EOVERFLOW;
  }
  abce->stackbase[abce->sp].typ = T_D;
  abce->stackbase[abce->sp].u.d = dbl;
  abce->sp++;
  return 0;
}
static inline int abce_push_fun(struct abce *abce, double fun_addr)
{
  if (abce->sp >= abce->stacklimit)
  {
    return -EOVERFLOW;
  }
  if ((double)(int64_t)fun_addr != fun_addr)
  {
    return -EINVAL;
  }
  abce->stackbase[abce->sp].typ = T_F;
  abce->stackbase[abce->sp].u.d = fun_addr;
  abce->sp++;
  return 0;
}

static inline int abce_add_byte(struct abce *abce, unsigned char byte)
{
  if (abce->bytecodesz >= abce->bytecodecap)
  {
    return -EFAULT;
  }
  abce->bytecode[abce->bytecodesz++] = byte;
  return 0;
}

static inline struct memblockarea*
memblock_arearefup(struct abce *abce, const struct memblock *mb)
{
  switch (mb->typ)
  {
    case T_T: case T_S: case T_IOS: case T_A: case T_SC:
      mb->u.area->refcnt++;
      return mb->u.area;
    default:
      abort();
  }
}

static inline struct memblock
memblock_create_string(struct abce *abce, const char *str, size_t sz)
{
  struct memblockarea *mba;
  struct memblock mb = {};
  mba = abce->alloc(NULL, sizeof(*mba) + sz + 1, abce->alloc_baton);
  mba->u.str.size = sz;
  memcpy(mba->u.str.buf, str, sz);
  mba->u.str.buf[sz] = '\0';
  mba->refcnt = 1;
  mb.typ = T_S;
  mb.u.area = mba;
  return mb;
}

static inline struct memblock
memblock_create_string_nul(struct abce *abce, const char *str)
{
  return memblock_create_string(abce, str, strlen(str));
}

static inline uint32_t str_hash(const char *str)
{
  size_t len = strlen(str);
  return murmur_buf(0x12345678U, str, len);
}
static inline uint32_t mb_str_hash(const struct memblock *mb)
{
  if (mb->typ != T_S)
  {
    abort();
  }
  return murmur_buf(0x12345678U, mb->u.area->u.str.buf, mb->u.area->u.str.size);
}
static inline int str_cmp_asym(const char *str, struct rb_tree_node *n2, void *ud)
{
  struct memblock_rb_entry *e = CONTAINER_OF(n2, struct memblock_rb_entry, n);
  size_t len1 = strlen(str);
  size_t len2, lenmin;
  int ret;
  char *str2;
  if (e->key.typ != T_S)
  {
    abort();
  }
  len2 = e->key.u.area->u.str.size;
  str2 = e->key.u.area->u.str.buf;
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

static inline int str_cmp_halfsym(
  const struct memblock *key, struct rb_tree_node *n2, void *ud)
{
  struct memblock_rb_entry *e2 = CONTAINER_OF(n2, struct memblock_rb_entry, n);
  size_t len1, len2, lenmin;
  int ret;
  char *str1, *str2;
  if (key->typ != T_S || e2->key.typ != T_S)
  {
    abort();
  }
  len1 = key->u.area->u.str.size;
  str1 = key->u.area->u.str.buf;
  len2 = e2->key.u.area->u.str.size;
  str2 = e2->key.u.area->u.str.buf;
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

static inline int str_cmp_sym(
  struct rb_tree_node *n1, struct rb_tree_node *n2, void *ud)
{
  struct memblock_rb_entry *e1 = CONTAINER_OF(n1, struct memblock_rb_entry, n);
  struct memblock_rb_entry *e2 = CONTAINER_OF(n2, struct memblock_rb_entry, n);
  size_t len1, len2, lenmin;
  int ret;
  char *str1, *str2;
  if (e1->key.typ != T_S || e2->key.typ != T_S)
  {
    abort();
  }
  len1 = e1->key.u.area->u.str.size;
  str1 = e2->key.u.area->u.str.buf;
  len2 = e2->key.u.area->u.str.size;
  str2 = e2->key.u.area->u.str.buf;
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

static inline const struct memblock *sc_get_myval_mb_area(
  const struct memblockarea *mba, const struct memblock *key)
{
  uint32_t hashval;
  size_t hashloc;
  struct rb_tree_node *n;
  if (key->typ != T_S)
  {
    abort();
  }
  hashval = mb_str_hash(key);
  hashloc = hashval & (mba->u.sc.size - 1);
  n = RB_TREE_NOCMP_FIND(&mba->u.sc.heads[hashloc], str_cmp_halfsym, NULL, key);
  if (n == NULL)
  {
    return NULL;
  }
  return &CONTAINER_OF(n, struct memblock_rb_entry, n)->val;
}

static inline const struct memblock *sc_get_myval_mb(
  const struct memblock *mb, const struct memblock *key)
{
  if (mb->typ != T_SC)
  {
    abort();
  }
  return sc_get_myval_mb_area(mb->u.area, key);
}

static inline const struct memblock *sc_get_myval_str_area(
  const struct memblockarea *mba, const char *str)
{
  uint32_t hashval;
  size_t hashloc;
  struct rb_tree_node *n;
  hashval = str_hash(str);
  hashloc = hashval & (mba->u.sc.size - 1);
  n = RB_TREE_NOCMP_FIND(&mba->u.sc.heads[hashloc], str_cmp_asym, NULL, str);
  if (n == NULL)
  {
    return NULL;
  }
  return &CONTAINER_OF(n, struct memblock_rb_entry, n)->val;
}

static inline const struct memblock *sc_get_myval_str(
  const struct memblock *mb, const char *str)
{
  if (mb->typ != T_SC)
  {
    abort();
  }
  return sc_get_myval_str_area(mb->u.area, str);
}

const struct memblock *sc_get_rec_mb_area(
  const struct memblockarea *mba, const struct memblock *it)
{
  const struct memblock *mb = sc_get_myval_mb_area(mba, it);
  if (mb != NULL)
  {
    return mb;
  }
  if (mba->u.sc.parent != NULL && !mba->u.sc.holey)
  {
    return sc_get_rec_mb_area(mba->u.sc.parent, it);
  }
  return NULL;
}

const struct memblock *
sc_get_rec_mb(const struct memblock *mb, const struct memblock *it)
{
  if (mb->typ != T_SC)
  {
    abort();
  }
  return sc_get_rec_mb_area(mb->u.area, it);
}

const struct memblock *sc_get_rec_str_area(
  const struct memblockarea *mba, const char *str)
{
  const struct memblock *mb = sc_get_myval_str_area(mba, str);
  if (mb != NULL)
  {
    return mb;
  }
  if (mba->u.sc.parent != NULL && !mba->u.sc.holey)
  {
    return sc_get_rec_str_area(mba->u.sc.parent, str);
  }
  return NULL;
}
const struct memblock *
sc_get_rec_str(const struct memblock *mb, const char *str)
{
  if (mb->typ != T_SC)
  {
    abort();
  }
  return sc_get_rec_str_area(mb->u.area, str);
}

static inline int sc_put_val_mb(
  struct abce *abce,
  const struct memblock *mb, const struct memblock *pkey, const struct memblock *pval)
{
  struct memblockarea *mba = mb->u.area;
  uint32_t hashval;
  struct memblock_rb_entry *e;
  size_t hashloc;
  int ret;
  if (mb->typ != T_SC || pkey->typ != T_S)
  {
    abort();
  }
  hashval = mb_str_hash(pkey);
  hashloc = hashval & (mba->u.sc.size - 1);
  e = abce->alloc(NULL, sizeof(*e), abce->alloc_baton);
  e->key = memblock_refup(abce, pkey);
  e->val = memblock_refup(abce, pval);
  ret = rb_tree_nocmp_insert_nonexist(&mba->u.sc.heads[hashloc],
                                      str_cmp_sym, NULL, &e->n);
  if (ret == 0)
  {
    return 0;
  }
  memblock_refdn(abce, &e->key);
  memblock_refdn(abce, &e->val);
  abce->alloc(e, 0, abce->alloc_baton);
  return ret;
}

static inline int sc_put_val_str(
  struct abce *abce,
  const struct memblock *mb, char *str, const struct memblock *pval)
{
  struct memblockarea *mba = mb->u.area;
  uint32_t hashval;
  struct memblock_rb_entry *e;
  size_t hashloc;
  int ret;
  if (mb->typ != T_SC)
  {
    abort();
  }
  hashval = str_hash(str);
  hashloc = hashval & (mba->u.sc.size - 1);
  e = abce->alloc(NULL, sizeof(*e), abce->alloc_baton);
  e->key = memblock_create_string(abce, str, strlen(str));
  e->val = memblock_refup(abce, pval);
  ret = rb_tree_nocmp_insert_nonexist(&mba->u.sc.heads[hashloc],
                                      str_cmp_sym, NULL, &e->n);
  if (ret == 0)
  {
    return 0;
  }
  memblock_refdn(abce, &e->key);
  memblock_refdn(abce, &e->val);
  abce->alloc(e, 0, abce->alloc_baton);
  return ret;
}


static inline size_t next_highest_power_of_2(size_t x)
{
  x--;
  x |= x >> 1;
  x |= x >> 2;
  x |= x >> 4;
  x |= x >> 8;
  x |= x >> 16;
#if SIZE_MAX > (4U*1024U*1024U*1024U)
  x |= x >> 32;
#endif
  x++;
  return x;
}

struct memblock memblock_create_scope(struct abce *abce, size_t capacity,
                                      const struct memblock *parent, int holey)
{
  struct memblockarea *mba;
  struct memblock mb = {};
  size_t i;

  capacity = next_highest_power_of_2(capacity);

  mba = abce->alloc(NULL, sizeof(*mba) + capacity * sizeof(*mba->u.sc.heads),
                    abce->alloc_baton);
  mba->u.sc.size = capacity;
  mba->u.sc.holey = holey;
  if (parent)
  {
    if (parent->typ == T_N)
    {
      mba->u.sc.parent = NULL;
    }
    else if (parent->typ != T_SC)
    {
      abort();
    }
    else
    {
      mba->u.sc.parent = memblock_arearefup(abce, parent);
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
  mb.typ = T_SC;
  mb.u.area = mba;
  return mb;
}

struct memblock memblock_create_scope_noparent(struct abce *abce, size_t capacity)
{
  return memblock_create_scope(abce, capacity, NULL, 0);
}

struct memblock memblock_create_tree(struct abce *abce)
{
  struct memblockarea *mba;
  struct memblock mb = {};
  mba = abce->alloc(NULL, sizeof(*mba), abce->alloc_baton);
  rb_tree_nocmp_init(&mba->u.tree.tree);
  mba->refcnt = 1;
  mb.typ = T_A;
  mb.u.area = mba;
  return mb;
}

struct memblock memblock_create_array(struct abce *abce)
{
  struct memblockarea *mba;
  struct memblock mb = {};
  mba = abce->alloc(NULL, sizeof(*mba), abce->alloc_baton);
  mba->u.ar.size = 0;
  mba->u.ar.capacity = 16;
  mba->u.ar.mbs =
    abce->alloc(NULL, 16*sizeof(*mba->u.ar.mbs), abce->alloc_baton);
  mba->refcnt = 1;
  mb.typ = T_A;
  mb.u.area = mba;
  return mb;
}

void memblock_arearefdn(struct abce *abce, struct memblockarea **mba, enum type typ);

void memblock_refdn(struct abce *abce, struct memblock *mb)
{
  switch (mb->typ)
  {
    case T_T:
    case T_IOS:
    case T_A:
    case T_S:
    case T_SC:
      memblock_arearefdn(abce, &mb->u.area, mb->typ);
      break;
    default:
      break;
  }
  mb->typ = T_N;
  mb->u.d = 0.0;
  mb->u.area = NULL;
}

void memblock_arearefdn(struct abce *abce, struct memblockarea **mbap, enum type typ)
{
  size_t i;
  struct memblockarea *mba = *mbap;
  if (mba == NULL)
  {
    return;
  }
  switch (typ)
  {
    case T_T:
      if (!--mba->refcnt)
      {
        while (mba->u.tree.tree.root != NULL)
        {
          struct memblock_rb_entry *mbe =
            CONTAINER_OF(mba->u.tree.tree.root,
                         struct memblock_rb_entry, n);
          memblock_refdn(abce, &mbe->key);
          memblock_refdn(abce, &mbe->val);
          rb_tree_nocmp_delete(&mba->u.tree.tree,
                               mba->u.tree.tree.root);
          abce->alloc(mbe, 0, abce->alloc_baton);
        }
        abce->alloc(mba, 0, abce->alloc_baton);
      }
      break;
    case T_IOS:
      if (!--mba->refcnt)
      {
        fclose(mba->u.ios.f);
        abce->alloc(mba, 0, abce->alloc_baton);
      }
      break;
    case T_A:
      if (!--mba->refcnt)
      {
        for (i = 0; i < mba->u.ar.size; i++)
        {
          memblock_refdn(abce, &mba->u.ar.mbs[i]);
        }
        abce->alloc(mba->u.ar.mbs, 0, abce->alloc_baton);
        abce->alloc(mba, 0, abce->alloc_baton);
      }
      break;
    case T_S:
      if (!--mba->refcnt)
      {
        abce->alloc(mba, 0, abce->alloc_baton);
      }
      break;
    case T_SC:
      memblock_arearefdn(abce, &mba->u.sc.parent, T_SC);
      if (!--mba->refcnt)
      {
        for (i = 0; i < mba->u.sc.size; i++)
        {
          while (mba->u.sc.heads[i].root != NULL)
          {
            struct memblock_rb_entry *mbe =
              CONTAINER_OF(mba->u.sc.heads[i].root,
                           struct memblock_rb_entry, n);
            memblock_refdn(abce, &mbe->key);
            memblock_refdn(abce, &mbe->val);
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

void memblock_dump_impl(const struct memblock *mb);

static inline void
memblock_array_pop_back(struct abce *abce,
                        struct memblock *mb, const struct memblock *it)
{
  if (mb->typ != T_A)
  {
    abort();
  }
  if (mb->u.area->u.ar.size <= 0)
  {
    abort();
  }
  memblock_refdn(abce, &mb->u.area->u.ar.mbs[--mb->u.area->u.ar.size]);
}

static inline void
memblock_array_append(struct abce *abce,
                      struct memblock *mb, const struct memblock *it)
{
  if (mb->typ != T_A)
  {
    abort();
  }
  if (mb->u.area->u.ar.size >= mb->u.area->u.ar.capacity)
  {
    size_t new_cap = 2*mb->u.area->u.ar.size + 1;
    struct memblock *mbs2;
    mbs2 = abce->alloc(mb->u.area->u.ar.mbs,
                       sizeof(*mb->u.area->u.ar.mbs)*new_cap,
                       abce->alloc_baton);
    mb->u.area->u.ar.capacity = new_cap;
    mb->u.area->u.ar.mbs = mbs2;
  }
  mb->u.area->u.ar.mbs[mb->u.area->u.ar.size++] = memblock_refup(abce, it);
}

void memblock_treedump(const struct rb_tree_node *n, int *first)
{
  struct memblock_rb_entry *e = CONTAINER_OF(n, struct memblock_rb_entry, n);
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
  memblock_treedump(n->left, first);
  memblock_dump_impl(&e->key);
  printf(": ");
  memblock_dump_impl(&e->val);
  memblock_treedump(n->right, first);
}

void dump_str(const char *str, size_t sz)
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

void memblock_dump_impl(const struct memblock *mb)
{
  size_t i;
  int first = 1;
  switch (mb->typ)
  {
    case T_PB:
      printf("pb");
      break;
    case T_N:
      printf("null");
      break;
    case T_D:
      printf("%.20g", mb->u.d);
      break;
    case T_B:
      printf("%s", mb->u.d ? "true" : "false");
      break;
    case T_F:
      printf("fun(%lld)", (long long)mb->u.d);
      break;
    case T_BP:
      printf("bp(%lld)", (long long)mb->u.d);
      break;
    case T_IP:
      printf("ip(%lld)", (long long)mb->u.d);
      break;
    case T_LP:
      printf("lp(%lld)", (long long)mb->u.d);
      break;
    case T_IOS:
      printf("ios(%p)", mb->u.area);
      break;
    case T_A:
      printf("[");
      for (i = 0; i < mb->u.area->u.ar.size; i++)
      {
        if (i != 0)
        {
          printf(", ");
        }
        memblock_dump_impl(&mb->u.area->u.ar.mbs[i]);
      }
      printf("]");
      break;
    case T_SC:
      printf("sc(%zu){", mb->u.area->u.sc.size);
      for (i = 0; i < mb->u.area->u.sc.size; i++)
      {
        memblock_treedump(mb->u.area->u.sc.heads[i].root, &first);
      }
      printf("}");
      break;
    case T_T:
      printf("{");
      memblock_treedump(mb->u.area->u.tree.tree.root, &first);
      printf("}");
      break;
    case T_S:
      dump_str(mb->u.area->u.str.buf, mb->u.area->u.str.size);
      break;
  }
}

void memblock_dump(const struct memblock *mb)
{
  memblock_dump_impl(mb);
  printf("\n");
}

static inline size_t topages(size_t limit)
{
  long pagesz = sysconf(_SC_PAGE_SIZE);
  size_t pages, actlimit;
  if (pagesz <= 0)
  {
    abort();
  }
  limit *= sizeof(struct memblock);
  pages = (limit + (pagesz-1)) / pagesz;
  actlimit = pages * pagesz;
  return actlimit;
}

struct memblock *alloc_stack(size_t limit)
{
  return mmap(NULL, topages(limit), PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
}

void free_stack(struct memblock *stackbase, size_t limit)
{
  if (munmap(stackbase, topages(limit)) != 0)
  {
    abort();
  }
}

void stacktest(struct abce *abce, struct memblock *stackbase, size_t limit)
{
  size_t sp = 0;
  size_t i;
  for (i = 0; i < 10000; i++)
  {
    stackbase[sp++] = memblock_create_array(abce);
  }

  while (sp > 0)
  {
    memblock_refdn(abce, &stackbase[--sp]);
  }
}

void stacktest_main(struct abce *abce)
{
  size_t limit = 1024*1024;
  struct memblock *stackbase = alloc_stack(limit);
  stacktest(abce, stackbase, limit);
  free_stack(stackbase, limit);
}

static inline int
fetch_b(uint8_t *b, struct abce *abce, unsigned char *addcode, size_t addsz)
{
  const size_t guard = 100;
  if (!((abce->ip >= 0 && abce->ip < abce->bytecodesz) ||
        (abce->ip >= -addsz-guard && abce->ip < -guard)))
  {
    return -EFAULT;
  }
  if (abce->ip >= 0)
  {
    *b = abce->bytecode[abce->ip++];
    return 0;
  }
  *b = addcode[abce->ip+guard+addsz];
  abce->ip++;
  return 0;
}

static inline int
fetch_d(double *d, struct abce *abce, unsigned char *addcode, size_t addsz)
{
  const size_t guard = 100;
  if (!((abce->ip >= 0 && abce->ip+8 <= abce->bytecodesz) ||
        (abce->ip >= -addsz-guard && abce->ip+8 <= -guard)))
  {
    return -EFAULT;
  }
  if (abce->ip >= 0)
  {
    memcpy(d, abce->bytecode + abce->ip, 8);
    abce->ip += 8;
    return 0;
  }
  memcpy(d, addcode + abce->ip + guard + addsz, 8);
  abce->ip += 8;
  return 0;
}

static inline int
fetch_i(uint16_t *ins, struct abce *abce, unsigned char *addcode, size_t addsz)
{
  uint8_t ophi, opmid, oplo;
  if (fetch_b(&ophi, abce, addcode, addsz) != 0)
  {
    return -EFAULT;
  }
  if (likely(ophi < 128))
  {
    *ins = ophi;
    return 0;
  }
  else if (unlikely((ophi & 0xC0) == 0x80))
  {
    return -EILSEQ;
  }
  else if (likely((ophi & 0xE0) == 0xC0))
  {
    if (fetch_b(&oplo, abce, addcode, addsz) != 0)
    {
      return -EFAULT;
    }
    if (unlikely((oplo & 0xC0) != 0x80))
    {
      return -EILSEQ;
    }
    *ins = ((ophi&0x1F) << 6) | (oplo & 0x3F);
    if (unlikely(*ins < 128))
    {
      return -EILSEQ;
    }
    return 0;
  }
  else if (likely((ophi & 0xF0) == 0xE0))
  {
    if (fetch_b(&opmid, abce, addcode, addsz) != 0)
    {
      return -EFAULT;
    }
    if (unlikely((opmid & 0xC0) != 0x80))
    {
      return -EILSEQ;
    }
    if (fetch_b(&oplo, abce, addcode, addsz) != 0)
    {
      return -EFAULT;
    }
    if (unlikely((oplo & 0xC0) != 0x80))
    {
      return -EILSEQ;
    }
    *ins = ((ophi&0xF) << 12) | ((opmid&0x3F) << 6) | (oplo & 0x3F);
    if (unlikely(*ins <= 0x7FF))
    {
      return -EILSEQ;
    }
    return 0;
  }
  else
  {
    return -EILSEQ;
  }
}

#define GETBOOLEAN(dbl, idx) \
  if(1) { \
    int _getdbl_rettmp = abce_getboolean((dbl), abce, (idx)); \
    if (_getdbl_rettmp != 0) \
    { \
      ret = _getdbl_rettmp; \
      break; \
    } \
  }
#define GETFUNADDR(dbl, idx) \
  if(1) { \
    int _getdbl_rettmp = abce_getfunaddr((dbl), abce, (idx)); \
    if (_getdbl_rettmp != 0) \
    { \
      ret = _getdbl_rettmp; \
      break; \
    } \
  }
#define VERIFYADDR(idx) \
  if(1) { \
    int _getdbl_rettmp = abce_verifyaddr(abce, (idx)); \
    if (_getdbl_rettmp != 0) \
    { \
      ret = _getdbl_rettmp; \
      break; \
    } \
  }
#define GETDBL(dbl, idx) \
  if(1) { \
    int _getdbl_rettmp = abce_getdbl((dbl), abce, (idx)); \
    if (_getdbl_rettmp != 0) \
    { \
      ret = _getdbl_rettmp; \
      break; \
    } \
  }
#define GETBP(idx) \
  if(1) { \
    int _getdbl_rettmp = abce_getbp(abce, (idx)); \
    if (_getdbl_rettmp != 0) \
    { \
      ret = _getdbl_rettmp; \
      break; \
    } \
  }
#define GETIP(idx) \
  if(1) { \
    int _getdbl_rettmp = abce_getip(abce, (idx)); \
    if (_getdbl_rettmp != 0) \
    { \
      ret = _getdbl_rettmp; \
      break; \
    } \
  }
#define GETMB(mb, idx) \
  if(1) { \
    int _getdbl_rettmp = abce_getmb((mb), abce, (idx)); \
    if (_getdbl_rettmp != 0) \
    { \
      ret = _getdbl_rettmp; \
      break; \
    } \
  }
#define POP(mb) \
  if(1) { \
    int _getdbl_rettmp = abce_pop(abce); \
    if (_getdbl_rettmp != 0) \
    { \
      abort(); \
    } \
  }

int engine(struct abce *abce, unsigned char *addcode, size_t addsz)
{
  // code:
  const size_t guard = 100;
  int ret;
  while (ret == -EAGAIN &&
         ((abce->ip >= 0 && abce->ip < abce->bytecodesz) ||
         (abce->ip >= -addsz-guard && abce->ip < -guard)))
  {
    uint16_t ins;
    if (fetch_i(&ins, abce, addcode, addsz) != 0)
    {
      ret = -EFAULT;
      break;
    }
    if (likely(ins < 64))
    {
      switch (ins)
      {
        case ABCE_OPCODE_NOP:
          break;
        case ABCE_OPCODE_PUSH_DBL:
        {
          double dbl;
          if (fetch_d(&dbl, abce, addcode, addsz) != 0)
          {
            ret = -EFAULT;
            break;
          }
          if (abce_push_double(abce, dbl) != 0)
          {
            ret = -EFAULT;
            break;
          }
          break;
        }
        case ABCE_OPCODE_PUSH_TRUE:
        {
          if (abce_push_boolean(abce, 1) != 0)
          {
            ret = -EFAULT;
            break;
          }
          break;
        }
        case ABCE_OPCODE_PUSH_FALSE:
        {
          if (abce_push_boolean(abce, 0) != 0)
          {
            ret = -EFAULT;
            break;
          }
          break;
        }
        case ABCE_OPCODE_PUSH_NIL:
        {
          if (abce_push_nil(abce) != 0)
          {
            ret = -EFAULT;
            break;
          }
          break;
        }
        case ABCE_OPCODE_FUNIFY:
        {
          double d;
          int rettmp;
          GETDBL(&d, -1);
          POP(abce);
          rettmp = abce_push_fun(abce, d);
          if (rettmp != 0)
          {
            ret = rettmp;
            break;
          }
          break;
        }
        case ABCE_OPCODE_CALL:
        {
          const size_t guard = 100;
          double argcnt;
          int64_t new_ip;
          uint16_t ins2;
          int rettmp;
          GETDBL(&argcnt, -1);
          GETFUNADDR(&new_ip, -2);
          POP(abce);
          POP(abce);
          // FIXME off by one?
          if (!((new_ip >= 0 && new_ip+10 <= abce->bytecodesz) ||
                (new_ip >= -addsz-guard && new_ip+10 <= -guard)))
          {
            return -EFAULT;
          }
          abce_push_bp(abce);
          abce_push_ip(abce);
          abce->ip = new_ip;
          rettmp = fetch_i(&ins2, abce, addcode, addsz);
          if (rettmp != 0)
          {
            return rettmp;
          }
          if (ins2 != ABCE_OPCODE_FUN_HEADER)
          {
            return -EINVAL;
          }
          double dbl;
          rettmp = fetch_d(&dbl, abce, addcode, addsz);
          if (rettmp != 0)
          {
            return rettmp;
          }
          if (dbl != (double)(uint64_t)argcnt)
          {
            return -EINVAL;
          }
          break;
        }
        case ABCE_OPCODE_RET:
        {
          struct memblock mb;
          GETIP(-2);
          GETBP(-3);
          GETMB(&mb, -1);
          POP(abce);
          POP(abce);
          POP(abce);
          if (abce_push_mb(abce, &mb) != 0)
          {
            abort();
          }
          memblock_refdn(abce, &mb);
          break;
        }
        /* stacktop - cntloc - cntargs - retval - locvar - ip - bp - args */
        case ABCE_OPCODE_RETEX2:
        {
          struct memblock mb;
          double cntloc, cntargs;
          size_t i;
          GETDBL(&cntloc, -1);
          GETDBL(&cntargs, -2);
          if (cntloc != (uint32_t)cntloc || cntargs != (uint32_t)cntargs)
          {
            ret = -EINVAL;
            break;
          }
          VERIFYADDR(-5 - cntloc - cntargs);
          GETMB(&mb, -3);
          POP(abce); // cntloc
          POP(abce); // cntargs
          POP(abce); // retval
          for (i = 0; i < cntloc; i++)
          {
            POP(abce);
          }
          POP(abce); // ip
          POP(abce); // bp
          for (i = 0; i < cntargs; i++)
          {
            POP(abce);
          }
          if (abce_push_mb(abce, &mb) != 0)
          {
            abort();
          }
          memblock_refdn(abce, &mb);
          break;
        }
        case ABCE_OPCODE_JMP:
        {
          const size_t guard = 100;
          double d;
          int64_t new_ip;
          GETDBL(&d, -1);
          POP(abce);
          new_ip = d;
          if (!((new_ip >= 0 && new_ip <= abce->bytecodesz) ||
                (new_ip >= -addsz-guard && new_ip <= -guard)))
          {
            return -EFAULT;
          }
          abce->ip = new_ip;
          break;
        }
        case ABCE_OPCODE_IF_NOT_JMP:
        {
          const size_t guard = 100;
          int b;
          double d;
          int64_t new_ip;
          GETBOOLEAN(&b, -1);
          GETDBL(&d, -2);
          POP(abce);
          POP(abce);
          new_ip = d;
          if (!((new_ip >= 0 && new_ip <= abce->bytecodesz) ||
                (new_ip >= -addsz-guard && new_ip <= -guard)))
          {
            return -EFAULT;
          }
          if (!b)
          {
            abce->ip = new_ip;
          }
          break;
        }
        case ABCE_OPCODE_BOOLEANIFY:
        {
          int b;
          GETBOOLEAN(&b, -1);
          POP(abce);
          if (abce_push_boolean(abce, b) != 0)
          {
            abort();
          }
          break;
        }
        case ABCE_OPCODE_EQ:
        {
          double d1, d2;
          GETDBL(&d2, -1);
          GETDBL(&d1, -2);
          POP(abce);
          POP(abce);
          if (abce_push_boolean(abce, !!(d1 == d2)) != 0)
          {
            abort();
          }
          break;
        }
        case ABCE_OPCODE_NE:
        {
          double d1, d2;
          GETDBL(&d2, -1);
          GETDBL(&d1, -2);
          POP(abce);
          POP(abce);
          if (abce_push_boolean(abce, !!(d1 != d2)) != 0)
          {
            abort();
          }
          break;
        }
        case ABCE_OPCODE_LOGICAL_AND:
        {
          int b1, b2;
          GETBOOLEAN(&b2, -1);
          GETBOOLEAN(&b1, -2);
          POP(abce);
          POP(abce);
          if (abce_push_boolean(abce, !!(b1 && b2)) != 0)
          {
            abort();
          }
          break;
        }
        case ABCE_OPCODE_BITWISE_AND:
        {
          double d1, d2;
          GETDBL(&d2, -1);
          GETDBL(&d1, -2);
          POP(abce);
          POP(abce);
          if (abce_push_double(abce, (((int64_t)d1) & ((int64_t)d2))) != 0)
          {
            abort();
          }
          break;
        }
        case ABCE_OPCODE_LOGICAL_OR:
        {
          int b1, b2;
          GETBOOLEAN(&b2, -1);
          GETBOOLEAN(&b1, -2);
          POP(abce);
          POP(abce);
          if (abce_push_boolean(abce, !!(b1 || b2)) != 0)
          {
            abort();
          }
          break;
        }
        case ABCE_OPCODE_BITWISE_OR:
        {
          double d1, d2;
          GETDBL(&d2, -1);
          GETDBL(&d1, -2);
          POP(abce);
          POP(abce);
          if (abce_push_double(abce, (((int64_t)d1) | ((int64_t)d2))) != 0)
          {
            abort();
          }
          break;
        }
        case ABCE_OPCODE_BITWISE_XOR:
        {
          double d1, d2;
          GETDBL(&d2, -1);
          GETDBL(&d1, -2);
          POP(abce);
          POP(abce);
          if (abce_push_double(abce, (((int64_t)d1) ^ ((int64_t)d2))) != 0)
          {
            abort();
          }
          break;
        }
        case ABCE_OPCODE_LOGICAL_NOT:
        {
          int b;
          GETBOOLEAN(&b, -1);
          POP(abce);
          if (abce_push_boolean(abce, !b) != 0)
          {
            abort();
          }
          break;
        }
        case ABCE_OPCODE_BITWISE_NOT:
        {
          double d;
          GETDBL(&d, -1);
          POP(abce);
          if (abce_push_double(abce, ~(int64_t)d) != 0)
          {
            abort();
          }
          break;
        }
        case ABCE_OPCODE_LT:
        {
          double d1, d2;
          GETDBL(&d2, -1);
          GETDBL(&d1, -2);
          POP(abce);
          POP(abce);
          if (abce_push_boolean(abce, !!(d1 < d2)) != 0)
          {
            abort();
          }
          break;
        }
        case ABCE_OPCODE_GT:
        {
          double d1, d2;
          GETDBL(&d2, -1);
          GETDBL(&d1, -2);
          POP(abce);
          POP(abce);
          if (abce_push_boolean(abce, !!(d1 > d2)) != 0)
          {
            abort();
          }
          break;
        }
        case ABCE_OPCODE_LE:
        {
          double d1, d2;
          GETDBL(&d2, -1);
          GETDBL(&d1, -2);
          POP(abce);
          POP(abce);
          if (abce_push_boolean(abce, !!(d1 <= d2)) != 0)
          {
            abort();
          }
          break;
        }
        case ABCE_OPCODE_GE:
        {
          double d1, d2;
          GETDBL(&d2, -1);
          GETDBL(&d1, -2);
          POP(abce);
          POP(abce);
          if (abce_push_boolean(abce, !!(d1 >= d2)) != 0)
          {
            abort();
          }
          break;
        }
        case ABCE_OPCODE_SHR:
        {
          double d1, d2;
          GETDBL(&d2, -1);
          GETDBL(&d1, -2);
          POP(abce);
          POP(abce);
          if (abce_push_double(abce, (((int64_t)d1) >> ((int64_t)d2))) != 0)
          {
            abort();
          }
          break;
        }
        case ABCE_OPCODE_SHL:
        {
          double d1, d2;
          GETDBL(&d2, -1);
          GETDBL(&d1, -2);
          POP(abce);
          POP(abce);
          if (abce_push_double(abce, (((int64_t)d1) << ((int64_t)d2))) != 0)
          {
            abort();
          }
          break;
        }
        case ABCE_OPCODE_ADD:
        {
          double d1, d2;
          GETDBL(&d2, -1);
          GETDBL(&d1, -2);
          POP(abce);
          POP(abce);
          if (abce_push_double(abce, (d1 + d2)) != 0)
          {
            abort();
          }
          break;
        }
        case ABCE_OPCODE_SUB:
        {
          double d1, d2;
          GETDBL(&d2, -1);
          GETDBL(&d1, -2);
          POP(abce);
          POP(abce);
          if (abce_push_double(abce, (d1 - d2)) != 0)
          {
            abort();
          }
          break;
        }
        case ABCE_OPCODE_MUL:
        {
          double d1, d2;
          GETDBL(&d2, -1);
          GETDBL(&d1, -2);
          POP(abce);
          POP(abce);
          if (abce_push_double(abce, (d1 * d2)) != 0)
          {
            abort();
          }
          break;
        }
        case ABCE_OPCODE_DIV:
        {
          double d1, d2;
          GETDBL(&d2, -1);
          GETDBL(&d1, -2);
          POP(abce);
          POP(abce);
          if (abce_push_double(abce, (d1 / d2)) != 0)
          {
            abort();
          }
          break;
        }
        case ABCE_OPCODE_MOD:
        {
          double d1, d2;
          GETDBL(&d2, -1);
          GETDBL(&d1, -2);
          POP(abce);
          POP(abce);
          if (abce_push_double(abce, (((int64_t)d1) % ((int64_t)d2))) != 0)
          {
            abort();
          }
          break;
        }
        case ABCE_OPCODE_UNARY_MINUS:
        {
          double d;
          GETDBL(&d, -1);
          POP(abce);
          if (abce_push_double(abce, -d) != 0)
          {
            abort();
          }
          break;
        }
      }
    }
    else if (likely(ins < 128))
    {
      printf("Trap short\n");
      abort();
    }
    else if (likely(ins < 0x400))
    {
      printf("mid\n");
      abort();
    }
    else if (likely(ins < 0x800))
    {
      printf("Trap mid\n");
      abort();
    }
    else if (likely(ins < 0x8000))
    {
      printf("long\n");
      abort();
    }
    else
    {
      printf("Trap long\n");
      abort();
    }
  }
  return ret;
}

int main(int argc, char **argv)
{
  struct abce real_abce = {.alloc = std_alloc};
  struct abce *abce = &real_abce;
  struct memblock mba = memblock_create_array(abce);
  struct memblock mbsc1 = memblock_create_scope_noparent(abce, 16);
  struct memblock mbsc2 = memblock_create_scope(abce, 16, &mbsc1, 0);
  struct memblock mbs1 = memblock_create_string_nul(abce, "foo");
  struct memblock mbs2 = memblock_create_string_nul(abce, "bar");
  struct memblock mbs3 = memblock_create_string_nul(abce, "baz");
  struct memblock mbs4 = memblock_create_string_nul(abce, "barf");
  struct memblock mbs5 = memblock_create_string_nul(abce, "quux");
  memblock_array_append(abce, &mba, &mbs1);
  memblock_array_append(abce, &mba, &mbs2);
  memblock_array_append(abce, &mba, &mbs3);
  sc_put_val_mb(abce, &mbsc1, &mbs1, &mbs2);
  sc_put_val_mb(abce, &mbsc1, &mbs3, &mbs4);
  sc_put_val_mb(abce, &mbsc2, &mbs2, &mbs1);
  sc_put_val_mb(abce, &mbsc2, &mbs4, &mbs3);
  memblock_dump(&mba);
  memblock_dump(&mbsc1);
  memblock_dump(&mbsc2);
  memblock_dump(sc_get_rec_str(&mbsc2, "foo"));
  memblock_dump(sc_get_rec_str(&mbsc2, "bar"));
  memblock_dump(sc_get_rec_str(&mbsc2, "baz"));
  memblock_dump(sc_get_rec_str(&mbsc2, "barf"));
  memblock_refdn(abce, &mba);
  memblock_refdn(abce, &mbsc1);
  memblock_refdn(abce, &mbsc2);
  memblock_refdn(abce, &mbs1);
  memblock_refdn(abce, &mbs2);
  memblock_refdn(abce, &mbs3);
  memblock_refdn(abce, &mbs4);
  memblock_refdn(abce, &mbs5);
  stacktest_main(abce);
  return 0;
}
