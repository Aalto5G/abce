#include "scopes.h"
#include "abce.h"

const struct abce_mb *abce_sc_get_rec_mb_area(
  const struct abce_mb_area *mba, const struct abce_mb *it)
{
  const struct abce_mb *mb = abce_sc_get_myval_mb_area(mba, it);
  if (mb != NULL)
  {
    return mb;
  }
  if (mba->u.sc.parent != NULL && !mba->u.sc.holey)
  {
    return abce_sc_get_rec_mb_area(mba->u.sc.parent, it);
  }
  return NULL;
}

const struct abce_mb *abce_sc_get_rec_str_area(
  const struct abce_mb_area *mba, const char *str)
{
  const struct abce_mb *mb = abce_sc_get_myval_str_area(mba, str);
  if (mb != NULL)
  {
    return mb;
  }
  if (mba->u.sc.parent != NULL && !mba->u.sc.holey)
  {
    return abce_sc_get_rec_str_area(mba->u.sc.parent, str);
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
  struct rb_tree_node *n;

  if (mb->typ != ABCE_T_SC || pkey->typ != ABCE_T_S)
  {
    abort();
  }
  hashval = abce_mb_str_hash(pkey);
  hashloc = hashval & (mba->u.sc.size - 1);

  n = RB_TREE_NOCMP_FIND(&mba->u.sc.heads[hashloc], abce_str_cmp_halfsym, NULL, pkey);
  if (n == NULL)
  {
    e = abce->alloc(NULL, sizeof(*e), abce->alloc_baton);
    if (e == NULL)
    {
      abce->err.code = ABCE_E_NO_MEM;
      abce->err.val2 = sizeof(*e);
      return -ENOMEM;
    }
    e->key = abce_mb_refup(abce, pkey);
    e->val = abce_mb_refup(abce, pval);
    ret = rb_tree_nocmp_insert_nonexist(&mba->u.sc.heads[hashloc],
                                        abce_str_cmp_sym, NULL, &e->n);
    if (ret != 0)
    {
      abort();
    }
    return 0;
  }
  e = CONTAINER_OF(n, struct abce_mb_rb_entry, n);
  abce_mb_refdn(abce, &e->val);
  e->val = abce_mb_refup(abce, pval);
  return 0;
}

int abce_sc_put_val_str(
  struct abce *abce,
  const struct abce_mb *mb, const char *str, const struct abce_mb *pval)
{
  struct abce_mb_area *mba = mb->u.area;
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
  e = abce->alloc(NULL, sizeof(*e), abce->alloc_baton);
  if (e == NULL)
  {
    abce->err.code = ABCE_E_NO_MEM;
    abce->err.val2 = sizeof(*e);
    return -ENOMEM;
  }
  e->key = abce_mb_create_string(abce, str, strlen(str));
  if (e->key.typ == ABCE_T_N)
  {
    abce->err.code = ABCE_E_NO_MEM;
    abce->err.val2 = strlen(str);
    return -ENOMEM;
  }
  e->val = abce_mb_refup(abce, pval);
  ret = rb_tree_nocmp_insert_nonexist(&mba->u.sc.heads[hashloc],
                                      abce_str_cmp_sym, NULL, &e->n);
  if (ret == 0)
  {
    return 0;
  }
  abce_mb_refdn(abce, &e->key);
  abce_mb_refdn(abce, &e->val);
  abce->alloc(e, 0, abce->alloc_baton);
  return ret;
}
