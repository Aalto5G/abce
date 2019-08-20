#include <stdlib.h>
#include "amyplanyyutils.h"

char *str =
  "@function $if($x)\n"
  "  @if (@false)\n"
  "    @dump(1)\n"
  "  @elseif (@false)\n"
  "    @dump(2)\n"
  "  @elseif (@false)\n"
  "    @dump(3)\n"
  "  @elseif (@false)\n"
  "    @dump(4)\n"
  "  @elseif (@true)\n"
  "    @dump(5)\n"
#if 0
  "  @else\n"
  "    @dump(6)\n"
#endif
  "  @endif\n"
  "@endfunction\n";

int main(int argc, char **argv)
{
  FILE *f = fmemopen(str, strlen(str), "r");
  struct amyplanyy amyplanyy = {};
  unsigned char tmpbuf[1024] = {0};
  size_t tmpsiz = 0;
  size_t i;

  amyplanyy_init(&amyplanyy);

  if (!f)
  {
    abort();
  }
  amyplanyydoparse(f, &amyplanyy);
  fclose(f);

  abce_add_ins_alt(tmpbuf, &tmpsiz, sizeof(tmpbuf), ABCE_OPCODE_PUSH_DBL);
  abce_add_double_alt(tmpbuf, &tmpsiz, sizeof(tmpbuf),
    abce_sc_get_rec_str_fun(&amyplanyy.abce.dynscope, "if", 1));
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
    if (ins == 194)
    {
      i++;
      ins = amyplanyy.abce.bytecode[i];
    }
    switch (ins)
    {
      case ABCE_OPCODE_PUSH_FALSE:
        printf("%.5zu: PUSH_FALSE", i);
        break;
      case ABCE_OPCODE_PUSH_TRUE:
        printf("%.5zu: PUSH_TRUE", i);
        break;
      case ABCE_OPCODE_PUSH_DBL:
        printf("%.5zu: PUSH_DBL", i);
        break;
      case 160:
        printf("%.5zu: DUMP", i);
        break;
      case ABCE_OPCODE_IF_NOT_JMP:
        printf("%.5zu: IF_NOT_JMP", i);
        break;
      case ABCE_OPCODE_JMP:
        printf("%.5zu: JMP", i);
        break;
      default:
        printf("%.5zu: ins %d", i, (int)ins);
    }
    //printf("%.5zu: ins %d", i, (int)ins);
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
