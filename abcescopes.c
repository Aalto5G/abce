#include "abcescopes.h"
#include "abce.h"

const struct abce_mb *abce_sc_get_rec_mb_area(
  const struct abce_mb_area *mba, const struct abce_mb *it, int rec)
{
  const struct abce_mb *mb = abce_sc_get_myval_mb_area(mba, it);
  if (mb != NULL)
  {
    return mb;
  }
  if (mba->u.sc.parent != NULL && !mba->u.sc.holey && rec)
  {
    return abce_sc_get_rec_mb_area(mba->u.sc.parent, it, rec);
  }
  return NULL;
}

const struct abce_mb *abce_sc_get_rec_str_area(
  const struct abce_mb_area *mba, const char *str, int rec)
{
  const struct abce_mb *mb = abce_sc_get_myval_str_area(mba, str);
  if (mb != NULL)
  {
    return mb;
  }
  if (mba->u.sc.parent != NULL && !mba->u.sc.holey && rec)
  {
    return abce_sc_get_rec_str_area(mba->u.sc.parent, str, rec);
  }
  return NULL;
}

int abce_sc_replace_val_mb(
  struct abce *abce,
  const struct abce_mb *mb, const struct abce_mb *pkey, const struct abce_mb *pval)
{
  struct abce_mb_area *mba = mb->u.area;
  uint32_t hashval;
  struct abce_mb_rb_entry *e;
  size_t hashloc;
  int ret;
  struct abce_rb_tree_node *n;

  if (mb->typ != ABCE_T_SC || pkey->typ != ABCE_T_S)
  {
    abort();
  }
  hashval = abce_mb_str_hash(pkey);
  hashloc = hashval & (mba->u.sc.size - 1);

  n = ABCE_RB_TREE_NOCMP_FIND(&mba->u.sc.heads[hashloc], abce_str_cmp_halfsym, NULL, pkey);
  if (n == NULL)
  {
    e = abce->alloc(NULL, 0, sizeof(*e), &abce->alloc_baton);
    if (e == NULL)
    {
      abce->err.code = ABCE_E_NO_MEM;
      abce->err.val2 = sizeof(*e);
      return -ENOMEM;
    }
    e->key = abce_mb_refup(abce, pkey);
    e->val = abce_mb_refup(abce, pval);
    ret = abce_rb_tree_nocmp_insert_nonexist(&mba->u.sc.heads[hashloc],
                                        abce_str_cmp_sym, NULL, &e->n);
    if (ret != 0)
    {
      abort();
    }
    return 0;
  }
  e = ABCE_CONTAINER_OF(n, struct abce_mb_rb_entry, n);
  abce_mb_refdn(abce, &e->val);
  e->val = abce_mb_refup(abce, pval);
  return 0;
}

int abce_sc_put_val_str(
  struct abce *abce,
  const struct abce_mb *mb, const char *str, const struct abce_mb *pval)
{
  struct abce_mb_area *mba = mb->u.area;
  struct abce_mb *mbtmp;
  uint32_t hashval;
  struct abce_mb_rb_entry *e;
  size_t hashloc;
  int ret;
  if (mb->typ != ABCE_T_SC)
  {
    abort();
  }
  hashval = abce_str_hash(str);
  hashloc = hashval & (mba->u.sc.size - 1);
  e = abce->alloc(NULL, 0, sizeof(*e), &abce->alloc_baton);
  if (e == NULL)
  {
    abce->err.code = ABCE_E_NO_MEM;
    abce->err.val2 = sizeof(*e);
    return -ENOMEM;
  }
  mbtmp = abce_mb_cpush_create_string(abce, str, strlen(str));
  if (mbtmp == NULL)
  {
    abce->err.code = ABCE_E_NO_MEM;
    abce->err.val2 = strlen(str);
    return -ENOMEM;
  }
  e->key = abce_mb_refup(abce, mbtmp);
  e->val = abce_mb_refup(abce, pval);
  ret = abce_rb_tree_nocmp_insert_nonexist(&mba->u.sc.heads[hashloc],
                                      abce_str_cmp_sym, NULL, &e->n);
  if (ret == 0)
  {
    abce_cpop(abce);
    return 0;
  }
  abce_mb_refdn(abce, &e->key);
  abce_mb_refdn(abce, &e->val);
  abce_cpop(abce);
  abce->alloc(e, sizeof(*e), 0, &abce->alloc_baton);
  return ret;
}

int abce_sc_put_val_str_maybe_old(
  struct abce *abce,
  const struct abce_mb *mb, const char *str, const struct abce_mb *pval,
  int maybe, struct abce_mb *mbold)
{
  struct abce_mb_area *mba = mb->u.area;
  uint32_t hashval;
  struct abce_mb_rb_entry *e;
  struct abce_mb *mbkey;
  size_t hashloc;
  int ret;
  struct abce_rb_tree_node *n;

  if (mb->typ != ABCE_T_SC)
  {
    abort();
  }
  hashval = abce_str_hash(str);
  hashloc = hashval & (mba->u.sc.size - 1);

  mbkey = abce_mb_cpush_create_string(abce, str, strlen(str));
  if (mbkey == NULL)
  {
    abce->err.code = ABCE_E_NO_MEM;
    abce->err.val2 = strlen(str);
    if (mbold)
    {
      mbold->typ = ABCE_T_N;
    }
    return -ENOMEM;
  }

  n = ABCE_RB_TREE_NOCMP_FIND(&mba->u.sc.heads[hashloc], abce_str_cmp_halfsym, NULL, mbkey);
  if (n != NULL)
  {
    struct abce_mb_rb_entry *e;
    e = ABCE_CONTAINER_OF(n, struct abce_mb_rb_entry, n);
    if (mbold)
    {
      *mbold = e->val; // no refdn or refup, transfer of ownership
    }
    else
    {
      abce_mb_refdn(abce, &e->val);
    }
    if (!maybe)
    {
      e->val = abce_mb_refup(abce, pval);
      abce_cpop(abce);
      return 0;
    }
    else
    {
      abce_cpop(abce);
      return -EEXIST;
    }
  }

  e = abce->alloc(NULL, 0, sizeof(*e), &abce->alloc_baton);
  if (e == NULL)
  {
    abce->err.code = ABCE_E_NO_MEM;
    abce->err.val2 = sizeof(*e);
    if (mbold)
    {
      mbold->typ = ABCE_T_N;
    }
    abce_cpop(abce);
    return -ENOMEM;
  }
  e->key = abce_mb_refup(abce, mbkey);
  e->val = abce_mb_refup(abce, pval);
  ret = abce_rb_tree_nocmp_insert_nonexist(&mba->u.sc.heads[hashloc],
                                      abce_str_cmp_sym, NULL, &e->n);
  if (ret == 0)
  {
    if (mbold)
    {
      mbold->typ = ABCE_T_N;
    }
    abce_cpop(abce);
    return 0;
  }
  abce_mb_refdn(abce, &e->key);
  abce_mb_refdn(abce, &e->val);
  abce->alloc(e, sizeof(*e), 0, &abce->alloc_baton);
  if (mbold)
  {
    mbold->typ = ABCE_T_N;
  }
  abce_cpop(abce);
  return ret;
}
