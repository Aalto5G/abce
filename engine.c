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
#include "string.h"
#include "trees.h"
#include "scopes.h"

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
      if (res.typ == ABCE_T_N)
      {
        return -ENOMEM;
      }
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
          abce->bp = abce->sp - 2 - (uint64_t)argcnt;
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
          if (abce_mb_array_append(abce, &mbar, &mb) != 0)
          {
            ret = -ENOMEM;
          }
          abce_mb_refdn(abce, &mbar);
          abce_mb_refdn(abce, &mb);
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
            abort();
          }
          abce_mb_refdn(abce, &mbt);
          abce_mb_refdn(abce, &mbstr);
          break;
        }
        case ABCE_OPCODE_CALL_IF_FUN:
        {
          struct abce_mb mb;
          GETMB(&mb, -1);
          if (mb.typ == ABCE_T_F)
          {
            const size_t guard = 100;
            double argcnt = 0.0;
            int64_t new_ip;
            uint16_t ins2;
            int rettmp;
            GETFUNADDR(&new_ip, -1);
            POP();
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
            abort();
          }
          abce_mb_refdn(abce, &mbt);
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
          POP();
          POP();
          POP();
          rettmp = abce_sc_replace_val_mb(abce, &mbsc, &mbs, &mbv);
          if (rettmp != 0)
          {
            ret = rettmp;
            // No break: we want to call all refdn statements
          }
          abce_mb_refdn(abce, &mbs);
          abce_mb_refdn(abce, &mbv);
          abce_mb_refdn(abce, &mbsc);
          break;
        }
        case ABCE_OPCODE_DICTDEL:
        {
          struct abce_mb mbt, mbstr;
          VERIFYMB(-2, ABCE_T_T);
          VERIFYMB(-1, ABCE_T_S);
          GETMB(&mbt, -2);
          GETMB(&mbstr, -1);
          POP();
          POP();
          if (abce_tree_del_str(abce, &mbt, &mbstr) != 0)
          {
            ret = -ENOENT;
            // No break: we want to call all refdn statements
          }
          abce_mb_refdn(abce, &mbt);
          abce_mb_refdn(abce, &mbstr);
          break;
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
          POP();
          for (i = 0; i < mbar.u.area->u.ar.size; i++)
          {
            if (abce_mb_array_append(abce, &mbar, &mbar.u.area->u.ar.mbs[i]) != 0)
            {
              ret = -ENOMEM;
              break;
            }
          }
          abce_mb_refdn(abce, &mbar);
          abce_mb_refdn(abce, &mbar2);
          break;
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
