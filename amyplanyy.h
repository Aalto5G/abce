#ifndef _AMYPLANYY_H_
#define _AMYPLANYY_H_

#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include "abce.h"
#include "amyplanlocvarctx.h"

#ifdef __cplusplus
extern "C" {
#endif

struct amyplan_escaped_string {
  size_t sz;
  char *str;
};

struct amyplan_csnippet {
  char *data;
  size_t len;
  size_t capacity;
};

static inline void amyplan_csadd(struct amyplan_csnippet *cs, char ch)
{
  if (cs->len + 2 >= cs->capacity)
  {
    size_t new_capacity = cs->capacity * 2 + 2;
    cs->data = (char*)realloc(cs->data, new_capacity);
    cs->capacity = new_capacity;
  }
  cs->data[cs->len] = ch;
  cs->data[cs->len + 1] = '\0';
  cs->len++;
}

static inline void amyplan_csaddstr(struct amyplan_csnippet *cs, char *str)
{
  size_t len = strlen(str);
  if (cs->len + len + 1 >= cs->capacity)
  {
    size_t new_capacity = cs->capacity * 2 + 2;
    if (new_capacity < cs->len + len + 1)
    {
      new_capacity = cs->len + len + 1;
    }
    cs->data = (char*)realloc(cs->data, new_capacity);
    cs->capacity = new_capacity;
  }
  memcpy(cs->data + cs->len, str, len);
  cs->len += len;
  cs->data[cs->len] = '\0';
}

struct amyplanyy {
  void *baton;
  struct abce abce;
  struct amyplan_locvarctx *ctx;
};

static inline void amyplanyy_init(struct amyplanyy *yy)
{
  abce_init(&yy->abce);
  yy->ctx = NULL;
}

// static inline void amyplanyy_get_parent_scope(

static inline size_t amyplan_symbol_add(struct amyplanyy *amyplanyy, const char *symbol, size_t symlen)
{
  return abce_cache_add_str(&amyplanyy->abce, symbol, symlen);
}
static inline size_t amyplanyy_add_fun_sym(struct amyplanyy *amyplanyy, const char *symbol, int maybe, size_t loc)
{
  struct abce_mb mb;
  mb.typ = ABCE_T_F;
  mb.u.d = loc;
  abce_sc_put_val_str(&amyplanyy->abce, &amyplanyy->abce.dynscope, symbol, &mb);
  return (size_t)-1;
}

static inline void amyplanyy_add_byte(struct amyplanyy *amyplanyy, uint16_t ins)
{
  abce_add_ins(&amyplanyy->abce, ins);
}

static inline void amyplanyy_add_double(struct amyplanyy *amyplanyy, double dbl)
{
  abce_add_double(&amyplanyy->abce, dbl);
}

static inline void amyplanyy_set_double(struct amyplanyy *amyplanyy, size_t i, double dbl)
{
  abce_set_double(&amyplanyy->abce, i, dbl);
}

static inline void amyplanyy_free(struct amyplanyy *amyplanyy)
{
  abce_free(&amyplanyy->abce);
  //free(amyplanyy->bytecode);
  memset(amyplanyy, 0, sizeof(*amyplanyy));
}

#ifdef __cplusplus
};
#endif

#endif
