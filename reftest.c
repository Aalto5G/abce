#include <stdlib.h>
#include "amyplanyyutils.h"

char *str =
  "@function $fibonacci($x)\n"
  "  @if($x<3)\n"
  "    @return @dyn[\"fibonacci2\"]($x - 1)\n"
  "  @endif\n"
  "  @return @dyn[\"fibonacci\"]($x - 1) + @dyn[\"fibonacci\"]($x - 2)\n"
  "@endfunction\n";

int main(int argc, char **argv)
{
  FILE *f = fmemopen(str, strlen(str), "r");
  struct amyplanyy amyplanyy = {};
  size_t i;

  amyplanyy_init(&amyplanyy);

  if (!f)
  {
    abort();
  }
  amyplanyydoparse(f, &amyplanyy);
  fclose(f);
  //amyplanyy.abce.gcblockcap = ;
  amyplanyy.abce.in_engine = 1;

  for (i = 0; i < 10*1000*1000; i++)
  {
    abce_mb_cpush_create_tree(&amyplanyy.abce);
    abce_cpop(&amyplanyy.abce);
  }

  amyplanyy_free(&amyplanyy);

  return 0;
}
