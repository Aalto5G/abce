#ifndef _DATATYPES_H_
#define _DATATYPES_H_

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

#define ABCE_DEFAULT_SCOPE_SIZE 8192
#define ABCE_DEFAULT_CACHE_SIZE 8192

#ifdef __cplusplus
extern "C" {
#endif

void *abce_std_alloc(void *old, size_t newsz, void *alloc_baton);

struct abce_const_str_len {
  const char *str;
  size_t len;
};

struct abce_mb;

struct abce_mb_tree {
  struct rb_tree_nocmp tree;
  size_t sz;
};
struct abce_mb_scope {
  int holey;
  struct abce_mb_area *parent;
  size_t size;
  struct rb_tree_nocmp heads[0];
};
struct abce_mb_array {
  struct abce_mb *mbs;
  size_t capacity;
  size_t size;
};
struct abce_mb_string {
  struct rb_tree_node node;
  size_t size;
  size_t locidx;
  char buf[0];
};
struct abce_mb_ios {
  FILE *f;
};
struct abce_mb_area {
  size_t refcnt;
  union {
    struct abce_mb_array ar;
    struct abce_mb_ios ios;
    struct abce_mb_scope sc;
    struct abce_mb_tree tree;
    struct abce_mb_string str;
  } u;
};
enum abce_type {
  ABCE_T_T,
  ABCE_T_D,
  ABCE_T_B,
  ABCE_T_F,
  ABCE_T_S,
  ABCE_T_PB, // packet buffer
  ABCE_T_IOS,
  ABCE_T_BP,
  ABCE_T_IP,
  ABCE_T_LP,
  ABCE_T_A,
  ABCE_T_SC,
  ABCE_T_N,
};
struct abce_mb {
  enum abce_type typ;
  union {
    double d;
    struct abce_mb_area *area;
  } u;
};
struct abce_mb_rb_entry {
  struct rb_tree_node n;
  struct abce_mb key; // must be a string!
  struct abce_mb val;
};

struct abce {
  void *(*alloc)(void *old, size_t newsz, void *alloc_baton);
  int (*trap)(struct abce*, uint16_t ins, unsigned char *addcode, size_t addsz);
  void *alloc_baton;
  void *userdata;
  // Stack and registers
  struct abce_mb *stackbase;
  size_t stacklimit;
  size_t sp;
  size_t bp;
  int64_t ip;
  // Byte code
  unsigned char *bytecode;
  size_t bytecodesz;
  size_t bytecodecap;
  // Object cache
  struct abce_mb *cachebase;
  size_t cachesz;
  size_t cachecap;
  struct rb_tree_nocmp strcache[ABCE_DEFAULT_CACHE_SIZE];
  // Dynamic scope
  struct abce_mb dynscope;
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

void abce_mb_arearefdn(struct abce *abce, struct abce_mb_area **mba, enum abce_type typ);

static inline void abce_mb_refdn(struct abce *abce, struct abce_mb *mb)
{
  switch (mb->typ)
  {
    case ABCE_T_T:
    case ABCE_T_IOS:
    case ABCE_T_A:
    case ABCE_T_S:
    case ABCE_T_SC:
      abce_mb_arearefdn(abce, &mb->u.area, mb->typ);
      break;
    default:
      break;
  }
  mb->typ = ABCE_T_N;
  mb->u.d = 0.0;
  mb->u.area = NULL;
}


static inline struct abce_mb
abce_mb_refup(struct abce *abce, const struct abce_mb *mb)
{
  switch (mb->typ)
  {
    case ABCE_T_T: case ABCE_T_S: case ABCE_T_IOS: case ABCE_T_A: case ABCE_T_SC:
      mb->u.area->refcnt++;
      break;
    default:
      break;
  }
  return *mb;
}

static inline int abce_cache_add(struct abce *abce, const struct abce_mb *mb)
{
  if (abce->cachesz >= abce->cachecap)
  {
    return -EOVERFLOW;
  }
  abce->cachebase[abce->cachesz++] = abce_mb_refup(abce, mb);
  return 0;
}


static inline int abce_pop(struct abce *abce)
{
  struct abce_mb *mb;
  if (abce->sp == 0 || abce->sp <= abce->bp)
  {
    return -EOVERFLOW;
  }
  mb = &abce->stackbase[--abce->sp];
  abce_mb_refdn(abce, mb);
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
  const struct abce_mb *mb;
  size_t addr;
  if (abce_calc_addr(&addr, abce, idx) != 0)
  {
    return -EOVERFLOW;
  }
  mb = &abce->stackbase[addr];
  if (mb->typ != ABCE_T_D || mb->typ != ABCE_T_B)
  {
    return -EINVAL;
  }
  *b = !!mb->u.d;
  return 0;
}

static inline int abce_getfunaddr(int64_t *paddr, struct abce *abce, int64_t idx)
{
  const struct abce_mb *mb;
  size_t addr;
  if (abce_calc_addr(&addr, abce, idx) != 0)
  {
    return -EOVERFLOW;
  }
  mb = &abce->stackbase[addr];
  if (mb->typ != ABCE_T_F)
  {
    return -EINVAL;
  }
  *paddr = mb->u.d;
  return 0;
}

static inline int abce_getbp(struct abce *abce, int64_t idx)
{
  const struct abce_mb *mb;
  size_t addr;
  size_t trial;
  if (abce_calc_addr(&addr, abce, idx) != 0)
  {
    return -EOVERFLOW;
  }
  mb = &abce->stackbase[addr];
  if (mb->typ != ABCE_T_BP)
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
  const struct abce_mb *mb;
  size_t addr;
  size_t trial;
  if (abce_calc_addr(&addr, abce, idx) != 0)
  {
    return -EOVERFLOW;
  }
  mb = &abce->stackbase[addr];
  if (mb->typ != ABCE_T_IP)
  {
    printf("invalid typ: %d\n", mb->typ);
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

static inline int abce_getmb(struct abce_mb *mb, struct abce *abce, int64_t idx)
{
  const struct abce_mb *mbptr;
  size_t addr;
  if (abce_calc_addr(&addr, abce, idx) != 0)
  {
    return -EOVERFLOW;
  }
  mbptr = &abce->stackbase[addr];
  *mb = abce_mb_refup(abce, mbptr);
  return 0;
}
static inline int abce_getmbsc(struct abce_mb *mb, struct abce *abce, int64_t idx)
{
  const struct abce_mb *mbptr;
  size_t addr;
  if (abce_calc_addr(&addr, abce, idx) != 0)
  {
    return -EOVERFLOW;
  }
  mbptr = &abce->stackbase[addr];
  if (mbptr->typ != ABCE_T_SC)
  {
    return -EINVAL;
  }
  *mb = abce_mb_refup(abce, mbptr);
  return 0;
}
static inline int abce_getmbar(struct abce_mb *mb, struct abce *abce, int64_t idx)
{
  const struct abce_mb *mbptr;
  size_t addr;
  if (abce_calc_addr(&addr, abce, idx) != 0)
  {
    return -EOVERFLOW;
  }
  mbptr = &abce->stackbase[addr];
  if (mbptr->typ != ABCE_T_A)
  {
    return -EINVAL;
  }
  *mb = abce_mb_refup(abce, mbptr);
  return 0;
}
static inline int abce_getmbstr(struct abce_mb *mb, struct abce *abce, int64_t idx)
{
  const struct abce_mb *mbptr;
  size_t addr;
  if (abce_calc_addr(&addr, abce, idx) != 0)
  {
    return -EOVERFLOW;
  }
  mbptr = &abce->stackbase[addr];
  if (mbptr->typ != ABCE_T_S)
  {
    return -EINVAL;
  }
  *mb = abce_mb_refup(abce, mbptr);
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
  const struct abce_mb *mb;
  size_t addr;
  if (abce_calc_addr(&addr, abce, idx) != 0)
  {
    return -EOVERFLOW;
  }
  printf("addr %d\n", (int)addr);
  mb = &abce->stackbase[addr];
  if (mb->typ != ABCE_T_D && mb->typ != ABCE_T_B)
  {
    return -EINVAL;
  }
  *d = mb->u.d;
  return 0;
}

static inline int abce_push_mb(struct abce *abce, const struct abce_mb *mb)
{
  if (abce->sp >= abce->stacklimit)
  {
    return -EOVERFLOW;
  }
  abce->stackbase[abce->sp] = abce_mb_refup(abce, mb);
  abce->sp++;
  return 0;
}

static inline int abce_push_boolean(struct abce *abce, int boolean)
{
  if (abce->sp >= abce->stacklimit)
  {
    return -EOVERFLOW;
  }
  abce->stackbase[abce->sp].typ = ABCE_T_D;
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
  abce->stackbase[abce->sp].typ = ABCE_T_N;
  abce->sp++;
  return 0;
}

static inline int abce_push_ip(struct abce *abce)
{
  if (abce->sp >= abce->stacklimit)
  {
    return -EOVERFLOW;
  }
  abce->stackbase[abce->sp].typ = ABCE_T_IP;
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
  abce->stackbase[abce->sp].typ = ABCE_T_BP;
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
  abce->stackbase[abce->sp].typ = ABCE_T_D;
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
  abce->stackbase[abce->sp].typ = ABCE_T_F;
  abce->stackbase[abce->sp].u.d = fun_addr;
  abce->sp++;
  return 0;
}

static inline int abce_add_ins(struct abce *abce, uint16_t ins)
{
  if (ins >= 2048)
  {
    if (abce->bytecodesz + 3 > abce->bytecodecap)
    {
      return -EFAULT;
    }
    abce->bytecode[abce->bytecodesz++] = (ins>>12) | 0xE0;
    abce->bytecode[abce->bytecodesz++] = ((ins>>6)&0x3F) | 0x80;
    abce->bytecode[abce->bytecodesz++] = ((ins)&0x3F) | 0x80;
    return 0;
  }
  else if (ins >= 128)
  {
    if (abce->bytecodesz + 2 > abce->bytecodecap)
    {
      return -EFAULT;
    }
    abce->bytecode[abce->bytecodesz++] = ((ins>>6)) | 0xC0;
    abce->bytecode[abce->bytecodesz++] = ((ins)&0x3F) | 0x80;
    return 0;
  }
  else
  {
    if (abce->bytecodesz >= abce->bytecodecap)
    {
      return -EFAULT;
    }
    abce->bytecode[abce->bytecodesz++] = ins;
    return 0;
  }
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

static inline struct abce_mb_area*
abce_mb_arearefup(struct abce *abce, const struct abce_mb *mb)
{
  switch (mb->typ)
  {
    case ABCE_T_T: case ABCE_T_S: case ABCE_T_IOS: case ABCE_T_A: case ABCE_T_SC:
      mb->u.area->refcnt++;
      return mb->u.area;
    default:
      abort();
  }
}

static inline struct abce_mb
abce_mb_create_string(struct abce *abce, const char *str, size_t sz)
{
  struct abce_mb_area *mba;
  struct abce_mb mb = {};
  mba = (struct abce_mb_area*)abce->alloc(NULL, sizeof(*mba) + sz + 1, abce->alloc_baton);
  mba->u.str.size = sz;
  memcpy(mba->u.str.buf, str, sz);
  mba->u.str.buf[sz] = '\0';
  mba->refcnt = 1;
  mb.typ = ABCE_T_S;
  mb.u.area = mba;
  return mb;
}

static inline struct abce_mb
abce_mb_create_string_nul(struct abce *abce, const char *str)
{
  return abce_mb_create_string(abce, str, strlen(str));
}

static inline uint32_t abce_str_hash(const char *str)
{
  size_t len = strlen(str);
  return murmur_buf(0x12345678U, str, len);
}
static inline uint32_t abce_str_len_hash(const struct abce_const_str_len *str_len)
{
  size_t len = str_len->len;
  return murmur_buf(0x12345678U, str_len->str, len);
}
static inline uint32_t abce_mb_str_hash(const struct abce_mb *mb)
{
  if (mb->typ != ABCE_T_S)
  {
    abort();
  }
  return murmur_buf(0x12345678U, mb->u.area->u.str.buf, mb->u.area->u.str.size);
}
static inline int abce_str_cache_cmp_asymlen(const struct abce_const_str_len *str_len, struct rb_tree_node *n2, void *ud)
{
  struct abce_mb_string *e = CONTAINER_OF(n2, struct abce_mb_string, node);
  size_t len1 = str_len->len;
  size_t len2, lenmin;
  int ret;
  char *str2;
  len2 = e->size;
  str2 = e->buf;
  lenmin = (len1 < len2) ? len1 : len2;
  ret = memcmp(str_len->str, str2, lenmin);
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
static inline int abce_str_cmp_asym(const char *str, struct rb_tree_node *n2, void *ud)
{
  struct abce_mb_rb_entry *e = CONTAINER_OF(n2, struct abce_mb_rb_entry, n);
  size_t len1 = strlen(str);
  size_t len2, lenmin;
  int ret;
  char *str2;
  if (e->key.typ != ABCE_T_S)
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

static inline int abce_str_cmp_halfsym(
  const struct abce_mb *key, struct rb_tree_node *n2, void *ud)
{
  struct abce_mb_rb_entry *e2 = CONTAINER_OF(n2, struct abce_mb_rb_entry, n);
  size_t len1, len2, lenmin;
  int ret;
  char *str1, *str2;
  if (key->typ != ABCE_T_S || e2->key.typ != ABCE_T_S)
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

static inline int abce_str_cache_cmp_sym(
  struct rb_tree_node *n1, struct rb_tree_node *n2, void *ud)
{
  struct abce_mb_string *e1 = CONTAINER_OF(n1, struct abce_mb_string, node);
  struct abce_mb_string *e2 = CONTAINER_OF(n2, struct abce_mb_string, node);
  size_t len1, len2, lenmin;
  int ret;
  char *str1, *str2;
  len1 = e1->size;
  str1 = e1->buf;
  len2 = e2->size;
  str2 = e2->buf;
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

static inline int abce_str_cmp_sym(
  struct rb_tree_node *n1, struct rb_tree_node *n2, void *ud)
{
  struct abce_mb_rb_entry *e1 = CONTAINER_OF(n1, struct abce_mb_rb_entry, n);
  struct abce_mb_rb_entry *e2 = CONTAINER_OF(n2, struct abce_mb_rb_entry, n);
  size_t len1, len2, lenmin;
  int ret;
  char *str1, *str2;
  if (e1->key.typ != ABCE_T_S || e2->key.typ != ABCE_T_S)
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

int64_t abce_cache_add_str(struct abce *abce, const char *str, size_t len);

static inline int64_t abce_cache_add_str_nul(struct abce *abce, const char *str)
{
  return abce_cache_add_str(abce, str, strlen(str));
}

// RFE remove inlining?
static inline const struct abce_mb *abce_sc_get_myval_mb_area(
  const struct abce_mb_area *mba, const struct abce_mb *key)
{
  uint32_t hashval;
  size_t hashloc;
  struct rb_tree_node *n;
  if (key->typ != ABCE_T_S)
  {
    abort();
  }
  hashval = abce_mb_str_hash(key);
  hashloc = hashval & (mba->u.sc.size - 1);
  n = RB_TREE_NOCMP_FIND(&mba->u.sc.heads[hashloc], abce_str_cmp_halfsym, NULL, key);
  if (n == NULL)
  {
    return NULL;
  }
  return &CONTAINER_OF(n, struct abce_mb_rb_entry, n)->val;
}

static inline const struct abce_mb *abce_sc_get_myval_mb(
  const struct abce_mb *mb, const struct abce_mb *key)
{
  if (mb->typ != ABCE_T_SC)
  {
    abort();
  }
  return abce_sc_get_myval_mb_area(mb->u.area, key);
}

// RFE remove inlining?
static inline const struct abce_mb *abce_sc_get_myval_str_area(
  const struct abce_mb_area *mba, const char *str)
{
  uint32_t hashval;
  size_t hashloc;
  struct rb_tree_node *n;
  hashval = abce_str_hash(str);
  hashloc = hashval & (mba->u.sc.size - 1);
  n = RB_TREE_NOCMP_FIND(&mba->u.sc.heads[hashloc], abce_str_cmp_asym, NULL, str);
  if (n == NULL)
  {
    return NULL;
  }
  return &CONTAINER_OF(n, struct abce_mb_rb_entry, n)->val;
}

static inline const struct abce_mb *abce_sc_get_myval_str(
  const struct abce_mb *mb, const char *str)
{
  if (mb->typ != ABCE_T_SC)
  {
    abort();
  }
  return abce_sc_get_myval_str_area(mb->u.area, str);
}

const struct abce_mb *abce_sc_get_rec_mb_area(
  const struct abce_mb_area *mba, const struct abce_mb *it);

static inline const struct abce_mb *
abce_sc_get_rec_mb(const struct abce_mb *mb, const struct abce_mb *it)
{
  if (mb->typ != ABCE_T_SC)
  {
    abort();
  }
  return abce_sc_get_rec_mb_area(mb->u.area, it);
}

const struct abce_mb *abce_sc_get_rec_str_area(
  const struct abce_mb_area *mba, const char *str);

static inline const struct abce_mb *
abce_sc_get_rec_str(const struct abce_mb *mb, const char *str)
{
  if (mb->typ != ABCE_T_SC)
  {
    abort();
  }
  return abce_sc_get_rec_str_area(mb->u.area, str);
}

int abce_sc_put_val_mb(
  struct abce *abce,
  const struct abce_mb *mb, const struct abce_mb *pkey, const struct abce_mb *pval);

int abce_sc_put_val_str(
  struct abce *abce,
  const struct abce_mb *mb, const char *str, const struct abce_mb *pval);


static inline size_t abce_next_highest_power_of_2(size_t x)
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

struct abce_mb abce_mb_create_scope(struct abce *abce, size_t capacity,
                                      const struct abce_mb *parent, int holey);

static inline struct abce_mb abce_mb_create_scope_noparent(struct abce *abce, size_t capacity)
{
  return abce_mb_create_scope(abce, capacity, NULL, 0);
}

static inline struct abce_mb abce_mb_create_tree(struct abce *abce)
{
  struct abce_mb_area *mba;
  struct abce_mb mb = {};
  mba = (struct abce_mb_area*)abce->alloc(NULL, sizeof(*mba), abce->alloc_baton);
  rb_tree_nocmp_init(&mba->u.tree.tree);
  mba->refcnt = 1;
  mb.typ = ABCE_T_A;
  mb.u.area = mba;
  return mb;
}

static inline struct abce_mb abce_mb_create_array(struct abce *abce)
{
  struct abce_mb_area *mba;
  struct abce_mb mb = {};
  mba = (struct abce_mb_area*)abce->alloc(NULL, sizeof(*mba), abce->alloc_baton);
  mba->u.ar.size = 0;
  mba->u.ar.capacity = 16;
  mba->u.ar.mbs =
    (struct abce_mb*)abce->alloc(NULL, 16*sizeof(*mba->u.ar.mbs), abce->alloc_baton);
  mba->refcnt = 1;
  mb.typ = ABCE_T_A;
  mb.u.area = mba;
  return mb;
}

void abce_mb_arearefdn(struct abce *abce, struct abce_mb_area **mbap, enum abce_type typ);

void abce_mb_dump_impl(const struct abce_mb *mb);

static inline void
abce_mb_array_pop_back(struct abce *abce,
                        struct abce_mb *mb, const struct abce_mb *it)
{
  if (mb->typ != ABCE_T_A)
  {
    abort();
  }
  if (mb->u.area->u.ar.size <= 0)
  {
    abort();
  }
  abce_mb_refdn(abce, &mb->u.area->u.ar.mbs[--mb->u.area->u.ar.size]);
}

static inline void
abce_mb_array_append(struct abce *abce,
                      struct abce_mb *mb, const struct abce_mb *it)
{
  if (mb->typ != ABCE_T_A)
  {
    abort();
  }
  if (mb->u.area->u.ar.size >= mb->u.area->u.ar.capacity)
  {
    size_t new_cap = 2*mb->u.area->u.ar.size + 1;
    struct abce_mb *mbs2;
    mbs2 = (struct abce_mb*)abce->alloc(mb->u.area->u.ar.mbs,
                       sizeof(*mb->u.area->u.ar.mbs)*new_cap,
                       abce->alloc_baton);
    mb->u.area->u.ar.capacity = new_cap;
    mb->u.area->u.ar.mbs = mbs2;
  }
  mb->u.area->u.ar.mbs[mb->u.area->u.ar.size++] = abce_mb_refup(abce, it);
}

void abce_mb_treedump(const struct rb_tree_node *n, int *first);

void abce_dump_str(const char *str, size_t sz);

void abce_mb_dump_impl(const struct abce_mb *mb);

static inline void abce_mb_dump(const struct abce_mb *mb)
{
  abce_mb_dump_impl(mb);
  printf("\n");
}

static inline size_t abce_topages(size_t limit)
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

struct abce_mb *abce_alloc_stack(size_t limit);

void abce_free_stack(struct abce_mb *stackbase, size_t limit);

unsigned char *abce_alloc_bcode(size_t limit);

void abce_free_bcode(unsigned char *bcodebase, size_t limit);

void abce_init(struct abce *abce);

void abce_free(struct abce *abce);

static inline int
abce_fetch_b(uint8_t *b, struct abce *abce, unsigned char *addcode, size_t addsz)
{
  const size_t guard = 100;
  if (!((abce->ip >= 0 && (size_t)abce->ip < abce->bytecodesz) ||
        (abce->ip >= -(int64_t)addsz-(int64_t)guard && abce->ip < -(int64_t)guard)))
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
abce_fetch_d(double *d, struct abce *abce, unsigned char *addcode, size_t addsz)
{
  const size_t guard = 100;
  if (!((abce->ip >= 0 && (size_t)abce->ip+8 <= abce->bytecodesz) ||
        (abce->ip >= -(int64_t)addsz-(int64_t)guard && abce->ip+8 <= -(int64_t)guard)))
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
abce_fetch_i(uint16_t *ins, struct abce *abce, unsigned char *addcode, size_t addsz)
{
  uint8_t ophi, opmid, oplo;
  if (abce_fetch_b(&ophi, abce, addcode, addsz) != 0)
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
    printf("EILSEQ 1\n");
    return -EILSEQ;
  }
  else if (likely((ophi & 0xE0) == 0xC0))
  {
    if (abce_fetch_b(&oplo, abce, addcode, addsz) != 0)
    {
      return -EFAULT;
    }
    if (unlikely((oplo & 0xC0) != 0x80))
    {
      printf("EILSEQ 2\n");
      return -EILSEQ;
    }
    *ins = ((ophi&0x1F) << 6) | (oplo & 0x3F);
    if (unlikely(*ins < 128))
    {
      printf("EILSEQ 3\n");
      return -EILSEQ;
    }
    return 0;
  }
  else if (likely((ophi & 0xF0) == 0xE0))
  {
    if (abce_fetch_b(&opmid, abce, addcode, addsz) != 0)
    {
      return -EFAULT;
    }
    if (unlikely((opmid & 0xC0) != 0x80))
    {
      printf("EILSEQ 4\n");
      return -EILSEQ;
    }
    if (abce_fetch_b(&oplo, abce, addcode, addsz) != 0)
    {
      return -EFAULT;
    }
    if (unlikely((oplo & 0xC0) != 0x80))
    {
      printf("EILSEQ 5\n");
      return -EILSEQ;
    }
    *ins = ((ophi&0xF) << 12) | ((opmid&0x3F) << 6) | (oplo & 0x3F);
    if (unlikely(*ins <= 0x7FF))
    {
      printf("EILSEQ 6\n");
      return -EILSEQ;
    }
    return 0;
  }
  else
  {
    printf("EILSEQ 7\n");
    return -EILSEQ;
  }
}

int
abce_mid(struct abce *abce, uint16_t ins, unsigned char *addcode, size_t addsz);

int abce_engine(struct abce *abce, unsigned char *addcode, size_t addsz);

#ifdef __cplusplus
};
#endif

#endif
