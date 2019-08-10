#include <stdlib.h>
#include "amyplanyyutils.h"

char *str =
  "@function $fiboeff($x)\n"
  "  @locvar $table = [1, 1, 1]\n"
  "  @locvar $y = 3\n"
  "  @while($y <= $x)\n"
  "    @append($table, $table[$y - 1] + $table[$y - 2])\n"
  "    $y = $y + 1\n"
  "  @endwhile\n"
  "  @return $table[$x]\n"
  "@endfunction\n";

int main(int argc, char **argv)
{
  FILE *f = fmemopen(str, strlen(str), "r");
  struct amyplanyy amyplanyy = {};
  unsigned char tmpbuf[1024] = {0};
  size_t tmpsiz = 0;
  //size_t i;

  amyplanyy_init(&amyplanyy);

  if (!f)
  {
    abort();
  }
  amyplanyydoparse(f, &amyplanyy);
  fclose(f);

  abce_add_ins_alt(tmpbuf, &tmpsiz, sizeof(tmpbuf), ABCE_OPCODE_PUSH_DBL);
  abce_add_double_alt(tmpbuf, &tmpsiz, sizeof(tmpbuf),
    abce_sc_get_rec_str_fun(&amyplanyy.abce.dynscope, "fiboeff", 1));
  abce_add_ins_alt(tmpbuf, &tmpsiz, sizeof(tmpbuf), ABCE_OPCODE_FUNIFY);
  abce_add_ins_alt(tmpbuf, &tmpsiz, sizeof(tmpbuf), ABCE_OPCODE_PUSH_DBL);
  abce_add_double_alt(tmpbuf, &tmpsiz, sizeof(tmpbuf), 7); // arg
  abce_add_ins_alt(tmpbuf, &tmpsiz, sizeof(tmpbuf), ABCE_OPCODE_PUSH_DBL);
  abce_add_double_alt(tmpbuf, &tmpsiz, sizeof(tmpbuf), 1); // arg cnt
  abce_add_ins_alt(tmpbuf, &tmpsiz, sizeof(tmpbuf), ABCE_OPCODE_CALL);
  abce_add_ins_alt(tmpbuf, &tmpsiz, sizeof(tmpbuf), ABCE_OPCODE_DUMP);
  abce_add_ins_alt(tmpbuf, &tmpsiz, sizeof(tmpbuf), ABCE_OPCODE_EXIT);

  amyplanyy.abce.ip = -tmpsiz-ABCE_GUARD;

  printf("ret %d\n", abce_engine(&amyplanyy.abce, tmpbuf, tmpsiz));
  printf("actual err %d\n", (int)amyplanyy.abce.err.code);
  printf("actual typ %d\n", (int)amyplanyy.abce.err.mb.typ);

  // FIXME doesn't support long instructions
#if 0
  i = 0;
  while (i < amyplanyy.abce.bytecodesz)
  {
    uint8_t ins = amyplanyy.abce.bytecode[i];
    printf("%.5zu: ins %d", i, (int)ins);
    i++;
    if (ins == ABCE_OPCODE_PUSH_DBL || ins == ABCE_OPCODE_FUN_HEADER)
    {
      double d;
      abce_get_dbl(&d, &amyplanyy.abce.bytecode[i]);
      i += 8;
      printf(", dbl %g", d);
    }
    printf("\n");
  }
#endif

  amyplanyy_free(&amyplanyy);

  return 0;
}
