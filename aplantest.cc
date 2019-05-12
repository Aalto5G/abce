#include "yyutils.h"
#include <iostream>

size_t symbol_add(struct aplanyy *aplanyy, const char *symbol, size_t symlen)
{
  abort();
}
size_t aplanyy_add_fun_sym(struct aplanyy *aplanyy, const char *symbol, int maybe, size_t loc)
{
  abort();
}

int main(int argc, char **argv)
{
  FILE *f = fopen("perf/test.amy", "r");
  struct aplanyy aplanyy = {};

  if (!f)
  {
    std::terminate();
  }
  aplanyydoparse(f, &aplanyy);
  fclose(f);

  return 0;
}
