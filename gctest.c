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
  //amyplanyy.abce.do_check_heap_on_gc = 1;

  //abce_gc(&amyplanyy.abce);

  for (i = 0; i < 10*1000*1000; i++)
  {
    struct abce_mb *mb1, *mb2;
    mb1 = abce_mb_cpush_create_array(&amyplanyy.abce);
    if (mb1 == NULL)
    {
      abort();
    }
    if (abce_push_mb(&amyplanyy.abce, mb1) != 0)
    {
      abort();
    }
    mb2 = abce_mb_cpush_create_array(&amyplanyy.abce);
    if (mb2 == NULL)
    {
      abort();
    }
    if (abce_push_mb(&amyplanyy.abce, mb2) != 0)
    {
      abort();
    }
    if (abce_mb_array_append(&amyplanyy.abce,
                             &amyplanyy.abce.stackbase[0],
                             &amyplanyy.abce.stackbase[1]) != 0)
    {
      abort();
    }
    if (abce_mb_array_append(&amyplanyy.abce,
                             &amyplanyy.abce.stackbase[1],
                             &amyplanyy.abce.stackbase[0]) != 0)
    {
      abort();
    }
    if (abce_pop(&amyplanyy.abce) != 0)
    {
      abort();
    }
    if (abce_pop(&amyplanyy.abce) != 0)
    {
      abort();
    }
    if (abce_cpop(&amyplanyy.abce) != 0)
    {
      abort();
    }
    if (abce_cpop(&amyplanyy.abce) != 0)
    {
      abort();
    }
    //abce_gc(&amyplanyy.abce);
  }

  amyplanyy_free(&amyplanyy);

  return 0;
}
