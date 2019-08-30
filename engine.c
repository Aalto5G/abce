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
#define GETMB(mb, idx) GETGENERIC(abce_getmb, mb, idx)
#define GETMBSC(mb, idx) GETGENERIC(abce_getmbsc, mb, idx)
#define GETMBAR(mb, idx) GETGENERIC(abce_getmbar, mb, idx)
#define GETMBPB(mb, idx) GETGENERIC(abce_getmbpb, mb, idx)
#define GETMBSTR(mb, idx) GETGENERIC(abce_getmbstr, mb, idx)

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
      struct abce_mb mb = {};
      struct abce_mb funname = {};
      GETDBL(&argcnt, -1);
      if (abce_unlikely((double)(size_t)argcnt != argcnt))
      {
        abce->err.code = ABCE_E_CALL_ARGCNT_NOT_UINT;
        abce->err.mb.typ = ABCE_T_D;
        abce->err.mb.u.d = argcnt;
        ret = -EINVAL;
        break;
      }
      GETMBSTR(&funname, -2-(int)(size_t)argcnt);
      POP(); // argcnt
      lua_getglobal(abce->dynscope.u.area->u.sc.lua, funname.u.area->u.str.buf);
      for (i = argcnt; i > 0; i--)
      {
        GETMB(&mb, -(int)i);
        mb_to_lua(abce->dynscope.u.area->u.sc.lua, &mb);
        abce_mb_refdn(abce, &mb);
      }
      for (i = 0; i < argcnt; i++)
      {
        POP(); // args
      }
      POP(); // funname
      abce_mb_refdn(abce, &funname);
      abce_push_rg(abce); // to capture backtrace in stages
      abce->err.code = ABCE_E_NONE;
      abce->err.mb.typ = ABCE_T_N;
      if (lua_pcall(abce->dynscope.u.area->u.sc.lua, argcnt, 1, 0) != 0)
      {
        abce_pop(abce);
        if (abce->err.code == ABCE_E_NONE)
        {
          struct abce_mb mberrstr = {};
          mb_from_lua(abce->dynscope.u.area->u.sc.lua, abce, -1);
          GETMB(&mberrstr, -1);
          abce->err.code = ABCE_E_LUA_ERR;
          abce->err.mb = mberrstr;
          abce_pop(abce);
        } // otherwise it's error from nested call
        ret = -EINVAL;
        break;
      }
      abce_pop(abce);
      mb_from_lua(abce->dynscope.u.area->u.sc.lua, abce, -1);
      return 0;
    }
#endif
    case ABCE_OPCODE_SCOPEVAR_NONRECURSIVE:
    {
      struct abce_mb mbsc, mbit;
      const struct abce_mb *ptr;
      GETMBSC(&mbsc, -2);
      GETMB(&mbit, -1);
      if (abce_unlikely(mbit.typ != ABCE_T_S))
      {
        abce->err.code = ABCE_E_EXPECT_STR;
        abce->err.mb = abce_mb_refup_noinline(abce, &mbit);
        abce->err.val2 = -2;
        abce_mb_refdn(abce, &mbit);
        abce_mb_refdn_typ(abce, &mbsc, ABCE_T_SC);
        return -EINVAL;
      }
      POP();
      POP();
      ptr = abce_sc_get_rec_mb(&mbsc, &mbit, 0);
      if (abce_unlikely(ptr == NULL))
      {
        abce->err.code = ABCE_E_SCOPEVAR_NOT_FOUND;
        abce->err.mb = abce_mb_refup_noinline(abce, &mbit);
        abce_mb_refdn_typ(abce, &mbit, ABCE_T_S);
        abce_mb_refdn_typ(abce, &mbsc, ABCE_T_SC);
        return -ENOENT;
      }
      if (abce_push_mb(abce, ptr) != 0)
      {
        abce_maybeabort();
      }
      abce_mb_refdn_typ(abce, &mbit, ABCE_T_S);
      abce_mb_refdn_typ(abce, &mbsc, ABCE_T_SC);
      return 0;
    }
    case ABCE_OPCODE_SCOPE_HAS_NONRECURSIVE:
    {
      struct abce_mb mbsc, mbit;
      const struct abce_mb *ptr;
      GETMBSC(&mbsc, -2);
      GETMB(&mbit, -1);
      if (abce_unlikely(mbit.typ != ABCE_T_S))
      {
        abce->err.code = ABCE_E_EXPECT_STR;
        abce->err.mb = abce_mb_refup_noinline(abce, &mbit);
        abce->err.val2 = -2;
        abce_mb_refdn(abce, &mbit);
        abce_mb_refdn_typ(abce, &mbsc, ABCE_T_SC);
        return -EINVAL;
      }
      POP();
      POP();
      ptr = abce_sc_get_rec_mb(&mbsc, &mbit, 0);
      if (abce_push_boolean(abce, ptr != NULL) != 0)
      {
        abce_maybeabort();
      }
      abce_mb_refdn_typ(abce, &mbit, ABCE_T_S);
      abce_mb_refdn_typ(abce, &mbsc, ABCE_T_SC);
      return 0;
    }
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
      rettmp = abce_strgsub_mb(abce, &res, &mbhaystack, &mbneedle, &mbsub);
      if (rettmp != 0)
      {
        abce_mb_refdn(abce, &mbhaystack);
        abce_mb_refdn(abce, &mbneedle);
        abce_mb_refdn(abce, &mbsub);
        return rettmp;
      }
      POP(); // do this late to avoid C-only refs to abce objs, confusing GC
      POP();
      POP();
      abce_push_mb(abce, &res);
      abce_mb_refdn(abce, &res);
      abce_mb_refdn(abce, &mbsub);
      abce_mb_refdn(abce, &mbhaystack);
      abce_mb_refdn(abce, &mbneedle);
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
      struct abce_mb res, mbbase;
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
      GETMBSTR(&mbbase, -3);
      if (end > mbbase.u.area->u.str.size)
      {
        abce_mb_refdn(abce, &mbbase);
        abce->err.code = ABCE_E_INDEX_OOB;
        abce->err.mb.typ = ABCE_T_D;
        abce->err.mb.u.d = end;
        return -ERANGE;
      }
      res = abce_mb_create_string(abce,
                                  mbbase.u.area->u.str.buf + (size_t)start,
                                  end - start);
      if (res.typ == ABCE_T_N)
      {
        return -ENOMEM;
      }
      POP(); // right before push to avoid GC crashes
      POP();
      POP();
      abce_push_mb(abce, &res);
      abce_mb_refdn(abce, &res);
      abce_mb_refdn(abce, &mbbase);
      return 0;
    }
    case ABCE_OPCODE_STR_FROMCHR:
    {
      double ch;
      char chch;
      struct abce_mb res;
      GETDBL(&ch, -1);
      if ((uint64_t)ch < 0 || (uint64_t)ch >= 256 || (double)(uint64_t)ch != ch)
      {
        abce->err.code = ABCE_E_INVALID_CH;
        abce->err.mb.typ = ABCE_T_D;
        abce->err.mb.u.d = ch;
        return -EINVAL;
      }
      chch = (char)(unsigned char)ch;
      res = abce_mb_create_string(abce, &chch, 1);
      if (res.typ == ABCE_T_N)
      {
        return -ENOMEM;
      }
      POP(); // right before push to avoid GC crashes
      abce_push_mb(abce, &res);
      abce_mb_refdn(abce, &res);
      return 0;
    }
    case ABCE_OPCODE_STRAPPEND:
    {
      struct abce_mb res, mbbase, mbextend;

      VERIFYMB(-1, ABCE_T_S);
      VERIFYMB(-2, ABCE_T_S);
      GETMBSTR(&mbextend, -1);
      GETMBSTR(&mbbase, -2);
      res = abce_mb_concat_string(abce,
                                  mbbase.u.area->u.str.buf,
                                  mbbase.u.area->u.str.size,
                                  mbextend.u.area->u.str.buf,
                                  mbextend.u.area->u.str.size);
      if (res.typ == ABCE_T_N)
      {
        abce_mb_refdn(abce, &mbbase);
        abce_mb_refdn(abce, &mbextend);
        return -ENOMEM;
      }
      POP(); // Careful with GC!
      POP();
      abce_push_mb(abce, &res);
      abce_mb_refdn(abce, &res);
      abce_mb_refdn(abce, &mbbase);
      abce_mb_refdn(abce, &mbextend);
      return 0;
    }
    case ABCE_OPCODE_STR_CMP:
    {
      struct abce_mb mb1, mb2;

      VERIFYMB(-1, ABCE_T_S);
      VERIFYMB(-2, ABCE_T_S);
      GETMBSTR(&mb2, -1);
      GETMBSTR(&mb1, -2);
      POP();
      POP();
      if (abce_push_double(abce, abce_str_cmp_sym_mb(&mb1, &mb2)) != 0)
      {
        abort();
      }
      abce_mb_refdn(abce, &mb1);
      abce_mb_refdn(abce, &mb2);
      return 0;
    }
    case ABCE_OPCODE_STR_LOWER:
    {
      struct abce_mb res, mbbase;
      size_t i;

      VERIFYMB(-1, ABCE_T_S);
      GETMBSTR(&mbbase, -1);
      res = abce_mb_create_string(abce,
                                  mbbase.u.area->u.str.buf,
                                  mbbase.u.area->u.str.size);
      if (res.typ == ABCE_T_N)
      {
        abce_mb_refdn(abce, &mbbase);
        return -ENOMEM;
      }
      for (i = 0; i < res.u.area->u.str.size; i++)
      {
        res.u.area->u.str.buf[i] = tolower((unsigned char)res.u.area->u.str.buf[i]);
      }
      POP(); // right before push to avoid GC crashes
      abce_push_mb(abce, &res);
      abce_mb_refdn(abce, &res);
      abce_mb_refdn(abce, &mbbase);
      return 0;
    }
    case ABCE_OPCODE_STR_UPPER:
    {
      struct abce_mb res, mbbase;
      size_t i;

      VERIFYMB(-1, ABCE_T_S);
      GETMBSTR(&mbbase, -1);
      res = abce_mb_create_string(abce,
                                  mbbase.u.area->u.str.buf,
                                  mbbase.u.area->u.str.size);
      if (res.typ == ABCE_T_N)
      {
        abce_mb_refdn(abce, &mbbase);
        return -ENOMEM;
      }
      for (i = 0; i < res.u.area->u.str.size; i++)
      {
        res.u.area->u.str.buf[i] = toupper((unsigned char)res.u.area->u.str.buf[i]);
      }
      POP(); // right before push to avoid GC crashes
      abce_push_mb(abce, &res);
      abce_mb_refdn(abce, &res);
      abce_mb_refdn(abce, &mbbase);
      return 0;
    }
    case ABCE_OPCODE_STRSET:
    {
      double loc, ch;
      struct abce_mb res, mbbase;
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
      GETMBSTR(&mbbase, -1);
      if ((uint64_t)loc < 0 || (uint64_t)loc >= mbbase.u.area->u.str.size)
      {
        abce->err.code = ABCE_E_INDEX_OOB;
        abce->err.mb.typ = ABCE_T_D;
        abce->err.mb.u.d = loc;
        abce_mb_refdn(abce, &mbbase);
        return -EINVAL;
      }
      if ((double)(uint64_t)loc != loc)
      {
        abce->err.code = ABCE_E_INDEX_NOT_INT;
        abce->err.mb.typ = ABCE_T_D;
        abce->err.mb.u.d = loc;
        abce_mb_refdn(abce, &mbbase);
        return -EINVAL;
      }
      res = abce_mb_create_string(abce,
                                  mbbase.u.area->u.str.buf,
                                  mbbase.u.area->u.str.size);
      if (res.typ == ABCE_T_N)
      {
        abce_mb_refdn(abce, &mbbase);
        return -ENOMEM;
      }
      res.u.area->u.str.buf[(uint64_t)loc] = (char)(unsigned char)ch;
      POP(); // right before push to avoid GC crashes
      POP();
      POP();
      abce_push_mb(abce, &res);
      abce_mb_refdn(abce, &res);
      abce_mb_refdn(abce, &mbbase);
      return 0;
    }
    case ABCE_OPCODE_STRWORDCNT:
    {
      struct abce_word_iter it = {};
      struct abce_mb mbbase;
      struct abce_mb mbsep;
      size_t i = 0;
      VERIFYMB(-1, ABCE_T_S);
      VERIFYMB(-2, ABCE_T_S);
      GETMBSTR(&mbsep, -1);
      GETMBSTR(&mbbase, -2);
      POP();
      POP();
      abce_word_iter_init(&it, mbbase.u.area->u.str.buf, mbbase.u.area->u.str.size,
                          mbsep.u.area->u.str.buf, mbsep.u.area->u.str.size);
      while (!abce_word_iter_at_end(&it))
      {
        i++;
        abce_word_iter_next(&it);
      }
      if (abce_push_double(abce, i) != 0)
      {
        abort();
      }
      abce_mb_refdn(abce, &mbbase);
      abce_mb_refdn(abce, &mbsep);
      return 0;
    }
    case ABCE_OPCODE_STRWORDLIST:
    {
      struct abce_word_iter it = {};
      struct abce_mb mbbase;
      struct abce_mb mbsep;
      struct abce_mb mbar;
      struct abce_mb mbit;
      VERIFYMB(-1, ABCE_T_S);
      VERIFYMB(-2, ABCE_T_S);
      GETMBSTR(&mbsep, -1);
      GETMBSTR(&mbbase, -2);
      mbar = abce_mb_create_array(abce);
      if (mbar.typ == ABCE_T_N)
      {
        abce_mb_refdn(abce, &mbbase);
        abce_mb_refdn(abce, &mbsep);
        return -ENOMEM;
      }
      abce->oneblock = abce_mb_refup(abce, &mbar); // make it visible to GC
      abce_word_iter_init(&it, mbbase.u.area->u.str.buf, mbbase.u.area->u.str.size,
                          mbsep.u.area->u.str.buf, mbsep.u.area->u.str.size);
      while (!abce_word_iter_at_end(&it))
      {
        // Here we have to be careful. We allocate memblocks, but we add them immediately
        // to the array, so if GC can see the array, GC can see our memblocks.
        mbit = abce_mb_create_string(abce, 
                                     mbbase.u.area->u.str.buf + it.start,
                                     it.end - it.start);
        if (mbit.typ == ABCE_T_N)
        {
          abce_mb_refdn(abce, &abce->oneblock);
          abce_mb_refdn(abce, &mbbase);
          abce_mb_refdn(abce, &mbsep);
          abce_mb_refdn(abce, &mbar);
          return -ENOMEM;
        }
        if (abce_mb_array_append(abce, &mbar, &mbit) != 0)
        {
          abce_mb_refdn(abce, &abce->oneblock);
          abce_mb_refdn(abce, &mbbase);
          abce_mb_refdn(abce, &mbsep);
          abce_mb_refdn(abce, &mbar);
          abce_mb_refdn(abce, &mbit);
          return -ENOMEM;
        }
        abce_mb_refdn(abce, &mbit);
        abce_word_iter_next(&it);
      }
      POP(); // Do this late to avoid confusing GC
      POP();
      if (abce_push_mb(abce, &mbar) != 0)
      {
        abort();
      }
      abce_mb_refdn(abce, &abce->oneblock);
      abce_mb_refdn(abce, &mbbase);
      abce_mb_refdn(abce, &mbsep);
      abce_mb_refdn(abce, &mbar);
      return 0;
    }
    case ABCE_OPCODE_STRWORD:
    {
      struct abce_word_iter it = {};
      struct abce_mb mbbase;
      struct abce_mb mbsep;
      struct abce_mb mbit;
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
      GETMBSTR(&mbsep, -2);
      GETMBSTR(&mbbase, -3);
      abce_word_iter_init(&it, mbbase.u.area->u.str.buf, mbbase.u.area->u.str.size,
                          mbsep.u.area->u.str.buf, mbsep.u.area->u.str.size);
      while (!abce_word_iter_at_end(&it))
      {
        if (i == (size_t)wordidx)
        {
          mbit = abce_mb_create_string(abce, 
                                       mbbase.u.area->u.str.buf + it.start,
                                       it.end - it.start);
          if (mbit.typ == ABCE_T_N)
          {
            abce_mb_refdn(abce, &mbbase);
            abce_mb_refdn(abce, &mbsep);
            return -ENOMEM;
          }
          POP(); // right before push to avoid GC crashes
          POP();
          POP();
          if (abce_push_mb(abce, &mbit) != 0)
          {
            abort();
          }
          abce_mb_refdn(abce, &mbit);
          abce_mb_refdn(abce, &mbbase);
          abce_mb_refdn(abce, &mbsep);
          return 0;
        }
        i++;
        abce_word_iter_next(&it);
      }
      abce_mb_refdn(abce, &mbbase);
      abce_mb_refdn(abce, &mbsep);
      abce->err.code = ABCE_E_INDEX_OOB;
      abce->err.mb.typ = ABCE_T_D;
      abce->err.mb.u.d = wordidx;
      return -ERANGE;
    }
    case ABCE_OPCODE_OUT:
    {
      struct abce_mb mbstr;
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
      GETMBSTR(&mbstr, -2);
      POP();
      POP();
      if (fwrite(mbstr.u.area->u.str.buf, 1, mbstr.u.area->u.str.size,
                 (streamidx == 0) ? stdout : stderr) 
          != mbstr.u.area->u.str.size)
      {
        abce->err.code = ABCE_E_IO_ERROR;
        abce_mb_refdn(abce, &mbstr);
        return -EIO;
      }
      abce_mb_refdn(abce, &mbstr);
      return 0;
    }
    case ABCE_OPCODE_ERROR:
    {
      struct abce_mb mbstr;
      VERIFYMB(-1, ABCE_T_S);
      GETMBSTR(&mbstr, -1);
      POP();
      if (fwrite(mbstr.u.area->u.str.buf, 1, mbstr.u.area->u.str.size, stderr)
          != mbstr.u.area->u.str.size)
      {
        abce->err.code = ABCE_E_IO_ERROR;
        abce_mb_refdn(abce, &mbstr);
        return -EIO;
      }
      abce->err.code = ABCE_E_ERROR_EXIT;
      abce_mb_refdn(abce, &mbstr);
      return -ECANCELED;
    }
    case ABCE_OPCODE_STR_REVERSE:
    {
      struct abce_mb res, mbbase;
      VERIFYMB(-1, ABCE_T_S);
      GETMBSTR(&mbbase, -1);
      res = abce_mb_create_string(abce,
                                  mbbase.u.area->u.str.buf,
                                  mbbase.u.area->u.str.size);
      if (res.typ == ABCE_T_N)
      {
        abce_mb_refdn(abce, &mbbase);
        return -ENOMEM;
      }
      for (size_t i = 0; i < mbbase.u.area->u.str.size/2; i++)
      {
        uint8_t tmp = mbbase.u.area->u.str.buf[i];
        mbbase.u.area->u.str.buf[i] =
          mbbase.u.area->u.str.buf[mbbase.u.area->u.str.size-i-1];
        mbbase.u.area->u.str.buf[mbbase.u.area->u.str.size-i-1] = tmp;
      }
      POP(); // right before push to avoid GC crashes
      abce_push_mb(abce, &res);
      abce_mb_refdn(abce, &res);
      abce_mb_refdn(abce, &mbbase);
      return 0;
    }
    case ABCE_OPCODE_STRREP:
    {
      struct abce_mb res, mbbase;
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
      GETMBSTR(&mbbase, -2);
      res = abce_mb_rep_string(abce,
                               mbbase.u.area->u.str.buf,
                               mbbase.u.area->u.str.size,
                               (size_t)cnt);
      if (res.typ == ABCE_T_N)
      {
        abce_mb_refdn(abce, &mbbase);
        return -ENOMEM;
      }
      POP(); // Careful with GC!
      POP();
      abce_push_mb(abce, &res);
      abce_mb_refdn(abce, &res);
      abce_mb_refdn(abce, &mbbase);
      return 0;
    }
    case ABCE_OPCODE_STRSTR:
    {
      struct abce_mb mbhaystack, mbneedle;
      const char *pos;

      VERIFYMB(-1, ABCE_T_S);
      VERIFYMB(-2, ABCE_T_S);
      GETMBSTR(&mbneedle, -1);
      GETMBSTR(&mbhaystack, -2);
      POP();
      POP();
      pos = abce_strstr(mbhaystack.u.area->u.str.buf, mbhaystack.u.area->u.str.size,
                        mbneedle.u.area->u.str.buf, mbneedle.u.area->u.str.size);

      if (pos == NULL)
      {
        abce_push_nil(abce);
      }
      else
      {
        abce_push_double(abce, pos - mbhaystack.u.area->u.str.buf);
      }
      abce_mb_refdn(abce, &mbneedle);
      abce_mb_refdn(abce, &mbhaystack);
      return 0;
    }
    case ABCE_OPCODE_STRLISTJOIN:
    {
      struct abce_mb mbar, mbjoiner;
      struct abce_mb mbres;
      struct abce_str_buf buf = {};
      size_t i;

      VERIFYMB(-1, ABCE_T_A);
      VERIFYMB(-2, ABCE_T_S);
      GETMBAR(&mbar, -1);
      GETMBSTR(&mbjoiner, -2);
      for (i = 0; i < mbar.u.area->u.ar.size; i++)
      {
        const struct abce_mb *mb = &mbar.u.area->u.ar.mbs[i];
        if (mb->typ != ABCE_T_S)
        {
          abce_str_buf_free(abce, &buf);
          abce_mb_refdn(abce, &mbar);
          abce_mb_refdn(abce, &mbjoiner);
          abce->err.code = ABCE_E_EXPECT_STR;
          abce->err.mb = abce_mb_refup(abce, mb);
          return -EINVAL;
        }
        if (i != 0)
        {
          if (abce_str_buf_add(abce, &buf, mbjoiner.u.area->u.str.buf, mbjoiner.u.area->u.str.size)
              != 0)
          {
            abce_str_buf_free(abce, &buf);
            abce_mb_refdn(abce, &mbar);
            abce_mb_refdn(abce, &mbjoiner);
            return -ENOMEM;
          }
        }
        if (abce_str_buf_add(abce, &buf, mbar.u.area->u.ar.mbs[i].u.area->u.str.buf, mbar.u.area->u.ar.mbs[i].u.area->u.str.size)
            != 0)
        {
          abce_str_buf_free(abce, &buf);
          abce_mb_refdn(abce, &mbar);
          abce_mb_refdn(abce, &mbjoiner);
          return -ENOMEM;
        }
      }
      mbres = abce_mb_create_string(abce, buf.buf, buf.sz);
      abce_str_buf_free(abce, &buf);
      if (mbres.typ == ABCE_T_N)
      {
        abce_mb_refdn(abce, &mbar);
        abce_mb_refdn(abce, &mbjoiner);
        return -ENOMEM;
      }
      POP(); // Do this late to avoid confusing the GC
      POP();
      abce_push_mb(abce, &mbres);
      abce_mb_refdn(abce, &mbres);
      abce_mb_refdn(abce, &mbar);
      abce_mb_refdn(abce, &mbjoiner);
      return 0;
    }
    case ABCE_OPCODE_TOSTRING:
    {
      char buf[64] = {0};
      double dbl;
      struct abce_mb mbres;
      GETDBL(&dbl, -1);
      if (snprintf(buf, sizeof(buf), "%g", dbl) >= sizeof(buf))
      {
        abort();
      }
      mbres = abce_mb_create_string(abce, buf, strlen(buf));
      if (mbres.typ == ABCE_T_N)
      {
        return -ENOMEM;
      }
      POP(); // right before push to avoid GC crashes
      abce_push_mb(abce, &mbres);
      abce_mb_refdn(abce, &mbres);
      return 0;
    }
    case ABCE_OPCODE_TONUMBER:
    {
      struct abce_mb str;
      char *endptr;
      double dbl;
      GETMBSTR(&str, -1);
      POP();
      if (   str.u.area->u.str.size == 0
          || str.u.area->u.str.size != strlen(str.u.area->u.str.buf))
      {
        abce->err.code = ABCE_E_NOT_A_NUMBER_STRING;
        abce->err.mb = abce_mb_refup(abce, &str);
        abce_mb_refdn(abce, &str);
        return -EINVAL;
      }
      dbl = strtod(str.u.area->u.str.buf, &endptr);
      if (*endptr != '\0')
      {
        abce->err.code = ABCE_E_NOT_A_NUMBER_STRING;
        abce->err.mb = abce_mb_refup(abce, &str);
        abce_mb_refdn(abce, &str);
        return -EINVAL;
      }
      abce_push_double(abce, dbl);
      abce_mb_refdn(abce, &str);
      return 0;
    }
    case ABCE_OPCODE_STRSTRIP:
    {
      struct abce_mb res, mbbase, mbsep;
      size_t start, end;
      VERIFYMB(-1, ABCE_T_S);
      VERIFYMB(-2, ABCE_T_S);
      GETMBSTR(&mbsep, -1);
      GETMBSTR(&mbbase, -2);
      abce_strip(mbbase.u.area->u.str.buf, mbbase.u.area->u.str.size,
                 mbsep.u.area->u.str.buf, mbsep.u.area->u.str.size,
                 &start, &end);
      if (end < start)
      {
        abort();
      }
      res = abce_mb_create_string(abce,
                                  mbbase.u.area->u.str.buf + start,
                                  end - start);
      if (res.typ == ABCE_T_N)
      {
        abce_mb_refdn(abce, &mbbase);
        abce_mb_refdn(abce, &mbsep);
        return -ENOMEM;
      }
      POP(); // right before push to avoid GC crashes
      POP();
      abce_push_mb(abce, &res);
      abce_mb_refdn(abce, &res);
      abce_mb_refdn(abce, &mbbase);
      abce_mb_refdn(abce, &mbsep);
      return 0;
    }
    case ABCE_OPCODE_SCOPE_PARENT:
    {
      struct abce_mb mbscparent;
      struct abce_mb mbsc;
      GETMBSC(&mbsc, -1);
      POP();
      // FIXME what if it's null?
      mbscparent = abce_mb_refuparea(abce, mbsc.u.area->u.sc.parent, ABCE_T_SC);
      abce_push_mb(abce, &mbscparent);
      abce_mb_refdn(abce, &mbsc);
      abce_mb_refdn(abce, &mbscparent);
      return 0;
    }
    case ABCE_OPCODE_SCOPE_NEW:
    {
      int holey;
      double locidx;
      struct abce_mb mbscnew;
      struct abce_mb mbscparent;
      GETBOOLEAN(&holey, -1);
      GETMBSC(&mbscparent, -2);
      mbscnew =
        abce_mb_create_scope(abce, ABCE_DEFAULT_SCOPE_SIZE, &mbscparent, holey);
      if (mbscnew.typ == ABCE_T_N)
      {
        abce_mb_refdn(abce, &mbscparent);
        return -ENOMEM;
      }
      locidx = mbscnew.u.area->u.sc.locidx;
      POP(); // do this late to avoid confusing GC
      POP();
      abce_push_double(abce, locidx);
      abce_mb_refdn(abce, &mbscnew);
      abce_mb_refdn(abce, &mbscparent);
      return 0;
    }
    case ABCE_OPCODE_APPENDALL_MAINTAIN: // RFE should this be moved elsewhere? A complex operation.
    {
      struct abce_mb mbar2;
      struct abce_mb mbar;
      size_t i;
      VERIFYMB(-2, ABCE_T_A);
      VERIFYMB(-1, ABCE_T_A);
      GETMBAR(&mbar, -2);
      GETMBAR(&mbar2, -1);
      // NB: Here we need to be very careful. We can't pop mbar2 until we
      //     are in a point where memory alloc cannot happen. Otherwise GC
      //     complains about reference count not being 0.
      for (i = 0; i < mbar2.u.area->u.ar.size; i++)
      {
        if (abce_mb_array_append(abce, &mbar, &mbar2.u.area->u.ar.mbs[i]) != 0)
        {
          ret = -ENOMEM;
          break;
        }
      }
      POP();
      abce_mb_refdn_typ(abce, &mbar, ABCE_T_A);
      abce_mb_refdn_typ(abce, &mbar2, ABCE_T_A);
      break;
    }
    case ABCE_OPCODE_PUSH_NEW_PB:
    {
      struct abce_mb mb;
      int rettmp;
      mb = abce_mb_create_pb(abce);
      if (mb.typ == ABCE_T_N)
      {
        ret = -ENOMEM;
        break;
      }
      rettmp = abce_push_mb(abce, &mb);
      if (rettmp != 0)
      {
        ret = rettmp;
        abce_mb_refdn(abce, &mb);
        break;
      }
      abce_mb_refdn_typ(abce, &mb, ABCE_T_PB);
      break;
    }
    case ABCE_OPCODE_PUSH_NEW_DICT:
    {
      struct abce_mb mb;
      int rettmp;
      mb = abce_mb_create_tree(abce);
      if (mb.typ == ABCE_T_N)
      {
        ret = -ENOMEM;
        break;
      }
      rettmp = abce_push_mb(abce, &mb);
      if (rettmp != 0)
      {
        ret = rettmp;
        abce_mb_refdn(abce, &mb);
        break;
      }
      abce_mb_refdn_typ(abce, &mb, ABCE_T_T);
      break;
    }
    case ABCE_OPCODE_PUSH_NEW_ARRAY:
    {
      struct abce_mb mb;
      int rettmp;
      mb = abce_mb_create_array(abce);
      if (mb.typ == ABCE_T_N)
      {
        ret = -ENOMEM;
        break;
      }
      rettmp = abce_push_mb(abce, &mb);
      if (rettmp != 0)
      {
        ret = rettmp;
      }
      abce_mb_refdn_typ(abce, &mb, ABCE_T_A);
      break;
    }
    case ABCE_OPCODE_DUP_NONRECURSIVE:
    {
      struct abce_mb mbold;
      struct abce_mb mbnew;
      size_t i;
      GETMB(&mbold, -1);
      if (abce_unlikely(mbold.typ != ABCE_T_A && mbold.typ != ABCE_T_T)) // FIXME T_PB
      {
        abce->err.code = ABCE_E_EXPECT_ARRAY_OR_TREE; // FIXME split
        abce->err.mb = abce_mb_refup_noinline(abce, &mbold);
        abce_mb_refdn(abce, &mbold);
        return -EINVAL;
      }
      if (mbold.typ == ABCE_T_A)
      {
        mbnew = abce_mb_create_array(abce);
        if (mbnew.typ == ABCE_T_N)
        {
          abce_mb_refdn(abce, &mbold);
          return -ENOMEM;
        }
        abce->oneblock = abce_mb_refup(abce, &mbnew); // make it visible to GC
        for (i = 0; i < mbold.u.area->u.ar.size; i++)
        {
          if (abce_mb_array_append(abce, &mbnew, &mbold.u.area->u.ar.mbs[i]) != 0)
          {
            abce_mb_refdn(abce, &abce->oneblock);
            abce_mb_refdn(abce, &mbold);
            abce_mb_refdn(abce, &mbnew);
            return -ENOMEM;
          }
        }
        POP(); // right before push to avoid GC crashes
        if (abce_push_mb(abce, &mbnew) != 0)
        {
          abort();
        }
        abce_mb_refdn(abce, &abce->oneblock);
        abce_mb_refdn(abce, &mbold);
        abce_mb_refdn(abce, &mbnew);
        return 0;
      }
      else if (mbold.typ == ABCE_T_T)
      {
        const struct abce_mb *key, *val;
        const struct abce_mb nil = {.typ = ABCE_T_N};
        mbnew = abce_mb_create_tree(abce);
        if (mbnew.typ == ABCE_T_N)
        {
          abce_mb_refdn(abce, &mbold);
          return -ENOMEM;
        }
        abce->oneblock = abce_mb_refup(abce, &mbnew); // make it visible to GC
        key = &nil;
        while (abce_tree_get_next(abce, &key, &val, &mbold, key) == 0)
        {
          if (abce_tree_set_str(abce, &mbold, key, val) != 0)
          {
            abce_mb_refdn(abce, &abce->oneblock);
            abce_mb_refdn(abce, &mbnew);
            abce_mb_refdn(abce, &mbold);
            return -ENOMEM;
          }
        }
        POP(); // right before push to avoid GC crashes
        if (abce_push_mb(abce, &mbnew) != 0)
        {
          abort();
        }
        abce_mb_refdn(abce, &abce->oneblock);
        abce_mb_refdn(abce, &mbold);
        abce_mb_refdn(abce, &mbnew);
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
      struct abce_mb mboldkey, mbt;
      const struct abce_mb *mbreskey, *mbresval;
      double dictidx;
      int rettmp;
      VERIFYMB(-1, ABCE_T_D);
      GETDBL(&dictidx, -1);
      GETMB(&mboldkey, -2);
      if (abce_unlikely(mboldkey.typ != ABCE_T_N && mboldkey.typ != ABCE_T_S))
      {
        abce->err.code = ABCE_E_TREE_ITER_NOT_STR_OR_NUL;
        abce->err.mb = abce_mb_refup_noinline(abce, &mboldkey);
        abce_mb_refdn(abce, &mboldkey);
        ret = -EINVAL;
        break;
      }
      POP();
      POP();
      rettmp = abce_verifymb(abce, (int64_t)dictidx, ABCE_T_T);
      if (rettmp != 0)
      {
        abce_mb_refdn(abce, &mboldkey);
        ret = rettmp;
        break;
      }
      if (abce_getmb(&mbt, abce, (int64_t)dictidx) != 0)
      {
        abce_maybeabort();
      }
      if (abce_tree_get_next(abce, &mbreskey, &mbresval, &mbt, &mboldkey) != 0)
      {
        if (abce_push_nil(abce) != 0)
        {
          abce_maybeabort();
        }
        if (abce_push_nil(abce) != 0)
        {
          abce_maybeabort();
        }
        abce_mb_refdn(abce, &mboldkey);
        abce_mb_refdn_typ(abce, &mbt, ABCE_T_T);
        break;
      }
      abce_push_mb(abce, mbreskey);
      abce_push_mb(abce, mbresval);
      abce_mb_refdn(abce, &mboldkey);
      abce_mb_refdn_typ(abce, &mbt, ABCE_T_T);
      break;
    }
    case ABCE_OPCODE_FP_CLASSIFY:
    case ABCE_OPCODE_FILE_OPEN:
    case ABCE_OPCODE_FILE_CLOSE:
    case ABCE_OPCODE_FILE_GET:
    case ABCE_OPCODE_FILE_SEEK_TELL:
    case ABCE_OPCODE_FILE_FLUSH:
    case ABCE_OPCODE_FILE_WRITE:
    case ABCE_OPCODE_MEMFILE_IOPEN:
    case ABCE_OPCODE_JSON_ENCODE:
    case ABCE_OPCODE_JSON_DECODE:
    case ABCE_OPCODE_LISTSPLICE:
    case ABCE_OPCODE_STRFMT:
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
  int was_in_engine;
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
          struct abce_mb mbar;
          GETMBAR(&mbar, -1);
          POP();
          if (mbar.u.area->u.ar.size == 0)
          {
            abce->err.code = ABCE_E_ARRAY_UNDERFLOW;
            ret = -ENOENT;
            abce_mb_refdn_typ(abce, &mbar, ABCE_T_A);
            break;
          }
          abce_mb_refdn(abce, &mbar.u.area->u.ar.mbs[--mbar.u.area->u.ar.size]);
          abce_mb_refdn_typ(abce, &mbar, ABCE_T_A);
          break;
        }
        case ABCE_OPCODE_LISTLEN:
        {
          struct abce_mb mbar;
          GETMBAR(&mbar, -1);
          POP();
          if (abce_push_double(abce, mbar.u.area->u.ar.size) != 0)
          {
            abce_maybeabort();
          }
          abce_mb_refdn_typ(abce, &mbar, ABCE_T_A);
          break;
        }
        case ABCE_OPCODE_PBSETLEN:
        {
          struct abce_mb mbpb;
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
          GETMBPB(&mbpb, -2);
          if (abce_mb_pb_resize(abce, &mbpb, (size_t)sz) != 0)
          {
            ret = -ENOMEM;
            abce_mb_refdn_typ(abce, &mbpb, ABCE_T_PB);
            break;
          }
          POP();
          POP();
          abce_mb_refdn_typ(abce, &mbpb, ABCE_T_PB);
          break;
        }
        case ABCE_OPCODE_PBSET:
        {
          struct abce_mb mbpb;
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
          GETDBL(&off, -2);
          GETDBL(&sz, -3);
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
          GETMBPB(&mbpb, -4);
          if (off + sz > mbpb.u.area->u.pb.size)
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
              abce_hdr_set8h(&mbpb.u.area->u.pb.buf[ioff], val);
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
              abce_hdr_set16n(&mbpb.u.area->u.pb.buf[ioff], val);
              break;
            case 2:
              abce_hdr_set32n(&mbpb.u.area->u.pb.buf[ioff], val);
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
              abce_hdr_set16n(&mbpb.u.area->u.pb.buf[ioff], abce_bswap16(val));
              break;
            case -2:
              abce_hdr_set32n(&mbpb.u.area->u.pb.buf[ioff], abce_bswap32(val));
              break;
            default:
              abort();
          }
          POP();
          POP();
          POP();
          POP();
outpbset:
          abce_mb_refdn_typ(abce, &mbpb, ABCE_T_PB);
          break;
        }
        case ABCE_OPCODE_PBGET:
        {
          struct abce_mb mbpb;
          double off;
          size_t ioff;
          double sz;
          int isz;
          uint32_t val;
          GETDBL(&off, -1);
          GETDBL(&sz, -2);
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
          GETMBPB(&mbpb, -3);
          if (off + sz > mbpb.u.area->u.pb.size)
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
              val = abce_hdr_get8h(&mbpb.u.area->u.pb.buf[ioff]);
              break;
            case 1:
              val = abce_hdr_get16n(&mbpb.u.area->u.pb.buf[ioff]);
              break;
            case 2:
              val = abce_hdr_get32n(&mbpb.u.area->u.pb.buf[ioff]);
              break;
            case -1:
              val = abce_bswap16(abce_hdr_get16n(&mbpb.u.area->u.pb.buf[ioff]));
              break;
            case -2:
              val = abce_bswap32(abce_hdr_get32n(&mbpb.u.area->u.pb.buf[ioff]));
              break;
            default:
              abort();
          }
          POP();
          POP();
          POP();
          if (abce_push_double(abce, val) != 0)
          {
            abce_maybeabort();
          }
          abce_mb_refdn_typ(abce, &mbpb, ABCE_T_PB);
          break;
        }
        case ABCE_OPCODE_PBLEN:
        {
          struct abce_mb mbpb;
          GETMBPB(&mbpb, -1);
          POP();
          if (abce_push_double(abce, mbpb.u.area->u.pb.size) != 0)
          {
            abce_maybeabort();
          }
          abce_mb_refdn_typ(abce, &mbpb, ABCE_T_PB);
          break;
        }
        case ABCE_OPCODE_STRLEN:
        {
          struct abce_mb mbstr;
          GETMBSTR(&mbstr, -1);
          POP();
          if (abce_push_double(abce, mbstr.u.area->u.str.size) != 0)
          {
            abce_maybeabort();
          }
          abce_mb_refdn_typ(abce, &mbstr, ABCE_T_S);
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
            abce_mb_refdn_typ(abce, &mbar, ABCE_T_A);
            abce->err.code = ABCE_E_INDEX_NOT_INT;
            abce->err.mb.typ = ABCE_T_D;
            abce->err.mb.u.d = loc;
            ret = -EINVAL;
            break;
          }
          locint = loc;
          if (locint < 0 || locint >= mbar.u.area->u.ar.size)
          {
            abce_mb_refdn(abce, &mbit);
            abce_mb_refdn_typ(abce, &mbar, ABCE_T_A);
            abce->err.code = ABCE_E_INDEX_OOB;
            abce->err.mb.typ = ABCE_T_D;
            abce->err.mb.u.d = loc;
            ret = -ERANGE;
            break;
          }
          abce_mb_refdn(abce, &mbar.u.area->u.ar.mbs[locint]);
          mbar.u.area->u.ar.mbs[locint] = mbit;
          abce_mb_refdn_typ(abce, &mbar, ABCE_T_A);
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
            abce->err.code = ABCE_E_INDEX_NOT_INT;
            abce->err.mb.typ = ABCE_T_D;
            abce->err.mb.u.d = loc;
            abce_mb_refdn_typ(abce, &mbstr, ABCE_T_S);
            ret = -EINVAL;
            break;
          }
          locint = loc;
          if (locint < 0 || locint >= mbstr.u.area->u.str.size)
          {
            abce->err.code = ABCE_E_INDEX_OOB;
            abce->err.mb.typ = ABCE_T_D;
            abce->err.mb.u.d = loc;
            abce_mb_refdn_typ(abce, &mbstr, ABCE_T_S);
            ret = -ERANGE;
            break;
          }
          if (abce_push_double(abce, (unsigned char)mbstr.u.area->u.str.buf[locint]) != 0)
          {
            abce_maybeabort();
          }
          abce_mb_refdn_typ(abce, &mbstr, ABCE_T_S);
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
            abce->err.code = ABCE_E_INDEX_NOT_INT;
            abce->err.mb.typ = ABCE_T_D;
            abce->err.mb.u.d = loc;
            ret = -EINVAL;
            abce_mb_refdn_typ(abce, &mbar, ABCE_T_A);
            break;
          }
          locint = loc;
          if (locint < 0 || locint >= mbar.u.area->u.ar.size)
          {
            abce->err.code = ABCE_E_INDEX_OOB;
            abce->err.mb.typ = ABCE_T_D;
            abce->err.mb.u.d = loc;
            ret = -ERANGE;
            abce_mb_refdn_typ(abce, &mbar, ABCE_T_A);
            break;
          }
          if (abce_push_mb(abce, &mbar.u.area->u.ar.mbs[locint]) != 0)
          {
            abce_maybeabort();
          }
          abce_mb_refdn_typ(abce, &mbar, ABCE_T_A);
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
          if (loc != (double)(uint64_t)loc)
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
          struct abce_mb mb;
          double loc;
          GETDBL(&loc, -1);
          POP();
          if (loc != (double)(uint64_t)loc)
          {
            abce->err.code = ABCE_E_STACK_IDX_NOT_UINT;
            abce->err.mb.typ = ABCE_T_D;
            abce->err.mb.u.d = loc;
            ret = -EINVAL;
            break;
          }
          GETMB(&mb, loc);
          if (abce_push_mb(abce, &mb) != 0)
          {
            abce_maybeabort();
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
            abce->err.code = ABCE_E_STACK_IDX_NOT_UINT;
            abce->err.mb.typ = ABCE_T_D;
            abce->err.mb.u.d = loc;
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
          //printf("ret, stack size %d\n", (int)abce->sp);
          GETIP(-2);
          //printf("gotten ip\n");
          GETBP(-3);
          //printf("gotten bp\n");
          GETMB(&mb, -1);
          //printf("gotten mb\n");
          POP(); // retval
          POP(); // ip
          POP(); // bp
          POP(); // funcall address
          if (abce_push_mb(abce, &mb) != 0)
          {
            abce_maybeabort();
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
          GETMB(&mb, -3);
          POP(); // cntloc
          POP(); // cntargs
          POP(); // retval
          for (i = 0; i < cntloc; i++)
          {
            POP();
          }
          GETIP(-1);
          GETBP(-2);
          POP(); // ip
          POP(); // bp
          for (i = 0; i < cntargs; i++)
          {
            POP();
          }
          POP(); // funcall address
          if (abce_unlikely(abce_push_mb(abce, &mb) != 0))
          {
            abce_maybeabort();
          }
          abce_mb_refdn(abce, &mb);
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
          struct abce_mb mb;
          GETMB(&mb, -1);
          if (abce_push_mb(abce, &mb) != 0)
          {
            abce->err.code = ABCE_E_STACK_OVERFLOW;
            abce->err.mb = mb; // transfer of ref
            ret = -EOVERFLOW;
            break;
          }
          abce_mb_refdn(abce, &mb);
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
          struct abce_mb mb;
          struct abce_mb mbar;
          GETMBAR(&mbar, -2);
          GETMB(&mb, -1); // can't fail if GETMBAR succeeded
          if (abce_mb_array_append(abce, &mbar, &mb) != 0)
          {
            ret = -ENOMEM;
          }
          POP(); // do this late to avoid confusing GC
          abce_mb_refdn_typ(abce, &mbar, ABCE_T_T);
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
          struct abce_mb mbsc, mbit;
          const struct abce_mb *ptr;
          GETMBSC(&mbsc, -2);
          GETMB(&mbit, -1);
          if (abce_unlikely(mbit.typ != ABCE_T_S))
          {
            abce->err.code = ABCE_E_EXPECT_STR;
            abce->err.mb = abce_mb_refup_noinline(abce, &mbit);
            abce->err.val2 = -2;
            abce_mb_refdn(abce, &mbit);
            abce_mb_refdn_typ(abce, &mbsc, ABCE_T_SC);
            ret = -EINVAL;
            break;
          }
          POP();
          POP();
          ptr = abce_sc_get_rec_mb(&mbsc, &mbit, 1);
          if (abce_unlikely(ptr == NULL))
          {
            abce->err.code = ABCE_E_SCOPEVAR_NOT_FOUND;
            abce->err.mb = abce_mb_refup_noinline(abce, &mbit);
            abce_mb_refdn_typ(abce, &mbit, ABCE_T_S);
            abce_mb_refdn_typ(abce, &mbsc, ABCE_T_SC);
            ret = -ENOENT;
            break;
          }
          if (abce_push_mb(abce, ptr) != 0)
          {
            abce_maybeabort();
          }
          abce_mb_refdn_typ(abce, &mbit, ABCE_T_S);
          abce_mb_refdn_typ(abce, &mbsc, ABCE_T_SC);
          break;
        }
        case ABCE_OPCODE_SCOPE_HAS:
        {
          struct abce_mb mbsc, mbit;
          const struct abce_mb *ptr;
          GETMBSC(&mbsc, -2);
          GETMB(&mbit, -1);
          if (abce_unlikely(mbit.typ != ABCE_T_S))
          {
            abce->err.code = ABCE_E_EXPECT_STR;
            abce->err.mb = abce_mb_refup_noinline(abce, &mbit);
            abce->err.val2 = -2;
            ret = -EINVAL;
            abce_mb_refdn(abce, &mbit);
            abce_mb_refdn_typ(abce, &mbsc, ABCE_T_SC);
            break;
          }
          POP();
          POP();
          ptr = abce_sc_get_rec_mb(&mbsc, &mbit, 1);
          if (abce_push_boolean(abce, ptr != NULL) != 0)
          {
            abce_maybeabort();
          }
          abce_mb_refdn_typ(abce, &mbit, ABCE_T_S);
          abce_mb_refdn_typ(abce, &mbsc, ABCE_T_SC);
          break;
        }
        case ABCE_OPCODE_GETSCOPE_DYN:
        {
          if (abce_unlikely(abce_push_mb(abce, &abce->dynscope) != 0))
          {
            abce->err.code = ABCE_E_STACK_OVERFLOW;
            abce->err.mb = abce_mb_refup_noinline(abce, &abce->dynscope);
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
            abce_maybeabort();
          }
          abce_mb_refdn(abce, &mb);
          break;
        }
        case ABCE_OPCODE_DICTSET_MAINTAIN:
        {
          struct abce_mb mbt, mbstr;
          struct abce_mb mbval;
          VERIFYMB(-3, ABCE_T_T);
          VERIFYMB(-2, ABCE_T_S);
          GETMB(&mbt, -3);
          GETMB(&mbstr, -2);
          GETMB(&mbval, -1);
          if (abce_tree_set_str(abce, &mbt, &mbstr, &mbval) != 0)
          {
            ret = -ENOMEM;
            // No break: we want to call all refdn statements
          }
          POP(); // do this late to avoid confusing GC
          POP();
          abce_mb_refdn_typ(abce, &mbt, ABCE_T_T);
          abce_mb_refdn_typ(abce, &mbstr, ABCE_T_S);
          abce_mb_refdn(abce, &mbval);
          break;
        }
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
              abce_maybeabort();
            }
          }
          else
          {
            if (abce_push_nil(abce) != 0) // FIXME really nil? Should be error?
            {
              abce_maybeabort();
            }
          }
          abce_mb_refdn_typ(abce, &mbt, ABCE_T_T);
          abce_mb_refdn_typ(abce, &mbstr, ABCE_T_S);
          break;
        }
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
          if (abce_push_boolean(abce, abce_tree_get_str(abce, &mbval, &mbt, &mbstr) == 0) != 0)
          {
            abce_maybeabort();
          }
          abce_mb_refdn_typ(abce, &mbt, ABCE_T_T);
          abce_mb_refdn_typ(abce, &mbstr, ABCE_T_S);
          break;
        }
        case ABCE_OPCODE_CALL_IF_FUN:
        {
          struct abce_mb mb;
          GETMB(&mb, -1);
          if (mb.typ == ABCE_T_F)
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
            // no refdn; functions are static data types
            break;
#endif
          }
          else
          {
            abce_mb_refdn(abce, &mb);
          }
          break;
        }
        case ABCE_OPCODE_DICTLEN:
        {
          struct abce_mb mbt;
          VERIFYMB(-1, ABCE_T_T);
          GETMB(&mbt, -1);
          POP();
          if (abce_push_double(abce, mbt.u.area->u.tree.sz) != 0)
          {
            abce_maybeabort();
          }
          abce_mb_refdn_typ(abce, &mbt, ABCE_T_T);
          break;
        }
        case ABCE_OPCODE_SCOPEVAR_SET:
        {
          struct abce_mb mbsc, mbs, mbv;
          int rettmp;
          VERIFYMB(-3, ABCE_T_SC);
          VERIFYMB(-2, ABCE_T_S);
          GETMB(&mbsc, -3);
          GETMB(&mbs, -2);
          GETMB(&mbv, -1);
          rettmp = abce_sc_replace_val_mb(abce, &mbsc, &mbs, &mbv);
          if (rettmp != 0)
          {
            ret = rettmp;
            // No break: we want to call all refdn statements
          }
          POP(); // Do this late to avoid confusing GC
          POP();
          POP();
          abce_mb_refdn_typ(abce, &mbs, ABCE_T_S);
          abce_mb_refdn(abce, &mbv);
          abce_mb_refdn_typ(abce, &mbsc, ABCE_T_SC);
          break;
        }
        case ABCE_OPCODE_DICTDEL:
        {
          struct abce_mb mbt, mbstr;
          VERIFYMB(-2, ABCE_T_T);
          VERIFYMB(-1, ABCE_T_S);
          GETMB(&mbt, -2);
          GETMB(&mbstr, -1);
          if (abce_unlikely(abce_tree_del_str(abce, &mbt, &mbstr) != 0))
          {
            abce->err.code = ABCE_E_TREE_ENTRY_NOT_FOUND;
            abce->err.mb = abce_mb_refup_noinline(abce, &mbstr);
            ret = -ENOENT;
            // No break: we want to call all refdn statements
          }
          POP(); // Just in case deleting allocates memory, do this late
          POP();
          abce_mb_refdn_typ(abce, &mbt, ABCE_T_T);
          abce_mb_refdn_typ(abce, &mbstr, ABCE_T_S);
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
