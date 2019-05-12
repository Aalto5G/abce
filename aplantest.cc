#include "yyutils.h"
#include <iostream>

/*
size_t symbol_add(struct aplanyy *aplanyy, const char *symbol, size_t symlen)
{
  abort();
}
*/

int main(int argc, char **argv)
{
  FILE *f = fopen("perf/test.amy", "r");
  struct aplanyy aplanyy = {};

  aplanyy_init(&aplanyy);

  if (!f)
  {
    std::terminate();
  }
  aplanyydoparse(f, &aplanyy);
  fclose(f);

  aplanyy_free(&aplanyy);

  return 0;
}
