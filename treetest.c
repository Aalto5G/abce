#include "abce.h"
#include "trees.h"

int main(int argc, char **argv)
{
  struct abce real_abce = {.alloc = abce_std_alloc};
  struct abce *abce = &real_abce;
  struct abce_mb mbt = abce_mb_create_tree(abce);
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

  abce_tree_set_str(abce, &mbt, &mb01, &nil);
  abce_tree_set_str(abce, &mbt, &mb03, &nil);
  abce_tree_set_str(abce, &mbt, &mb04, &nil);
  abce_tree_set_str(abce, &mbt, &mb06, &nil);
  abce_tree_set_str(abce, &mbt, &mb07, &nil);
  abce_tree_set_str(abce, &mbt, &mb08, &nil);
  abce_tree_set_str(abce, &mbt, &mb10, &nil);
  abce_tree_set_str(abce, &mbt, &mb13, &nil);
  abce_tree_set_str(abce, &mbt, &mb14, &nil);

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

  return 0;
}
