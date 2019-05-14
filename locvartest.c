#include "locvarctx.h"

int main(int argc, char **argv)
{
  struct abce_locvarctx *ctx0, *ctx1, *ctx2;
  ctx0 = abce_locvarctx_alloc(NULL, 3+2, -1, -1);
  abce_locvarctx_add(ctx0, "ctx0");
  ctx1 = abce_locvarctx_alloc(ctx0, 0, 2, 3);
  abce_locvarctx_add(ctx1, "ctx1");
  ctx2 = abce_locvarctx_alloc(ctx1, 0, -1, -1);
  abce_locvarctx_add(ctx2, "ctx2");
  printf("%d\n", (int)abce_locvarctx_search_rec(ctx2, "ctx0"));
  printf("%d\n", (int)abce_locvarctx_search_rec(ctx2, "ctx1"));
  printf("%d\n", (int)abce_locvarctx_search_rec(ctx2, "ctx2"));
  printf("break %d\n", (int)abce_locvarctx_break(ctx2, 1));
  printf("break %d\n", (int)abce_locvarctx_break(ctx2, 2));
  printf("sz %d\n", (int)abce_locvarctx_sz(ctx2));
  printf("recursive sz %d\n", (int)abce_locvarctx_recursive_sz(ctx2));
  abce_locvarctx_free(ctx2);
  abce_locvarctx_free(ctx1);
  abce_locvarctx_free(ctx0);
  return 0;
}
