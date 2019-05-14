#ifndef _ABCE_STRING_H_
#define _ABCE_STRING_H_

#include "abce.h"

int abce_strgsub(struct abce *abce,
                 char **res, size_t *ressz,
                 const char *haystack, size_t haystacksz,
                 const char *needle, size_t needlesz,
                 const char *sub, size_t subsz);

static inline
int abce_strgsub_mb(struct abce *abce,
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
  if (res->typ == ABCE_T_N)
  {
    return -ENOMEM;
  }
  abce->alloc(resstr, 0, abce->alloc_baton);
  return 0;
}

#endif
