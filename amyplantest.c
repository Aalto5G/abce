#include <stdlib.h>
#include "amyplanyyutils.h"

int main(int argc, char **argv)
{
  FILE *f = fopen("perf/test.amy", "r");
  struct amyplanyy amyplanyy = {};

  amyplanyy_init(&amyplanyy);

  if (!f)
  {
    abort();
  }
  amyplanyydoparse(f, &amyplanyy);
  fclose(f);

  amyplanyy_free(&amyplanyy);

  return 0;
}
