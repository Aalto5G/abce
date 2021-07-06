#ifndef _ABCE_STRING_H_
#define _ABCE_STRING_H_

#include "abce.h"

#ifdef __cplusplus
extern "C" {
#endif

int abce_strgsub(struct abce *abce,
                 char **res, size_t *ressz, size_t *rescap,
                 const char *haystack, size_t haystacksz,
                 const char *needle, size_t needlesz,
                 const char *sub, size_t subsz);

static inline
int abce_cpush_strgsub_mb(struct abce *abce,
                          const struct abce_mb *haystack,
                          const struct abce_mb *needle,
                          const struct abce_mb *sub)
{
  char *resstr;
  size_t ressz;
  size_t rescap;
  int retval;
  if (haystack->typ != ABCE_T_S || needle->typ != ABCE_T_S || sub->typ != ABCE_T_S)
  {
    return -EINVAL;
  }
  retval = abce_strgsub(abce, &resstr, &ressz, &rescap,
                        haystack->u.area->u.str.buf, haystack->u.area->u.str.size,
                        needle->u.area->u.str.buf, needle->u.area->u.str.size,
                        sub->u.area->u.str.buf, sub->u.area->u.str.size);
  if (retval != 0)
  {
    return retval;
  }
  if (abce_mb_cpush_create_string(abce, resstr, ressz) == NULL)
  {
    return -ENOMEM;
  }
  abce->alloc(resstr, rescap, 0, &abce->alloc_baton);
  return 0;
}

#ifdef __cplusplus
};
#endif

#endif
