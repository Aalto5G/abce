#ifndef _ABCEAPI_H_
#define _ABCEAPI_H_

#include <stddef.h>

struct abce;

struct abce *abceapi_new(void);

void abceapi_free(struct abce *abce);

int abceapi_isvalid(struct abce *abce, int stackidx);

int abceapi_istree(struct abce *abce, int stackidx);
int abceapi_isios(struct abce *abce, int stackidx);
int abceapi_isarray(struct abce *abce, int stackidx);
int abceapi_isstr(struct abce *abce, int stackidx);
int abceapi_ispb(struct abce *abce, int stackidx);
int abceapi_isscope(struct abce *abce, int stackidx);
int abceapi_isdbl(struct abce *abce, int stackidx);
int abceapi_isbool(struct abce *abce, int stackidx);
int abceapi_isfun(struct abce *abce, int stackidx);
int abceapi_isbp(struct abce *abce, int stackidx);
int abceapi_isip(struct abce *abce, int stackidx);
int abceapi_isnil(struct abce *abce, int stackidx);

int abceapi_getbool(struct abce *abce, int stackidx);
double abceapi_getdbl(struct abce *abce, int stackidx);
const char *abceapi_getstr(struct abce *abce, int stackidx, size_t *len);
const char *abceapi_getpbstr(struct abce *abce, int stackidx, size_t *len);

int abceapi_pushnewarray(struct abce *abce);
int abceapi_pushnewtree(struct abce *abce);
int abceapi_pushnewpb(struct abce *abce, const char *str, size_t len);
int abceapi_pushnewstr(struct abce *abce, const char *str, size_t len);
int abceapi_pushnil(struct abce *abce);
int abceapi_pushbool(struct abce *abce, int b);
int abceapi_pushdbl(struct abce *abce, double dbl);
int abceapi_pushstack(struct abce *abce, int stackidx);
int abceapi_exchangetop(struct abce *abce, int stackidx);
int abceapi_pop(struct abce *abce);
int abceapi_popmany(struct abce *abce, int cnt);

size_t abceapi_getarraylen(struct abce *abce, int stackidx);
size_t abceapi_gettreesize(struct abce *abce, int stackidx);
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

int abceapi_dump(struct abce *abce);

#endif
