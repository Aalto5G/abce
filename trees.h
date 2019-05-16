#ifndef _TREES_H_
#define _TREES_H_

#include <stddef.h>
#include "abce.h"

static inline int
abce_tree_get_str(struct abce *abce, struct abce_mb **mbres,
                  struct abce_mb *mbt, const struct abce_mb *mbkey)
{
  struct rb_tree_node *n;
  if (mbt->typ != ABCE_T_T || mbkey->typ != ABCE_T_S)
  {
    abort();
  }
  n = RB_TREE_NOCMP_FIND(&mbt->u.area->u.tree.tree, abce_str_cmp_halfsym, NULL, mbkey);
  if (n == NULL)
  {
    return -ENOENT;
  }
  *mbres = &CONTAINER_OF(n, struct abce_mb_rb_entry, n)->val;
  return 0;
}
static inline int
abce_tree_del_str(struct abce *abce,
                  struct abce_mb *mbt, const struct abce_mb *mbkey)
{
  struct rb_tree_node *n;
  struct abce_mb_rb_entry *mbe;
  if (mbt->typ != ABCE_T_T || mbkey->typ != ABCE_T_S)
  {
    abort();
  }
  n = RB_TREE_NOCMP_FIND(&mbt->u.area->u.tree.tree, abce_str_cmp_halfsym, NULL, mbkey);
  if (n == NULL)
  {
    return -ENOENT;
  }
  mbe = CONTAINER_OF(n, struct abce_mb_rb_entry, n);
  abce_mb_refdn(abce, &mbe->key);
  abce_mb_refdn(abce, &mbe->val);
  rb_tree_nocmp_delete(&mbt->u.area->u.tree.tree, n);
  abce->alloc(mbe, 0, abce->alloc_baton);
  return 0;
}

static inline int
abce_tree_set_str(struct abce *abce,
                  struct abce_mb *mbt,
                  const struct abce_mb *mbkey,
                  const struct abce_mb *mbval)
{
  struct abce_mb *mbres;
  struct abce_mb_rb_entry *e;
  if (mbt->typ != ABCE_T_T)
  {
    abort();
  }
  if (mbkey->typ != ABCE_T_S)
  {
    abort();
  }
  if (abce_tree_get_str(abce, &mbres, mbt, mbkey) == 0)
  {
    abce_mb_refdn(abce, mbres);
    *mbres = abce_mb_refup(abce, mbval);
    return 0;
  }
  e = abce->alloc(NULL, sizeof(*e), abce->alloc_baton);
  if (e == NULL)
  {
    abce->err.code = ABCE_E_NO_MEM;
    abce->err.mb = abce_mb_refup(abce, mbkey);
    abce->err.val2 = sizeof(*e);
    return -ENOMEM;
  }
  e->key = abce_mb_refup(abce, mbkey);
  e->val = abce_mb_refup(abce, mbval);
  if (rb_tree_nocmp_insert_nonexist(&mbt->u.area->u.tree.tree, abce_str_cmp_sym, NULL, &e->n) != 0)
  {
    abort();
  }
  mbt->u.area->u.tree.sz += 1;
  return 0;
}

// FIXME verify if algorithm correct
static inline int
abce_tree_get_next(struct abce *abce,
                   const struct abce_mb **mbreskey,
                   const struct abce_mb **mbresval,
                   const struct abce_mb *mbt,
                   const struct abce_mb *mbkey)
{
  struct rb_tree_node *node;
  struct abce_mb_rb_entry *mbe;
  if (mbt->typ != ABCE_T_T)
  {
    abort();
  }
  if (mbkey->typ != ABCE_T_S && mbkey->typ != ABCE_T_N)
  {
    abort();
  }
  node = mbt->u.area->u.tree.tree.root;
  if (mbkey->typ == ABCE_T_N)
  {
    for (;;)
    {
      if (node->left == NULL)
      {
        break;
      }
      node = node->left;
    }
    goto out;
  }
  while (node != NULL)
  {
    int res = abce_str_cmp_halfsym(mbkey, node, NULL);
    if (res < 0)
    {
      if (node->left == NULL)
      {
        goto out;
        //break;
      }
      node = node->left;
    }
    else if (res > 0)
    {
      if (node->right == NULL)
      {
        break;
      }
      node = node->right;
    }
    else if (res == 0)
    {
      break;
    }
  }
  if (node->right != NULL)
  {
    node = node->right;
    for (;;)
    {
      if (node->left == NULL)
      {
        break;
      }
      node = node->left;
    }
  }
  else
  {
    while (node->parent && node->parent->right == node)
    {
      node = node->parent;
    }
    node = node->parent;
  }
out:
  if (node == NULL)
  {
    *mbreskey = NULL;
    *mbresval = NULL;
    return -ENOENT;
  }
  mbe = CONTAINER_OF(node, struct abce_mb_rb_entry, n);
  *mbreskey = &mbe->key;
  *mbresval = &mbe->val;
  return 0;
}

#endif
