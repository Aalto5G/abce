#include "abceapi.h"
#include "abce.h"

#include <stddef.h>
#include <stdlib.h>

// FIXME restrictor functions
struct abce *abceapi_new(void)
{
  struct abce *res = malloc(sizeof(struct abce));
  if (res == NULL)
  {
    return NULL;
  }
  abce_init(res);
  return res;
}

void abceapi_free(struct abce *abce)
{
  if (abce == NULL)
  {
    return;
  }
  abce_free(abce);
  free(abce);
}

int abceapi_isvalid(struct abce *abce, int stackidx)
{
  int res;
  struct abce_err err_old = abce->err;
  res = abce_verifyaddr(abce, stackidx) == 0;
  abce->err = err_old;
  return res;
}

int abceapi_istree(struct abce *abce, int stackidx)
{
  int res;
  struct abce_err err_old = abce->err;
  res = abce_verifymb(abce, stackidx, ABCE_T_T) == 0;
  abce->err = err_old;
  return res;
}
int abceapi_isios(struct abce *abce, int stackidx)
{
  int res;
  struct abce_err err_old = abce->err;
  res = abce_verifymb(abce, stackidx, ABCE_T_IOS) == 0;
  abce->err = err_old;
  return res;
}
int abceapi_isarray(struct abce *abce, int stackidx)
{
  int res;
  struct abce_err err_old = abce->err;
  res = abce_verifymb(abce, stackidx, ABCE_T_A) == 0;
  abce->err = err_old;
  return res;
}
int abceapi_isstr(struct abce *abce, int stackidx)
{
  int res;
  struct abce_err err_old = abce->err;
  res = abce_verifymb(abce, stackidx, ABCE_T_S) == 0;
  abce->err = err_old;
  return res;
}
int abceapi_ispb(struct abce *abce, int stackidx)
{
  int res;
  struct abce_err err_old = abce->err;
  res = abce_verifymb(abce, stackidx, ABCE_T_PB) == 0;
  abce->err = err_old;
  return res;
}
int abceapi_isscope(struct abce *abce, int stackidx)
{
  int res;
  struct abce_err err_old = abce->err;
  res = abce_verifymb(abce, stackidx, ABCE_T_SC) == 0;
  abce->err = err_old;
  return res;
}
int abceapi_isdbl(struct abce *abce, int stackidx)
{
  int res;
  struct abce_err err_old = abce->err;
  res = abce_verifymb(abce, stackidx, ABCE_T_D) == 0;
  abce->err = err_old;
  return res;
}
int abceapi_isbool(struct abce *abce, int stackidx)
{
  int res;
  struct abce_err err_old = abce->err;
  res = abce_verifymb(abce, stackidx, ABCE_T_B) == 0;
  abce->err = err_old;
  return res;
}
int abceapi_isfun(struct abce *abce, int stackidx)
{
  int res;
  struct abce_err err_old = abce->err;
  res = abce_verifymb(abce, stackidx, ABCE_T_F) == 0;
  abce->err = err_old;
  return res;
}
int abceapi_isbp(struct abce *abce, int stackidx)
{
  int res;
  struct abce_err err_old = abce->err;
  res = abce_verifymb(abce, stackidx, ABCE_T_BP) == 0;
  abce->err = err_old;
  return res;
}
int abceapi_isip(struct abce *abce, int stackidx)
{
  int res;
  struct abce_err err_old = abce->err;
  res = abce_verifymb(abce, stackidx, ABCE_T_IP) == 0;
  abce->err = err_old;
  return res;
}
int abceapi_isnil(struct abce *abce, int stackidx)
{
  int res;
  struct abce_err err_old = abce->err;
  res = abce_verifymb(abce, stackidx, ABCE_T_N) == 0;
  abce->err = err_old;
  return res;
}

int abceapi_getbool(struct abce *abce, int stackidx)
{
  int res;
  int b;
  struct abce_err err_old = abce->err;
  res = abce_getboolean(&b, abce, stackidx) == 0;
  abce->err = err_old;
  if (res)
  {
    return b;
  }
  return 0;
}
double abceapi_getdbl(struct abce *abce, int stackidx)
{
  int res;
  double d;
  struct abce_err err_old = abce->err;
  res = abce_getdbl(&d, abce, stackidx) == 0;
  abce->err = err_old;
  if (res)
  {
    return d;
  }
  return 0;
}
const char *abceapi_getstr(struct abce *abce, int stackidx, size_t *len)
{
  return NULL; // FIXME implement
}
const char *abceapi_getpbstr(struct abce *abce, int stackidx, size_t *len)
{
  return NULL; // FIXME implement
}

int abceapi_pushnewarray(struct abce *abce)
{
  struct abce_mb mb;
  struct abce_err err_old = abce->err;
  int ret;
  mb = abce_mb_create_array(abce);
  if (mb.typ == ABCE_T_N)
  {
    abce->err = err_old;
    return 0;
  }
  ret = abce_push_mb(abce, &mb) == 0;
  abce_mb_refdn_typ(abce, &mb, ABCE_T_A);
  abce->err = err_old;
  return ret;
}
int abceapi_pushnewtree(struct abce *abce)
{
  struct abce_mb mb;
  struct abce_err err_old = abce->err;
  int ret;
  mb = abce_mb_create_tree(abce);
  if (mb.typ == ABCE_T_N)
  {
    abce->err = err_old;
    return 0;
  }
  ret = abce_push_mb(abce, &mb) == 0;
  abce_mb_refdn_typ(abce, &mb, ABCE_T_A);
  abce->err = err_old;
  return ret;
}
int abceapi_pushnewpb(struct abce *abce, const char *str, size_t len)
{
  abort(); // FIXME implement
}
int abceapi_pushnewstr(struct abce *abce, const char *str, size_t len)
{
  struct abce_mb mb;
  struct abce_err err_old = abce->err;
  int ret;
  mb = abce_mb_create_string(abce, str, len);
  if (mb.typ == ABCE_T_N)
  {
    abce->err = err_old;
    return 0;
  }
  ret = abce_push_mb(abce, &mb) == 0;
  abce_mb_refdn_typ(abce, &mb, ABCE_T_A);
  abce->err = err_old;
  return ret;
}
int abceapi_pushnil(struct abce *abce)
{
  int res;
  struct abce_err err_old = abce->err;
  res = abce_push_nil(abce) == 0;
  abce->err = err_old;
  return res;
}
int abceapi_pushbool(struct abce *abce, int b)
{
  int res;
  struct abce_err err_old = abce->err;
  res = abce_push_boolean(abce, !!b) == 0;
  abce->err = err_old;
  return res;
}
int abceapi_pushdbl(struct abce *abce, double dbl)
{
  int res;
  struct abce_err err_old = abce->err;
  res = abce_push_double(abce, dbl) == 0;
  abce->err = err_old;
  return res;
}
int abceapi_pushstack(struct abce *abce, int stackidx)
{
  int res;
  struct abce_err err_old = abce->err;
  struct abce_mb mb;
  res = abce_getmb(&mb, abce, stackidx) == 0;
  if (!res)
  {
    abce->err = err_old;
    return res;
  }
  res = abce_push_mb(abce, &mb);
  abce_mb_refdn(abce, &mb);
  abce->err = err_old;
  return res;
}
int abceapi_exchangetop(struct abce *abce, int stackidx)
{
  struct abce_mb mbtmp;
  size_t addr, addrm1;
  struct abce_err err_old = abce->err;
  if (abce_calc_addr(&addr, abce, stackidx) != 0)
  {
    abce->err = err_old;
    return 0;
  }
  if (abce_calc_addr(&addrm1, abce, -1) != 0)
  {
    abce->err = err_old;
    return 0;
  }
  mbtmp = abce->stackbase[addr];
  abce->stackbase[addr] = abce->stackbase[addrm1];
  abce->stackbase[addrm1] = mbtmp;
  return 0;
}
int abceapi_pop(struct abce *abce)
{
  int res;
  struct abce_err err_old = abce->err;
  res = abce_pop(abce) == 0;
  abce->err = err_old;
  return res;
}
int abceapi_popmany(struct abce *abce, int cnt)
{
  int i;
  for (i = 0; i < cnt; i++)
  {
    if (!abceapi_pop(abce))
    {
      return 0;
    }
  }
  return 1;
}

size_t abceapi_getarraylen(struct abce *abce, int stackidx)
{
  int res;
  size_t ressz;
  struct abce_err err_old = abce->err;
  struct abce_mb mb;
  res = abce_getmb(&mb, abce, stackidx) == 0;
  if (!res)
  {
    abce->err = err_old;
    return (size_t)-1;
  }
  if (mb.typ != ABCE_T_A)
  {
    abce->err = err_old;
    return (size_t)-1;
  }
  ressz = mb.u.area->u.ar.size;
  abce_mb_refdn(abce, &mb);
  abce->err = err_old;
  return ressz;
}
size_t abceapi_gettreesize(struct abce *abce, int stackidx)
{
  int res;
  size_t ressz;
  struct abce_err err_old = abce->err;
  struct abce_mb mb;
  res = abce_getmb(&mb, abce, stackidx) == 0;
  if (!res)
  {
    abce->err = err_old;
    return (size_t)-1;
  }
  if (mb.typ != ABCE_T_T)
  {
    abce->err = err_old;
    return (size_t)-1;
  }
  ressz = mb.u.area->u.tree.sz;
  abce_mb_refdn(abce, &mb);
  abce->err = err_old;
  return ressz;
}
int abceapi_getarray(struct abce *abce);
int abceapi_gettree(struct abce *abce);
int abceapi_setarray(struct abce *abce);
int abceapi_settree(struct abce *abce);
int abceapi_deltree(struct abce *abce);
int abceapi_arrayappend(struct abce *abce);
int abceapi_arrayappendmany(struct abce *abce);
int abceapi_arraypop(struct abce *abce);
int abceapi_hastree(struct abce *abce);
int abceapi_hasscope(struct abce *abce);
int abceapi_next(struct abce *abce);
int abceapi_getdynscope(struct abce *abce);
int abceapi_scopevar(struct abce *abce);
int abceapi_scopevarset(struct abce *abce);

int abceapi_call(struct abce *abce);
int abceapi_call_if_fun(struct abce *abce);
