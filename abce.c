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
  abce->trusted = 1;
  abce->alloc = abce_std_alloc;
  abce->bytes_alloced = 0;
  abce->bytes_cap = SIZE_MAX;
  abce->trap = NULL;
  abce->alloc_baton = NULL;
  abce->ins_budget_fn = NULL;
  abce->ins_budget_baton = NULL;
  abce->userdata = NULL;
  abce->stacklimit = 1024*1024;
  abce->stackbase = abce_alloc_stack(abce->stacklimit);
  abce->btcap = 1024*1024;
  abce->btsz = 0;
  abce->btbase = abce_alloc_stack(abce->btcap);
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
      abce_rb_tree_nocmp_delete(&abce->strcache[i],
                                abce->strcache[i].root);
#if 0
      abce_mb_arearefdn(abce, &area, ABCE_T_S);
#endif
    }
  }
#endif
  for (i = 0; i < abce->btsz; i++)
  {
    abce_mb_refdn(abce, &abce->btbase[i]);
  }
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
  abce_free_stack(abce->btbase, abce->btcap);
  abce->btbase = NULL;
  abce->btcap = 0;
  abce_err_free(abce, &abce->err);
}

struct abce_mb abce_mb_concat_string(struct abce *abce, const char *str1, size_t sz1,
                                     const char *str2, size_t sz2)
{
  struct abce_mb_area *mba;
  struct abce_mb mb = {};
  mba = (struct abce_mb_area*)abce->alloc(NULL, 0, sizeof(*mba) + sz1 + sz2 + 1, abce);
  if (mba == NULL)
  {
    abce->err.code = ABCE_E_NO_MEM;
    abce->err.val2 = sizeof(*mba) + sz1 + sz2 + 1;
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
  mba = (struct abce_mb_area*)abce->alloc(NULL, 0, sizeof(*mba) + sz*rep + 1, abce);
  if (mba == NULL)
  {
    abce->err.code = ABCE_E_NO_MEM;
    abce->err.val2 = sizeof(*mba) + sz*rep + 1;
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
  mba = (struct abce_mb_area*)abce->alloc(NULL, 0, sizeof(*mba) + sz + 1, abce);
  if (mba == NULL)
  {
    abce->err.code = ABCE_E_NO_MEM;
    abce->err.mb.typ = ABCE_T_D;
    abce->err.mb.u.d = sz;
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

struct abce_mb abce_mb_create_pb(struct abce *abce)
{
  struct abce_mb_area *mba;
  struct abce_mb mb = {};
  mba = (struct abce_mb_area*)abce->alloc(NULL, 0, sizeof(*mba), abce);
  if (mba == NULL)
  {
    abce->err.code = ABCE_E_NO_MEM;
    abce->err.val2 = sizeof(*mba);
    mb.typ = ABCE_T_N;
    return mb;
  }
  mba->u.pb.size = 0;
  mba->u.pb.capacity = 128;
  mba->u.pb.buf =
    (char*)abce->alloc(NULL, 0, 128, abce);
  if (mba->u.pb.buf == NULL)
  {
    mba->u.pb.capacity = 0; // This is the simplest way forward.
  }
  mba->refcnt = 1;
  mb.typ = ABCE_T_PB;
  mb.u.area = mba;
  return mb;
}

struct abce_mb abce_mb_create_tree(struct abce *abce)
{
  struct abce_mb_area *mba;
  struct abce_mb mb = {};
  mba = (struct abce_mb_area*)abce->alloc(NULL, 0, sizeof(*mba), abce);
  if (mba == NULL)
  {
    abce->err.code = ABCE_E_NO_MEM;
    abce->err.val2 = sizeof(*mba);
    mb.typ = ABCE_T_N;
    return mb;
  }
  abce_rb_tree_nocmp_init(&mba->u.tree.tree);
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
  mba = (struct abce_mb_area*)abce->alloc(NULL, 0, sizeof(*mba), abce);
  if (mba == NULL)
  {
    abce->err.code = ABCE_E_NO_MEM;
    abce->err.val2 = sizeof(*mba);
    mb.typ = ABCE_T_N;
    return mb;
  }
  mba->u.ar.size = 0;
  mba->u.ar.capacity = 16;
  mba->u.ar.mbs =
    (struct abce_mb*)abce->alloc(NULL, 0, 16*sizeof(*mba->u.ar.mbs), abce);
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
                     sizeof(*mb->u.area->u.ar.mbs)*mb->u.area->u.ar.capacity,
                     sizeof(*mb->u.area->u.ar.mbs)*new_cap,
                     abce);
  if (mbs2 == NULL)
  {
    abce->err.code = ABCE_E_NO_MEM;
    abce->err.mb = abce_mb_refup(abce, mb);
    abce->err.val2 = new_cap*sizeof(*mb->u.area->u.ar.mbs);
    return -ENOMEM;
  }
  mb->u.area->u.ar.capacity = new_cap;
  mb->u.area->u.ar.mbs = mbs2;
  return 0;
}

int abce_mb_pb_do_resize(struct abce *abce, const struct abce_mb *mbpb, size_t newsz)
{
  size_t new_capacity;
  char *new_buf;
  if (mbpb->typ != ABCE_T_PB)
  {
    abort();
  }
  if (newsz <= mbpb->u.area->u.pb.capacity)
  {
    abort();
  }
  new_capacity = 2*mbpb->u.area->u.pb.capacity + 1;
  if (new_capacity < newsz)
  {
    new_capacity = newsz;
  }
  new_buf = (char*)abce->alloc(mbpb->u.area->u.pb.buf, mbpb->u.area->u.pb.capacity, new_capacity, abce);
  if (new_buf == NULL)
  {
    abce->err.code = ABCE_E_NO_MEM;
    abce->err.val2 = new_capacity;
    return -ENOMEM;
  }
  mbpb->u.area->u.pb.buf = new_buf;
  mbpb->u.area->u.pb.capacity = new_capacity;
  memset(&mbpb->u.area->u.pb.buf[mbpb->u.area->u.pb.size], 0,
         newsz - mbpb->u.area->u.pb.size);
  mbpb->u.area->u.pb.size = newsz;
  return 0;
}

struct abce_mb
abce_mb_refup_noinline(struct abce *abce, const struct abce_mb *mb)
{
  return abce_mb_refup(abce, mb);
}

void
abce_mb_refdn_noinline(struct abce *abce, struct abce_mb *mb)
{
  abce_mb_refdn(abce, mb);
}

int
abce_fetch_i_tail(uint8_t ophi, uint16_t *ins, struct abce *abce, unsigned char *addcode, size_t addsz)
{
  uint8_t opmid, oplo;
  if (abce_unlikely((ophi & 0xC0) == 0x80))
  {
    //printf("EILSEQ 1\n");
    abce->err.code = ABCE_E_ILLEGAL_INSTRUCTION;
    abce->err.mb.typ = ABCE_T_D;
    abce->err.mb.u.d = (1<<16) | ophi;
    return -EILSEQ;
  }
  else if (abce_likely((ophi & 0xE0) == 0xC0))
  {
    if (abce_fetch_b(&oplo, abce, addcode, addsz) != 0)
    {
      return -EFAULT;
    }
    if (abce_unlikely((oplo & 0xC0) != 0x80))
    {
      //printf("EILSEQ 2\n");
      abce->err.code = ABCE_E_ILLEGAL_INSTRUCTION;
      abce->err.mb.typ = ABCE_T_D;
      abce->err.mb.u.d = (2<<16) | ophi;
      return -EILSEQ;
    }
    *ins = ((ophi&0x1F) << 6) | (oplo & 0x3F);
    if (abce_unlikely(*ins < 128))
    {
      //printf("EILSEQ 3\n");
      abce->err.code = ABCE_E_ILLEGAL_INSTRUCTION;
      abce->err.mb.typ = ABCE_T_D;
      abce->err.mb.u.d = (3<<16) | *ins;
      return -EILSEQ;
    }
    return 0;
  }
  else if (abce_likely((ophi & 0xF0) == 0xE0))
  {
    if (abce_fetch_b(&opmid, abce, addcode, addsz) != 0)
    {
      return -EFAULT;
    }
    if (abce_unlikely((opmid & 0xC0) != 0x80))
    {
      //printf("EILSEQ 4\n");
      abce->err.code = ABCE_E_ILLEGAL_INSTRUCTION;
      abce->err.mb.typ = ABCE_T_D;
      abce->err.mb.u.d = (4<<16) | opmid;
      return -EILSEQ;
    }
    if (abce_fetch_b(&oplo, abce, addcode, addsz) != 0)
    {
      return -EFAULT;
    }
    if (abce_unlikely((oplo & 0xC0) != 0x80))
    {
      //printf("EILSEQ 5\n");
      abce->err.code = ABCE_E_ILLEGAL_INSTRUCTION;
      abce->err.mb.typ = ABCE_T_D;
      abce->err.mb.u.d = (5<<16) | opmid;
      return -EILSEQ;
    }
    *ins = ((ophi&0xF) << 12) | ((opmid&0x3F) << 6) | (oplo & 0x3F);
    if (abce_unlikely(*ins <= 0x7FF))
    {
      //printf("EILSEQ 6\n");
      abce->err.code = ABCE_E_ILLEGAL_INSTRUCTION;
      abce->err.mb.typ = ABCE_T_D;
      abce->err.mb.u.d = (6<<16) | *ins;
      return -EILSEQ;
    }
    return 0;
  }
  else
  {
    //printf("EILSEQ 7\n");
    abce->err.code = ABCE_E_ILLEGAL_INSTRUCTION;
    abce->err.mb.typ = ABCE_T_D;
    abce->err.mb.u.d = (7<<16) | ophi;
    return -EILSEQ;
  }
}
