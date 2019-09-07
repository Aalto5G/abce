#include <stdlib.h>
#include "amyplanyyutils.h"

char *str =
  "@function $error()\n"
  "  @error(\"x\")\n"
  "  @return 1\n"
  "@endfunction\n"
  "@function $shortcut()\n"
  "  @locvar $truthy = 1\n"
  "  @locvar $falsy = 0\n"
  "  @dump($truthy || @D$error())\n"
  "  @dump($falsy && @D$error())\n"
  "  @dump($truthy || $truthy)\n"
  "  @dump($truthy || $falsy)\n"
  "  @dump($falsy || $truthy)\n"
  "  @dump($falsy || $falsy)\n"
  "  @dump($truthy && $truthy)\n"
  "  @dump($truthy && $falsy)\n"
  "  @dump($falsy && $truthy)\n"
  "  @dump($falsy && $falsy)\n"
  "@endfunction\n";

int main(int argc, char **argv)
{
  FILE *f = fmemopen(str, strlen(str), "r");
  struct amyplanyy amyplanyy = {};
  unsigned char tmpbuf[1024] = {0};
  size_t tmpsiz = 0;

  amyplanyy_init(&amyplanyy);

  if (!f)
  {
    abort();
  }
  amyplanyydoparse(f, &amyplanyy);
  fclose(f);

  {
    tmpsiz = 0;
    amyplanyy.abce.sp = 0;
    abce_add_ins_alt(tmpbuf, &tmpsiz, sizeof(tmpbuf), ABCE_OPCODE_PUSH_DBL);
    abce_add_double_alt(tmpbuf, &tmpsiz, sizeof(tmpbuf),
      abce_sc_get_rec_str_fun(&amyplanyy.abce.dynscope, "shortcut", 1));
    abce_add_ins_alt(tmpbuf, &tmpsiz, sizeof(tmpbuf), ABCE_OPCODE_FUNIFY);

    abce_add_ins_alt(tmpbuf, &tmpsiz, sizeof(tmpbuf), ABCE_OPCODE_PUSH_DBL);
    abce_add_double_alt(tmpbuf, &tmpsiz, sizeof(tmpbuf), 0); // arg cnt
    abce_add_ins_alt(tmpbuf, &tmpsiz, sizeof(tmpbuf), ABCE_OPCODE_CALL);
    abce_add_ins_alt(tmpbuf, &tmpsiz, sizeof(tmpbuf), ABCE_OPCODE_EXIT);
    printf("ret %d\n", abce_engine(&amyplanyy.abce, tmpbuf, tmpsiz));
    printf("sp %zu\n", amyplanyy.abce.sp);
    printf("\n");
  }

  amyplanyy_free(&amyplanyy);

  return 0;
}
