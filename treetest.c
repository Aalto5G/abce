#include "abce.h"
#include "trees.h"

void mydump_tre(int indent, const struct abce_rb_tree_node *node)
{
  int i;
  if (node == NULL)
  {
    return;
  }
  for (i = 0; i < indent; i++)
  {
    printf(" ");
  }
  printf("key: %s\n", CONTAINER_OF(node, struct abce_mb_rb_entry, n)->key.u.area->u.str.buf);
  for (i = 0; i < indent; i++)
  {
    printf(" ");
  }
  printf("left:\n");
  mydump_tre(indent+1, node->left);
  for (i = 0; i < indent; i++)
  {
    printf(" ");
  }
  printf("right:\n");
  mydump_tre(indent+1, node->right);
}

int main(int argc, char **argv)
{
  int i;
  struct abce real_abce = {.alloc = abce_std_alloc};
  struct abce *abce = &real_abce;
  struct abce_mb mbt = abce_mb_create_tree(abce);
  struct abce_mb mbs[20];
  struct abce_mb mb01 = abce_mb_create_string_nul(abce, "01");
  struct abce_mb mb03 = abce_mb_create_string_nul(abce, "03");
  struct abce_mb mb04 = abce_mb_create_string_nul(abce, "04");
  struct abce_mb mb06 = abce_mb_create_string_nul(abce, "06");
  struct abce_mb mb07 = abce_mb_create_string_nul(abce, "07");
  struct abce_mb mb08 = abce_mb_create_string_nul(abce, "08");
  struct abce_mb mb10 = abce_mb_create_string_nul(abce, "10");
  struct abce_mb mb13 = abce_mb_create_string_nul(abce, "13");
  struct abce_mb mb14 = abce_mb_create_string_nul(abce, "14");
  struct abce_mb nil = {.typ = ABCE_T_N};
  const struct abce_mb *mbkey, *mbval;

  mbs[0] = abce_mb_create_string_nul(abce, "00");
  mbs[1] = abce_mb_create_string_nul(abce, "01");
  mbs[2] = abce_mb_create_string_nul(abce, "02");
  mbs[3] = abce_mb_create_string_nul(abce, "03");
  mbs[4] = abce_mb_create_string_nul(abce, "04");
  mbs[5] = abce_mb_create_string_nul(abce, "05");
  mbs[6] = abce_mb_create_string_nul(abce, "06");
  mbs[7] = abce_mb_create_string_nul(abce, "07");
  mbs[8] = abce_mb_create_string_nul(abce, "08");
  mbs[9] = abce_mb_create_string_nul(abce, "09");
  mbs[10] = abce_mb_create_string_nul(abce, "10");
  mbs[11] = abce_mb_create_string_nul(abce, "11");
  mbs[12] = abce_mb_create_string_nul(abce, "12");
  mbs[13] = abce_mb_create_string_nul(abce, "13");
  mbs[14] = abce_mb_create_string_nul(abce, "14");
  mbs[15] = abce_mb_create_string_nul(abce, "15");
  mbs[16] = abce_mb_create_string_nul(abce, "16");
  mbs[17] = abce_mb_create_string_nul(abce, "17");
  mbs[18] = abce_mb_create_string_nul(abce, "18");
  mbs[19] = abce_mb_create_string_nul(abce, "19");

  abce_tree_set_str(abce, &mbt, &mb01, &nil);
  abce_tree_set_str(abce, &mbt, &mb03, &nil);
  abce_tree_set_str(abce, &mbt, &mb04, &nil);
  abce_tree_set_str(abce, &mbt, &mb06, &nil);
  abce_tree_set_str(abce, &mbt, &mb07, &nil);
  abce_tree_set_str(abce, &mbt, &mb08, &nil);
  abce_tree_set_str(abce, &mbt, &mb10, &nil);
  abce_tree_set_str(abce, &mbt, &mb13, &nil);
  abce_tree_set_str(abce, &mbt, &mb14, &nil);

  mydump_tre(0, mbt.u.area->u.tree.tree.root);
  mbkey = &nil;
  while (abce_tree_get_next(abce, &mbkey, &mbval, &mbt, mbkey) == 0)
  {
    printf("%s\n", mbkey->u.area->u.str.buf);
  }

  printf("----------\n");
  for (i = 0; i < 20; i++)
  {
    if (abce_tree_get_next(abce, &mbkey, &mbval, &mbt, &mbs[i]) != 0)
    {
      printf("%d -ENOENT\n", i);
    }
    else
    {
      printf("%d %s\n", i, mbkey->u.area->u.str.buf);
    }
  }

  abce_mb_refdn(abce, &mbt);
  abce_mb_refdn(abce, &mb01);
  abce_mb_refdn(abce, &mb03);
  abce_mb_refdn(abce, &mb04);
  abce_mb_refdn(abce, &mb06);
  abce_mb_refdn(abce, &mb07);
  abce_mb_refdn(abce, &mb08);
  abce_mb_refdn(abce, &mb10);
  abce_mb_refdn(abce, &mb13);
  abce_mb_refdn(abce, &mb14);

  for (i = 0; i < 20; i++)
  {
    abce_mb_refdn(abce, &mbs[i]);
  }

  return 0;
}
