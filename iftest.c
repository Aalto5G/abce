#include <stdlib.h>
#include "amyplanyyutils.h"

char *str =
  "@function $if($x0,$x1,$x2,$x3,$x4,$x5,$x6)\n"
  "  @if ($x0)\n"
  "    @return 0\n"
  "  @elseif ($x1)\n"
  "    @return 1\n"
  "  @elseif ($x2)\n"
  "    @return 2\n"
  "  @elseif ($x3)\n"
  "    @return 3\n"
  "  @elseif ($x4)\n"
  "    @return 4\n"
  "  @elseif ($x5)\n"
  "    @return 5\n"
  "  @elseif ($x6)\n"
  "    @return 6\n"
#if 0 // Adjust this to check with and without @else
  "  @else\n"
  "    @return 7\n"
#endif
  "  @endif\n"
  "  @return 7\n"
  "@endfunction\n";

int same_in_c(int *x)
{
  size_t i;
  for (i = 0; i < 7; i++)
  {
    if (x[i])
    {
      return i;
    }
  }
  return 7;
}

int main(int argc, char **argv)
{
  FILE *f = fmemopen(str, strlen(str), "r");
  struct amyplanyy amyplanyy = {};
  unsigned char tmpbuf[1024] = {0};
  size_t tmpsiz = 0;
  //size_t i;
  int x[7];

  amyplanyy_init(&amyplanyy);

  if (!f)
  {
    abort();
  }
  amyplanyydoparse(f, &amyplanyy);
  fclose(f);

  for (x[0] = 0; x[0] < 2; x[0]++)
  for (x[1] = 0; x[1] < 2; x[1]++)
  for (x[2] = 0; x[2] < 2; x[2]++)
  for (x[3] = 0; x[3] < 2; x[3]++)
  for (x[4] = 0; x[4] < 2; x[4]++)
  for (x[5] = 0; x[5] < 2; x[5]++)
  for (x[6] = 0; x[6] < 2; x[6]++)
  {
    tmpsiz = 0;
    amyplanyy.abce.sp = 0;
    abce_add_ins_alt(tmpbuf, &tmpsiz, sizeof(tmpbuf), ABCE_OPCODE_PUSH_DBL);
    abce_add_double_alt(tmpbuf, &tmpsiz, sizeof(tmpbuf),
      abce_sc_get_rec_str_fun(&amyplanyy.abce.dynscope, "if", 1));
    abce_add_ins_alt(tmpbuf, &tmpsiz, sizeof(tmpbuf), ABCE_OPCODE_FUNIFY);

    abce_add_ins_alt(tmpbuf, &tmpsiz, sizeof(tmpbuf),
                     x[0] ? ABCE_OPCODE_PUSH_TRUE : ABCE_OPCODE_PUSH_FALSE);
    abce_add_ins_alt(tmpbuf, &tmpsiz, sizeof(tmpbuf),
                     x[1] ? ABCE_OPCODE_PUSH_TRUE : ABCE_OPCODE_PUSH_FALSE);
    abce_add_ins_alt(tmpbuf, &tmpsiz, sizeof(tmpbuf),
                     x[2] ? ABCE_OPCODE_PUSH_TRUE : ABCE_OPCODE_PUSH_FALSE);
    abce_add_ins_alt(tmpbuf, &tmpsiz, sizeof(tmpbuf),
                     x[3] ? ABCE_OPCODE_PUSH_TRUE : ABCE_OPCODE_PUSH_FALSE);
    abce_add_ins_alt(tmpbuf, &tmpsiz, sizeof(tmpbuf),
                     x[4] ? ABCE_OPCODE_PUSH_TRUE : ABCE_OPCODE_PUSH_FALSE);
    abce_add_ins_alt(tmpbuf, &tmpsiz, sizeof(tmpbuf),
                     x[5] ? ABCE_OPCODE_PUSH_TRUE : ABCE_OPCODE_PUSH_FALSE);
    abce_add_ins_alt(tmpbuf, &tmpsiz, sizeof(tmpbuf),
                     x[6] ? ABCE_OPCODE_PUSH_TRUE : ABCE_OPCODE_PUSH_FALSE);

    abce_add_ins_alt(tmpbuf, &tmpsiz, sizeof(tmpbuf), ABCE_OPCODE_PUSH_DBL);
    abce_add_double_alt(tmpbuf, &tmpsiz, sizeof(tmpbuf), 7); // arg cnt
    abce_add_ins_alt(tmpbuf, &tmpsiz, sizeof(tmpbuf), ABCE_OPCODE_CALL);
    //abce_add_ins_alt(tmpbuf, &tmpsiz, sizeof(tmpbuf), ABCE_OPCODE_DUMP);
    //abce_add_ins_alt(tmpbuf, &tmpsiz, sizeof(tmpbuf), ABCE_OPCODE_POP);
    abce_add_ins_alt(tmpbuf, &tmpsiz, sizeof(tmpbuf), ABCE_OPCODE_EXIT);
    printf("ret %d\n", abce_engine(&amyplanyy.abce, tmpbuf, tmpsiz));
    printf("sp %zu\n", amyplanyy.abce.sp);
    printf("num %g\n", amyplanyy.abce.stackbase[0].u.d);
    printf("same_in_c %d\n", same_in_c(x));
    if ((int)amyplanyy.abce.stackbase[0].u.d != same_in_c(x))
    {
      abort();
    }
    printf("\n");
  }

#if 0
  for (i = 0; i < 30000; i++)
  {
    amyplanyy.abce.ip = -tmpsiz-ABCE_GUARD;
    abce_engine(&amyplanyy.abce, tmpbuf, tmpsiz);
  }

  amyplanyy.abce.ip = -tmpsiz-ABCE_GUARD;

  printf("ret %d\n", abce_engine(&amyplanyy.abce, tmpbuf, tmpsiz));
  printf("actual err %d\n", (int)amyplanyy.abce.err.code);
  printf("actual typ %d\n", (int)amyplanyy.abce.err.mb.typ);
#endif

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
