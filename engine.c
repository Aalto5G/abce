#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <ctype.h>
#include "abcerbtree.h"
#include "abcemurmur.h"
#include "abcecontainerof.h"
#include "abcelikely.h"
#include "abceopcodes.h"
#include "abce.h"
#include "abcestring.h"
#include "abcetrees.h"
#include "abcescopes.h"
#include "abceworditer.h"
#include "abce_caj.h"

#define POPABORTS 1

static inline void abce_maybeabort()
{
  abort();
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

#define GETORVERIFY1(fun, idx) \
  if(1) { \
    int _getdbl_rettmp = fun(abce, (idx)); \
    if (_getdbl_rettmp != 0) \
    { \
      ret = _getdbl_rettmp; \
      break; \
    } \
  }

#define GETGENERIC(fun, val, idx) \
  if(1) { \
    int _getgen_rettmp = fun((val), abce, (idx)); \
    if (_getgen_rettmp != 0) \
    { \
      ret = _getgen_rettmp; \
      break; \
    } \
  }

#define VERIFYADDR(idx) GETORVERIFY1(abce_verifyaddr, idx)

#define GETBOOLEAN(dbl, idx) GETGENERIC(abce_getboolean, dbl, idx)
#define GETFUNADDR(dbl, idx) GETGENERIC(abce_getfunaddr, dbl, idx)
#define GETDBL(dbl, idx) GETGENERIC(abce_getdbl, dbl, idx)
#define GETBP(idx) GETORVERIFY1(abce_getbp, idx)
#define GETIP(idx) GETORVERIFY1(abce_getip, idx)
#define GETMBPTR(mb, idx) GETGENERIC(abce_getmbptr, mb, idx)
#define GETMBSCPTR(mb, idx) GETGENERIC(abce_getmbscptr, mb, idx)
#define GETMBARPTR(mb, idx) GETGENERIC(abce_getmbarptr, mb, idx)
#define GETMBPBPTR(mb, idx) GETGENERIC(abce_getmbpbptr, mb, idx)
#define GETMBSTRPTR(mb, idx) GETGENERIC(abce_getmbstrptr, mb, idx)
#define GETMBIOSPTR(mb, idx) GETGENERIC(abce_getmbiosptr, mb, idx)

static inline void abce_npoppushnil(struct abce *abce, size_t n)
{
	int ret;
	struct abce_mb nn;
	nn.typ = ABCE_T_N;
	nn.u.d = 0;
	ret = abce_mb_stackreplace(abce, -(int64_t)n, &nn);
#if POPABORTS
	if (ret != 0)
	{
		abort();
	}
#endif
	for (size_t i = 1; i < n; i++)
	{
		ret = abce_pop(abce);
#if POPABORTS
		if (ret != 0)
		{
			abort();
		}
#endif
	}
}
static inline void abce_npoppushdbl(struct abce *abce, size_t n, double d)
{
	int ret;
	struct abce_mb dd;
	dd.typ = ABCE_T_D;
	dd.u.d = d;
	ret = abce_mb_stackreplace(abce, -(int64_t)n, &dd);
#if POPABORTS
	if (ret != 0)
	{
		abort();
	}
#endif
	for (size_t i = 1; i < n; i++)
	{
		ret = abce_pop(abce);
#if POPABORTS
		if (ret != 0)
		{
			abort();
		}
#endif
	}
}
static inline void abce_npoppushbool(struct abce *abce, size_t n, int b)
{
	int ret;
	struct abce_mb bb;
	bb.typ = ABCE_T_B;
	bb.u.d = !!b;
	ret = abce_mb_stackreplace(abce, -(int64_t)n, &bb);
#if POPABORTS
	if (ret != 0)
	{
		abort();
	}
#endif
	for (size_t i = 1; i < n; i++)
	{
		ret = abce_pop(abce);
#if POPABORTS
		if (ret != 0)
		{
			abort();
		}
#endif
	}
}
static inline void abce_npoppush(struct abce *abce, size_t n, const struct abce_mb *mb)
{
	int ret;
	ret = abce_mb_stackreplace(abce, -(int64_t)n, mb);
#if POPABORTS
	if (ret != 0)
	{
		abort();
	}
#endif
	for (size_t i = 1; i < n; i++)
	{
		ret = abce_pop(abce);
#if POPABORTS
		if (ret != 0)
		{
			abort();
		}
#endif
	}
}
static inline void abce_npoppusharea(struct abce *abce, size_t n, enum abce_type typ, struct abce_mb_area *mba)
{
	struct abce_mb mb;
	mb.typ = typ;
	mb.u.area = mba;
	abce_npoppush(abce, n, &mb);
}
static inline void abce_npoppushc(struct abce *abce, size_t n)
{
	struct abce_mb *mb;
	if (abce_unlikely(abce->csp == 0))
	{
		abort();
	}
	mb = &abce->cstackbase[abce->csp-1];
	abce_npoppush(abce, n, mb);
}

#if POPABORTS
#define POP() \
  if(1) { \
    int _getdbl_rettmp = abce_pop(abce); \
    if (_getdbl_rettmp != 0) \
    { \
      abort(); \
    } \
  }
#else
#define POP() abce_pop(abce)
#endif

void abce_ncpop(struct abce *abce, size_t n)
{
  size_t i;
  for (i = 0; i < n; i++)
  {
    abce_cpop(abce);
  }
}

int
abce_mid(struct abce *abce, uint16_t ins, unsigned char *addcode, size_t addsz)
{
  int ret = 0;
  switch (ins)
  {
#ifdef WITH_LUA
    case ABCE_OPCODE_LUACALL:
    {
      size_t i;
      double argcnt;
      struct abce_mb *mb = NULL;
      struct abce_mb *mbsc = NULL;
      struct abce_mb *funname = NULL;
      GETDBL(&argcnt, -1);
      if (abce_unlikely((double)(size_t)argcnt != argcnt))
      {
        abce->err.code = ABCE_E_CALL_ARGCNT_NOT_UINT;
        abce->err.mb.typ = ABCE_T_D;
        abce->err.mb.u.d = argcnt;
        ret = -EINVAL;
        break;
      }
      VERIFYMB(-2-(int)(size_t)argcnt, ABCE_T_S);
      VERIFYMB(-3-(int)(size_t)argcnt, ABCE_T_SC);
      GETMBSTRPTR(&funname, -2-(int)(size_t)argcnt);
      GETMBSCPTR(&mbsc, -3-(int)(size_t)argcnt);
      POP(); // argcnt

      if (abce_ensure_lua(mbsc.u.area, abce) != 0)
      {
        ret = -ENOMEM;
        break;
      }

      lua_getglobal(mbsc.u.area->u.sc.lua, funname.u.area->u.str.buf);
      for (i = argcnt; i > 0; i--)
      {
        GETMBPTR(&mb, -(int)i);
        mb_to_lua(mbsc.u.area->u.sc.lua, &mb);
      }
      for (i = 0; i < argcnt; i++)
      {
        POP(); // args
      }
      POP(); // funname
      abce_push_rg(abce); // to capture backtrace in stages
      abce->err.code = ABCE_E_NONE;
      abce->err.mb.typ = ABCE_T_N;
      if (lua_pcall(mbsc.u.area->u.sc.lua, argcnt, 1, 0) != 0)
      {
        abce_pop(abce); // rg
        if (abce->err.code == ABCE_E_NONE)
        {
          struct abce_mb *mberrstr = NULL;
          mb_from_lua(mbsc.u.area->u.sc.lua, abce, -1);
          GETMBPTR(&mberrstr, -1);
          abce->err.code = ABCE_E_LUA_ERR;
          abce_mb_errreplace_noinline(abce, mberrstr);
          abce_pop(abce);
        } // otherwise it's error from nested call
        abce_pop(abce); // mbsc
        ret = -EINVAL;
        break;
      }
      abce_pop(abce); // rg
      mb_from_lua(mbsc.u.area->u.sc.lua, abce, -1);
      GETMBPTR(&mb, -1); // retval
      abce_npoppush(abce, 2, mb);
      return 0;
    }
#endif
    case ABCE_OPCODE_SCOPEVAR_NONRECURSIVE:
    {
      struct abce_mb *mbsc, *mbit;
      const struct abce_mb *ptr;
      GETMBSCPTR(&mbsc, -2);
      GETMBPTR(&mbit, -1);
      if (abce_unlikely(mbit->typ != ABCE_T_S))
      {
        abce->err.code = ABCE_E_EXPECT_STR;
        abce_mb_errreplace_noinline(abce, mbit);
        abce->err.val2 = -2;
        return -EINVAL;
      }
      ptr = abce_sc_get_rec_mb(mbsc, mbit, 0);
      if (abce_unlikely(ptr == NULL))
      {
        abce->err.code = ABCE_E_SCOPEVAR_NOT_FOUND;
        abce_mb_errreplace_noinline(abce, mbit);
        return -ENOENT;
      }
      abce_npoppush(abce, 2, ptr);
      return 0;
    }
    case ABCE_OPCODE_SCOPE_HAS_NONRECURSIVE:
    {
      struct abce_mb *mbsc, *mbit;
      const struct abce_mb *ptr;
      GETMBSCPTR(&mbsc, -2);
      GETMBPTR(&mbit, -1);
      if (abce_unlikely(mbit->typ != ABCE_T_S))
      {
        abce->err.code = ABCE_E_EXPECT_STR;
        abce_mb_errreplace_noinline(abce, mbit);
        abce->err.val2 = -2;
        return -EINVAL;
      }
      ptr = abce_sc_get_rec_mb(mbsc, mbit, 0);
      abce_npoppushbool(abce, 2, ptr != NULL);
      return 0;
    }
    case ABCE_OPCODE_DUMP:
    {
      struct abce_mb *mbptr;
      GETMBPTR(&mbptr, -1);
      abce_mb_dump(mbptr);
      POP();
      return 0;
    }
    case ABCE_OPCODE_ABS:
    {
      double dbl;
      GETDBL(&dbl, -1);
      POP();
      if (abce_push_double(abce, fabs(dbl)) != 0)
      {
        abce_maybeabort();
      }
      return 0;
    }
    case ABCE_OPCODE_TRUNC:
    {
      double dbl;
      GETDBL(&dbl, -1);
      POP();
      if (abce_push_double(abce, trunc(dbl)) != 0)
      {
        abce_maybeabort();
      }
      return 0;
    }
    case ABCE_OPCODE_ROUND:
    {
      double dbl;
      GETDBL(&dbl, -1);
      POP();
      if (abce_push_double(abce, round(dbl)) != 0)
      {
        abce_maybeabort();
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
        abce_maybeabort();
      }
      return 0;
    }
    case ABCE_OPCODE_GETENV:
    {
      struct abce_mb *mbbase;
      char *myenv;
      GETMBSTRPTR(&mbbase, -1);
      myenv = getenv(mbbase->u.area->u.str.buf);
      if (myenv == NULL)
      {
        if (abce_cpush_nil(abce) != 0)
        {
          return -ENOMEM;
        }
      }
      else
      {
        if (abce_mb_cpush_create_string(abce,
                                        myenv,
                                        strlen(myenv)) == NULL)
        {
          return -ENOMEM;
        }
      }
      abce_npoppushc(abce, 1);
      abce_cpop(abce);
      return 0;
    }
    case ABCE_OPCODE_LOG:
    {
      double dbl;
      GETDBL(&dbl, -1);
      POP();
      if (abce_push_double(abce, log(dbl)) != 0)
      {
        abce_maybeabort();
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
        abce_maybeabort();
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
        abce_maybeabort();
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
        abce_maybeabort();
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
        abce_maybeabort();
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
        abce_maybeabort();
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
        abce_maybeabort();
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
        abce_maybeabort();
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
        abce_maybeabort();
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
        abce_maybeabort();
      }
      return 0;
    }
    case ABCE_OPCODE_EXIT:
    {
      abce->err.code = ABCE_E_EXIT;
      return -EINTR;
    }
    case ABCE_OPCODE_STRGSUB:
    {
      struct abce_mb *mbhaystack, *mbneedle, *mbsub;
      int rettmp = abce_getmbstrptr(&mbhaystack, abce, -3);
      if (rettmp != 0)
      {
        return rettmp;
      }
      rettmp = abce_getmbstrptr(&mbneedle, abce, -2);
      if (rettmp != 0)
      {
        return rettmp;
      }
      rettmp = abce_getmbstrptr(&mbsub, abce, -1);
      if (rettmp != 0)
      {
        return rettmp;
      }
      rettmp = abce_cpush_strgsub_mb(abce, mbhaystack, mbneedle, mbsub);
      if (rettmp != 0)
      {
        return rettmp;
      }
      abce_npoppushc(abce, 3);
      abce_cpop(abce);
      return 0;
    }
    case ABCE_OPCODE_IMPORT:
      abce->err.code = ABCE_E_NOTSUP_INSTRUCTION;
      abce->err.mb.typ = ABCE_T_D;
      abce->err.mb.u.d = ins;
      return -ENOTSUP;
    case ABCE_OPCODE_FUN_TRAILER:
      abce->err.code = ABCE_E_RUN_INTO_FUNC;
      return -EACCES;
    // String functions
    case ABCE_OPCODE_STRSUB:
    {
      struct abce_mb *mbbase;
      double start, end;

      GETDBL(&end, -1);
      if (end < 0)
      {
        abce->err.code = ABCE_E_INDEX_OOB;
        abce->err.mb.typ = ABCE_T_D;
        abce->err.mb.u.d = end;
        return -ERANGE;
      }
      if ((double)(size_t)end != end)
      {
        abce->err.code = ABCE_E_INDEX_NOT_INT;
        abce->err.mb.typ = ABCE_T_D;
        abce->err.mb.u.d = end;
        return -ERANGE;
      }
      GETDBL(&start, -2);
      if (start < 0)
      {
        abce->err.code = ABCE_E_INDEX_OOB;
        abce->err.mb.typ = ABCE_T_D;
        abce->err.mb.u.d = start;
        return -ERANGE;
      }
      if ((double)(size_t)start != start)
      {
        abce->err.code = ABCE_E_INDEX_NOT_INT;
        abce->err.mb.typ = ABCE_T_D;
        abce->err.mb.u.d = start;
        return -ERANGE;
      }
      if (end < start)
      {
        abce->err.code = ABCE_E_INDEX_OOB;
        abce->err.mb.typ = ABCE_T_D;
        abce->err.mb.u.d = start;
        abce->err.val2 = end;
        return -ERANGE;
      }
      GETMBSTRPTR(&mbbase, -3);
      if (end > mbbase->u.area->u.str.size)
      {
        abce->err.code = ABCE_E_INDEX_OOB;
        abce->err.mb.typ = ABCE_T_D;
        abce->err.mb.u.d = end;
        return -ERANGE;
      }
      if (abce_mb_cpush_create_string(abce,
                                      mbbase->u.area->u.str.buf + (size_t)start,
                                      end - start) == NULL)
      {
        return -ENOMEM;
      }
      abce_npoppushc(abce, 3);
      abce_cpop(abce);
      return 0;
    }
    case ABCE_OPCODE_STR_FROMCHR:
    {
      double ch;
      char chch;
      GETDBL(&ch, -1);
      if ((uint64_t)ch < 0 || (uint64_t)ch >= 256 || (double)(uint64_t)ch != ch)
      {
        abce->err.code = ABCE_E_INVALID_CH;
        abce->err.mb.typ = ABCE_T_D;
        abce->err.mb.u.d = ch;
        return -EINVAL;
      }
      chch = (char)(unsigned char)ch;
      if (abce_mb_cpush_create_string(abce, &chch, 1) == NULL)
      {
        return -ENOMEM;
      }
      abce_npoppushc(abce, 1);
      abce_cpop(abce);
      return 0;
    }
    case ABCE_OPCODE_STRAPPEND:
    {
      struct abce_mb *res, *mbbase, *mbextend;

      VERIFYMB(-1, ABCE_T_S);
      VERIFYMB(-2, ABCE_T_S);
      GETMBSTRPTR(&mbextend, -1);
      GETMBSTRPTR(&mbbase, -2);
      res = abce_mb_cpush_concat_string(abce,
                                        mbbase->u.area->u.str.buf,
                                        mbbase->u.area->u.str.size,
                                        mbextend->u.area->u.str.buf,
                                        mbextend->u.area->u.str.size);
      if (res == NULL)
      {
        return -ENOMEM;
      }
      abce_npoppushc(abce, 2);
      abce_cpop(abce);
      return 0;
    }
    case ABCE_OPCODE_STR_CMP:
    {
      struct abce_mb *mb1, *mb2;

      VERIFYMB(-1, ABCE_T_S);
      VERIFYMB(-2, ABCE_T_S);
      GETMBSTRPTR(&mb2, -1);
      GETMBSTRPTR(&mb1, -2);
      abce_npoppushdbl(abce, 2, abce_str_cmp_sym_mb(mb1, mb2));
      return 0;
    }
    case ABCE_OPCODE_STR_LOWER:
    {
      struct abce_mb *res, *mbbase;
      size_t i;

      VERIFYMB(-1, ABCE_T_S);
      GETMBSTRPTR(&mbbase, -1);
      res = abce_mb_cpush_create_string(abce,
                                        mbbase->u.area->u.str.buf,
                                        mbbase->u.area->u.str.size);
      if (res == NULL)
      {
        return -ENOMEM;
      }
      for (i = 0; i < res->u.area->u.str.size; i++)
      {
        res->u.area->u.str.buf[i] = tolower((unsigned char)res->u.area->u.str.buf[i]);
      }
      abce_npoppushc(abce, 1);
      abce_cpop(abce);
      return 0;
    }
    case ABCE_OPCODE_STR_UPPER:
    {
      struct abce_mb *res, *mbbase;
      size_t i;

      VERIFYMB(-1, ABCE_T_S);
      GETMBSTRPTR(&mbbase, -1);
      res = abce_mb_cpush_create_string(abce,
                                        mbbase->u.area->u.str.buf,
                                        mbbase->u.area->u.str.size);
      if (res == NULL)
      {
        return -ENOMEM;
      }
      for (i = 0; i < res->u.area->u.str.size; i++)
      {
        res->u.area->u.str.buf[i] = toupper((unsigned char)res->u.area->u.str.buf[i]);
      }
      abce_npoppushc(abce, 1);
      abce_cpop(abce);
      return 0;
    }
    case ABCE_OPCODE_STRSET:
    {
      double loc, ch;
      struct abce_mb *res, *mbbase;
      GETDBL(&ch, -1);
      GETDBL(&loc, -2);
      VERIFYMB(-3, ABCE_T_S);
      if ((uint64_t)ch < 0 || (uint64_t)ch >= 256 || (double)(uint64_t)ch != ch)
      {
        abce->err.code = ABCE_E_INVALID_CH;
        abce->err.mb.typ = ABCE_T_D;
        abce->err.mb.u.d = ch;
        return -EINVAL;
      }
      GETMBSTRPTR(&mbbase, -3);
      if ((int64_t)loc < 0 || (uint64_t)loc >= mbbase->u.area->u.str.size)
      {
        abce->err.code = ABCE_E_INDEX_OOB;
        abce->err.mb.typ = ABCE_T_D;
        abce->err.mb.u.d = loc;
        return -EINVAL;
      }
      if ((double)(uint64_t)loc != loc)
      {
        abce->err.code = ABCE_E_INDEX_NOT_INT;
        abce->err.mb.typ = ABCE_T_D;
        abce->err.mb.u.d = loc;
        return -EINVAL;
      }
      res = abce_mb_cpush_create_string(abce,
                                        mbbase->u.area->u.str.buf,
                                        mbbase->u.area->u.str.size);
      if (res == NULL)
      {
        return -ENOMEM;
      }
      res->u.area->u.str.buf[(uint64_t)loc] = (char)(unsigned char)ch;
      abce_npoppushc(abce, 3);
      abce_cpop(abce);
      return 0;
    }
    case ABCE_OPCODE_STRWORDCNT:
    {
      struct abce_word_iter it = {};
      struct abce_mb *mbbase;
      struct abce_mb *mbbasemaybenil;
      struct abce_mb *mbsep;
      size_t i = 0;
      VERIFYMB(-1, ABCE_T_S);
      GETMBPTR(&mbbasemaybenil, -2);
      if (mbbasemaybenil->typ == ABCE_T_N)
      {
        abce_npoppushdbl(abce, 2, 0);
        return 0;
      }
      VERIFYMB(-2, ABCE_T_S);
      GETMBSTRPTR(&mbsep, -1);
      GETMBSTRPTR(&mbbase, -2);
      abce_word_iter_init(&it, mbbase->u.area->u.str.buf, mbbase->u.area->u.str.size,
                          mbsep->u.area->u.str.buf, mbsep->u.area->u.str.size);
      while (!abce_word_iter_at_end(&it))
      {
        i++;
        abce_word_iter_next(&it);
      }
      abce_npoppushdbl(abce, 2, i);
      return 0;
    }
    case ABCE_OPCODE_STRWORDLIST:
    {
      struct abce_word_iter it = {};
      struct abce_mb *mbbase;
      struct abce_mb *mbbasemaybenil;
      struct abce_mb *mbsep;
      struct abce_mb *mbar;
      struct abce_mb *mbit;
      VERIFYMB(-1, ABCE_T_S);
      GETMBPTR(&mbbasemaybenil, -2);
      if (mbbasemaybenil->typ == ABCE_T_N)
      {
        mbar = abce_mb_cpush_create_array(abce);
        if (mbar == NULL)
        {
          return -ENOMEM;
        }
        abce_npoppushc(abce, 2);
        abce_cpop(abce);
        return 0;
      }
      VERIFYMB(-2, ABCE_T_S);
      GETMBSTRPTR(&mbsep, -1);
      GETMBSTRPTR(&mbbase, -2);
      mbar = abce_mb_cpush_create_array(abce);
      if (mbar == NULL)
      {
        return -ENOMEM;
      }
      abce_word_iter_init(&it, mbbase->u.area->u.str.buf, mbbase->u.area->u.str.size,
                          mbsep->u.area->u.str.buf, mbsep->u.area->u.str.size);
      while (!abce_word_iter_at_end(&it))
      {
        // Here we have to be careful. We allocate memblocks, but we add them immediately
        // to the array, so if GC can see the array, GC can see our memblocks.
        mbit = abce_mb_cpush_create_string(abce, 
                                           mbbase->u.area->u.str.buf + it.start,
                                           it.end - it.start);
        if (mbit == NULL)
        {
          abce_cpop(abce);
          return -ENOMEM;
        }
        if (abce_mb_array_append(abce, mbar, mbit) != 0)
        {
          abce_cpop(abce);
          abce_cpop(abce);
          return -ENOMEM;
        }
        abce_cpop(abce);
        abce_word_iter_next(&it);
      }
      abce_npoppushc(abce, 2);
      abce_cpop(abce);
      return 0;
    }
    case ABCE_OPCODE_STRWORD:
    {
      struct abce_word_iter it = {};
      struct abce_mb *mbbase;
      struct abce_mb *mbsep;
      struct abce_mb *mbit;
      double wordidx;
      size_t i = 0;
      VERIFYMB(-2, ABCE_T_S);
      VERIFYMB(-3, ABCE_T_S);
      GETDBL(&wordidx, -1);
      if (wordidx < 0)
      {
        abce->err.code = ABCE_E_INDEX_OOB;
        abce->err.mb.typ = ABCE_T_D;
        abce->err.mb.u.d = wordidx;
        return -EINVAL;
      }
      if ((double)(uint64_t)wordidx != wordidx)
      {
        abce->err.code = ABCE_E_INDEX_NOT_INT;
        abce->err.mb.typ = ABCE_T_D;
        abce->err.mb.u.d = wordidx;
        return -EINVAL;
      }
      GETMBSTRPTR(&mbsep, -2);
      GETMBSTRPTR(&mbbase, -3);
      abce_word_iter_init(&it, mbbase->u.area->u.str.buf, mbbase->u.area->u.str.size,
                          mbsep->u.area->u.str.buf, mbsep->u.area->u.str.size);
      while (!abce_word_iter_at_end(&it))
      {
        if (i == (size_t)wordidx)
        {
          mbit = abce_mb_cpush_create_string(abce, 
                                             mbbase->u.area->u.str.buf + it.start,
                                             it.end - it.start);
          if (mbit == NULL)
          {
            return -ENOMEM;
          }
	  abce_npoppushc(abce, 3);
          abce_cpop(abce);
          return 0;
        }
        i++;
        abce_word_iter_next(&it);
      }
      abce->err.code = ABCE_E_INDEX_OOB;
      abce->err.mb.typ = ABCE_T_D;
      abce->err.mb.u.d = wordidx;
      return -ERANGE;
    }
    case ABCE_OPCODE_OUT:
    {
      struct abce_mb *mbstr;
      double streamidx;
      VERIFYMB(-2, ABCE_T_S);
      GETDBL(&streamidx, -1);
      if (streamidx != 0 && streamidx != 1)
      {
        abce->err.code = ABCE_E_INVALID_STREAMIDX;
        abce->err.mb.typ = ABCE_T_D;
        abce->err.mb.u.d = streamidx;
        return -EINVAL;
      }
      GETMBSTRPTR(&mbstr, -2);
      if (fwrite(mbstr->u.area->u.str.buf, 1, mbstr->u.area->u.str.size,
                 (streamidx == 0) ? stdout : stderr) 
          != mbstr->u.area->u.str.size)
      {
        abce->err.code = ABCE_E_IO_ERROR;
        POP();
        POP();
        return -EIO;
      }
      POP();
      POP();
      return 0;
    }
    case ABCE_OPCODE_ERROR:
    {
      struct abce_mb *mbstr;
      VERIFYMB(-1, ABCE_T_S);
      GETMBSTRPTR(&mbstr, -1);
      if (fwrite(mbstr->u.area->u.str.buf, 1, mbstr->u.area->u.str.size, stderr)
          != mbstr->u.area->u.str.size)
      {
        abce->err.code = ABCE_E_IO_ERROR;
        POP();
        return -EIO;
      }
      abce->err.code = ABCE_E_ERROR_EXIT;
      POP();
      return -ECANCELED;
    }
    case ABCE_OPCODE_STR_REVERSE:
    {
      struct abce_mb *mbbase;
      VERIFYMB(-1, ABCE_T_S);
      GETMBSTRPTR(&mbbase, -1);
      if (abce_mb_cpush_create_string(abce,
                                      mbbase->u.area->u.str.buf,
                                      mbbase->u.area->u.str.size) == NULL)
      {
        return -ENOMEM;
      }
      mbbase = &abce->cstackbase[abce->csp-1];
      for (size_t i = 0; i < mbbase->u.area->u.str.size/2; i++)
      {
        uint8_t tmp = mbbase->u.area->u.str.buf[i];
        mbbase->u.area->u.str.buf[i] =
          mbbase->u.area->u.str.buf[mbbase->u.area->u.str.size-i-1];
        mbbase->u.area->u.str.buf[mbbase->u.area->u.str.size-i-1] = tmp;
      }
      abce_npoppushc(abce, 1);
      abce_cpop(abce);
      return 0;
    }
    case ABCE_OPCODE_STRREP:
    {
      struct abce_mb *res, *mbbase;
      double cnt;

      VERIFYMB(-1, ABCE_T_D);
      VERIFYMB(-2, ABCE_T_S);
      GETDBL(&cnt, -1);
      if ((double)(size_t)cnt != cnt)
      {
        abce->err.code = ABCE_E_REPCNT_NOT_UINT;
        abce->err.mb.typ = ABCE_T_D;
        abce->err.mb.u.d = cnt;
        return -EINVAL;
      }
      GETMBSTRPTR(&mbbase, -2);
      res = abce_mb_cpush_rep_string(abce,
                                     mbbase->u.area->u.str.buf,
                                     mbbase->u.area->u.str.size,
                                     (size_t)cnt);
      if (res == NULL)
      {
        return -ENOMEM;
      }
      abce_npoppushc(abce, 2);
      abce_cpop(abce);
      return 0;
    }
    case ABCE_OPCODE_STRSTR:
    {
      struct abce_mb *mbhaystack, *mbneedle;
      const char *pos;

      VERIFYMB(-1, ABCE_T_S);
      VERIFYMB(-2, ABCE_T_S);
      GETMBSTRPTR(&mbneedle, -1);
      GETMBSTRPTR(&mbhaystack, -2);
      pos = abce_strstr(mbhaystack->u.area->u.str.buf, mbhaystack->u.area->u.str.size,
                        mbneedle->u.area->u.str.buf, mbneedle->u.area->u.str.size);

      if (pos == NULL)
      {
        abce_npoppushnil(abce, 2);
      }
      else
      {
        abce_npoppushdbl(abce, 2, pos - mbhaystack->u.area->u.str.buf);
      }
      return 0;
    }
    case ABCE_OPCODE_STRLISTJOIN:
    {
      struct abce_mb *mbar, *mbjoiner;
      struct abce_str_buf buf = {};
      size_t i;

      VERIFYMB(-1, ABCE_T_A);
      VERIFYMB(-2, ABCE_T_S);
      GETMBARPTR(&mbar, -1);
      GETMBSTRPTR(&mbjoiner, -2);
      for (i = 0; i < mbar->u.area->u.ar.size; i++)
      {
        const struct abce_mb *mb = &mbar->u.area->u.ar.mbs[i];
        if (mb->typ != ABCE_T_S)
        {
          abce_str_buf_free(abce, &buf);
          abce->err.code = ABCE_E_EXPECT_STR;
          abce_mb_errreplace_noinline(abce, mb);
          return -EINVAL;
        }
        if (i != 0)
        {
          if (abce_str_buf_add(abce, &buf, mbjoiner->u.area->u.str.buf, mbjoiner->u.area->u.str.size)
              != 0)
          {
            abce_str_buf_free(abce, &buf);
            return -ENOMEM;
          }
        }
        if (abce_str_buf_add(abce, &buf, mbar->u.area->u.ar.mbs[i].u.area->u.str.buf, mbar->u.area->u.ar.mbs[i].u.area->u.str.size)
            != 0)
        {
          abce_str_buf_free(abce, &buf);
          return -ENOMEM;
        }
      }
      if (abce_mb_cpush_create_string(abce, buf.buf, buf.sz) == NULL)
      {
        abce_str_buf_free(abce, &buf);
        return -ENOMEM;
      }
      abce_str_buf_free(abce, &buf);
      abce_npoppushc(abce, 2);
      abce_cpop(abce);
      return 0;
    }
    case ABCE_OPCODE_TOSTRING:
    {
      char buf[64] = {0};
      double dbl;
      GETDBL(&dbl, -1);
      if (snprintf(buf, sizeof(buf), "%g", dbl) >= sizeof(buf))
      {
        abort();
      }
      if (abce_mb_cpush_create_string(abce, buf, strlen(buf)) == NULL)
      {
        return -ENOMEM;
      }
      POP(); // right before push to avoid GC crashes
      abce_push_c(abce);
      abce_cpop(abce);
      return 0;
    }
    case ABCE_OPCODE_TONUMBER:
    {
      struct abce_mb *str;
      char *endptr;
      double dbl;
      GETMBSTRPTR(&str, -1);
      if (   str->u.area->u.str.size == 0
          || str->u.area->u.str.size != strlen(str->u.area->u.str.buf))
      {
        abce->err.code = ABCE_E_NOT_A_NUMBER_STRING;
        abce_mb_errreplace_noinline(abce, str);
        return -EINVAL;
      }
      dbl = strtod(str->u.area->u.str.buf, &endptr);
      if (*endptr != '\0')
      {
        abce->err.code = ABCE_E_NOT_A_NUMBER_STRING;
        abce_mb_errreplace_noinline(abce, str);
        return -EINVAL;
      }
      abce_npoppushdbl(abce, 1, dbl);
      return 0;
    }
    case ABCE_OPCODE_STRSTRIP:
    {
      struct abce_mb *res, *mbbase, *mbsep;
      size_t start, end;
      VERIFYMB(-1, ABCE_T_S);
      VERIFYMB(-2, ABCE_T_S);
      GETMBSTRPTR(&mbsep, -1);
      GETMBSTRPTR(&mbbase, -2);
      abce_strip(mbbase->u.area->u.str.buf, mbbase->u.area->u.str.size,
                 mbsep->u.area->u.str.buf, mbsep->u.area->u.str.size,
                 &start, &end);
      if (end < start)
      {
        abort();
      }
      res = abce_mb_cpush_create_string(abce,
                                        mbbase->u.area->u.str.buf + start,
                                        end - start);
      if (res == NULL)
      {
        return -ENOMEM;
      }
      abce_npoppushc(abce, 2);
      abce_cpop(abce);
      return 0;
    }
    case ABCE_OPCODE_SCOPE_PARENT:
    {
      struct abce_mb *mbsc;
      GETMBSCPTR(&mbsc, -1);
      if (mbsc->u.area->u.sc.parent == NULL)
      {
        abce_npoppushnil(abce, 1);
        return 0;
      }
      abce_npoppusharea(abce, 1, ABCE_T_SC, mbsc->u.area->u.sc.parent);
      return 0;
    }
    case ABCE_OPCODE_SCOPE_NEW:
    {
      int holey;
      double locidx;
      struct abce_mb *mbscnew;
      struct abce_mb *mbscparent;
      GETBOOLEAN(&holey, -1);
      GETMBSCPTR(&mbscparent, -2);
      mbscnew = abce_mb_cpush_create_scope(abce, ABCE_DEFAULT_SCOPE_SIZE, mbscparent, holey);
      if (mbscnew == NULL)
      {
        return -ENOMEM;
      }
      locidx = mbscnew->u.area->u.sc.locidx;
      abce_npoppushdbl(abce, 2, locidx);
      abce_cpop(abce);
      return 0;
    }
    case ABCE_OPCODE_APPENDALL_MAINTAIN: // RFE should this be moved elsewhere? A complex operation.
    {
      struct abce_mb *mbar2;
      struct abce_mb *mbar;
      size_t i;
      VERIFYMB(-2, ABCE_T_A);
      VERIFYMB(-1, ABCE_T_A);
      GETMBARPTR(&mbar, -2);
      GETMBARPTR(&mbar2, -1);
      // NB: Here we need to be very careful. We can't pop mbar2 until we
      //     are in a point where memory alloc cannot happen. Otherwise GC
      //     complains about reference count not being 0.
      for (i = 0; i < mbar2->u.area->u.ar.size; i++)
      {
        if (abce_mb_array_append(abce, mbar, &mbar2->u.area->u.ar.mbs[i]) != 0)
        {
          ret = -ENOMEM;
          break;
        }
      }
      POP();
      break;
    }
    case ABCE_OPCODE_PUSH_NEW_PB:
    {
      struct abce_mb *mb;
      int rettmp;
      mb = abce_mb_cpush_create_pb(abce);
      if (mb == NULL)
      {
        ret = -ENOMEM;
        break;
      }
      rettmp = abce_push_c(abce);
      if (rettmp != 0)
      {
        ret = rettmp;
        abce_cpop(abce);
        break;
      }
      abce_cpop(abce);
      break;
    }
    case ABCE_OPCODE_PUSH_NEW_DICT:
    {
      struct abce_mb *mb;
      int rettmp;
      mb = abce_mb_cpush_create_tree(abce);
      if (mb == NULL)
      {
        ret = -ENOMEM;
        break;
      }
      rettmp = abce_push_c(abce);
      if (rettmp != 0)
      {
        ret = rettmp;
        abce_cpop(abce);
        break;
      }
      abce_cpop(abce);
      break;
    }
    case ABCE_OPCODE_PUSH_NEW_ARRAY:
    {
      struct abce_mb *mb;
      int rettmp;
      mb = abce_mb_cpush_create_array(abce);
      if (mb == NULL)
      {
        ret = -ENOMEM;
        break;
      }
      rettmp = abce_push_c(abce);
      if (rettmp != 0)
      {
        ret = rettmp;
      }
      abce_cpop(abce);
      break;
    }
    case ABCE_OPCODE_DUP_NONRECURSIVE:
    {
      struct abce_mb *mbold;
      struct abce_mb *mbnew;
      size_t i;
      GETMBPTR(&mbold, -1);
      if (abce_unlikely(mbold->typ != ABCE_T_A && mbold->typ != ABCE_T_T)) // FIXME T_PB
      {
        abce->err.code = ABCE_E_EXPECT_ARRAY_OR_TREE; // FIXME split
        abce_mb_errreplace_noinline(abce, mbold);
        return -EINVAL;
      }
      if (mbold->typ == ABCE_T_A)
      {
        mbnew = abce_mb_cpush_create_array(abce);
        if (mbnew == NULL)
        {
          return -ENOMEM;
        }
        for (i = 0; i < mbold->u.area->u.ar.size; i++)
        {
          if (abce_mb_array_append(abce, mbnew, &mbold->u.area->u.ar.mbs[i]) != 0)
          {
            abce_cpop(abce);
            return -ENOMEM;
          }
        }
	abce_npoppushc(abce, 1);
        abce_cpop(abce);
        return 0;
      }
      else if (mbold->typ == ABCE_T_T)
      {
        const struct abce_mb *key, *val;
        const struct abce_mb nil = {.typ = ABCE_T_N};
        mbnew = abce_mb_cpush_create_tree(abce);
        if (mbnew == NULL)
        {
          return -ENOMEM;
        }
        key = &nil;
        while (abce_tree_get_next(abce, &key, &val, mbold, key) == 0)
        {
          if (abce_tree_set_str(abce, mbold, key, val) != 0)
          {
            abce_cpop(abce);
            return -ENOMEM;
          }
        }
	abce_npoppushc(abce, 1);
        abce_cpop(abce);
      }
      else
      {
        abort();
      }
      abort();
    }
    case ABCE_OPCODE_DICTNEXT_SAFE:
    /*
     * stack before: (bottom) - oldkey - dictloc - (top)
     * stack after: (bottom) - newkey - newval - (top)
     *
     * How to use in loops (except this doesn't support "break"):
     *   push nil
     * iter:
     *   push -1 # dict location after popping both
     *   getnext # pushes new key and val
     *   push -2
     *   push_stack
     *   ABCE_OPCODE_TYPE
     *   push ABCE_T_N
     *   ne
     *   push addressof(iterend)
     *   if_not_jmp_fwd
     *     stmt1
     *     stmt2
     *     stmt3
     *   pop
     *   push addressof(iter)
     *   jmp
     * iterend:
     *
     * How to use in loops with break:
     *   push nil
     *   push addressof(iter)
     *   jmp
     * break:
     *   push addresof(iterend)
     *   jmp
     * iter:
     *   push -1 # dict location after popping both
     *   getnext # pushes new key and val
     *   push -2
     *   push_stack
     *   ABCE_OPCODE_TYPE
     *   push ABCE_T_N
     *   ne
     *   push addressof(iterend)
     *   if_not_jmp_fwd
     *     stmt1
     *     stmt2
     *     stmt3
     *   pop
     *   push addressof(iter)
     *   jmp
     * iterend:
     *
     * How to use to get just the next key:
     * push oldkey
     * push dictloc
     * getnext # stack after: (b) - newkey - newval - (t)
     * pop # stack after: (b) - newkey - (t)
     */
    {
      struct abce_mb *mboldkey, *mbt;
      const struct abce_mb *mbreskey, *mbresval;
      int64_t dictidx = -3;
      int rettmp;
      int prev = 0;
      VERIFYMB(-1, ABCE_T_B);
      GETBOOLEAN(&prev, -1);
      GETMBPTR(&mboldkey, -2);
      if (abce_unlikely(mboldkey->typ != ABCE_T_N && mboldkey->typ != ABCE_T_S))
      {
        abce->err.code = ABCE_E_TREE_ITER_NOT_STR_OR_NUL;
        abce_mb_errreplace_noinline(abce, mboldkey);
        ret = -EINVAL;
        break;
      }
      rettmp = abce_verifymb(abce, dictidx, ABCE_T_T);
      if (rettmp != 0)
      {
        ret = rettmp;
        break;
      }
      if (abce_getmbptr(&mbt, abce, dictidx) != 0)
      {
        abce_maybeabort();
      }
      int itres = 0;
      if (prev)
      {
        itres = abce_tree_get_prev(abce, &mbreskey, &mbresval, mbt, mboldkey);
      }
      else
      {
        itres = abce_tree_get_next(abce, &mbreskey, &mbresval, mbt, mboldkey);
      }
      if (itres != 0)
      {
        struct abce_mb nil;
        nil.typ = ABCE_T_N;
        nil.u.d = 0;
        abce_mb_stackreplace(abce, -2, &nil);
        abce_mb_stackreplace(abce, -1, &nil);
        break;
      }
      abce_mb_stackreplace(abce, -2, mbreskey);
      abce_mb_stackreplace(abce, -1, mbresval);
      break;
    }
    case ABCE_OPCODE_STR2PB:
    {
      struct abce_mb *mbstr;
      size_t sz;
      struct abce_mb *mb;
      GETMBSTRPTR(&mbstr, -1);
      sz = mbstr->u.area->u.str.size;
      mb = abce_mb_cpush_create_pb_from_buf(abce, mbstr->u.area->u.str.buf, sz);
      if (mb == NULL)
      {
        return -ENOMEM;
      }
      abce_npoppushc(abce, 1);
      abce_cpop(abce);
      break;
    }
    case ABCE_OPCODE_PB2STR:
    {
      struct abce_mb *mbpb;
      struct abce_mb *mb;
      double nbytes;
      double off;
      GETDBL(&nbytes, -1);
      GETDBL(&off, -2);
      GETMBPBPTR(&mbpb, -3);
      if ((double)(size_t)nbytes != nbytes)
      {
        abce->err.code = ABCE_E_INDEX_NOT_INT;
        abce->err.mb.typ = ABCE_T_D;
        abce->err.mb.u.d = nbytes;
        return -ERANGE;
      }
      if ((double)(size_t)off != off)
      {
        abce->err.code = ABCE_E_INDEX_NOT_INT;
        abce->err.mb.typ = ABCE_T_D;
        abce->err.mb.u.d = off;
        return -ERANGE;
      }
      if (nbytes < 0)
      {
        abce->err.code = ABCE_E_NEGATIVE;
        abce->err.mb.typ = ABCE_T_D;
        abce->err.mb.u.d = nbytes;
        return -ERANGE;
      }
      if (off < 0)
      {
        abce->err.code = ABCE_E_NEGATIVE;
        abce->err.mb.typ = ABCE_T_D;
        abce->err.mb.u.d = off;
        return -ERANGE;
      }
      if (mbpb->u.area->u.pb.size < (size_t)nbytes+(size_t)off)
      {
        abce->err.code = ABCE_E_PB_SET_OOB;
        abce->err.mb.typ = ABCE_T_D;
        abce->err.mb.u.d = nbytes;
        return -ERANGE;
      }
      mb = abce_mb_cpush_create_string(abce, &mbpb->u.area->u.pb.buf[(size_t)off], nbytes);
      if (mb == NULL)
      {
        return -ENOMEM;
      }
      abce_npoppushc(abce, 3);
      abce_cpop(abce);
      break;
    }
    case ABCE_OPCODE_CHOMP:
    {
      struct abce_mb *mbstr;
      size_t sz;
      GETMBSTRPTR(&mbstr, -1);
      sz = mbstr->u.area->u.str.size;
      if (sz > 0 && mbstr->u.area->u.str.buf[sz-1] == '\n')
      {
        if (abce_mb_cpush_create_string(abce,
                                        mbstr->u.area->u.str.buf,
                                        sz-1) == NULL)
        {
          return -ENOMEM;
        }
        abce_npoppushc(abce, 1);
        abce_cpop(abce);
      }
      break;
    }
    case ABCE_OPCODE_FILE_OPEN:
    {
      struct abce_mb *mbpath;
      struct abce_mb *mbmode;
      FILE *f;
      GETMBSTRPTR(&mbmode, -1);
      GETMBSTRPTR(&mbpath, -2);
      if (strcmp(mbmode->u.area->u.str.buf, "r") != 0 &&
          strcmp(mbmode->u.area->u.str.buf, "r+") != 0 &&
          strcmp(mbmode->u.area->u.str.buf, "w") != 0 &&
          strcmp(mbmode->u.area->u.str.buf, "w+") != 0 &&
          strcmp(mbmode->u.area->u.str.buf, "a") != 0 &&
          strcmp(mbmode->u.area->u.str.buf, "a+") != 0 &&
          strcmp(mbmode->u.area->u.str.buf, "rb") != 0 &&
          strcmp(mbmode->u.area->u.str.buf, "r+b") != 0 &&
          strcmp(mbmode->u.area->u.str.buf, "wb") != 0 &&
          strcmp(mbmode->u.area->u.str.buf, "w+b") != 0 &&
          strcmp(mbmode->u.area->u.str.buf, "ab") != 0 &&
          strcmp(mbmode->u.area->u.str.buf, "a+b") != 0)
      {
        abce->err.code = ABCE_E_INVALID_MODE;
        abce_mb_errreplace_noinline(abce, mbmode);
        return -EINVAL;
      }
      f = fopen(mbpath->u.area->u.str.buf, mbmode->u.area->u.str.buf);
      if (f == NULL)
      {
        abce->err.code = ABCE_E_FILE_OPEN;
        return -ENOENT;
      }
      if (abce_mb_cpush_create_ios(abce, f) == NULL)
      {
        return -ENOMEM;
      }
      abce_npoppushc(abce, 2);
      abce_cpop(abce);
      break;
    }
    case ABCE_OPCODE_FILE_CLOSE:
    {
      struct abce_mb *mbios;
      GETMBIOSPTR(&mbios, -1);
      if (mbios->u.area->u.ios.f != NULL)
      {
        fclose(mbios->u.area->u.ios.f);
	mbios->u.area->u.ios.f = NULL;
      }
      abce_npoppushnil(abce, 1);
      break;
    }
    case ABCE_OPCODE_FILE_GETDELIM:
    {
      struct abce_mb *mbios;
      struct abce_mb *mbpb;
      struct abce_mb *mbdelim;
      struct abce_mb *mbmaxbytes;
      ssize_t maxbytes;
      double off;
      size_t bytes_read = 0;
      char delim = '\0';
      int hasdelim = 0;
      GETMBPTR(&mbmaxbytes, -1);
      GETDBL(&off, -2);
      GETMBPBPTR(&mbpb, -3);
      GETMBPTR(&mbdelim, -4);
      GETMBIOSPTR(&mbios, -5);
      if (mbdelim->typ == ABCE_T_S)
      {
        if (mbdelim->u.area->u.str.size != 1)
        {
          abce->err.code = ABCE_E_EXPECT_CHAR;
          abce_mb_errreplace_noinline(abce, mbdelim);
          return -EINVAL;
        }
        delim = mbdelim->u.area->u.str.buf[0];
        hasdelim = 1;
      }
      else if (mbdelim->typ == ABCE_T_N)
      {
        delim = '\0';
        hasdelim = 0;
      }
      else
      {
        abce->err.code = ABCE_E_EXPECT_STR;
        abce_mb_errreplace_noinline(abce, mbdelim);
        return -EINVAL;
      }
      if (mbmaxbytes->typ == ABCE_T_N)
      {
        maxbytes = -1;
      }
      else if (mbmaxbytes->typ == ABCE_T_D)
      {
        if ((double)(size_t)mbmaxbytes->u.d != mbmaxbytes->u.d)
        {
          abce->err.code = ABCE_E_INDEX_NOT_INT;
          abce->err.mb.typ = ABCE_T_D;
          abce->err.mb.u.d = mbmaxbytes->u.d;
          return -ERANGE;
        }
        if (mbmaxbytes->u.d < 0)
        {
          abce->err.code = ABCE_E_NEGATIVE;
          abce->err.mb.typ = ABCE_T_D;
          abce->err.mb.u.d = mbmaxbytes->u.d;
          return -ERANGE;
        }
        maxbytes = mbmaxbytes->u.d;
      }
      else
      {
        abce->err.code = ABCE_E_EXPECT_DBL;
        abce_mb_errreplace_noinline(abce, mbmaxbytes);
        return -EINVAL;
      }
      if ((double)(size_t)off != off)
      {
        abce->err.code = ABCE_E_INDEX_NOT_INT;
        abce->err.mb.typ = ABCE_T_D;
        abce->err.mb.u.d = off;
        return -ERANGE;
      }
      if (off < 0)
      {
        abce->err.code = ABCE_E_NEGATIVE;
        abce->err.mb.typ = ABCE_T_D;
        abce->err.mb.u.d = off;
        return -ERANGE;
      }
      if (mbios->u.area->u.ios.f == NULL)
      {
        abce->err.code = ABCE_E_FILE_IS_CLOSED;
        return -EINVAL;
      }
#if 0
      if (hasdelim && maxbytes == -1)
      {
        size_t n = 0;
        ssize_t bytes_read;
        char *lineptr = NULL;
        bytes_read = getdelim(&lineptr, &n, delim, mbios->u.area->u.ios.f);
        if (bytes_read > 0 && bytes_read + (size_t)off > mbpb->u.area->u.pb.size)
        {
          if (abce_mb_pb_resize(abce, mbpb, bytes_read + (size_t)off))
          {
            free(lineptr);
            return -ENOMEM;
          }
        }
        if (bytes_read > 0)
        {
          memcpy(&mbpb->u.area->u.pb.buf[(size_t)off], lineptr, bytes_read);
        }
        free(lineptr);
        abce_npoppushdbl(abce, 5, bytes_read);
        break;
      }
      else
#endif
      {
        int chr;
        size_t curoff = (size_t)off;
        int resized = 0;
        for (;;)
	{
          chr = getc(mbios->u.area->u.ios.f);
	  if (chr == EOF)
	  {
            if (resized && abce_mb_pb_resize(abce, mbpb, curoff))
            {
              return -ENOMEM;
            }
            break;
	  }
          if (curoff >= mbpb->u.area->u.pb.size)
          {
            if (abce_mb_pb_resize(abce, mbpb, curoff + mbpb->u.area->u.pb.size + 1))
            {
              return -ENOMEM;
            }
            resized = 1;
          }
          mbpb->u.area->u.pb.buf[curoff] = (char)chr;
          curoff++;
          bytes_read++;
          if ((char)chr == delim && hasdelim)
          {
            if (resized && abce_mb_pb_resize(abce, mbpb, curoff))
            {
              return -ENOMEM;
            }
            break;
          }
          if (maxbytes >= 0 && curoff-(size_t)off >= maxbytes)
          {
            if (resized && abce_mb_pb_resize(abce, mbpb, curoff))
            {
              return -ENOMEM;
            }
            break;
          }
        }
      }
      abce_npoppushdbl(abce, 5, bytes_read);
      break;
    }
    case ABCE_OPCODE_FILE_GET:
    {
      struct abce_mb *mbios;
      struct abce_mb *mbpb;
      double nbytes;
      double off;
      size_t bytes_read;
      GETDBL(&nbytes, -1);
      GETDBL(&off, -2);
      GETMBPBPTR(&mbpb, -3);
      GETMBIOSPTR(&mbios, -4);
      if ((double)(size_t)nbytes != nbytes)
      {
        abce->err.code = ABCE_E_INDEX_NOT_INT;
        abce->err.mb.typ = ABCE_T_D;
        abce->err.mb.u.d = nbytes;
        return -ERANGE;
      }
      if ((double)(size_t)off != off)
      {
        abce->err.code = ABCE_E_INDEX_NOT_INT;
        abce->err.mb.typ = ABCE_T_D;
        abce->err.mb.u.d = off;
        return -ERANGE;
      }
      if (nbytes < 0)
      {
        abce->err.code = ABCE_E_NEGATIVE;
        abce->err.mb.typ = ABCE_T_D;
        abce->err.mb.u.d = nbytes;
        return -ERANGE;
      }
      if (off < 0)
      {
        abce->err.code = ABCE_E_NEGATIVE;
        abce->err.mb.typ = ABCE_T_D;
        abce->err.mb.u.d = off;
        return -ERANGE;
      }
      if (mbpb->u.area->u.pb.size < (size_t)nbytes+(size_t)off)
      {
        abce->err.code = ABCE_E_PB_SET_OOB;
        abce->err.mb.typ = ABCE_T_D;
        abce->err.mb.u.d = nbytes;
        return -ERANGE;
      }
      if (mbios->u.area->u.ios.f == NULL)
      {
        abce->err.code = ABCE_E_FILE_IS_CLOSED;
        return -EINVAL;
      }
      bytes_read = fread(&mbpb->u.area->u.pb.buf[(size_t)off], 1, (size_t)nbytes, mbios->u.area->u.ios.f);
      abce_npoppushdbl(abce, 4, bytes_read);
      break;
    }
    case ABCE_OPCODE_FILE_SEEK_TELL:
    {
      struct abce_mb *mbios;
      double whence;
      double off;
      int iwhence;
      GETDBL(&whence, -1);
      GETDBL(&off, -2);
      GETMBIOSPTR(&mbios, -3);
      if (whence == 0)
      {
        iwhence = SEEK_SET;
      }
      else if (whence == 1)
      {
        iwhence = SEEK_CUR;
      }
      else if (whence == 2)
      {
        iwhence = SEEK_END;
      }
      else
      {
        abce->err.code = ABCE_E_INVALID_WHENCE;
        abce->err.mb.typ = ABCE_T_D;
        abce->err.mb.u.d = whence;
        return -EINVAL;
      }
      if ((double)(size_t)off != off)
      {
        abce->err.code = ABCE_E_INDEX_NOT_INT;
        abce->err.mb.typ = ABCE_T_D;
        abce->err.mb.u.d = off;
        return -ERANGE;
      }
      if (mbios->u.area->u.ios.f == NULL)
      {
        abce->err.code = ABCE_E_FILE_IS_CLOSED;
        return -EINVAL;
      }
      fseek(mbios->u.area->u.ios.f, (long)off, iwhence);
      abce_npoppushdbl(abce, 3, ftell(mbios->u.area->u.ios.f));
      break;
    }
    case ABCE_OPCODE_FILE_FLUSH:
    {
      struct abce_mb *mbios;
      GETMBIOSPTR(&mbios, -1);
      if (mbios->u.area->u.ios.f == NULL)
      {
        abce->err.code = ABCE_E_FILE_IS_CLOSED;
        return -EINVAL;
      }
      fflush(mbios->u.area->u.ios.f);
      abce_npoppushnil(abce, 1);
      break;
    }
    case ABCE_OPCODE_FILE_WRITE:
    {
      struct abce_mb *mbios;
      struct abce_mb *mbpb;
      double nbytes;
      double off;
      size_t bytes_written;
      GETDBL(&nbytes, -1);
      GETDBL(&off, -2);
      GETMBPBPTR(&mbpb, -3);
      GETMBIOSPTR(&mbios, -4);
      if ((double)(size_t)nbytes != nbytes)
      {
        abce->err.code = ABCE_E_INDEX_NOT_INT;
        abce->err.mb.typ = ABCE_T_D;
        abce->err.mb.u.d = nbytes;
        return -ERANGE;
      }
      if ((double)(size_t)off != off)
      {
        abce->err.code = ABCE_E_INDEX_NOT_INT;
        abce->err.mb.typ = ABCE_T_D;
        abce->err.mb.u.d = off;
        return -ERANGE;
      }
      if (nbytes < 0)
      {
        abce->err.code = ABCE_E_NEGATIVE;
        abce->err.mb.typ = ABCE_T_D;
        abce->err.mb.u.d = nbytes;
        return -ERANGE;
      }
      if (off < 0)
      {
        abce->err.code = ABCE_E_NEGATIVE;
        abce->err.mb.typ = ABCE_T_D;
        abce->err.mb.u.d = off;
        return -ERANGE;
      }
      if (mbpb->u.area->u.pb.size < (size_t)nbytes + (size_t)off)
      {
        abce->err.code = ABCE_E_PB_SET_OOB;
        abce->err.mb.typ = ABCE_T_D;
        abce->err.mb.u.d = nbytes;
        return -ERANGE;
      }
      if (mbios->u.area->u.ios.f == NULL)
      {
        abce->err.code = ABCE_E_FILE_IS_CLOSED;
        return -EINVAL;
      }
      bytes_written = fwrite(&mbpb->u.area->u.pb.buf[(size_t)off], 1, (size_t)nbytes, mbios->u.area->u.ios.f);
      abce_npoppushdbl(abce, 4, bytes_written);
      break;
    }
    case ABCE_OPCODE_LISTSPLICE:
    {
      struct abce_mb *mbar;
      struct abce_mb *mbar2;
      double start, end;
      size_t i;
      GETDBL(&end, -1);
      if (end < 0)
      {
        abce->err.code = ABCE_E_INDEX_OOB;
        abce->err.mb.typ = ABCE_T_D;
        abce->err.mb.u.d = end;
        return -ERANGE;
      }
      if ((double)(size_t)end != end)
      {
        abce->err.code = ABCE_E_INDEX_NOT_INT;
        abce->err.mb.typ = ABCE_T_D;
        abce->err.mb.u.d = end;
        return -ERANGE;
      }
      GETDBL(&start, -2);
      if (start < 0)
      {
        abce->err.code = ABCE_E_INDEX_OOB;
        abce->err.mb.typ = ABCE_T_D;
        abce->err.mb.u.d = start;
        return -ERANGE;
      }
      if ((double)(size_t)start != start)
      {
        abce->err.code = ABCE_E_INDEX_NOT_INT;
        abce->err.mb.typ = ABCE_T_D;
        abce->err.mb.u.d = start;
        return -ERANGE;
      }
      if (end < start)
      {
        abce->err.code = ABCE_E_INDEX_OOB;
        abce->err.mb.typ = ABCE_T_D;
        abce->err.mb.u.d = start;
        abce->err.val2 = end;
        return -ERANGE;
      }
      GETMBARPTR(&mbar, -3);
      if (end > mbar->u.area->u.ar.size)
      {
        abce->err.code = ABCE_E_INDEX_OOB;
        abce->err.mb.typ = ABCE_T_D;
        abce->err.mb.u.d = end;
        return -ERANGE;
      }
      mbar2 = abce_mb_cpush_create_array(abce);
      if (mbar2 == NULL)
      {
        return -ENOMEM;
      }
      for (i = start; i < end; i++)
      {
        if (abce_mb_array_append(abce, mbar2, &mbar->u.area->u.ar.mbs[i]) != 0)
        {
          abce_cpop(abce);
          return -ENOMEM;
        }
      }
      abce_npoppushc(abce, 3);
      abce_cpop(abce);
      break;
    }
    case ABCE_OPCODE_JSON_ENCODE:
    {
      int ret;
      struct abce_mb *mb;
      GETMBPTR(&mb, -1);
      ret = abce_json_encode_cpush(abce, mb);
      if (ret != 0)
      {
        return ret;
      }
      abce_npoppushc(abce, 1);
      abce_cpop(abce);
      break;
    }
    case ABCE_OPCODE_JSON_DECODE:
    {
      int ret;
      struct abce_mb *mbstr;
      struct abce_pullcaj_ctx ctx;
      struct abce_pullcaj_event_info ev;
      int pushed = 0;
      size_t pushedc = 0;
      struct abce_mb *mbtop;
      struct abce_mb *mb;
      struct abce_mb *mbkey;
      GETMBSTRPTR(&mbstr, -1);
      abce_pullcaj_init(&ctx);
      abce_pullcaj_set_buf(&ctx, mbstr->u.area->u.str.buf, mbstr->u.area->u.str.size, 1);
      while ((ret = abce_pullcaj_get_event(&ctx, &ev)) > 0)
      {
        switch (ev.ev) { // ev.key, ev.keysz
          case ABCE_CAJ_EV_START_DICT:
            if (!pushed)
            {
              mbtop = abce_mb_cpush_create_tree(abce);
              if (mbtop == NULL)
              {
                abce_ncpop(abce, pushedc);
                return -ENOMEM;
              }
              pushedc++;
            }
            else if (ev.key)
            {
              mb = abce_mb_cpush_create_tree(abce);
              if (mb == NULL)
              {
                abce_ncpop(abce, pushedc);
                return -ENOMEM;
              }
              pushedc++;
              mbkey = abce_mb_cpush_create_string(abce, ev.key, ev.keysz);
              if (mbkey == NULL)
              {
                abce_ncpop(abce, pushedc);
                return -ENOMEM;
              }
              pushedc++;
              abce_tree_set_str(abce, &abce->cstackbase[abce->csp-3], mbkey, mb);
              abce_cpop(abce);
              pushedc--;
            }
            else
            {
              mb = abce_mb_cpush_create_tree(abce);
              if (mbtop == NULL)
              {
                abce_ncpop(abce, pushedc);
                return -ENOMEM;
              }
              pushedc++;
              abce_mb_array_append(abce, &abce->cstackbase[abce->csp-2], mb);
            }
            break;
          case ABCE_CAJ_EV_END_DICT:
            abce_cpop(abce);
            pushedc--;
            break;
          case ABCE_CAJ_EV_START_ARRAY:
            if (!pushed)
            {
              mbtop = abce_mb_cpush_create_array(abce);
              if (mbtop == NULL)
              {
                abce_ncpop(abce, pushedc);
                return -ENOMEM;
              }
              pushedc++;
            }
            else if (ev.key)
            {
              mb = abce_mb_cpush_create_array(abce);
              if (mb == NULL)
              {
                abce_ncpop(abce, pushedc);
                return -ENOMEM;
              }
              pushedc++;
              mbkey = abce_mb_cpush_create_string(abce, ev.key, ev.keysz);
              if (mbkey == NULL)
              {
                abce_ncpop(abce, pushedc);
                return -ENOMEM;
              }
              pushedc++;
              abce_tree_set_str(abce, &abce->cstackbase[abce->csp-3], mbkey, mb);
              abce_cpop(abce);
              pushedc--;
            }
            else
            {
              mb = abce_mb_cpush_create_array(abce);
              if (mb == NULL)
              {
                abce_ncpop(abce, pushedc);
                return -ENOMEM;
              }
              pushedc++;
              abce_mb_array_append(abce, &abce->cstackbase[abce->csp-2], mb);
            }
            break;
          case ABCE_CAJ_EV_END_ARRAY:
            abce_cpop(abce);
            pushedc--;
            break;
          case ABCE_CAJ_EV_NULL:
            if (!pushed)
            {
              if (abce_cpush_nil(abce) != 0)
              {
                abce_ncpop(abce, pushedc);
                return -ENOMEM;
              }
              pushedc++;
              mbtop = &abce->cstackbase[abce->csp-1];
            }
            else if (ev.key)
            {
              if (abce_cpush_nil(abce) != 0)
              {
                abce_ncpop(abce, pushedc);
                return -ENOMEM;
              }
              pushedc++;
              mb = &abce->cstackbase[abce->csp-1];
              mbkey = abce_mb_cpush_create_string(abce, ev.key, ev.keysz);
              if (mbkey == NULL)
              {
                abce_ncpop(abce, pushedc);
                return -ENOMEM;
              }
              pushedc++;
              abce_tree_set_str(abce, &abce->cstackbase[abce->csp-3], mbkey, mb);
              abce_cpop(abce);
              pushedc--;
              abce_cpop(abce);
              pushedc--;
            }
            else
            {
              if (abce_cpush_nil(abce) != 0)
              {
                abce_ncpop(abce, pushedc);
                return -ENOMEM;
              }
              pushedc++;
              mb = &abce->cstackbase[abce->csp-1];
              abce_mb_array_append(abce, &abce->cstackbase[abce->csp-2], mb);
              abce_cpop(abce);
              pushedc--;
            }
            break;
          case ABCE_CAJ_EV_STR:
            // ev.u.str.val, ev.u.str.valsz
            if (!pushed)
            {
              mbtop = abce_mb_cpush_create_string(abce, ev.u.str.val, ev.u.str.valsz);
              if (mbtop == NULL)
              {
                abce_ncpop(abce, pushedc);
                return -ENOMEM;
              }
              pushedc++;
            }
            else if (ev.key)
            {
              mb = abce_mb_cpush_create_string(abce, ev.u.str.val, ev.u.str.valsz);
              if (mb == NULL)
              {
                abce_ncpop(abce, pushedc);
                return -ENOMEM;
              }
              pushedc++;
              mbkey = abce_mb_cpush_create_string(abce, ev.key, ev.keysz);
              if (mbkey == NULL)
              {
                abce_ncpop(abce, pushedc);
                return -ENOMEM;
              }
              pushedc++;
              abce_tree_set_str(abce, &abce->cstackbase[abce->csp-3], mbkey, mb);
              abce_cpop(abce);
              pushedc--;
              abce_cpop(abce);
              pushedc--;
            }
            else
            {
              mb = abce_mb_cpush_create_string(abce, ev.u.str.val, ev.u.str.valsz);
              if (mb == NULL)
              {
                abce_ncpop(abce, pushedc);
                return -ENOMEM;
              }
              pushedc++;
              abce_mb_array_append(abce, &abce->cstackbase[abce->csp-2], mb);
              abce_cpop(abce);
              pushedc--;
            }
            break;
          case ABCE_CAJ_EV_NUM:
            // ev.u.num.d
            if (!pushed)
            {
              if (abce_cpush_double(abce, ev.u.num.d) != 0)
              {
                abce_ncpop(abce, pushedc);
                return -ENOMEM;
              }
              pushedc++;
              mbtop = &abce->cstackbase[abce->csp-1];
            }
            else if (ev.key)
            {
              if (abce_cpush_double(abce, ev.u.num.d) != 0)
              {
                abce_ncpop(abce, pushedc);
                return -ENOMEM;
              }
              pushedc++;
              mb = &abce->cstackbase[abce->csp-1];
              mbkey = abce_mb_cpush_create_string(abce, ev.key, ev.keysz);
              if (mbkey == NULL)
              {
                abce_ncpop(abce, pushedc);
                return -ENOMEM;
              }
              pushedc++;
              abce_tree_set_str(abce, &abce->cstackbase[abce->csp-3], mbkey, mb);
              abce_cpop(abce);
              pushedc--;
              abce_cpop(abce);
              pushedc--;
            }
            else
            {
              if (abce_cpush_double(abce, ev.u.num.d) != 0)
              {
                abce_ncpop(abce, pushedc);
                return -ENOMEM;
              }
              pushedc++;
              mb = &abce->cstackbase[abce->csp-1];
              abce_mb_array_append(abce, &abce->cstackbase[abce->csp-2], mb);
              abce_cpop(abce);
              pushedc--;
            }
            break;
          case ABCE_CAJ_EV_BOOL:
            // ev.u.b.b
            if (!pushed)
            {
              if (abce_cpush_boolean(abce, ev.u.b.b) != 0)
              {
                abce_ncpop(abce, pushedc);
                return -ENOMEM;
              }
              pushedc++;
              mbtop = &abce->cstackbase[abce->csp-1];
            }
            else if (ev.key)
            {
              if (abce_cpush_boolean(abce, ev.u.b.b) != 0)
              {
                abce_ncpop(abce, pushedc);
                return -ENOMEM;
              }
              pushedc++;
              mb = &abce->cstackbase[abce->csp-1];
              mbkey = abce_mb_cpush_create_string(abce, ev.key, ev.keysz);
              if (mbkey == NULL)
              {
                abce_ncpop(abce, pushedc);
                return -ENOMEM;
              }
              pushedc++;
              abce_tree_set_str(abce, &abce->cstackbase[abce->csp-3], mbkey, mb);
              abce_cpop(abce);
              pushedc--;
              abce_cpop(abce);
              pushedc--;
            }
            else
            {
              if (abce_cpush_boolean(abce, ev.u.b.b) != 0)
              {
                abce_ncpop(abce, pushedc);
                return -ENOMEM;
              }
              pushedc++;
              mb = &abce->cstackbase[abce->csp-1];
              abce_mb_array_append(abce, &abce->cstackbase[abce->csp-2], mb);
              abce_cpop(abce);
              pushedc--;
            }
            break;
        }
      }
      if (pushedc != 1)
      {
        return -EINVAL;
      }
      abce_npoppushc(abce, 1);
      abce_cpop(abce);
      break;
    }
    case ABCE_OPCODE_FP_CLASSIFY:
    {
      double d;
      int c;
      GETDBL(&d, -1);
      switch (fpclassify(d))
      {
        case FP_NAN:
          c = 4;
	  break;
        case FP_INFINITE:
          c = 3;
	  break;
	case FP_ZERO:
          c = 0;
	  break;
	case FP_SUBNORMAL:
	  c = 1;
	  break;
	case FP_NORMAL:
	  c = 2;
	  break;
	default:
	  abort();
      }
      abce_npoppushdbl(abce, 1, c);
      break;
    }
    case ABCE_OPCODE_STRFMT:
    case ABCE_OPCODE_MEMFILE_IOPEN:
    default:
      abce->err.code = ABCE_E_UNKNOWN_INSTRUCTION;
      abce->err.mb.typ = ABCE_T_D;
      abce->err.mb.u.d = ins;
      return -EILSEQ;
  }
  return ret;
}

static struct abce_mb abce_fun_stringify(struct abce *abce, int64_t ip, unsigned char *addcode, size_t addsz)
{
  int64_t ip_tmp = abce->ip;
  uint16_t ins;
  double dbl;
  size_t dblsz;
  const struct abce_mb nil = {.typ = ABCE_T_N};
  struct abce_err err = abce->err;
  abce->ip = ip;
  abce->err.code = ABCE_E_NONE;
  abce->err.opcode = ABCE_OPCODE_NOP;
  abce->err.mb.typ = ABCE_T_N;
  abce->err.mb.u.d = 0;
  //printf("Called stringify %lld\n", (long long)ip);
  for (;;)
  {
    if (abce_fetch_i(&ins, abce, addcode, addsz) != 0)
    {
      abce_err_free(abce, &abce->err);
      abce->err = err;
      //printf("Fetch i\n");
      abce->ip = ip_tmp;
      //printf("Stringifying nil\n");
      return nil;
    }
    //printf("Fetched i %d\n", (int)ins);
    switch (ins)
    {
      case ABCE_OPCODE_PUSH_DBL:
        abce->ip += 8;
        break;
      case ABCE_OPCODE_FUN_TRAILER:
        //printf("Trailer\n");
        if (abce_fetch_d(&dbl, abce, addcode, addsz) != 0)
        {
          abce_err_free(abce, &abce->err);
          abce->err = err;
          abce->ip = ip_tmp;
          return nil;
        }
        dblsz = dbl;
        if (dblsz >= abce->cachesz)
        {
          abce->ip = ip_tmp;
          abce->err = err;
          return nil;
        }
        abce->ip = ip_tmp;
        abce->err = err;
        //printf("Stringifying fun\n");
        return abce_mb_refup(abce, &abce->cachebase[dblsz]);
      case ABCE_OPCODE_FUN_HEADER:
        //printf("Header\n");
        abce->ip = ip_tmp;
        abce->err = err;
        return nil;
      default:
        break;
    }
  }
  abce->ip = ip_tmp;
}

static int abce_bt_gather(struct abce *abce, unsigned char *addcode, size_t addsz)
{
  size_t i;
  if (abce->btsz >= abce->btcap)
  {
    return -ENOMEM;
  }
  abce->btbase[abce->btsz++] = abce_fun_stringify(abce, abce->ip, addcode, addsz);
  for (i = abce->sp; i > 0; i--)
  {
    if (abce->stackbase[i-1].typ == ABCE_T_IP)
    {
      if (abce->btsz >= abce->btcap)
      {
        return -ENOMEM;
      }
      abce->btbase[abce->btsz++] =
        abce_fun_stringify(abce, abce->stackbase[i-1].u.d, addcode, addsz);
    }
    if (abce->stackbase[i-1].typ == ABCE_T_RG)
    {
      return 0;
    }
  }
  return 0;
}

int abce_engine(struct abce *abce, unsigned char *addcode, size_t addsz)
{
  // code:
  const size_t guard = ABCE_GUARD;
  int ret = -EAGAIN;
  double argcnt;
  int64_t new_ip;
  uint16_t ins = 0;
  int was_in_engine;
  size_t oldcsp;
  was_in_engine = abce->in_engine;
  abce->in_engine = 1;
  if (addcode != NULL)
  {
    abce->ip = -(int64_t)addsz-(int64_t)guard;
  }
  else
  {
    abce->ip = 0;
  }
  oldcsp = abce->csp;
  while (ret == -EAGAIN &&
         ((abce->ip >= 0 && (size_t)abce->ip < abce->bytecodesz) ||
         (abce->ip >= -(int64_t)addsz-(int64_t)guard && abce->ip < -(int64_t)guard)))
  {
    oldcsp = abce->csp;
    if (abce_fetch_i(&ins, abce, addcode, addsz) != 0)
    {
      ret = -EFAULT;
      break;
    }
    if (abce_unlikely(abce->ins_budget_fn != NULL))
    {
      int ret2;
      ret2 = abce->ins_budget_fn(&abce->ins_budget_baton, ins);
      if (ret2)
      {
        ret = ret2;
        break;
      }
    }
#if 0
    if (ins_budget == 0)
    {
      abce->err.code = ABCE_E_TIME_BUDGET_EXCEEDED;
      ret = -EDQUOT;
    }
    else if (ins_budget != UINT64_MAX)
    {
      ins_budget--;
    }
#endif
    //printf("fetched ins %d\n", (int)ins); 
    if (abce_likely(ins < 64))
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
            ret = -EOVERFLOW;
            break;
          }
          break;
        }
        case ABCE_OPCODE_PUSH_FALSE:
        {
          if (abce_push_boolean(abce, 0) != 0)
          {
            ret = -EOVERFLOW;
            break;
          }
          break;
        }
        case ABCE_OPCODE_PUSH_NIL:
        {
          if (abce_push_nil(abce) != 0)
          {
            ret = -EOVERFLOW;
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
          const size_t guard = ABCE_GUARD;
          //uint16_t ins2;
          uint8_t inshi;
          //uint8_t inslo;
          int rettmp;
          GETDBL(&argcnt, -1);
          if (abce_unlikely((double)(size_t)argcnt != argcnt))
          {
            abce->err.code = ABCE_E_CALL_ARGCNT_NOT_UINT;
            abce->err.mb.typ = ABCE_T_D;
            abce->err.mb.u.d = argcnt;
            ret = -EINVAL;
            break;
          }
          GETFUNADDR(&new_ip, -2-(int)(size_t)argcnt);
          POP(); // argcnt
calltrailer:
          // FIXME off by one?
          if (abce_unlikely(!((new_ip >= 0 && (size_t)new_ip+10 <= abce->bytecodesz) ||
                (new_ip >= -(int64_t)addsz-(int64_t)guard && new_ip+10 <= -(int64_t)guard))))
          {
            abce->err.code = ABCE_E_BYTECODE_FAULT;
            abce->err.mb.typ = ABCE_T_D;
            abce->err.mb.u.d = new_ip;
            ret = -EFAULT;
            break;
          }
          if (abce_unlikely(abce_push_bp(abce) != 0))
          {
            ret = -EOVERFLOW;
            break;
            // Can fail in case of ABCE_OPCODE_CALL_IF_FUN
            //printf("Cannot fail\n");
            //abort(); // Can't fail, we just popped one value
          }
          if (abce_unlikely(abce_push_ip(abce) != 0))
          {
            ret = -EOVERFLOW;
            break;
          }
          abce->ip = new_ip;
          abce->bp = abce->sp - 2 - (int)(uint64_t)argcnt;
#if 1
          rettmp = abce_fetch_b(&inshi, abce, addcode, addsz);
          if (abce_unlikely(rettmp != 0))
          {
            ret = rettmp;
            break;
          }
          if (abce_unlikely(inshi != ABCE_OPCODE_FUN_HEADER))
          {
            abce->err.code = ABCE_E_EXPECT_FUN_HEADER;
            abce->err.mb.typ = ABCE_T_D;
            abce->err.mb.u.d = new_ip;
            ret = -EINVAL;
            break;
          }
#else
          rettmp = abce_fetch_b(&inshi, abce, addcode, addsz);
          if (rettmp != 0)
          {
            ret = rettmp;
            break;
          }
          if (inshi != ((ABCE_OPCODE_FUN_HEADER>>6) | 0xC0))
          {
            ret = -EINVAL;
            break;
          }
          rettmp = abce_fetch_b(&inslo, abce, addcode, addsz);
          if (rettmp != 0)
          {
            ret = rettmp;
            break;
          }
          if (inslo != ((ABCE_OPCODE_FUN_HEADER&0x3F) | 0x80))
          {
            ret = -EINVAL;
            break;
          }
#endif
#if 0
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
#endif
          double dbl;
          rettmp = abce_fetch_d(&dbl, abce, addcode, addsz);
          if (abce_unlikely(rettmp != 0))
          {
            ret = rettmp;
            break;
          }
          if (abce_unlikely(dbl != (double)(uint64_t)argcnt))
          {
            abce->err.code = ABCE_E_INVALID_ARG_CNT;
            abce->err.mb.typ = ABCE_T_D;
            abce->err.mb.u.d = dbl;
            abce->err.val2 = argcnt;
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
          struct abce_mb *mbar;
          GETMBARPTR(&mbar, -1);
          if (mbar->u.area->u.ar.size == 0)
          {
            abce->err.code = ABCE_E_ARRAY_UNDERFLOW;
            ret = -ENOENT;
            break;
          }
          if (abce_cpush_mb(abce, &mbar->u.area->u.ar.mbs[mbar->u.area->u.ar.size-1]) != 0)
          {
            abce->err.code = ABCE_E_STACK_OVERFLOW;
            ret = -EOVERFLOW;
            break;
          }
          abce_mb_refdn(abce, &mbar->u.area->u.ar.mbs[--mbar->u.area->u.ar.size]);
          abce_npoppushc(abce, 1);
          abce_cpop(abce);
          break;
        }
        case ABCE_OPCODE_LISTLEN:
        {
          struct abce_mb *mbar;
          GETMBARPTR(&mbar, -1);
          abce_npoppushdbl(abce, 1, mbar->u.area->u.ar.size);
          break;
        }
        case ABCE_OPCODE_PBSETLEN:
        {
          struct abce_mb *mbpb;
          double sz;
          GETDBL(&sz, -1);
          if ((double)(size_t)sz != sz)
          {
            abce->err.code = ABCE_E_PB_NEW_LEN_NOT_UINT;
            abce->err.mb.typ = ABCE_T_D;
            abce->err.mb.u.d = sz;
            ret = -EINVAL;
            break;
          }
          GETMBPBPTR(&mbpb, -2);
          if (abce_mb_pb_resize(abce, mbpb, (size_t)sz) != 0)
          {
            ret = -ENOMEM;
            break;
          }
          POP();
          POP();
          break;
        }
        case ABCE_OPCODE_PBSET:
        {
          struct abce_mb *mbpb;
          double off;
          size_t ioff;
          double sz;
          double valdbl;
          int isz;
          uint32_t val;
          GETDBL(&valdbl, -1);
          val = (uint32_t)valdbl;
          if ((double)val != valdbl)
          {
            abce->err.code = ABCE_E_PB_VAL_OOB;
            abce->err.mb.typ = ABCE_T_D;
            abce->err.mb.u.d = valdbl;
            ret = -EINVAL;
            break;
          }
          GETDBL(&off, -3);
          GETDBL(&sz, -2);
          if ((double)(size_t)off != off)
          {
            abce->err.code = ABCE_E_PB_OFF_NOT_UINT;
            abce->err.mb.typ = ABCE_T_D;
            abce->err.mb.u.d = valdbl;
            ret = -EINVAL;
            break;
          }
          ioff = off;
          if (sz != 0 && sz != -1 && sz != -2 &&
              sz != 0 && sz != 1 && sz != 2)
          {
            abce->err.code = ABCE_E_PB_OPSZ_INVALID;
            abce->err.mb.typ = ABCE_T_D;
            abce->err.mb.u.d = sz;
            ret = -EINVAL;
            break;
          }
          isz = sz;
          GETMBPBPTR(&mbpb, -4);
          if (off + sz > mbpb->u.area->u.pb.size)
          {
            abce->err.code = ABCE_E_PB_SET_OOB;
            abce->err.mb.typ = ABCE_T_D;
            abce->err.mb.u.d = off;
            ret = -ERANGE;
            break;
          }
          switch (isz)
          {
            case 0:
              if ((uint32_t)(uint8_t)val != val)
              {
                abce->err.code = ABCE_E_PB_VAL_OOB;
                abce->err.mb.typ = ABCE_T_D;
                abce->err.mb.u.d = valdbl;
                ret = -EINVAL;
                goto outpbset;
              }
              abce_hdr_set8h(&mbpb->u.area->u.pb.buf[ioff], val);
              break;
            case 1:
              if ((uint32_t)(uint16_t)val != val)
              {
                abce->err.code = ABCE_E_PB_VAL_OOB;
                abce->err.mb.typ = ABCE_T_D;
                abce->err.mb.u.d = valdbl;
                ret = -EINVAL;
                goto outpbset;
              }
              abce_hdr_set16n(&mbpb->u.area->u.pb.buf[ioff], val);
              break;
            case 2:
              abce_hdr_set32n(&mbpb->u.area->u.pb.buf[ioff], val);
              break;
            case -1:
              if ((uint32_t)(uint16_t)val != val)
              {
                abce->err.code = ABCE_E_PB_VAL_OOB;
                abce->err.mb.typ = ABCE_T_D;
                abce->err.mb.u.d = valdbl;
                ret = -EINVAL;
                goto outpbset;
              }
              abce_hdr_set16n(&mbpb->u.area->u.pb.buf[ioff], abce_bswap16(val));
              break;
            case -2:
              abce_hdr_set32n(&mbpb->u.area->u.pb.buf[ioff], abce_bswap32(val));
              break;
            default:
              abort();
          }
          POP();
          POP();
          POP();
          POP();
outpbset:
          break;
        }
        case ABCE_OPCODE_PBGET:
        {
          struct abce_mb *mbpb;
          double off;
          size_t ioff;
          double sz;
          int isz;
          uint32_t val;
          GETDBL(&off, -2);
          GETDBL(&sz, -1);
          if ((double)(size_t)off != off)
          {
            abce->err.code = ABCE_E_PB_OFF_NOT_UINT;
            abce->err.mb.typ = ABCE_T_D;
            abce->err.mb.u.d = off;
            ret = -EINVAL;
            break;
          }
          ioff = off;
          if (sz != -0 && sz != -1 && sz != -2 &&
              sz != 0 && sz != 1 && sz != 2)
          {
            abce->err.code = ABCE_E_PB_OPSZ_INVALID;
            abce->err.mb.typ = ABCE_T_D;
            abce->err.mb.u.d = sz;
            ret = -EINVAL;
            break;
          }
          isz = sz;
          GETMBPBPTR(&mbpb, -3);
          if (off + sz > mbpb->u.area->u.pb.size)
          {
            abce->err.code = ABCE_E_PB_GET_OOB;
            abce->err.mb.typ = ABCE_T_D;
            abce->err.mb.u.d = off;
            ret = -ERANGE;
            break;
          }
          switch (isz)
          {
            case 0:
              val = abce_hdr_get8h(&mbpb->u.area->u.pb.buf[ioff]);
              break;
            case 1:
              val = abce_hdr_get16n(&mbpb->u.area->u.pb.buf[ioff]);
              break;
            case 2:
              val = abce_hdr_get32n(&mbpb->u.area->u.pb.buf[ioff]);
              break;
            case -1:
              val = abce_bswap16(abce_hdr_get16n(&mbpb->u.area->u.pb.buf[ioff]));
              break;
            case -2:
              val = abce_bswap32(abce_hdr_get32n(&mbpb->u.area->u.pb.buf[ioff]));
              break;
            default:
              abort();
          }
          abce_npoppushdbl(abce, 3, val);
          break;
        }
        case ABCE_OPCODE_PBLEN:
        {
          struct abce_mb *mbpb;
          GETMBPBPTR(&mbpb, -1);
          abce_npoppushdbl(abce, 1, mbpb->u.area->u.pb.size);
          break;
        }
        case ABCE_OPCODE_STRLEN:
        {
          struct abce_mb *mbstr;
          GETMBSTRPTR(&mbstr, -1);
          abce_npoppushdbl(abce, 1, mbstr->u.area->u.str.size);
          break;
        }
        case ABCE_OPCODE_LISTSET:
        {
          struct abce_mb *mbit;
          struct abce_mb *mbar;
          double loc;
          int64_t locint;
          // Note the order. MBAR is the one that's most likely to fail.
          GETDBL(&loc, -2);
          GETMBARPTR(&mbar, -3);
          GETMBPTR(&mbit, -1);
          if (loc != (double)(uint64_t)loc)
          {
            abce->err.code = ABCE_E_INDEX_NOT_INT;
            abce->err.mb.typ = ABCE_T_D;
            abce->err.mb.u.d = loc;
            ret = -EINVAL;
            break;
          }
          locint = loc;
          if (locint < 0 || locint >= mbar->u.area->u.ar.size)
          {
            abce->err.code = ABCE_E_INDEX_OOB;
            abce->err.mb.typ = ABCE_T_D;
            abce->err.mb.u.d = loc;
            ret = -ERANGE;
            break;
          }
	  abce_mb_refreplace(abce, &mbar->u.area->u.ar.mbs[locint], mbit);
          POP();
          POP();
          POP();
          break;
        }
        case ABCE_OPCODE_STRGET:
        {
          struct abce_mb *mbstr;
          double loc;
          int64_t locint;
          GETDBL(&loc, -1);
          GETMBSTRPTR(&mbstr, -2);
          if (loc != (double)(uint64_t)loc)
          {
            abce->err.code = ABCE_E_INDEX_NOT_INT;
            abce->err.mb.typ = ABCE_T_D;
            abce->err.mb.u.d = loc;
            ret = -EINVAL;
            break;
          }
          locint = loc;
          if (locint < 0 || locint >= mbstr->u.area->u.str.size)
          {
            abce->err.code = ABCE_E_INDEX_OOB;
            abce->err.mb.typ = ABCE_T_D;
            abce->err.mb.u.d = loc;
            ret = -ERANGE;
            break;
          }
          abce_npoppushdbl(abce, 2, (unsigned char)mbstr->u.area->u.str.buf[locint]);
          break;
        }
        case ABCE_OPCODE_LISTGET:
        {
          struct abce_mb *mbar;
          double loc;
          int64_t locint;
          GETDBL(&loc, -1);
          GETMBARPTR(&mbar, -2);
          if (loc != (double)(uint64_t)loc)
          {
            abce->err.code = ABCE_E_INDEX_NOT_INT;
            abce->err.mb.typ = ABCE_T_D;
            abce->err.mb.u.d = loc;
            ret = -EINVAL;
            break;
          }
          locint = loc;
          if (locint < 0 || locint >= mbar->u.area->u.ar.size)
          {
            abce->err.code = ABCE_E_INDEX_OOB;
            abce->err.mb.typ = ABCE_T_D;
            abce->err.mb.u.d = loc;
            ret = -ERANGE;
            break;
          }
          abce_npoppush(abce, 2, &mbar->u.area->u.ar.mbs[locint]);
          break;
        }
        case ABCE_OPCODE_EXCHANGE_TOP:
        {
          struct abce_mb mbtmp;
          double loc;
          size_t addr;
          size_t addrm1;
          GETDBL(&loc, -1);
          POP();
          if (loc != (double)(int64_t)loc)
          {
            abce->err.code = ABCE_E_STACK_IDX_NOT_UINT;
            abce->err.mb.typ = ABCE_T_D;
            abce->err.mb.u.d = loc;
            ret = -EINVAL;
            break;
          }
          if (abce_calc_addr(&addr, abce, loc) != 0)
          {
            ret = -EOVERFLOW;
            break;
          }
          if (abce_calc_addr(&addrm1, abce, -1) != 0)
          {
            ret = -EOVERFLOW;
            break;
          }
          mbtmp = abce->stackbase[addr];
          abce->stackbase[addr] = abce->stackbase[addrm1];
          abce->stackbase[addrm1] = mbtmp;
          break;
        }
        case ABCE_OPCODE_PUSH_STACK:
        {
          double loc;
          size_t addr;
          GETDBL(&loc, -1);
          if (loc != (double)(int64_t)loc)
          {
            abce->err.code = ABCE_E_STACK_IDX_NOT_UINT;
            abce->err.mb.typ = ABCE_T_D;
            abce->err.mb.u.d = loc;
            ret = -EINVAL;
            break;
          }
          if (loc < 0)
          {
            loc -= 1; // FIXME overflow?
          }
          if (abce_calc_addr(&addr, abce, loc) != 0)
          {
            ret = -EOVERFLOW;
            break;
          }
          abce_mb_stackreplace(abce, -1, &abce->stackbase[addr]);
          break;
        }
        case ABCE_OPCODE_SET_STACK:
        {
          struct abce_mb *mb;
          double loc;
          size_t addr;
          GETDBL(&loc, -2);
          if (loc != (double)(int64_t)loc)
          {
            abce->err.code = ABCE_E_STACK_IDX_NOT_UINT;
            abce->err.mb.typ = ABCE_T_D;
            abce->err.mb.u.d = loc;
            ret = -EINVAL;
            break;
          }
          GETMBPTR(&mb, -1);
          //POP();
          //POP();
	  if (loc < 0)
	  {
            loc -= 2; // FIXME overflow?
	  }
          if (abce_calc_addr(&addr, abce, loc) != 0)
          {
            ret = -EOVERFLOW;
            break;
          }
	  abce_mb_refreplace(abce, &abce->stackbase[addr], mb);
	  POP();
	  POP();
          //abce_mb_refdn(abce, &abce->stackbase[addr]);
          //abce->stackbase[addr] = mb;
          break;
        }
        case ABCE_OPCODE_RET:
        {
          struct abce_mb *mb;
          //printf("ret, stack size %d\n", (int)abce->sp);
          GETIP(-2);
          //printf("gotten ip\n");
          GETBP(-3);
          //printf("gotten bp\n");
          GETMBPTR(&mb, -1);
          //printf("gotten mb\n");
          //POP(); // retval
          //POP(); // ip
          //POP(); // bp
          //POP(); // funcall address
          abce_npoppush(abce, 4, mb);
          break;
        }
        /* stacktop - cntloc - cntargs - retval - locvar - ip - bp - args */
        case ABCE_OPCODE_RETEX2:
        {
          struct abce_mb *mb;
          double cntloc, cntargs;
          size_t i;
          GETDBL(&cntloc, -1);
          GETDBL(&cntargs, -2);
          if (abce_unlikely(cntloc != (uint32_t)cntloc))
          {
            abce->err.code = ABCE_E_RET_LOCVARCNT_NOT_UINT;
            abce->err.mb.typ = ABCE_T_D;
            abce->err.mb.u.d = cntloc;
            ret = -EINVAL;
            break;
          }
          if (abce_unlikely(cntargs != (uint32_t)cntargs))
          {
            abce->err.code = ABCE_E_RET_ARGCNT_NOT_UINT;
            abce->err.mb.typ = ABCE_T_D;
            abce->err.mb.u.d = cntargs;
            ret = -EINVAL;
            break;
          }
          VERIFYADDR(-5 - cntloc - cntargs);
          GETMBPTR(&mb, -3);
          GETBP(-5-(int64_t)cntloc); // So that stackreplace works
          //printf("BP1 %zu\n", abce->bp);
          if (abce_mb_stackreplace(abce, -6-(int64_t)cntloc-(int64_t)cntargs, mb) != 0)
          {
            abort();
          }
          POP(); // cntloc
          POP(); // cntargs
          POP(); // retval
          for (i = 0; i < cntloc; i++)
          {
            POP();
          }
          GETIP(-1);
          GETBP(-2);
          //printf("BP2 %zu\n", abce->bp);
          POP(); // ip
          POP(); // bp
          for (i = 0; i < cntargs; i++)
          {
            POP();
          }
          //POP(); // funcall address, replaced by stackreplace
          break;
        }
        case ABCE_OPCODE_JMP:
        {
          const size_t guard = ABCE_GUARD;
          double d;
          GETDBL(&d, -1);
          POP();
          new_ip = d;
          if (!((new_ip >= 0 && (size_t)new_ip <= abce->bytecodesz) ||
                (new_ip >= -(int64_t)addsz-(int64_t)guard && new_ip <= -(int64_t)guard)))
          {
            abce->err.code = ABCE_E_BYTECODE_FAULT;
            abce->err.mb.typ = ABCE_T_D;
            abce->err.mb.u.d = new_ip;
            ret = -EFAULT;
            break;
          }
          abce->ip = new_ip;
          break;
        }
        case ABCE_OPCODE_IF_NOT_JMP:
        {
          const size_t guard = ABCE_GUARD;
          int b;
          double d;
          /* Note the clever order of arguments. This allows code within a
           * loop to push false and do an unconditional jump to just right
           * after the expression pushing the boolean value. Thus, the break
           * statement doesn't need to know the jump address forwards; it can
           * jump backwards. The compiler can be a single-pass one, updating
           * only one jump address forwards, the main loop condition test. */
          GETBOOLEAN(&b, -2);
          GETDBL(&d, -1);
          POP();
          POP();
          new_ip = d;
          if (!((new_ip >= 0 && (size_t)new_ip <= abce->bytecodesz) ||
                (new_ip >= -(int64_t)addsz-(int64_t)guard && new_ip <= -(int64_t)guard)))
          {
            abce->err.code = ABCE_E_BYTECODE_FAULT;
            abce->err.mb.typ = ABCE_T_D;
            abce->err.mb.u.d = new_ip;
            ret = -EFAULT;
            break;
          }
          if (!b)
          {
            abce->ip = new_ip;
          }
          break;
        }
        case ABCE_OPCODE_TOP:
        {
          struct abce_mb *mb;
          GETMBPTR(&mb, -1);
          if (abce_push_mb(abce, mb) != 0)
          {
            abce->err.code = ABCE_E_STACK_OVERFLOW;
            abce_mb_errreplace_noinline(abce, mb);
            ret = -EOVERFLOW;
            break;
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
            abce_maybeabort();
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
            abce_maybeabort();
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
            abce_maybeabort();
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
            abce_maybeabort();
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
            abce_maybeabort();
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
            abce_maybeabort();
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
            abce_maybeabort();
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
            abce_maybeabort();
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
            abce_maybeabort();
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
            abce_maybeabort();
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
            abce_maybeabort();
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
            abce_maybeabort();
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
            abce_maybeabort();
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
            abce_maybeabort();
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
            abce_maybeabort();
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
            abce_maybeabort();
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
            abce_maybeabort();
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
            abce_maybeabort();
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
            abce_maybeabort();
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
            abce_maybeabort();
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
            abce_maybeabort();
          }
          break;
        }
        case ABCE_OPCODE_INT_TO_UINT:
        {
          double d, sz;
          uint8_t i;
          GETDBL(&d, -1);
          GETDBL(&sz, -2);
          i = sz;
#if 0
          if (i != sz)
          {
            abce->err.code = ABCE_E_INTCONVERT_SZ_NOT_UINT;
            abce->err.mb.typ = ABCE_T_D;
            abce->err.mb.u.d = sz;
            ret = -EINVAL;
            break;
          }
#endif
          if (sz != 0 && sz != 1 && sz != 2)
          {
            abce->err.code = ABCE_E_INTCONVERT_SZ_NOTSUP;
            abce->err.mb.typ = ABCE_T_D;
            abce->err.mb.u.d = i;
            ret = -EINVAL;
            break;
          }
          POP();
          POP();
          switch (i)
          {
            case 0:
              if (abce_push_double(abce, (uint8_t)(int8_t)d) != 0)
              {
                abce_maybeabort();
              }
              break;
            case 1:
              if (abce_push_double(abce, (uint16_t)(int16_t)d) != 0)
              {
                abce_maybeabort();
              }
              break;
            case 2:
              if (abce_push_double(abce, (uint32_t)(int32_t)d) != 0)
              {
                abce_maybeabort();
              }
              break;
            default:
              abce_maybeabort();
          }
          break;
        }
        case ABCE_OPCODE_UINT_TO_INT:
        {
          double d, sz;
          uint8_t i;
          GETDBL(&d, -1);
          GETDBL(&sz, -2);
          i = sz;
#if 0
          if (i != sz)
          {
            abce->err.code = ABCE_E_INTCONVERT_SZ_NOT_UINT;
            abce->err.mb.typ = ABCE_T_D;
            abce->err.mb.u.d = sz;
            ret = -EINVAL;
            break;
          }
#endif
          if (sz != 0 && sz != 1 && sz != 2)
          {
            abce->err.code = ABCE_E_INTCONVERT_SZ_NOTSUP;
            abce->err.mb.typ = ABCE_T_D;
            abce->err.mb.u.d = i;
            ret = -EINVAL;
            break;
          }
          POP();
          POP();
          switch (i)
          {
            case 0:
              if (abce_push_double(abce, (int8_t)(uint8_t)d) != 0)
              {
                abce_maybeabort();
              }
              break;
            case 1:
              if (abce_push_double(abce, (int16_t)(uint16_t)d) != 0)
              {
                abce_maybeabort();
              }
              break;
            case 2:
              if (abce_push_double(abce, (int32_t)(uint32_t)d) != 0)
              {
                abce_maybeabort();
              }
              break;
            default:
              abce_maybeabort();
          }
          break;
        }
        case ABCE_OPCODE_APPEND_MAINTAIN:
        {
          struct abce_mb *mb;
          struct abce_mb *mbar;
          GETMBARPTR(&mbar, -2);
          GETMBPTR(&mb, -1); // can't fail if GETMBAR succeeded
          if (abce_mb_array_append(abce, mbar, mb) != 0)
          {
            ret = -ENOMEM;
          }
          POP(); // do this late to avoid confusing GC
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
            abce->err.code = ABCE_E_CACHE_IDX_NOT_INT;
            abce->err.mb.typ = ABCE_T_D;
            abce->err.mb.u.d = dbl;
            ret = -EINVAL;
            break;
          }
          i64 = dbl;
          if (i64 >= abce->cachesz)
          {
            abce->err.code = ABCE_E_CACHE_IDX_OOB;
            abce->err.mb.typ = ABCE_T_D;
            abce->err.mb.u.d = dbl;
            ret = -EOVERFLOW;
            break;
          }
          if (abce_push_mb(abce, &abce->cachebase[i64]) != 0)
          {
            abce_maybeabort();
          }
          break;
        }
        case ABCE_OPCODE_SCOPEVAR:
        {
          struct abce_mb *mbsc, *mbit;
          const struct abce_mb *ptr;
          GETMBSCPTR(&mbsc, -2);
          GETMBPTR(&mbit, -1);
          if (abce_unlikely(mbit->typ != ABCE_T_S))
          {
            abce->err.code = ABCE_E_EXPECT_STR;
            abce_mb_errreplace_noinline(abce, mbit);
            abce->err.val2 = -2;
            ret = -EINVAL;
            break;
          }
          ptr = abce_sc_get_rec_mb(mbsc, mbit, 1);
          if (abce_unlikely(ptr == NULL))
          {
            abce->err.code = ABCE_E_SCOPEVAR_NOT_FOUND;
            abce_mb_errreplace_noinline(abce, mbit);
            ret = -ENOENT;
            break;
          }
          abce_npoppush(abce, 2, ptr);
          break;
        }
        case ABCE_OPCODE_SCOPE_HAS:
        {
          struct abce_mb *mbsc, *mbit;
          const struct abce_mb *ptr;
          GETMBSCPTR(&mbsc, -2);
          GETMBPTR(&mbit, -1);
          if (abce_unlikely(mbit->typ != ABCE_T_S))
          {
            abce->err.code = ABCE_E_EXPECT_STR;
            abce_mb_errreplace_noinline(abce, mbit);
            abce->err.val2 = -2;
            ret = -EINVAL;
            break;
          }
          ptr = abce_sc_get_rec_mb(mbsc, mbit, 1);
          abce_npoppushbool(abce, 2, ptr != NULL);
          break;
        }
        case ABCE_OPCODE_GETSCOPE_DYN:
        {
          if (abce_unlikely(abce_push_mb(abce, &abce->dynscope) != 0))
          {
            abce->err.code = ABCE_E_STACK_OVERFLOW;
            abce_mb_errreplace_noinline(abce, &abce->dynscope);
            ret = -EOVERFLOW;
            break;
          }
          break;
        }
        case ABCE_OPCODE_TYPE:
        {
          struct abce_mb *mb;
          GETMBPTR(&mb, -1);
          abce_npoppushdbl(abce, 1, mb->typ);
          break;
        }
        case ABCE_OPCODE_DICTSET_MAINTAIN:
        {
          struct abce_mb *mbt, *mbstr;
          struct abce_mb *mbval;
          VERIFYMB(-3, ABCE_T_T);
          VERIFYMB(-2, ABCE_T_S);
          GETMBPTR(&mbt, -3);
          GETMBPTR(&mbstr, -2);
          GETMBPTR(&mbval, -1);
          if (abce_tree_set_str(abce, mbt, mbstr, mbval) != 0)
          {
            ret = -ENOMEM;
            // No break: we fall through
          }
          POP(); // do this late to avoid confusing GC
          POP();
          break;
        }
        case ABCE_OPCODE_DICTGET:
        {
          struct abce_mb *mbt, *mbstr;
          struct abce_mb *mbval;
          VERIFYMB(-2, ABCE_T_T);
          VERIFYMB(-1, ABCE_T_S);
          GETMBPTR(&mbt, -2);
          GETMBPTR(&mbstr, -1);
          if (abce_tree_get_str(abce, &mbval, mbt, mbstr) == 0)
          {
            abce_npoppush(abce, 2, (const struct abce_mb*)mbval);
          }
          else
          {
            abce_npoppushnil(abce, 2); // FIXME really nil? Should be error?
          }
          break;
        }
        case ABCE_OPCODE_DICTHAS:
        {
          struct abce_mb *mbt, *mbstr;
          struct abce_mb *mbval;
          VERIFYMB(-2, ABCE_T_T);
          VERIFYMB(-1, ABCE_T_S);
          GETMBPTR(&mbt, -2);
          GETMBPTR(&mbstr, -1);
          abce_npoppushbool(abce, 2, abce_tree_get_str(abce, &mbval, mbt, mbstr) == 0);
          break;
        }
        case ABCE_OPCODE_CALL_IF_FUN:
        {
          struct abce_mb *mb;
          GETMBPTR(&mb, -1);
          if (mb->typ == ABCE_T_F)
          {
#if 0
            const size_t guard = ABCE_GUARD;
            double argcnt = 0.0;
            int64_t new_ip;
            uint16_t ins2;
            int rettmp;
#endif
            GETFUNADDR(&new_ip, -1);
            //POP(); // funcall address popped by RET instruction
            argcnt = 0.0;
            goto calltrailer;
#if 0
            // FIXME off by one?
            if (!((new_ip >= 0 && (size_t)new_ip+10 <= abce->bytecodesz) ||
                  (new_ip >= -(int64_t)addsz-(int64_t)guard && new_ip+10 <= -(int64_t)guard)))
            {
              ret = -EFAULT;
              break;
            }
            if (abce_push_bp(abce) != 0)
            {
              abort();
            }
            if (abce_push_ip(abce) != 0)
            {
              POP();
              ret = -EOVERFLOW;
              break;
            }
            abce->ip = new_ip;
            abce->bp = abce->sp - 2 - 0;
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
#endif
          }
          break;
        }
        case ABCE_OPCODE_DICTLEN:
        {
          struct abce_mb *mbt;
          VERIFYMB(-1, ABCE_T_T);
          GETMBPTR(&mbt, -1);
	  abce_npoppushdbl(abce, 1, mbt->u.area->u.tree.sz);
          break;
        }
        case ABCE_OPCODE_SCOPEVAR_SET:
        {
          struct abce_mb *mbsc, *mbs, *mbv;
          int rettmp;
          VERIFYMB(-3, ABCE_T_SC);
          VERIFYMB(-2, ABCE_T_S);
          GETMBPTR(&mbsc, -3);
          GETMBPTR(&mbs, -2);
          GETMBPTR(&mbv, -1);
          rettmp = abce_sc_replace_val_mb(abce, mbsc, mbs, mbv);
          if (rettmp != 0)
          {
            ret = rettmp;
            // No break: we want to call all refdn statements
          }
          POP(); // Do this late to avoid confusing GC
          POP();
          POP();
          break;
        }
        case ABCE_OPCODE_DICTDEL:
        {
          struct abce_mb *mbt, *mbstr;
          VERIFYMB(-2, ABCE_T_T);
          VERIFYMB(-1, ABCE_T_S);
          GETMBPTR(&mbt, -2);
          GETMBPTR(&mbstr, -1);
          if (abce_unlikely(abce_tree_del_str(abce, mbt, mbstr) != 0))
          {
            abce->err.code = ABCE_E_TREE_ENTRY_NOT_FOUND;
            abce_mb_errreplace_noinline(abce, mbstr);
            ret = -ENOENT;
            // No break: we want to call all refdn statements
          }
          POP(); // Just in case deleting allocates memory, do this late
          POP();
          break;
        }
        case ABCE_OPCODE_FUN_HEADER:
          abce->err.code = ABCE_E_RUN_INTO_FUNC;
          ret = -EACCES;
          break;
        default:
        {
          //printf("Invalid instruction %d\n", (int)ins);
          abce->err.code = ABCE_E_UNKNOWN_INSTRUCTION;
          abce->err.mb.typ = ABCE_T_D;
          abce->err.mb.u.d = ins;
          ret = -EILSEQ;
          break;
        }
      }
    }
    else if (abce_likely(ins < 128))
    {
      int ret2 = abce->trap(&abce->trap_baton, ins, addcode, addsz);
      if (ret2 != 0)
      {
        ret = ret2;
        break;
      }
    }
    else if (abce_likely(ins < 0x400))
    {
      int ret2 = abce_mid(abce, ins, addcode, addsz);
      if (ret2 != 0)
      {
        ret = ret2;
        break;
      }
    }
    else if (abce_likely(ins < 0x800))
    {
      int ret2 = abce->trap(&abce->trap_baton, ins, addcode, addsz);
      if (ret2 != 0)
      {
        ret = ret2;
        break;
      }
    }
    else if (abce_likely(ins < 0x8000))
    {
      abce->err.code = ABCE_E_UNKNOWN_INSTRUCTION;
      abce->err.mb.typ = ABCE_T_D;
      abce->err.mb.u.d = ins;
      ret = -EILSEQ;
    }
    else
    {
      int ret2 = abce->trap(&abce->trap_baton, ins, addcode, addsz);
      if (ret2 != 0)
      {
        ret = ret2;
        break;
      }
    }
    if (abce->csp < oldcsp)
    {
      fprintf(stderr, "Warning: C stack shrunk in engine, ins=%u, ret=%d\n", (unsigned)ins, ret);
    }
    else if (abce->csp > oldcsp)
    {
      fprintf(stderr, "Warning: C stack grew in engine, popping stuff, ins=%u, ret=%d\n", (unsigned)ins, ret);
      while (abce->csp > oldcsp)
      {
        abce_cpop(abce);
      }
    }
  }
  if (abce->csp < oldcsp)
  {
    fprintf(stderr, "Warning: C stack shrunk in engine, ins=%u, ret=%d\n", (unsigned)ins, ret);
  }
  else if (abce->csp > oldcsp)
  {
    fprintf(stderr, "Warning: C stack grew in engine, popping stuff, ins=%u, ret=%d\n", (unsigned)ins, ret);
    while (abce->csp > oldcsp)
    {
      abce_cpop(abce);
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
  if (ret != 0)
  {
    abce_bt_gather(abce, addcode, addsz); // RFE re-entrancy
  }
  abce->in_engine = was_in_engine;
  return ret;
}
