#include "abce.h"

int main(int argc, char **argv)
{
  struct abce abce = {};
  int64_t a, b;
  abce_init(&abce);

  a = abce_cache_add_str_nul(&abce, "foo");
  printf("a %d\n", (int)a);
  b = abce_cache_add_str_nul(&abce, "bar");
  printf("b %d\n", (int)b);
  b = abce_cache_add_str_nul(&abce, "bar");
  printf("b %d\n", (int)b);
  b = abce_cache_add_str_nul(&abce, "bar");
  printf("b %d\n", (int)b);
  b = abce_cache_add_str_nul(&abce, "bar");
  printf("b %d\n", (int)b);

  abce_add_ins(&abce, ABCE_OPCODE_PUSH_DBL);
  abce_add_double(&abce, 4 + 2*2 + 2*8); // jmp offset
  abce_add_ins(&abce, ABCE_OPCODE_FUNIFY);
  abce_add_ins(&abce, ABCE_OPCODE_PUSH_DBL);
  abce_add_double(&abce, 0); // arg cnt
  abce_add_ins(&abce, ABCE_OPCODE_CALL);
  abce_add_ins(&abce, ABCE_OPCODE_DUMP);
  abce_add_ins(&abce, ABCE_OPCODE_EXIT);

  abce_add_ins(&abce, ABCE_OPCODE_FUN_HEADER);
  abce_add_double(&abce, 0);

  abce_add_ins(&abce, ABCE_OPCODE_PUSH_NEW_ARRAY); // lists

  abce_add_ins(&abce, ABCE_OPCODE_PUSH_NEW_ARRAY); // list

  abce_add_ins(&abce, ABCE_OPCODE_PUSH_DBL);
  abce_add_double(&abce, a);
  abce_add_ins(&abce, ABCE_OPCODE_PUSH_FROM_CACHE);
  abce_add_ins(&abce, ABCE_OPCODE_APPEND_MAINTAIN);

  abce_add_ins(&abce, ABCE_OPCODE_APPEND_MAINTAIN);

  abce_add_ins(&abce, ABCE_OPCODE_PUSH_NEW_ARRAY); // list

  abce_add_ins(&abce, ABCE_OPCODE_PUSH_DBL);
  abce_add_double(&abce, b);
  abce_add_ins(&abce, ABCE_OPCODE_PUSH_FROM_CACHE);
  abce_add_ins(&abce, ABCE_OPCODE_APPEND_MAINTAIN);

  abce_add_ins(&abce, ABCE_OPCODE_APPEND_MAINTAIN);

  abce_add_ins(&abce, ABCE_OPCODE_RET);

  printf("%d\n", abce_engine(&abce, NULL, 0, UINT64_MAX));

  abce_free(&abce);

  printf("alloced after free %zu\n", abce.bytes_alloced);
/*
  struct abce real_abce = {.alloc = std_alloc};
  struct abce *abce = &real_abce;
  struct abce_mb mba = abce_mb_create_array(abce);
  struct abce_mb mbsc1 = abce_mb_create_scope_noparent(abce, 16);
  struct abce_mb mbsc2 = abce_mb_create_scope(abce, 16, &mbsc1, 0);
  struct abce_mb mbs1 = abce_mb_create_string_nul(abce, "foo");
  struct abce_mb mbs2 = abce_mb_create_string_nul(abce, "bar");
  struct abce_mb mbs3 = abce_mb_create_string_nul(abce, "baz");
  struct abce_mb mbs4 = abce_mb_create_string_nul(abce, "barf");
  struct abce_mb mbs5 = abce_mb_create_string_nul(abce, "quux");
  abce_mb_array_append(abce, &mba, &mbs1);
  abce_mb_array_append(abce, &mba, &mbs2);
  abce_mb_array_append(abce, &mba, &mbs3);
  sc_put_val_mb(abce, &mbsc1, &mbs1, &mbs2);
  sc_put_val_mb(abce, &mbsc1, &mbs3, &mbs4);
  sc_put_val_mb(abce, &mbsc2, &mbs2, &mbs1);
  sc_put_val_mb(abce, &mbsc2, &mbs4, &mbs3);
  abce_mb_dump(&mba);
  abce_mb_dump(&mbsc1);
  abce_mb_dump(&mbsc2);
  abce_mb_dump(sc_get_rec_str(&mbsc2, "foo"));
  abce_mb_dump(sc_get_rec_str(&mbsc2, "bar"));
  abce_mb_dump(sc_get_rec_str(&mbsc2, "baz"));
  abce_mb_dump(sc_get_rec_str(&mbsc2, "barf"));
  abce_mb_refdn(abce, &mba);
  abce_mb_refdn(abce, &mbsc1);
  abce_mb_refdn(abce, &mbsc2);
  abce_mb_refdn(abce, &mbs1);
  abce_mb_refdn(abce, &mbs2);
  abce_mb_refdn(abce, &mbs3);
  abce_mb_refdn(abce, &mbs4);
  abce_mb_refdn(abce, &mbs5);
  stacktest_main(abce);
*/
  return 0;
}
