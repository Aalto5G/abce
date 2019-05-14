#include "abce.h"

struct abce_mb *abce_alloc_stack(size_t limit)
{
  return mmap(NULL, abce_topages(limit * sizeof(struct abce_mb)), PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
}

void abce_free_stack(struct abce_mb *stackbase, size_t limit)
{
  if (munmap(stackbase, abce_topages(limit * sizeof(struct abce_mb))) != 0)
  {
    abort();
  }
}

unsigned char *abce_alloc_bcode(size_t limit)
{
  return mmap(NULL, abce_topages(limit), PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
}

void abce_free_bcode(unsigned char *bcodebase, size_t limit)
{
  if (munmap(bcodebase, abce_topages(limit)) != 0)
  {
    abort();
  }
}

void abce_init(struct abce *abce)
{
  memset(abce, 0, sizeof(*abce));
  abce->alloc = abce_std_alloc;
  abce->trap = NULL;
  abce->alloc_baton = NULL;
  abce->userdata = NULL;
  abce->stacklimit = 1024*1024;
  abce->stackbase = abce_alloc_stack(abce->stacklimit);
  abce->sp = 0;
  abce->bp = 0;
  abce->ip = 0;
  abce->bytecodecap = 32*1024*1024;
  abce->bytecode = abce_alloc_bcode(abce->bytecodecap);
  abce->bytecodesz = 0;

  abce->cachecap = 1024*1024;
  abce->cachebase = abce_alloc_stack(abce->cachecap);
  abce->cachesz = 0;

  abce->dynscope = abce_mb_create_scope_noparent(abce, ABCE_DEFAULT_SCOPE_SIZE);
  if (abce->dynscope.typ == ABCE_T_N)
  {
    abort();
  }
}

void abce_free(struct abce *abce)
{
  size_t i;
  abce_mb_refdn(abce, &abce->dynscope);
#if 0
  for (i = 0; i < sizeof(abce->strcache)/sizeof(*abce->strcache); i++)
  {
    while (abce->strcache[i].root != NULL)
    {
#if 0
      struct abce_mb_string *mbe =
        CONTAINER_OF(abce->strcache[i].root,
                     struct abce_mb_string, node);
      struct abce_mb_area *area = CONTAINER_OF(mbe, struct abce_mb_area, u.str);
#endif
      rb_tree_nocmp_delete(&abce->strcache[i],
                           abce->strcache[i].root);
#if 0
      abce_mb_arearefdn(abce, &area, ABCE_T_S);
#endif
    }
  }
#endif
  for (i = 0; i < abce->cachesz; i++)
  {
    abce_mb_refdn(abce, &abce->cachebase[i]);
  }
  for (i = 0; i < abce->sp; i++)
  {
    abce_mb_refdn(abce, &abce->stackbase[i]);
  }
  abce_free_stack(abce->stackbase, abce->stacklimit);
  abce->stackbase = NULL;
  abce->stacklimit = 0;
  abce_free_bcode(abce->bytecode, abce->bytecodecap);
  abce->bytecode = NULL;
  abce->bytecodecap = 0;
  abce_free_stack(abce->cachebase, abce->cachecap);
  abce->cachebase = NULL;
  abce->cachecap = 0;
}

struct abce_mb abce_mb_concat_string(struct abce *abce, const char *str1, size_t sz1,
                                     const char *str2, size_t sz2)
{
  struct abce_mb_area *mba;
  struct abce_mb mb = {};
  mba = (struct abce_mb_area*)abce->alloc(NULL, sizeof(*mba) + sz1 + sz2 + 1, abce->alloc_baton);
  if (mba == NULL)
  {
    mb.typ = ABCE_T_N;
    return mb;
  }
  mba->u.str.size = sz1 + sz2;
  memcpy(mba->u.str.buf, str1, sz1);
  memcpy(mba->u.str.buf + sz1, str2, sz2);
  mba->u.str.buf[sz1 + sz2] = '\0';
  mba->refcnt = 1;
  mb.typ = ABCE_T_S;
  mb.u.area = mba;
  return mb;
}

struct abce_mb abce_mb_rep_string(struct abce *abce, const char *str, size_t sz, size_t rep)
{
  struct abce_mb_area *mba;
  struct abce_mb mb = {};
  size_t i;
  mba = (struct abce_mb_area*)abce->alloc(NULL, sizeof(*mba) + sz*rep + 1, abce->alloc_baton);
  if (mba == NULL)
  {
    mb.typ = ABCE_T_N;
    return mb;
  }
  mba->u.str.size = sz;
  for (i = 0; i < rep; i++)
  {
    memcpy(mba->u.str.buf + i*sz, str, sz);
  }
  mba->u.str.buf[rep*sz] = '\0';
  mba->refcnt = 1;
  mb.typ = ABCE_T_S;
  mb.u.area = mba;
  return mb;
}

struct abce_mb abce_mb_create_string(struct abce *abce, const char *str, size_t sz)
{
  struct abce_mb_area *mba;
  struct abce_mb mb = {};
  mba = (struct abce_mb_area*)abce->alloc(NULL, sizeof(*mba) + sz + 1, abce->alloc_baton);
  if (mba == NULL)
  {
    mb.typ = ABCE_T_N;
    return mb;
  }
  mba->u.str.size = sz;
  memcpy(mba->u.str.buf, str, sz);
  mba->u.str.buf[sz] = '\0';
  mba->refcnt = 1;
  mb.typ = ABCE_T_S;
  mb.u.area = mba;
  return mb;
}

struct abce_mb abce_mb_create_tree(struct abce *abce)
{
  struct abce_mb_area *mba;
  struct abce_mb mb = {};
  mba = (struct abce_mb_area*)abce->alloc(NULL, sizeof(*mba), abce->alloc_baton);
  if (mba == NULL)
  {
    mb.typ = ABCE_T_N;
    return mb;
  }
  rb_tree_nocmp_init(&mba->u.tree.tree);
  mba->u.tree.sz = 0;
  mba->refcnt = 1;
  mb.typ = ABCE_T_T;
  mb.u.area = mba;
  return mb;
}

struct abce_mb abce_mb_create_array(struct abce *abce)
{
  struct abce_mb_area *mba;
  struct abce_mb mb = {};
  mba = (struct abce_mb_area*)abce->alloc(NULL, sizeof(*mba), abce->alloc_baton);
  if (mba == NULL)
  {
    mb.typ = ABCE_T_N;
    return mb;
  }
  mba->u.ar.size = 0;
  mba->u.ar.capacity = 16;
  mba->u.ar.mbs =
    (struct abce_mb*)abce->alloc(NULL, 16*sizeof(*mba->u.ar.mbs), abce->alloc_baton);
  if (mba->u.ar.mbs == NULL)
  {
    mba->u.ar.capacity = 0; // This is the simplest way forward.
  }
  mba->refcnt = 1;
  mb.typ = ABCE_T_A;
  mb.u.area = mba;
  return mb;
}

int abce_mb_array_append_grow(struct abce *abce, struct abce_mb *mb)
{
  size_t new_cap = 2*mb->u.area->u.ar.size + 1;
  struct abce_mb *mbs2;
  mbs2 = (struct abce_mb*)abce->alloc(mb->u.area->u.ar.mbs,
                     sizeof(*mb->u.area->u.ar.mbs)*new_cap,
                     abce->alloc_baton);
  if (mbs2 == NULL)
  {
    return -ENOMEM;
  }
  mb->u.area->u.ar.capacity = new_cap;
  mb->u.area->u.ar.mbs = mbs2;
  return 0;
}
