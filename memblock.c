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

static int
abce_tree_get_str(struct abce *abce, struct abce_mb **mbres,
                  struct abce_mb *mbt, const struct abce_mb *mbkey)
{
  struct rb_tree_node *n;
  if (mbt->typ != ABCE_T_T)
  {
    abort();
  }
  n = RB_TREE_NOCMP_FIND(&mbt->u.area->u.tree.tree, abce_str_cmp_halfsym, NULL, mbkey);
  if (n == NULL)
  {
    return -ENOENT;
  }
  *mbres = &CONTAINER_OF(n, struct abce_mb_rb_entry, n)->val;
  return 0;
}

static int
abce_tree_set_str(struct abce *abce,
                  struct abce_mb *mbt,
                  const struct abce_mb *mbkey,
                  const struct abce_mb *mbval)
{
  struct abce_mb *mbres;
  struct abce_mb_rb_entry *e;
  if (mbt->typ != ABCE_T_T)
  {
    abort();
  }
  if (mbkey->typ != ABCE_T_S)
  {
    abort();
  }
  if (abce_tree_get_str(abce, &mbres, mbt, mbkey) == 0)
  {
    abce_mb_refdn(abce, mbres);
    *mbres = abce_mb_refup(abce, mbval);
    return 0;
  }
  e = abce->alloc(NULL, sizeof(*e), abce->alloc_baton);
  if (e == NULL)
  {
    return -ENOMEM;
  }
  e->key = abce_mb_refup(abce, mbkey);
  e->val = abce_mb_refup(abce, mbval);
  if (rb_tree_nocmp_insert_nonexist(&mbt->u.area->u.tree.tree, abce_str_cmp_sym, NULL, &e->n) != 0)
  {
    abort();
  }
  mbt->u.area->u.tree.sz += 1;
  return 0;
}

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
  mb.u.area->u.str.locidx = abce->cachesz;
  abce->cachebase[abce->cachesz++] = mb;
  if (rb_tree_nocmp_insert_nonexist(&abce->strcache[hashloc], abce_str_cache_cmp_sym, NULL, &mb.u.area->u.str.node) != 0)
  {
    abort();
  }
  return mb.u.area->u.str.locidx;
}

const struct abce_mb *abce_sc_get_rec_mb_area(
  const struct abce_mb_area *mba, const struct abce_mb *it)
{
  const struct abce_mb *mb = abce_sc_get_myval_mb_area(mba, it);
  if (mb != NULL)
  {
    return mb;
  }
  if (mba->u.sc.parent != NULL && !mba->u.sc.holey)
  {
    return abce_sc_get_rec_mb_area(mba->u.sc.parent, it);
  }
  return NULL;
}

const struct abce_mb *abce_sc_get_rec_str_area(
  const struct abce_mb_area *mba, const char *str)
{
  const struct abce_mb *mb = abce_sc_get_myval_str_area(mba, str);
  if (mb != NULL)
  {
    return mb;
  }
  if (mba->u.sc.parent != NULL && !mba->u.sc.holey)
  {
    return abce_sc_get_rec_str_area(mba->u.sc.parent, str);
  }
  return NULL;
}

int abce_sc_put_val_mb(
  struct abce *abce,
  const struct abce_mb *mb, const struct abce_mb *pkey, const struct abce_mb *pval)
{
  struct abce_mb_area *mba = mb->u.area;
  uint32_t hashval;
  struct abce_mb_rb_entry *e;
  size_t hashloc;
  int ret;
  if (mb->typ != ABCE_T_SC || pkey->typ != ABCE_T_S)
  {
    abort();
  }
  hashval = abce_mb_str_hash(pkey);
  hashloc = hashval & (mba->u.sc.size - 1);
  e = abce->alloc(NULL, sizeof(*e), abce->alloc_baton);
  e->key = abce_mb_refup(abce, pkey);
  e->val = abce_mb_refup(abce, pval);
  ret = rb_tree_nocmp_insert_nonexist(&mba->u.sc.heads[hashloc],
                                      abce_str_cmp_sym, NULL, &e->n);
  if (ret == 0)
  {
    return 0;
  }
  abce_mb_refdn(abce, &e->key);
  abce_mb_refdn(abce, &e->val);
  abce->alloc(e, 0, abce->alloc_baton);
  return ret;
}

int abce_sc_put_val_str(
  struct abce *abce,
  const struct abce_mb *mb, const char *str, const struct abce_mb *pval)
{
  struct abce_mb_area *mba = mb->u.area;
  uint32_t hashval;
  struct abce_mb_rb_entry *e;
  size_t hashloc;
  int ret;
  if (mb->typ != ABCE_T_SC)
  {
    abort();
  }
  hashval = abce_str_hash(str);
  hashloc = hashval & (mba->u.sc.size - 1);
  e = abce->alloc(NULL, sizeof(*e), abce->alloc_baton);
  e->key = abce_mb_create_string(abce, str, strlen(str));
  e->val = abce_mb_refup(abce, pval);
  ret = rb_tree_nocmp_insert_nonexist(&mba->u.sc.heads[hashloc],
                                      abce_str_cmp_sym, NULL, &e->n);
  if (ret == 0)
  {
    return 0;
  }
  abce_mb_refdn(abce, &e->key);
  abce_mb_refdn(abce, &e->val);
  abce->alloc(e, 0, abce->alloc_baton);
  return ret;
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
  newbuf = abce->alloc(buf->str, newcap, abce->alloc_baton);
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

static int abce_strgsub(struct abce *abce,
                        char **res, size_t *ressz,
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
    abce->alloc(buf.str, 0, abce->alloc_baton);
    *res = NULL;
    *ressz = 0;
    return -ENOMEM;
  }
  *res = buf.str;
  *ressz = buf.sz;
  return 0;
}

static int abce_strgsub_mb(struct abce *abce,
                           struct abce_mb *res,
                           const struct abce_mb *haystack,
                           const struct abce_mb *needle,
                           const struct abce_mb *sub)
{
  char *resstr;
  size_t ressz;
  int retval;
  if (haystack->typ != ABCE_T_S || needle->typ != ABCE_T_S || sub->typ != ABCE_T_S)
  {
    return -EINVAL;
  }
  retval = abce_strgsub(abce, &resstr, &ressz,
                        haystack->u.area->u.str.buf, haystack->u.area->u.str.size,
                        needle->u.area->u.str.buf, needle->u.area->u.str.size,
                        sub->u.area->u.str.buf, sub->u.area->u.str.size);
  if (retval != 0)
  {
    return retval;
  }
  *res = abce_mb_create_string(abce, resstr, ressz);
  abce->alloc(resstr, 0, abce->alloc_baton);
  return 0;
}

#define VERIFYMB(idx, type) \
  if(1) { \
    int _getdbl_rettmp = abce_verifymb(abce, (idx), (type)); \
    if (_getdbl_rettmp != 0) \
    { \
      ret = _getdbl_rettmp; \
      break; \
    } \
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
#define GETMBSC(mb, idx) \
  if(1) { \
    int _getdbl_rettmp = abce_getmbsc((mb), abce, (idx)); \
    if (_getdbl_rettmp != 0) \
    { \
      ret = _getdbl_rettmp; \
      break; \
    } \
  }
#define GETMBAR(mb, idx) \
  if(1) { \
    int _getdbl_rettmp = abce_getmbar((mb), abce, (idx)); \
    if (_getdbl_rettmp != 0) \
    { \
      ret = _getdbl_rettmp; \
      break; \
    } \
  }
#define GETMBSTR(mb, idx) \
  if(1) { \
    int _getdbl_rettmp = abce_getmbstr((mb), abce, (idx)); \
    if (_getdbl_rettmp != 0) \
    { \
      ret = _getdbl_rettmp; \
      break; \
    } \
  }
#define POP() \
  if(1) { \
    int _getdbl_rettmp = abce_pop(abce); \
    if (_getdbl_rettmp != 0) \
    { \
      abort(); \
    } \
  }

int
abce_mid(struct abce *abce, uint16_t ins, unsigned char *addcode, size_t addsz)
{
  int ret = 0;
  switch (ins)
  {
    case ABCE_OPCODE_DUMP:
    {
      struct abce_mb mb;
      GETMB(&mb, -1);
      POP();
      abce_mb_dump(&mb);
      abce_mb_refdn(abce, &mb);
      return 0;
    }
    case ABCE_OPCODE_ABS:
    {
      double dbl;
      GETDBL(&dbl, -1);
      POP();
      if (abce_push_double(abce, fabs(dbl)) != 0)
      {
        abort();
      }
      return 0;
    }
    case ABCE_OPCODE_SQRT:
    {
      double dbl;
      GETDBL(&dbl, -1);
      POP();
      if (abce_push_double(abce, sqrt(dbl)) != 0)
      {
        abort();
      }
      return 0;
    }
    case ABCE_OPCODE_LOG:
    {
      double dbl;
      GETDBL(&dbl, -1);
      POP();
      if (abce_push_double(abce, log(dbl)) != 0)
      {
        abort();
      }
      return 0;
    }
    case ABCE_OPCODE_EXP:
    {
      double dbl;
      GETDBL(&dbl, -1);
      POP();
      if (abce_push_double(abce, exp(dbl)) != 0)
      {
        abort();
      }
      return 0;
    }
    case ABCE_OPCODE_FLOOR:
    {
      double dbl;
      GETDBL(&dbl, -1);
      POP();
      if (abce_push_double(abce, floor(dbl)) != 0)
      {
        abort();
      }
      return 0;
    }
    case ABCE_OPCODE_CEIL:
    {
      double dbl;
      GETDBL(&dbl, -1);
      POP();
      if (abce_push_double(abce, ceil(dbl)) != 0)
      {
        abort();
      }
      return 0;
    }
    case ABCE_OPCODE_COS:
    {
      double dbl;
      GETDBL(&dbl, -1);
      POP();
      if (abce_push_double(abce, cos(dbl)) != 0)
      {
        abort();
      }
      return 0;
    }
    case ABCE_OPCODE_SIN:
    {
      double dbl;
      GETDBL(&dbl, -1);
      POP();
      if (abce_push_double(abce, sin(dbl)) != 0)
      {
        abort();
      }
      return 0;
    }
    case ABCE_OPCODE_TAN:
    {
      double dbl;
      GETDBL(&dbl, -1);
      POP();
      if (abce_push_double(abce, tan(dbl)) != 0)
      {
        abort();
      }
      return 0;
    }
    case ABCE_OPCODE_ACOS:
    {
      double dbl;
      GETDBL(&dbl, -1);
      POP();
      if (abce_push_double(abce, acos(dbl)) != 0)
      {
        abort();
      }
      return 0;
    }
    case ABCE_OPCODE_ASIN:
    {
      double dbl;
      GETDBL(&dbl, -1);
      POP();
      if (abce_push_double(abce, asin(dbl)) != 0)
      {
        abort();
      }
      return 0;
    }
    case ABCE_OPCODE_ATAN:
    {
      double dbl;
      GETDBL(&dbl, -1);
      POP();
      if (abce_push_double(abce, atan(dbl)) != 0)
      {
        abort();
      }
      return 0;
    }
    case ABCE_OPCODE_EXIT:
    {
      return -EINTR;
    }
    case ABCE_OPCODE_STRGSUB:
    {
      struct abce_mb res, mbhaystack, mbneedle, mbsub;
      int rettmp = abce_getmbstr(&mbhaystack, abce, -3);
      if (rettmp != 0)
      {
        return rettmp;
      }
      rettmp = abce_getmbstr(&mbneedle, abce, -2);
      if (rettmp != 0)
      {
        abce_mb_refdn(abce, &mbhaystack);
        return rettmp;
      }
      rettmp = abce_getmbstr(&mbsub, abce, -1);
      if (rettmp != 0)
      {
        abce_mb_refdn(abce, &mbhaystack);
        abce_mb_refdn(abce, &mbneedle);
        return rettmp;
      }
      POP();
      POP();
      POP();
      rettmp = abce_strgsub_mb(abce, &res, &mbhaystack, &mbneedle, &mbsub);
      if (rettmp != 0)
      {
        abce_mb_refdn(abce, &mbhaystack);
        abce_mb_refdn(abce, &mbneedle);
        abce_mb_refdn(abce, &mbsub);
        return rettmp;
      }
      abce_push_mb(abce, &res);
      abce_mb_refdn(abce, &res);
      abce_mb_refdn(abce, &mbsub);
      abce_mb_refdn(abce, &mbhaystack);
      abce_mb_refdn(abce, &mbneedle);
      return 0;
    }
    case ABCE_OPCODE_IMPORT:
      return -ENOTSUP;
    case ABCE_OPCODE_FUN_HEADER:
    case ABCE_OPCODE_FUN_TRAILER:
      return -EACCES;
    // String functions
    case ABCE_OPCODE_STRSUB:
    {
      struct abce_mb res, mbbase;
      double start, end;

      GETDBL(&end, -1);
      if (end < 0 || (double)(size_t)end != end)
      {
        return -ERANGE;
      }
      GETDBL(&start, -2);
      if (start < 0 || (double)(size_t)start != start || end < start)
      {
        return -ERANGE;
      }
      GETMBSTR(&mbbase, -3);
      if (end > mbbase.u.area->u.str.size)
      {
        abce_mb_refdn(abce, &mbbase);
        return -ERANGE;
      }
      POP();
      POP();
      POP();
      res = abce_mb_create_string(abce,
                                  mbbase.u.area->u.str.buf + (size_t)start,
                                  end - start);
      abce_push_mb(abce, &res);
      abce_mb_refdn(abce, &res);
      abce_mb_refdn(abce, &mbbase);
      return 0;
    }
    case ABCE_OPCODE_STR_FROMCHR:
    case ABCE_OPCODE_STRAPPEND:
    case ABCE_OPCODE_STR_CMP:
    case ABCE_OPCODE_STRLISTJOIN:
    case ABCE_OPCODE_STR_LOWER:
    case ABCE_OPCODE_STR_UPPER:
    case ABCE_OPCODE_STR_REVERSE:
    case ABCE_OPCODE_STRSTR:
    case ABCE_OPCODE_STRREP:
    case ABCE_OPCODE_STRFMT:
    case ABCE_OPCODE_STRSET:
    case ABCE_OPCODE_STRSTRIP:
    case ABCE_OPCODE_STRWORD:
    case ABCE_OPCODE_STRWORDLIST:
    case ABCE_OPCODE_STRWORDCNT:
    // String conversion
    case ABCE_OPCODE_TOSTRING:
    case ABCE_OPCODE_TONUMBER:
    // Misc
    case ABCE_OPCODE_DUP_NONRECURSIVE:
    case ABCE_OPCODE_ERROR:
    case ABCE_OPCODE_OUT:
    case ABCE_OPCODE_LISTSPLICE:
    case ABCE_OPCODE_SCOPE_NEW:
    default:
      return -EILSEQ;
  }
  return ret;
}

int abce_engine(struct abce *abce, unsigned char *addcode, size_t addsz)
{
  // code:
  const size_t guard = 100;
  int ret = -EAGAIN;
  if (addcode != NULL)
  {
    abce->ip = -(int64_t)addsz-(int64_t)guard;
  }
  else
  {
    abce->ip = 0;
  }
  while (ret == -EAGAIN &&
         ((abce->ip >= 0 && (size_t)abce->ip < abce->bytecodesz) ||
         (abce->ip >= -(int64_t)addsz-(int64_t)guard && abce->ip < -(int64_t)guard)))
  {
    uint16_t ins;
    if (abce_fetch_i(&ins, abce, addcode, addsz) != 0)
    {
      ret = -EFAULT;
      break;
    }
    printf("fetched ins %d\n", (int)ins); 
    if (likely(ins < 64))
    {
      switch (ins)
      {
        case ABCE_OPCODE_NOP:
          break;
        case ABCE_OPCODE_PUSH_DBL:
        {
          double dbl;
          if (abce_fetch_d(&dbl, abce, addcode, addsz) != 0)
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
          POP();
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
          POP();
          POP();
          // FIXME off by one?
          if (!((new_ip >= 0 && (size_t)new_ip+10 <= abce->bytecodesz) ||
                (new_ip >= -(int64_t)addsz-(int64_t)guard && new_ip+10 <= -(int64_t)guard)))
          {
            ret = -EFAULT;
            break;
          }
          abce_push_bp(abce);
          abce_push_ip(abce);
          abce->ip = new_ip;
          rettmp = abce_fetch_i(&ins2, abce, addcode, addsz);
          if (rettmp != 0)
          {
            ret = rettmp;
            break;
          }
          if (ins2 != ABCE_OPCODE_FUN_HEADER)
          {
            ret = -EINVAL;
            break;
          }
          double dbl;
          rettmp = abce_fetch_d(&dbl, abce, addcode, addsz);
          if (rettmp != 0)
          {
            ret = rettmp;
            break;
          }
          if (dbl != (double)(uint64_t)argcnt)
          {
            ret = -EINVAL;
            break;
          }
          break;
        }
        case ABCE_OPCODE_POP_MANY:
        {
          double cnt;
          size_t idx;
          GETDBL(&cnt, -1);
          POP();
          for (idx = 0; idx < cnt; idx++)
          {
            int rettmp = abce_pop(abce);
            if (rettmp != 0)
            {
              ret = -EOVERFLOW;
              break;
            }
          }
          break;
        }
        case ABCE_OPCODE_POP:
        {
          int _getdbl_rettmp = abce_pop(abce);
          if (_getdbl_rettmp != 0)
          {
            ret = -EOVERFLOW;
            break;
          }
          break;
        }
        case ABCE_OPCODE_LISTPOP:
        {
          struct abce_mb mbar;
          GETMBAR(&mbar, -1);
          POP();
          if (mbar.u.area->u.ar.size == 0)
          {
            abce_mb_refdn(abce, &mbar);
            ret = -ENOENT;
            break;
          }
          abce_mb_refdn(abce, &mbar.u.area->u.ar.mbs[--mbar.u.area->u.ar.size]);
          abce_mb_refdn(abce, &mbar);
          break;
        }
        case ABCE_OPCODE_LISTLEN:
        {
          struct abce_mb mbar;
          GETMBAR(&mbar, -1);
          POP();
          if (abce_push_double(abce, mbar.u.area->u.ar.size) != 0)
          {
            abort();
          }
          abce_mb_refdn(abce, &mbar);
          break;
        }
        case ABCE_OPCODE_STRLEN:
        {
          struct abce_mb mbstr;
          GETMBSTR(&mbstr, -1);
          POP();
          if (abce_push_double(abce, mbstr.u.area->u.str.size) != 0)
          {
            abort();
          }
          abce_mb_refdn(abce, &mbstr);
          break;
        }
        case ABCE_OPCODE_LISTSET:
        {
          struct abce_mb mbit;
          struct abce_mb mbar;
          double loc;
          int64_t locint;
          // Note the order. MBAR is the one that's most likely to fail.
          GETDBL(&loc, -2);
          GETMBAR(&mbar, -3);
          GETMB(&mbit, -1);
          POP();
          POP();
          POP();
          if (loc != (double)(uint64_t)loc)
          {
            abce_mb_refdn(abce, &mbit);
            abce_mb_refdn(abce, &mbar);
            ret = -EINVAL;
            break;
          }
          locint = loc;
          if (locint < 0 || locint >= mbar.u.area->u.ar.size)
          {
            abce_mb_refdn(abce, &mbit);
            abce_mb_refdn(abce, &mbar);
            ret = -ERANGE;
            break;
          }
          abce_mb_refdn(abce, &mbar.u.area->u.ar.mbs[locint]);
          mbar.u.area->u.ar.mbs[locint] = mbit;
          abce_mb_refdn(abce, &mbar);
          break;
        }
        case ABCE_OPCODE_STRGET:
        {
          struct abce_mb mbstr;
          double loc;
          int64_t locint;
          GETDBL(&loc, -1);
          GETMBSTR(&mbstr, -2);
          POP();
          POP();
          if (loc != (double)(uint64_t)loc)
          {
            ret = -EINVAL;
            break;
          }
          locint = loc;
          if (locint < 0 || locint >= mbstr.u.area->u.str.size)
          {
            ret = -ERANGE;
            break;
          }
          if (abce_push_double(abce, (unsigned char)mbstr.u.area->u.str.buf[locint]) != 0)
          {
            abort();
          }
          abce_mb_refdn(abce, &mbstr);
          break;
        }
        case ABCE_OPCODE_LISTGET:
        {
          struct abce_mb mbar;
          double loc;
          int64_t locint;
          GETDBL(&loc, -1);
          GETMBAR(&mbar, -2);
          POP();
          POP();
          if (loc != (double)(uint64_t)loc)
          {
            ret = -EINVAL;
            break;
          }
          locint = loc;
          if (locint < 0 || locint >= mbar.u.area->u.ar.size)
          {
            ret = -ERANGE;
            break;
          }
          if (abce_push_mb(abce, &mbar.u.area->u.ar.mbs[locint]) != 0)
          {
            abort();
          }
          abce_mb_refdn(abce, &mbar);
          break;
        }
        case ABCE_OPCODE_PUSH_STACK:
        {
          struct abce_mb mb;
          double loc;
          GETDBL(&loc, -1);
          POP();
          if (loc != (double)(uint64_t)loc)
          {
            ret = -EINVAL;
            break;
          }
          GETMB(&mb, loc);
          if (abce_push_mb(abce, &mb) != 0)
          {
            abort();
          }
          abce_mb_refdn(abce, &mb);
          break;
        }
        case ABCE_OPCODE_SET_STACK:
        {
          struct abce_mb mb;
          double loc;
          size_t addr;
          GETDBL(&loc, -2);
          if (loc != (double)(uint64_t)loc)
          {
            ret = -EINVAL;
            break;
          }
          GETMB(&mb, -1);
          POP();
          POP();
          if (abce_calc_addr(&addr, abce, loc) != 0)
          {
            abce_mb_refdn(abce, &mb);
            ret = -EOVERFLOW;
            break;
          }
          abce_mb_refdn(abce, &abce->stackbase[addr]);
          abce->stackbase[addr] = mb;
          break;
        }
        case ABCE_OPCODE_RET:
        {
          struct abce_mb mb;
          printf("ret, stack size %d\n", (int)abce->sp);
          GETIP(-2);
          printf("gotten ip\n");
          GETBP(-3);
          printf("gotten bp\n");
          GETMB(&mb, -1);
          printf("gotten mb\n");
          POP();
          POP();
          POP();
          if (abce_push_mb(abce, &mb) != 0)
          {
            abort();
          }
          abce_mb_refdn(abce, &mb);
          break;
        }
        /* stacktop - cntloc - cntargs - retval - locvar - ip - bp - args */
        case ABCE_OPCODE_RETEX2:
        {
          struct abce_mb mb;
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
          POP(); // cntloc
          POP(); // cntargs
          POP(); // retval
          for (i = 0; i < cntloc; i++)
          {
            POP();
          }
          POP(); // ip
          POP(); // bp
          for (i = 0; i < cntargs; i++)
          {
            POP();
          }
          if (abce_push_mb(abce, &mb) != 0)
          {
            abort();
          }
          abce_mb_refdn(abce, &mb);
          break;
        }
        case ABCE_OPCODE_JMP:
        {
          const size_t guard = 100;
          double d;
          int64_t new_ip;
          GETDBL(&d, -1);
          POP();
          new_ip = d;
          if (!((new_ip >= 0 && (size_t)new_ip <= abce->bytecodesz) ||
                (new_ip >= -(int64_t)addsz-(int64_t)guard && new_ip <= -(int64_t)guard)))
          {
            ret = -EFAULT;
            break;
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
          POP();
          POP();
          new_ip = d;
          if (!((new_ip >= 0 && (size_t)new_ip <= abce->bytecodesz) ||
                (new_ip >= -(int64_t)addsz-(int64_t)guard && new_ip <= -(int64_t)guard)))
          {
            ret = -EFAULT;
            break;
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
          POP();
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
          POP();
          POP();
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
          POP();
          POP();
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
          POP();
          POP();
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
          POP();
          POP();
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
          POP();
          POP();
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
          POP();
          POP();
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
          POP();
          POP();
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
          POP();
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
          POP();
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
          POP();
          POP();
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
          POP();
          POP();
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
          POP();
          POP();
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
          POP();
          POP();
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
          POP();
          POP();
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
          POP();
          POP();
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
          POP();
          POP();
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
          POP();
          POP();
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
          POP();
          POP();
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
          POP();
          POP();
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
          POP();
          POP();
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
          POP();
          if (abce_push_double(abce, -d) != 0)
          {
            abort();
          }
          break;
        }
        case ABCE_OPCODE_PUSH_NEW_ARRAY:
        {
          struct abce_mb mb;
          int rettmp;
          mb = abce_mb_create_array(abce); // FIXME errors
          rettmp = abce_push_mb(abce, &mb);
          if (rettmp != 0)
          {
            ret = rettmp;
            abce_mb_refdn(abce, &mb);
            break;
          }
          abce_mb_refdn(abce, &mb);
          break;
        }
        case ABCE_OPCODE_APPEND_MAINTAIN:
        {
          struct abce_mb mb;
          struct abce_mb mbar;
          GETMBAR(&mbar, -2);
          GETMB(&mb, -1); // can't fail if GETMBAR succeeded
          POP();
          abce_mb_array_append(abce, &mbar, &mb); // FIXME errors
          abce_mb_refdn(abce, &mbar);
          abce_mb_refdn(abce, &mb);
          break;
        }
        case ABCE_OPCODE_PUSH_NEW_DICT:
        {
          struct abce_mb mb;
          int rettmp;
          mb = abce_mb_create_tree(abce); // FIXME errors
          rettmp = abce_push_mb(abce, &mb);
          if (rettmp != 0)
          {
            ret = rettmp;
            abce_mb_refdn(abce, &mb);
            break;
          }
          abce_mb_refdn(abce, &mb);
          break;
        }
        case ABCE_OPCODE_PUSH_FROM_CACHE:
        {
          double dbl;
          int64_t i64;
          GETDBL(&dbl, -1);
          POP();
          if ((double)(uint64_t)dbl != dbl)
          {
            ret = -EINVAL;
            break;
          }
          i64 = dbl;
          if (i64 >= abce->cachesz)
          {
            ret = -EOVERFLOW;
            break;
          }
          if (abce_push_mb(abce, &abce->cachebase[i64]) != 0)
          {
            abort();
          }
          break;
        }
        case ABCE_OPCODE_SCOPEVAR:
        {
          struct abce_mb mbsc, mbit;
          const struct abce_mb *ptr;
          GETMBSC(&mbsc, -1);
          GETMB(&mbit, -2);
          if (mbit.typ != ABCE_T_S)
          {
            abce_mb_refdn(abce, &mbsc);
            abce_mb_refdn(abce, &mbit);
            ret = -EINVAL;
            break;
          }
          POP();
          POP();
          ptr = abce_sc_get_rec_mb(&mbsc, &mbit);
          if (ptr == NULL)
          {
            ret = -ENOENT;
            break;
          }
          if (abce_push_mb(abce, ptr) != 0)
          {
            abort();
          }
          break;
        }
        case ABCE_OPCODE_SCOPE_HAS:
        {
          struct abce_mb mbsc, mbit;
          const struct abce_mb *ptr;
          GETMBSC(&mbsc, -1);
          GETMB(&mbit, -2);
          if (mbit.typ != ABCE_T_S)
          {
            abce_mb_refdn(abce, &mbsc);
            abce_mb_refdn(abce, &mbit);
            ret = -EINVAL;
            break;
          }
          POP();
          POP();
          ptr = abce_sc_get_rec_mb(&mbsc, &mbit);
          if (abce_push_boolean(abce, ptr != NULL) != 0)
          {
            abort();
          }
          break;
        }
        case ABCE_OPCODE_GETSCOPE_DYN:
        {
          if (abce_push_mb(abce, &abce->dynscope) != 0)
          {
            ret = -EOVERFLOW;
            break;
          }
          break;
        }
        case ABCE_OPCODE_TYPE:
        {
          struct abce_mb mb;
          GETMB(&mb, -1);
          POP();
          if (abce_push_double(abce, mb.typ) != 0)
          {
            abort();
          }
          abce_mb_refdn(abce, &mb);
          break;
        }
        case ABCE_OPCODE_APPENDALL_MAINTAIN: // RFE should this be moved elsewhere? A complex operation.
        case ABCE_OPCODE_DICTSET_MAINTAIN:
        {
          struct abce_mb mbt, mbstr;
          struct abce_mb mbval;
          VERIFYMB(-3, ABCE_T_T);
          VERIFYMB(-2, ABCE_T_S);
          GETMB(&mbt, -3);
          GETMB(&mbstr, -2);
          GETMB(&mbval, -1);
          POP();
          POP();
          if (abce_tree_set_str(abce, &mbt, &mbstr, &mbval) != 0)
          {
            ret = -ENOMEM;
            // No break: we want to call all refdn statements
          }
          abce_mb_refdn(abce, &mbt);
          abce_mb_refdn(abce, &mbstr);
          abce_mb_refdn(abce, &mbval);
          break;
        }
        case ABCE_OPCODE_DICTDEL:
        case ABCE_OPCODE_DICTGET:
        {
          struct abce_mb mbt, mbstr;
          struct abce_mb *mbval;
          VERIFYMB(-2, ABCE_T_T);
          VERIFYMB(-1, ABCE_T_S);
          GETMB(&mbt, -2);
          GETMB(&mbstr, -1);
          POP();
          POP();
          if (abce_tree_get_str(abce, &mbval, &mbt, &mbstr) == 0)
          {
            if (abce_push_mb(abce, (const struct abce_mb*)mbval) != 0)
            {
              abort();
            }
          }
          else
          {
            if (abce_push_nil(abce) != 0)
            {
              abort();
            }
          }
          abce_mb_refdn(abce, &mbt);
          abce_mb_refdn(abce, &mbstr);
          break;
        }
        case ABCE_OPCODE_DICTNEXT_SAFE:
        case ABCE_OPCODE_DICTHAS:
        {
          struct abce_mb mbt, mbstr;
          struct abce_mb *mbval;
          VERIFYMB(-2, ABCE_T_T);
          VERIFYMB(-1, ABCE_T_S);
          GETMB(&mbt, -2);
          GETMB(&mbstr, -1);
          POP();
          POP();
          if (abce_push_double(abce, abce_tree_get_str(abce, &mbval, &mbt, &mbstr) == 0) != 0)
          {
            abort();
          }
          abce_mb_refdn(abce, &mbt);
          abce_mb_refdn(abce, &mbstr);
          break;
        }
        case ABCE_OPCODE_SCOPEVAR_SET:
        case ABCE_OPCODE_CALL_IF_FUN:
        case ABCE_OPCODE_DICTLEN: // RFE should this be moved elsewhere? A complex operation.
        default:
        {
          printf("Invalid instruction %d\n", (int)ins);
          ret = EILSEQ;
          break;
        }
      }
    }
    else if (likely(ins < 128))
    {
      int ret2 = abce->trap(abce, ins, addcode, addsz);
      if (ret2 != 0)
      {
        ret = ret2;
        break;
      }
    }
    else if (likely(ins < 0x400))
    {
      int ret2 = abce_mid(abce, ins, addcode, addsz);
      if (ret2 != 0)
      {
        ret = ret2;
        break;
      }
    }
    else if (likely(ins < 0x800))
    {
      int ret2 = abce->trap(abce, ins, addcode, addsz);
      if (ret2 != 0)
      {
        ret = ret2;
        break;
      }
    }
    else if (likely(ins < 0x8000))
    {
      printf("long\n");
      abort();
    }
    else
    {
      int ret2 = abce->trap(abce, ins, addcode, addsz);
      if (ret2 != 0)
      {
        ret = ret2;
        break;
      }
    }
  }
  if (ret == -EAGAIN)
  {
    ret = 0;
  }
  if (ret == -EINTR)
  {
    ret = 0;
  }
  return ret;
}
