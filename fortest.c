#include <stdlib.h>
#include "amyplanyyutils.h"

char *str =
  "@function $for($x)\n"
  "  @locvar $y = 0\n"
  "  @for($y = 1, $y <= $x, $y = $y + 1)\n"
  "    @if($y == $x - 3)\n"
  "      @continue\n"
  "    @endif\n"
  "    @if($y == $x - 1)\n"
  "      @break\n"
  "    @endif\n"
  "    @dump($y)\n"
  "  @endfor\n"
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
    abce_sc_get_rec_str_fun(&amyplanyy.abce.dynscope, "for", 1));
  abce_add_ins_alt(tmpbuf, &tmpsiz, sizeof(tmpbuf), ABCE_OPCODE_FUNIFY);
  abce_add_ins_alt(tmpbuf, &tmpsiz, sizeof(tmpbuf), ABCE_OPCODE_PUSH_DBL);
  abce_add_double_alt(tmpbuf, &tmpsiz, sizeof(tmpbuf), 10); // arg
  abce_add_ins_alt(tmpbuf, &tmpsiz, sizeof(tmpbuf), ABCE_OPCODE_PUSH_DBL);
  abce_add_double_alt(tmpbuf, &tmpsiz, sizeof(tmpbuf), 1); // arg cnt
  abce_add_ins_alt(tmpbuf, &tmpsiz, sizeof(tmpbuf), ABCE_OPCODE_CALL);
  abce_add_ins_alt(tmpbuf, &tmpsiz, sizeof(tmpbuf), ABCE_OPCODE_DUMP);
  //abce_add_ins_alt(tmpbuf, &tmpsiz, sizeof(tmpbuf), ABCE_OPCODE_POP);
  abce_add_ins_alt(tmpbuf, &tmpsiz, sizeof(tmpbuf), ABCE_OPCODE_EXIT);

#if 0
  for (i = 0; i < 30000; i++)
  {
    amyplanyy.abce.ip = -tmpsiz-ABCE_GUARD;
    abce_engine(&amyplanyy.abce, tmpbuf, tmpsiz);
  }
#endif

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
