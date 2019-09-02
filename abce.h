#ifndef _ABCE_H_
#define _ABCE_H_

#include "abcedatatypes.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef WITH_LUA
void mb_to_lua(lua_State *lua, const struct abce_mb *mb);
void mb_from_lua(lua_State *lua, struct abce *abce, int idx);
#endif

void *abce_std_alloc(void *old, size_t oldsz, size_t newsz, void **pbaton);

void *abce_jm_alloc(void *old, size_t oldsz, size_t newsz, void **pbaton);

static inline int is_little_endian(void)
{
  double d;
  memcpy(&d, "\x00\x00\x00\x00\x00\x00I@", 8);
  if (d == 50.0)
  {
    return 1;
  }
  memcpy(&d, "@I\x00\x00\x00\x00\x00\x00", 8);
  if (d == 50.0)
  {
    return 0;
  }
  abort(); // Not IEEE 754
}

static inline uint16_t abce_bswap16(uint16_t u16)
{
  u16 =
    ((u16 & 0xFF00U) >>  8) |
    ((u16 & 0x00FFU) <<  8);
  return u16;
}

static inline uint32_t abce_bswap32(uint32_t u32)
{
  u32 =
    ((u32 & 0xFF000000U) >> 24) |
    ((u32 & 0x00FF0000U) >>  8) |
    ((u32 & 0x0000FF00U) <<  8) |
    ((u32 & 0x000000FFU) << 24);
  return u32;
}

static inline void abce_put_dbl(double dbl, void *dest)
{
  uint64_t u64;
  memcpy(&u64, &dbl, sizeof(u64));
  if (is_little_endian())
  {
    u64 =
      ((u64 & 0xFF00000000000000ULL) >> 56) |
      ((u64 & 0x00FF000000000000ULL) >> 40) |
      ((u64 & 0x0000FF0000000000ULL) >> 24) |
      ((u64 & 0x000000FF00000000ULL) >>  8) |
      ((u64 & 0x00000000FF000000ULL) <<  8) |
      ((u64 & 0x0000000000FF0000ULL) << 24) |
      ((u64 & 0x000000000000FF00ULL) << 40) |
      ((u64 & 0x00000000000000FFULL) << 56);
  }
  memcpy(dest, &u64, sizeof(u64));
}

static inline int abce_set_double(struct abce *abce, size_t idx, double dbl)
{
  if (idx + 8 > abce->bytecodesz)
  {
    return -EFAULT;
  }
  abce_put_dbl(dbl, &abce->bytecode[idx]);
  return 0;
}

static inline int
abce_add_double_alt(void *bcode, size_t *bsz, size_t cap, double dbl)
{
  unsigned char *bytecode = (unsigned char*)bcode;
  if ((*bsz) + 8 > cap)
  {
    return -EFAULT;
  }
  abce_put_dbl(dbl, &bytecode[*bsz]);
  (*bsz) += 8;
  return 0;
}

static inline int abce_add_double(struct abce *abce, double dbl)
{
  return abce_add_double_alt(
    abce->bytecode, &abce->bytecodesz, abce->bytecodecap, dbl);
}

void abce_mb_gc_refdn(struct abce *abce, struct abce_mb_area *mba, enum abce_type typ);

void abce_mb_do_arearefdn(struct abce *abce, struct abce_mb_area **mbap, enum abce_type typ);

static inline void abce_mb_arearefdn(struct abce *abce, struct abce_mb_area **mbap, enum abce_type typ)
{
  struct abce_mb_area *mba = *mbap;
  if (mba == NULL)
  {
    return;
  }
  if (!--mba->refcnt)
  {
    abce_mb_do_arearefdn(abce, mbap, typ);
  }
}

static inline int abce_is_dynamic_type(enum abce_type typ)
{
  switch (typ)
  {
    case ABCE_T_T:
    case ABCE_T_IOS:
    case ABCE_T_A:
    case ABCE_T_S:
    case ABCE_T_PB:
    case ABCE_T_SC:
      return 1;
    default:
      return 0;
  }
}


static inline void abce_mb_refdn_typ(struct abce *abce, struct abce_mb *mb, enum abce_type typ)
{
  if (abce_is_dynamic_type(typ))
  {
    abce_mb_arearefdn(abce, &mb->u.area, mb->typ);
  }
  mb->typ = ABCE_T_N;
  mb->u.d = 0.0;
  mb->u.area = NULL;
}

static inline void abce_mb_refdn(struct abce *abce, struct abce_mb *mb)
{
  if (abce_is_dynamic_type(mb->typ))
  {
    abce_mb_arearefdn(abce, &mb->u.area, mb->typ);
  }
  mb->typ = ABCE_T_N;
  mb->u.d = 0.0;
  mb->u.area = NULL;
}

void
abce_mb_refdn_noinline(struct abce *abce, struct abce_mb *mb);

struct abce_mb
abce_mb_refup_noinline(struct abce *abce, const struct abce_mb *mb);

static inline struct abce_mb
abce_mb_refup(struct abce *abce, const struct abce_mb *mb)
{
  if (abce_is_dynamic_type(mb->typ))
  {
    mb->u.area->refcnt++;
  }
  return *mb;
}

static inline int64_t abce_cache_add(struct abce *abce, const struct abce_mb *mb)
{
  int64_t res;
  if (abce->cachesz >= abce->cachecap)
  {
    return -EOVERFLOW;
  }
  res = abce->cachesz;
  abce->cachebase[abce->cachesz++] = abce_mb_refup(abce, mb);
  return res;
}


static inline int abce_pop(struct abce *abce)
{
  struct abce_mb *mb;
  if (abce->sp == 0 || abce->sp <= abce->bp)
  {
    abce->err.code = ABCE_E_STACK_UNDERFLOW;
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
    if (abce_unlikely(addr >= abce->sp || addr < abce->bp))
    {
      abce->err.code = ABCE_E_STACK_IDX_OOB;
      abce->err.mb.typ = ABCE_T_D;
      abce->err.mb.u.d = idx;
      return -EOVERFLOW;
    }
  }
  else
  {
    addr = abce->bp + idx;
    if (abce_unlikely(addr >= abce->sp || addr < abce->bp))
    {
      abce->err.code = ABCE_E_STACK_IDX_OOB;
      abce->err.mb.typ = ABCE_T_D;
      abce->err.mb.u.d = idx;
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
  if (abce_unlikely(mb->typ != ABCE_T_D && mb->typ != ABCE_T_B))
  {
    abce->err.code = ABCE_E_EXPECT_BOOL;
    abce->err.mb = abce_mb_refup_noinline(abce, mb);
    abce->err.val2 = idx;
    return -EINVAL;
  }
  *b = !!mb->u.d;
  return 0;
}

static inline int abce_verifymb(struct abce *abce, int64_t idx, enum abce_type typ)
{
  const struct abce_mb *mb;
  size_t addr;
  if (abce_calc_addr(&addr, abce, idx) != 0)
  {
    return -EOVERFLOW;
  }
  mb = &abce->stackbase[addr];
  if (abce_unlikely(mb->typ != typ))
  {
    abce->err.code = (enum abce_errcode)typ; // Same numbers valid for both
    abce->err.mb = abce_mb_refup_noinline(abce, mb);
    abce->err.val2 = idx;
    return -EINVAL;
  }
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
  if (abce_unlikely(mb->typ != ABCE_T_F))
  {
    abce->err.code = ABCE_E_EXPECT_FUNC;
    abce->err.mb = abce_mb_refup_noinline(abce, mb);
    abce->err.val2 = idx;
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
  if (abce_unlikely(mb->typ != ABCE_T_BP))
  {
    abce->err.code = ABCE_E_EXPECT_BP;
    abce->err.mb = abce_mb_refup_noinline(abce, mb);
    abce->err.val2 = idx;
    return -EINVAL;
  }
  trial = mb->u.d;
  if (abce_unlikely(trial != mb->u.d))
  {
    abce->err.code = ABCE_E_REG_NOT_INT;
    abce->err.mb = abce_mb_refup_noinline(abce, mb);
    abce->err.val2 = idx;
    return -EINVAL;
  }
  abce->bp = trial;
  return 0;
}

static inline int abce_getip(struct abce *abce, int64_t idx)
{
  const struct abce_mb *mb;
  size_t addr;
  int64_t trial;
  if (abce_calc_addr(&addr, abce, idx) != 0)
  {
    return -EOVERFLOW;
  }
  mb = &abce->stackbase[addr];
  if (abce_unlikely(mb->typ != ABCE_T_IP))
  {
    //printf("invalid typ: %d\n", mb->typ);
    abce->err.code = ABCE_E_EXPECT_IP;
    abce->err.mb = abce_mb_refup_noinline(abce, mb);
    abce->err.val2 = idx;
    return -EINVAL;
  }
  trial = mb->u.d;
  if (abce_unlikely(trial != mb->u.d))
  {
    abce->err.code = ABCE_E_REG_NOT_INT;
    abce->err.mb = abce_mb_refup_noinline(abce, mb);
    abce->err.val2 = idx;
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
static inline int abce_getmbtyped(struct abce_mb *mb, struct abce *abce, int64_t idx, enum abce_type typ)
{
  const struct abce_mb *mbptr;
  size_t addr;
  if (abce_calc_addr(&addr, abce, idx) != 0)
  {
    return -EOVERFLOW;
  }
  mbptr = &abce->stackbase[addr];
  if (abce_unlikely(mbptr->typ != typ))
  {
    abce->err.code = (enum abce_errcode)typ; // Same numbers valid for both
    abce->err.mb = abce_mb_refup_noinline(abce, mbptr);
    abce->err.val2 = idx;
    return -EINVAL;
  }
  *mb = abce_mb_refup(abce, mbptr);
  return 0;
}
static inline int abce_getmbsc(struct abce_mb *mb, struct abce *abce, int64_t idx)
{
  return abce_getmbtyped(mb, abce, idx, ABCE_T_SC);
}
static inline int abce_getmbar(struct abce_mb *mb, struct abce *abce, int64_t idx)
{
  return abce_getmbtyped(mb, abce, idx, ABCE_T_A);
}
static inline int abce_getmbpb(struct abce_mb *mb, struct abce *abce, int64_t idx)
{
  return abce_getmbtyped(mb, abce, idx, ABCE_T_PB);
}
static inline int abce_getmbstr(struct abce_mb *mb, struct abce *abce, int64_t idx)
{
  return abce_getmbtyped(mb, abce, idx, ABCE_T_S);
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
  //printf("addr %d\n", (int)addr);
  mb = &abce->stackbase[addr];
  if (abce_unlikely(mb->typ != ABCE_T_D && mb->typ != ABCE_T_B))
  {
    abce->err.code = ABCE_E_EXPECT_DBL;
    abce->err.mb = abce_mb_refup_noinline(abce, mb);
    abce->err.val2 = idx;
    return -EINVAL;
  }
  *d = mb->u.d;
  return 0;
}

static inline int abce_push_mb(struct abce *abce, const struct abce_mb *mb)
{
  if (abce_unlikely(abce->sp >= abce->stacklimit))
  {
    abce->err.code = ABCE_E_STACK_OVERFLOW;
    abce->err.mb = abce_mb_refup_noinline(abce, mb);
    return -EOVERFLOW;
  }
  abce->stackbase[abce->sp] = abce_mb_refup(abce, mb);
  abce->sp++;
  return 0;
}

static inline int abce_push_boolean(struct abce *abce, int boolean)
{
  if (abce_unlikely(abce->sp >= abce->stacklimit))
  {
    abce->err.code = ABCE_E_STACK_OVERFLOW;
    abce->err.mb.typ = ABCE_T_B;
    abce->err.mb.u.d = boolean;
    return -EOVERFLOW;
  }
  abce->stackbase[abce->sp].typ = ABCE_T_B;
  abce->stackbase[abce->sp].u.d = boolean ? 1.0 : 0.0;
  abce->sp++;
  return 0;
}

static inline int abce_push_nil(struct abce *abce)
{
  if (abce_unlikely(abce->sp >= abce->stacklimit))
  {
    abce->err.code = ABCE_E_STACK_OVERFLOW;
    abce->err.mb.typ = ABCE_T_N;
    return -EOVERFLOW;
  }
  abce->stackbase[abce->sp].typ = ABCE_T_N;
  abce->sp++;
  return 0;
}

static inline int abce_push_ip(struct abce *abce)
{
  if (abce_unlikely(abce->sp >= abce->stacklimit))
  {
    abce->err.code = ABCE_E_STACK_OVERFLOW;
    abce->err.mb.typ = ABCE_T_IP;
    abce->err.mb.u.d = abce->ip;
    return -EOVERFLOW;
  }
  abce->stackbase[abce->sp].typ = ABCE_T_IP;
  abce->stackbase[abce->sp].u.d = abce->ip;
  abce->sp++;
  return 0;
}
static inline int abce_push_rg(struct abce *abce)
{
  if (abce_unlikely(abce->sp >= abce->stacklimit))
  {
    abce->err.code = ABCE_E_STACK_OVERFLOW;
    abce->err.mb.typ = ABCE_T_RG;
    abce->err.mb.u.d = 0;
    return -EOVERFLOW;
  }
  abce->stackbase[abce->sp].typ = ABCE_T_RG;
  abce->stackbase[abce->sp].u.d = 0;
  abce->sp++;
  return 0;
}
static inline int abce_push_bp(struct abce *abce)
{
  if (abce_unlikely(abce->sp >= abce->stacklimit))
  {
    abce->err.code = ABCE_E_STACK_OVERFLOW;
    abce->err.mb.typ = ABCE_T_BP;
    abce->err.mb.u.d = abce->bp;
    return -EOVERFLOW;
  }
  abce->stackbase[abce->sp].typ = ABCE_T_BP;
  abce->stackbase[abce->sp].u.d = abce->bp;
  abce->sp++;
  return 0;
}
static inline int abce_push_double(struct abce *abce, double dbl)
{
  if (abce_unlikely(abce->sp >= abce->stacklimit))
  {
    abce->err.code = ABCE_E_STACK_OVERFLOW;
    abce->err.mb.typ = ABCE_T_D;
    abce->err.mb.u.d = dbl;
    return -EOVERFLOW;
  }
  abce->stackbase[abce->sp].typ = ABCE_T_D;
  abce->stackbase[abce->sp].u.d = dbl;
  abce->sp++;
  return 0;
}
static inline int abce_push_fun(struct abce *abce, double fun_addr)
{
  if (abce_unlikely(abce->sp >= abce->stacklimit))
  {
    abce->err.code = ABCE_E_STACK_OVERFLOW;
    abce->err.mb.typ = ABCE_T_F;
    abce->err.mb.u.d = fun_addr;
    return -EOVERFLOW;
  }
  if (abce_unlikely((double)(int64_t)fun_addr != fun_addr))
  {
    abce->err.code = ABCE_E_FUNADDR_NOT_INT;
    abce->err.mb.typ = ABCE_T_F;
    abce->err.mb.u.d = fun_addr;
    return -EINVAL;
  }
  abce->stackbase[abce->sp].typ = ABCE_T_F;
  abce->stackbase[abce->sp].u.d = fun_addr;
  abce->sp++;
  return 0;
}

static inline int
abce_add_ins_alt(void *bcode, size_t *bsz, size_t cap, uint16_t ins)
{
  unsigned char *bytecode = (unsigned char*)bcode;
  if (ins >= 2048)
  {
    if ((*bsz) + 3 > cap)
    {
      return -EFAULT;
    }
    bytecode[(*bsz)++] = (ins>>12) | 0xE0;
    bytecode[(*bsz)++] = ((ins>>6)&0x3F) | 0x80;
    bytecode[(*bsz)++] = ((ins)&0x3F) | 0x80;
    return 0;
  }
  else if (ins >= 128)
  {
    if ((*bsz) + 2 > cap)
    {
      return -EFAULT;
    }
    bytecode[(*bsz)++] = ((ins>>6)) | 0xC0;
    bytecode[(*bsz)++] = ((ins)&0x3F) | 0x80;
    return 0;
  }
  else
  {
    if ((*bsz) >= cap)
    {
      return -EFAULT;
    }
    bytecode[(*bsz)++] = ins;
    return 0;
  }
}

static inline int abce_add_ins(struct abce *abce, uint16_t ins)
{
  return abce_add_ins_alt(abce->bytecode, &abce->bytecodesz, abce->bytecodecap,
                          ins);
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
  if (!abce_is_dynamic_type(mb->typ))
  {
    abort();
  }
  mb->u.area->refcnt++;
  return mb->u.area;
}

static inline struct abce_mb
abce_mb_refuparea(struct abce *abce, struct abce_mb_area *mba,
                  enum abce_type typ)
{
  struct abce_mb mb = {};
  if (mba == NULL)
  {
    mb.typ = ABCE_T_N;
    mb.u.d = 0;
    return mb;
  }
  if (!abce_is_dynamic_type(typ))
  {
    abort();
  }
  mb.typ = typ;
  mb.u.area = mba;
  mba->refcnt++;
  return mb;
}

struct abce_mb abce_mb_create_string(struct abce *abce, const char *str, size_t sz);

struct abce_mb abce_mb_create_string_to_be_filled(struct abce *abce, size_t sz);

struct abce_mb abce_mb_concat_string(struct abce *abce, const char *str1, size_t sz1,
                                     const char *str2, size_t sz2);

struct abce_mb abce_mb_rep_string(struct abce *abce, const char *str1, size_t sz1,
                                  size_t cnt);

static inline struct abce_mb
abce_mb_create_string_nul(struct abce *abce, const char *str)
{
  return abce_mb_create_string(abce, str, strlen(str));
}

static inline uint32_t abce_str_hash(const char *str)
{
  size_t len = strlen(str);
  return abce_murmur_buf(0x12345678U, str, len);
}
static inline uint32_t abce_str_len_hash(const struct abce_const_str_len *str_len)
{
  size_t len = str_len->len;
  return abce_murmur_buf(0x12345678U, str_len->str, len);
}
static inline uint32_t abce_mb_str_hash(const struct abce_mb *mb)
{
  if (mb->typ != ABCE_T_S)
  {
    abort();
  }
  return abce_murmur_buf(0x12345678U, mb->u.area->u.str.buf, mb->u.area->u.str.size);
}
static inline int abce_str_cache_cmp_asymlen(const struct abce_const_str_len *str_len, struct abce_rb_tree_node *n2, void *ud)
{
  struct abce_mb_string *e = ABCE_CONTAINER_OF(n2, struct abce_mb_string, node);
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
static inline int abce_str_cmp_asym(const char *str, struct abce_rb_tree_node *n2, void *ud)
{
  struct abce_mb_rb_entry *e = ABCE_CONTAINER_OF(n2, struct abce_mb_rb_entry, n);
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
  const struct abce_mb *key, struct abce_rb_tree_node *n2, void *ud)
{
  struct abce_mb_rb_entry *e2 = ABCE_CONTAINER_OF(n2, struct abce_mb_rb_entry, n);
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
  struct abce_rb_tree_node *n1, struct abce_rb_tree_node *n2, void *ud)
{
  struct abce_mb_string *e1 = ABCE_CONTAINER_OF(n1, struct abce_mb_string, node);
  struct abce_mb_string *e2 = ABCE_CONTAINER_OF(n2, struct abce_mb_string, node);
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

static inline int abce_str_cmp_sym_mb(
  const struct abce_mb *mb1, const struct abce_mb *mb2)
{
  size_t len1, len2, lenmin;
  int ret;
  const char *str1, *str2;
  if (mb1->typ != ABCE_T_S || mb2->typ != ABCE_T_S)
  {
    abort();
  }
  len1 = mb1->u.area->u.str.size;
  str1 = mb1->u.area->u.str.buf;
  len2 = mb2->u.area->u.str.size;
  str2 = mb2->u.area->u.str.buf;
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
  struct abce_rb_tree_node *n1, struct abce_rb_tree_node *n2, void *ud)
{
  struct abce_mb_rb_entry *e1 = ABCE_CONTAINER_OF(n1, struct abce_mb_rb_entry, n);
  struct abce_mb_rb_entry *e2 = ABCE_CONTAINER_OF(n2, struct abce_mb_rb_entry, n);
  if (e1->key.typ != ABCE_T_S || e2->key.typ != ABCE_T_S)
  {
    abort();
  }
  return abce_str_cmp_sym_mb(&e1->key, &e2->key);
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
  struct abce_rb_tree_node *n;
  if (key->typ != ABCE_T_S)
  {
    abort();
  }
  hashval = abce_mb_str_hash(key);
  hashloc = hashval & (mba->u.sc.size - 1);
  n = ABCE_RB_TREE_NOCMP_FIND(&mba->u.sc.heads[hashloc], abce_str_cmp_halfsym, NULL, key);
  if (n == NULL)
  {
    return NULL;
  }
  return &ABCE_CONTAINER_OF(n, struct abce_mb_rb_entry, n)->val;
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
  struct abce_rb_tree_node *n;
  hashval = abce_str_hash(str);
  hashloc = hashval & (mba->u.sc.size - 1);
  n = ABCE_RB_TREE_NOCMP_FIND(&mba->u.sc.heads[hashloc], abce_str_cmp_asym, NULL, str);
  if (n == NULL)
  {
    return NULL;
  }
  return &ABCE_CONTAINER_OF(n, struct abce_mb_rb_entry, n)->val;
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
  const struct abce_mb_area *mba, const struct abce_mb *it, int rec);

static inline const struct abce_mb *
abce_sc_get_rec_mb(const struct abce_mb *mb, const struct abce_mb *it, int rec)
{
  if (mb->typ != ABCE_T_SC)
  {
    abort();
  }
  return abce_sc_get_rec_mb_area(mb->u.area, it, rec);
}

const struct abce_mb *abce_sc_get_rec_str_area(
  const struct abce_mb_area *mba, const char *str, int rec);

static inline const struct abce_mb *
abce_sc_get_rec_str(const struct abce_mb *mb, const char *str, int rec)
{
  if (mb->typ != ABCE_T_SC)
  {
    abort();
  }
  return abce_sc_get_rec_str_area(mb->u.area, str, rec);
}

static inline int64_t
abce_sc_get_rec_str_fun(const struct abce_mb *mb, const char *str, int rec)
{
  const struct abce_mb *mb2 = abce_sc_get_rec_str(mb, str, rec);
  if (mb2 == NULL || mb2->typ != ABCE_T_F)
  {
    abort();
  }
  return mb2->u.d;
}

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

void abce_mb_arearefdn(struct abce *abce, struct abce_mb_area **mbap, enum abce_type typ);

struct abce_dump_list {
  struct abce_dump_list *parent;
  struct abce_mb_area *area;
};

void abce_mb_dump_impl(const struct abce_mb *mb, struct abce_dump_list *ll);

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

int abce_mb_array_append_grow(struct abce *abce, struct abce_mb *mb);

static inline int
abce_mb_array_append(struct abce *abce,
                      struct abce_mb *mb, const struct abce_mb *it)
{
  if (mb->typ != ABCE_T_A)
  {
    abort();
  }
  if (mb->u.area->u.ar.size >= mb->u.area->u.ar.capacity)
  {
    if (abce_mb_array_append_grow(abce, mb) != 0)
    {
      return -ENOMEM;
    }
  }
  mb->u.area->u.ar.mbs[mb->u.area->u.ar.size++] = abce_mb_refup(abce, it);
  return 0;
}

void abce_mb_treedump(const struct abce_rb_tree_node *n, int *first,
                      struct abce_dump_list *ll);

void abce_dump_str(const char *str, size_t sz);

static inline void abce_mb_dump(const struct abce_mb *mb)
{
  abce_mb_dump_impl(mb, NULL);
  printf("\n");
}

struct abce_mb *abce_alloc_stack(struct abce *abce, size_t limit);

void abce_free_stack(struct abce *abce, struct abce_mb *stackbase, size_t limit);

unsigned char *abce_alloc_bcode(struct abce *abce, size_t limit);

void abce_free_bcode(struct abce *abce, unsigned char *bcodebase, size_t limit);

void abce_init_opts(struct abce *abce, int map_shared);

static inline void abce_init(struct abce *abce)
{
  abce_init_opts(abce, 0);
}

void abce_free_bt(struct abce *abce);

void abce_free(struct abce *abce);

enum {
  ABCE_GUARD = 100,
};

static inline int
abce_fetch_b(uint8_t *b, struct abce *abce, unsigned char *addcode, size_t addsz)
{
  const size_t guard = ABCE_GUARD;
  if (abce_unlikely(!((abce->ip >= 0 && (size_t)abce->ip < abce->bytecodesz) ||
        (abce->ip >= -(int64_t)addsz-(int64_t)guard && abce->ip < -(int64_t)guard))))
  {
    abce->err.code = ABCE_E_BYTECODE_FAULT;
    abce->err.mb.typ = ABCE_T_D;
    abce->err.mb.u.d = abce->ip;
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
static inline void abce_get_dbl(double *dbl, const void *src)
{
  uint64_t u64;
  memcpy(&u64, src, sizeof(u64));
  if (is_little_endian())
  {
    u64 =
      ((u64 & 0xFF00000000000000ULL) >> 56) |
      ((u64 & 0x00FF000000000000ULL) >> 40) |
      ((u64 & 0x0000FF0000000000ULL) >> 24) |
      ((u64 & 0x000000FF00000000ULL) >>  8) |
      ((u64 & 0x00000000FF000000ULL) <<  8) |
      ((u64 & 0x0000000000FF0000ULL) << 24) |
      ((u64 & 0x000000000000FF00ULL) << 40) |
      ((u64 & 0x00000000000000FFULL) << 56);
  }
  memcpy(dbl, &u64, sizeof(u64));
}


static inline int
abce_fetch_d(double *d, struct abce *abce, unsigned char *addcode, size_t addsz)
{
  const size_t guard = ABCE_GUARD;
  if (abce_unlikely(!((abce->ip >= 0 && (size_t)abce->ip+8 <= abce->bytecodesz) ||
        (abce->ip >= -(int64_t)addsz-(int64_t)guard && abce->ip+8 <= -(int64_t)guard))))
  {
    abce->err.code = ABCE_E_BYTECODE_FAULT;
    abce->err.mb.typ = ABCE_T_D;
    abce->err.mb.u.d = abce->ip;
    return -EFAULT;
  }
  if (abce->ip >= 0)
  {
    abce_get_dbl(d, abce->bytecode + abce->ip);
    abce->ip += 8;
    return 0;
  }
  abce_get_dbl(d, addcode + abce->ip + guard + addsz);
  abce->ip += 8;
  return 0;
}

int
abce_fetch_i_tail(uint8_t ophi, uint16_t *ins, struct abce *abce, unsigned char *addcode, size_t addsz);

static inline int
abce_fetch_i(uint16_t *ins, struct abce *abce, unsigned char *addcode, size_t addsz)
{
  uint8_t ophi;
  if (abce_fetch_b(&ophi, abce, addcode, addsz) != 0)
  {
    return -EFAULT;
  }
  if (abce_likely(ophi < 128))
  {
    *ins = ophi;
    return 0;
  }
  return abce_fetch_i_tail(ophi, ins, abce, addcode, addsz);
}

struct abce_mb abce_mb_create_tree(struct abce *abce);

struct abce_mb abce_mb_create_pb(struct abce *abce);

struct abce_mb abce_mb_create_pb_from_buf(struct abce *abce, const void *buf, size_t sz);

struct abce_mb abce_mb_create_array(struct abce *abce);

int
abce_mid(struct abce *abce, uint16_t ins, unsigned char *addcode, size_t addsz);

int abce_engine(struct abce *abce, unsigned char *addcode, size_t addsz);

int abce_mb_pb_do_resize(struct abce *abce, const struct abce_mb *mbpb, size_t newsz);

static inline int abce_mb_pb_resize(struct abce *abce, const struct abce_mb *mbpb,
                                    size_t newsz)
{
  if (mbpb->typ != ABCE_T_PB)
  {
    abort();
  }
  if (newsz <= mbpb->u.area->u.pb.size)
  {
    mbpb->u.area->u.pb.size = newsz;
    return 0;
  }
  if (newsz <= mbpb->u.area->u.pb.capacity)
  {
    memset(&mbpb->u.area->u.pb.buf[mbpb->u.area->u.pb.size], 0,
           newsz - mbpb->u.area->u.pb.size);
    mbpb->u.area->u.pb.size = newsz;
    return 0;
  }
  return abce_mb_pb_do_resize(abce, mbpb, newsz);
}

static inline void abce_err_init(struct abce_err *err)
{
  err->code = ABCE_E_NONE;
  err->opcode = ABCE_OPCODE_NOP;
  err->mb.typ = ABCE_T_N;
}

static inline void abce_err_free(struct abce *abce, struct abce_err *err)
{
  abce_mb_refdn(abce, &err->mb);
  err->code = ABCE_E_NONE;
  err->opcode = ABCE_OPCODE_NOP;
  err->mb.typ = ABCE_T_N;
}

void abce_free_gcblock_one(struct abce *abce, size_t locidx);

void abce_maybe_mv_obj_to_scratch_tail(struct abce *abce, const struct abce_mb *obj);

static inline void abce_maybe_mv_obj_to_scratch(struct abce *abce, const struct abce_mb *obj)
{
  struct abce_mb_area *mba;

  if (!abce_is_dynamic_type(obj->typ))
  {
    return; // static type
  }
  mba = obj->u.area;
  if (mba->refcnt == 0) // already freed
  {
    abort();
    return;
  }
  if (--mba->refcnt)
  {
    return;
  }
  abce->gcblockbase[mba->locidx] = abce->gcblockbase[--abce->gcblocksz];
  abce->gcblockbase[mba->locidx].u.area->locidx = mba->locidx;

  switch (obj->typ)
  {
    case ABCE_T_IOS:
    case ABCE_T_S:
    case ABCE_T_PB:
      abce_maybe_mv_obj_to_scratch_tail(abce, obj);
      return;
    case ABCE_T_T:
    case ABCE_T_A:
    case ABCE_T_SC:
      break;
    default:
      abort();
  }

  abce->gcblockbase[--abce->scratchstart] = *obj;
}

void abce_mb_gc_free(struct abce *abce, struct abce_mb_area *mba, enum abce_type typ);

static inline void abce_setup_mb_for_gc(struct abce *abce, struct abce_mb_area *mba, enum abce_type typ)
{
  size_t locidx = abce->gcblocksz;
  if (locidx >= abce->gcblockcap)
  {
    abort();
  }
  abce->gcblockbase[locidx].typ = typ;
  abce->gcblockbase[locidx].u.area = mba;
  mba->locidx = locidx;
  abce->gcblocksz++;
}

void abce_gc(struct abce *abce);

void abce_compact(struct abce *abce);

const char *abce_err_to_str(enum abce_errcode code);

#ifdef WITH_LUA
int abce_ensure_lua(struct abce_mb_area *mba, struct abce *abce);
#endif

#ifdef __cplusplus
};
#endif

#endif
