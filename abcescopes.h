#ifndef _SCOPES_H_
#define _SCOPES_H_

#include "abcedatatypes.h"

#ifdef __cplusplus
extern "C" {
#endif

const struct abce_mb *abce_sc_get_rec_mb_area(
  const struct abce_mb_area *mba, const struct abce_mb *it, int rec);

const struct abce_mb *abce_sc_get_rec_str_area(
  const struct abce_mb_area *mba, const char *str, int rec);

int abce_sc_replace_val_mb(
  struct abce *abce,
  const struct abce_mb *mb, const struct abce_mb *pkey, const struct abce_mb *pval);

int abce_sc_put_val_str(
  struct abce *abce,
  const struct abce_mb *mb, const char *str, const struct abce_mb *pval);

int abce_sc_put_val_str_maybe_old(
  struct abce *abce,
  const struct abce_mb *mb, const char *str, const struct abce_mb *pval,
  int maybe, struct abce_mb *mbold);

#ifdef __cplusplus
};
#endif

#endif
