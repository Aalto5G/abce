#include <stdlib.h>
#include "yyutils.h"

int main(int argc, char **argv)
{
  FILE *f = fopen("perf/test.amy", "r");
  struct aplanyy aplanyy = {};

  aplanyy_init(&aplanyy);

  if (!f)
  {
    abort();
  }
  aplanyydoparse(f, &aplanyy);
  fclose(f);

  aplanyy_free(&aplanyy);

  return 0;
}
