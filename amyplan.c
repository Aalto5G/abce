#include <stdlib.h>
#include "amyplanyyutils.h"
#include "abcetrees.h"

void usage(const char *argv0)
{
  fprintf(stderr, "Usage: %s file.amy\n", argv0);
  exit(1);
}

int main(int argc, char **argv, char **envp)
{
  FILE *f;
  struct amyplanyy amyplanyy = {};
  unsigned char tmpbuf[1024] = {0};
  size_t tmpsiz = 0;
  struct abce_mb *mbar = NULL;
  struct abce_mb *mbt = NULL;
  int64_t idx;
  int i;
  int ret;
  char **envit;

  if (argc < 1)
  {
    usage(argv[0]);
  }
  if (argc == 1)
  {
    usage(argv[0]); // REPL not implemented yet
  }
  f = fopen(argv[1], "r");

  amyplanyy_init(&amyplanyy);

  if (!f)
  {
    fprintf(stderr, "Can't open file %s\n", argv[1]);
    exit(1);
  }
  amyplanyydoparse(f, &amyplanyy);
  fclose(f);

  tmpsiz = 0;
  amyplanyy.abce.sp = 0;
  abce_add_ins_alt(tmpbuf, &tmpsiz, sizeof(tmpbuf), ABCE_OPCODE_PUSH_DBL);
  abce_add_double_alt(tmpbuf, &tmpsiz, sizeof(tmpbuf),
    abce_sc_get_rec_str_fun(&amyplanyy.abce.dynscope, "main", 1));
  abce_add_ins_alt(tmpbuf, &tmpsiz, sizeof(tmpbuf), ABCE_OPCODE_FUNIFY);

  mbar = abce_mb_cpush_create_array(&amyplanyy.abce);
  for (i = 1; i < argc; i++)
  {
    struct abce_mb *mb;
    mb = abce_mb_cpush_create_string(&amyplanyy.abce, argv[i], strlen(argv[i]));
    if (abce_mb_array_append(&amyplanyy.abce, mbar, mb) != 0)
    {
      fprintf(stderr, "Out of memory\n");
      exit(1);
    }
    abce_cpop(&amyplanyy.abce);
  }
  idx = abce_cache_add(&amyplanyy.abce, mbar);
  abce_cpop(&amyplanyy.abce);
  abce_add_ins_alt(tmpbuf, &tmpsiz, sizeof(tmpbuf), ABCE_OPCODE_PUSH_DBL);
  abce_add_double_alt(tmpbuf, &tmpsiz, sizeof(tmpbuf), idx);
  abce_add_ins_alt(tmpbuf, &tmpsiz, sizeof(tmpbuf), ABCE_OPCODE_PUSH_FROM_CACHE);

  mbt = abce_mb_cpush_create_tree(&amyplanyy.abce);
  for (envit = envp; *envit != NULL; envit++)
  {
    struct abce_mb *mbkey, *mbval;
    char *env = *envit;
    char *res;
    res = strchr(env, '=');
    if (res != NULL) {
      mbkey = abce_mb_cpush_create_string(&amyplanyy.abce, env, res - env);
      mbval = abce_mb_cpush_create_string(&amyplanyy.abce, res+1, strlen(res+1));
      if (abce_tree_set_str(&amyplanyy.abce, mbt, mbkey, mbval) != 0)
      {
        fprintf(stderr, "Out of memory\n");
        exit(1);
      }
      abce_cpop(&amyplanyy.abce);
      abce_cpop(&amyplanyy.abce);
    }
  }
  idx = abce_cache_add(&amyplanyy.abce, mbt);
  abce_cpop(&amyplanyy.abce);
  abce_add_ins_alt(tmpbuf, &tmpsiz, sizeof(tmpbuf), ABCE_OPCODE_PUSH_DBL);
  abce_add_double_alt(tmpbuf, &tmpsiz, sizeof(tmpbuf), idx);
  abce_add_ins_alt(tmpbuf, &tmpsiz, sizeof(tmpbuf), ABCE_OPCODE_PUSH_FROM_CACHE);

  abce_add_ins_alt(tmpbuf, &tmpsiz, sizeof(tmpbuf), ABCE_OPCODE_PUSH_DBL);
  abce_add_double_alt(tmpbuf, &tmpsiz, sizeof(tmpbuf), 2); // fun arg cnt
  abce_add_ins_alt(tmpbuf, &tmpsiz, sizeof(tmpbuf), ABCE_OPCODE_CALL);
  abce_add_ins_alt(tmpbuf, &tmpsiz, sizeof(tmpbuf), ABCE_OPCODE_EXIT);
  if (abce_engine(&amyplanyy.abce, tmpbuf, tmpsiz) != 0)
  {
    return 255;
  }
  if (amyplanyy.abce.sp < 1)
  {
    return 254;
  }
  if (amyplanyy.abce.stackbase[amyplanyy.abce.sp-1].typ != ABCE_T_D)
  {
    return 253;
  }
  ret = (int)amyplanyy.abce.stackbase[amyplanyy.abce.sp-1].u.d;
  if (amyplanyy.abce.stackbase[amyplanyy.abce.sp-1].u.d != (double)ret)
  {
    return 252;
  }

  amyplanyy_free(&amyplanyy);

  return ret;
}
