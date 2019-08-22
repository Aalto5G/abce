#include "abce.h"
#include "abcetrees.h"

void *do_mmap_madvise(size_t bytes)
{
  void *ptr;
  bytes = abce_topages(bytes);
  // Ugh. I wish all systems had simple and compatible interface.
#ifdef MAP_ANON
  ptr = mmap(NULL, bytes, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANON, -1, 0);
#else
  #ifdef MAP_ANONYMOUS
    #ifdef MAP_NORESERVE
  ptr = mmap(NULL, bytes, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS|MAP_NORESERVE, -1, 0);
    #else
  ptr = mmap(NULL, bytes, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    #endif
  #else
  {
    int fd;
    fd = open("/dev/zero", O_RDWR);
    if (fd < 0)
    {
      abort();
    }
    ptr = mmap(NULL, bytes, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS|MAP_NORESERVE, fd, 0);
    close(fd);
  }
  #endif
#endif
#ifdef MADV_DONTNEED
  #ifdef __linux__
  if (ptr && ptr != MAP_FAILED)
  {
    madvise(ptr, bytes, MADV_DONTNEED); // Linux-ism
  }
  #endif
#endif
  if (ptr == MAP_FAILED)
  {
    return NULL;
  }
  return ptr;
}

void do_mmap_compact(void *ptr, size_t bytes_in_use, size_t bytes_total)
{
  char *ptr2;
  int errno_save;
  bytes_total = abce_topages(bytes_total);
  bytes_in_use = abce_topages(bytes_in_use);
  ptr2 = ptr;
  ptr2 += bytes_in_use;
  errno_save = errno;
  munmap(ptr2, bytes_total - bytes_in_use);
  errno = errno_save;
  // don't report errors
}

struct abce_mb *abce_alloc_stack(size_t limit)
{
  return do_mmap_madvise(abce_topages(limit * sizeof(struct abce_mb)));
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
  return do_mmap_madvise(abce_topages(limit));
}

void abce_free_bcode(unsigned char *bcodebase, size_t limit)
{
  if (munmap(bcodebase, abce_topages(limit)) != 0)
  {
    abort();
  }
}

void abce_compact(struct abce *abce)
{
  do_mmap_compact(abce->stackbase,
                  abce->sp * sizeof(struct abce_mb),
                  abce->stacklimit * sizeof(struct abce_mb));
  do_mmap_compact(abce->gcblockbase,
                  abce->gcblocksz * sizeof(struct abce_mb),
                  abce->gcblockcap * sizeof(struct abce_mb));
  do_mmap_compact(abce->btbase,
                  abce->btsz * sizeof(struct abce_mb),
                  abce->btcap * sizeof(struct abce_mb));
  do_mmap_compact(abce->bytecode,
                  abce->bytecodesz,
                  abce->bytecodecap);
  do_mmap_compact(abce->cachebase,
                  abce->cachesz * sizeof(struct abce_mb),
                  abce->cachecap * sizeof(struct abce_mb));
}

void abce_init(struct abce *abce)
{
  memset(abce, 0, sizeof(*abce));
  abce->in_engine = 0;
  abce->do_check_heap_on_gc = 0; // Warning: setting this to 1 WILL crash!
  abce->lastbytes_alloced = 0;
  abce->lastgcblocksz = 0;
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
  abce->gcblockcap = 1024*1024;
  abce->gcblocksz = 0;
  abce->scratchstart = abce->gcblockcap;
  abce->gcblockbase = abce_alloc_stack(abce->gcblockcap);
  abce->btcap = 1024*1024;
  abce->btsz = 0;
  abce->btbase = abce_alloc_stack(abce->btcap);
  abce->sp = 0;
  abce->bp = 0;
  abce->ip = 0;
  abce->bytecodecap = 32*1024*1024;
  abce->bytecode = abce_alloc_bcode(abce->bytecodecap);
  abce->bytecodesz = 0;
  abce->oneblock.typ = ABCE_T_N;

  abce->cachecap = 1024*1024;
  abce->cachebase = abce_alloc_stack(abce->cachecap);
  abce->cachesz = 0;

  abce->dynscope = abce_mb_create_scope_noparent(abce, ABCE_DEFAULT_SCOPE_SIZE);
  if (abce->dynscope.typ == ABCE_T_N)
  {
    abort();
  }
}

void abce_free_gcblock_one(struct abce *abce, size_t locidx)
{
  if (locidx >= abce->gcblocksz)
  {
    abort();
  }
  abce->gcblockbase[locidx] = abce->gcblockbase[--abce->gcblocksz];
  abce->gcblockbase[locidx].u.area->locidx = locidx;
}

struct abce_gcqe {
  enum abce_type typ;
  struct abce_mb_area *mba;
};

void abce_enqueue_stackentry_mb(const struct abce_mb *mb,
                                struct abce_gcqe *stackbase, size_t *stackidx, size_t stackcap);

void abce_mark_tree(struct abce *abce, struct abce_rb_tree_node *n,
                    struct abce_gcqe *stackbase, size_t *stackidx, size_t stackcap)
{
  struct abce_mb_rb_entry *mbe;
  if (n == NULL)
  {
    return;
  }
  mbe = ABCE_CONTAINER_OF(n, struct abce_mb_rb_entry, n);
  abce_enqueue_stackentry_mb(&mbe->key, stackbase, stackidx, stackcap);
  abce_enqueue_stackentry_mb(&mbe->val, stackbase, stackidx, stackcap);
  if (n->left)
  {
    abce_mark_tree(abce, n->left, stackbase, stackidx, stackcap);
  }
  if (n->right)
  {
    abce_mark_tree(abce, n->right, stackbase, stackidx, stackcap);
  }
}

void abce_enqueue_stackentry(enum abce_type typ, struct abce_mb_area *mba,
                             struct abce_gcqe *stackbase, size_t *stackidx, size_t stackcap)
{
  if (mba->locidx == (size_t)-1)
  {
    return;
  }
  if (*stackidx >= stackcap)
  {
    abort();
  }
  mba->locidx = (size_t)-1;
  switch (typ)
  {
    case ABCE_T_T:
    case ABCE_T_A:
    case ABCE_T_SC:
      stackbase[*stackidx].typ = typ;
      stackbase[*stackidx].mba = mba;
      (*stackidx)++;
      break;
    default:
      break;
  }
}

void abce_enqueue_stackentry_mb(const struct abce_mb *mb,
                                struct abce_gcqe *stackbase, size_t *stackidx, size_t stackcap)
{
  switch (mb->typ)
  {
    case ABCE_T_T:
    case ABCE_T_A:
    case ABCE_T_SC:
    case ABCE_T_IOS:
    case ABCE_T_S:
    case ABCE_T_PB:
      abce_enqueue_stackentry(mb->typ, mb->u.area, stackbase, stackidx, stackcap);
      break;
    default:
      break;
  }
}

void abce_mark(struct abce *abce, struct abce_mb_area *mba, enum abce_type typ,
               struct abce_gcqe *stackbase, size_t *stackidx, size_t stackcap)
{
  size_t i;
  if (mba->locidx == (size_t)-1)
  {
    return;
  }
  abce_enqueue_stackentry(typ, mba, stackbase, stackidx, stackcap);
  while ((*stackidx) > 0)
  {
    typ = stackbase[(*stackidx) - 1].typ;
    mba = stackbase[(*stackidx) - 1].mba;
    (*stackidx)--;
    switch (typ)
    {
      case ABCE_T_T:
        abce_mark_tree(abce, mba->u.tree.tree.root, stackbase, stackidx, stackcap);
        break;
      case ABCE_T_A:
        for (i = 0; i < mba->u.ar.size; i++)
        {
          abce_enqueue_stackentry_mb(&mba->u.ar.mbs[i], stackbase, stackidx, stackcap);
        }
        break;
      case ABCE_T_SC:
        if (mba->u.sc.parent)
        {
          abce_enqueue_stackentry(ABCE_T_SC, mba->u.sc.parent, stackbase, stackidx, stackcap);
        }
        for (i = 0; i < mba->u.sc.size; i++)
        {
          if (abce_unlikely(mba->u.sc.heads[i].root != NULL))
          {
            abce_mark_tree(abce, mba->u.sc.heads[i].root, stackbase, stackidx, stackcap);
          }
        }
        break;
      case ABCE_T_IOS:
      case ABCE_T_S:
      case ABCE_T_PB:
        break;
      default:
        abort();
    }
  }
}

void abce_mark_mb(struct abce *abce, const struct abce_mb *mb,
                  struct abce_gcqe *stackbase, size_t *stackidx, size_t stackcap)
{
  switch (mb->typ)
  {
    case ABCE_T_T:
    case ABCE_T_A:
    case ABCE_T_SC:
    case ABCE_T_IOS:
    case ABCE_T_S:
    case ABCE_T_PB:
      abce_mark(abce, mb->u.area, mb->typ, stackbase, stackidx, stackcap);
    default:
      break;
  }
}

size_t *abce_alloc_refcs(size_t limit)
{
  return do_mmap_madvise(abce_topages(limit * sizeof(size_t)));
}

void abce_free_refcs(size_t *stackbase, size_t limit)
{
  if (munmap(stackbase, abce_topages(limit * sizeof(size_t))) != 0)
  {
    abort();
  }
}

struct abce_gcqe *abce_alloc_gcqueue(size_t limit)
{
  return do_mmap_madvise(abce_topages(limit * sizeof(struct abce_gcqe)));
}

void abce_free_gcqueue(struct abce_gcqe *stackbase, size_t limit)
{
  if (munmap(stackbase, abce_topages(limit * sizeof(struct abce_gcqe))) != 0)
  {
    abort();
  }
}

void abce_check_heap_object(struct abce *abce, size_t *stackbase, struct abce_mb *mb)
{
  if (!abce_is_dynamic_type(mb->typ))
  {
    return;
  }
  stackbase[mb->u.area->locidx]++;
}

void abce_deep_check_heap_object(struct abce *abce, size_t *stackbase, struct abce_mb *mb)
{
  struct abce_mb obj;
  const struct abce_mb *key, *val;
  const struct abce_mb nil = {.typ = ABCE_T_N};
  size_t i;

  if (!abce_is_dynamic_type(mb->typ))
  {
    abort();
  }

  switch (mb->typ)
  {
    case ABCE_T_T:
      key = &nil;
      while (abce_tree_get_next(abce, &key, &val, &obj, key) == 0)
      {
        if (!abce_is_dynamic_type(key->typ))
        {
          abort();
        }
        stackbase[key->u.area->locidx]++;
        if (abce_is_dynamic_type(val->typ))
        {
          stackbase[val->u.area->locidx]++;
        }
      }
      break;
    case ABCE_T_A:
      for (i = 0; i < mb->u.area->u.ar.size; i++)
      {
        if (abce_is_dynamic_type(mb->u.area->u.ar.mbs[i].typ))
        {
          stackbase[mb->u.area->u.ar.mbs[i].u.area->locidx]++;
        }
      }
      break;

    case ABCE_T_SC:
      if (mb->u.area->u.sc.parent)
      {
        stackbase[mb->u.area->u.sc.parent->locidx]++;
      }
      for (i = 0; i < mb->u.area->u.sc.size; i++)
      {
        key = &nil;
        while (abce_rbtree_get_next(&key, &val, &mb->u.area->u.sc.heads[i], key) == 0)
        {
          if (!abce_is_dynamic_type(key->typ))
          {
            abort();
          }
          stackbase[key->u.area->locidx]++;
          if (abce_is_dynamic_type(val->typ))
          {
            stackbase[val->u.area->locidx]++;
          }
        }
      }
      break;

    case ABCE_T_S:
    case ABCE_T_IOS:
    case ABCE_T_PB:
      break;
    default:
      abort();
  }
}

void abce_check_heap(struct abce *abce)
{
  const size_t safety_margin = 100;
  size_t *stackbase;
  size_t stackcap;
  size_t i;

  stackcap = abce->gcblocksz + safety_margin;
  stackbase = abce_alloc_refcs(stackcap);

  if (abce->oneblock.typ != ABCE_T_N)
  {
    abce_check_heap_object(abce, stackbase, &abce->oneblock);
  }
  abce_check_heap_object(abce, stackbase, &abce->dynscope);
  abce_check_heap_object(abce, stackbase, &abce->err.mb);
  for (i = 0; i < abce->sp; i++)
  {
    abce_check_heap_object(abce, stackbase, &abce->stackbase[i]);
  }
  for (i = 0; i < abce->cachesz; i++)
  {
    abce_check_heap_object(abce, stackbase, &abce->cachebase[i]);
  }
  for (i = 0; i < abce->btsz; i++)
  {
    abce_check_heap_object(abce, stackbase, &abce->btbase[i]);
  }

  i = 0;
  while (i < abce->gcblocksz)
  {
    if (abce->gcblockbase[i].u.area->locidx == (size_t)-1)
    {
      abort();
    }
    abce_deep_check_heap_object(abce, stackbase, &abce->gcblockbase[i]);
    i++;
  }
  i = 0;
  while (i < abce->gcblocksz)
  {
    if (abce->gcblockbase[i].u.area->locidx == (size_t)-1)
    {
      abort();
    }
    if (abce->gcblockbase[i].u.area->refcnt !=
        stackbase[abce->gcblockbase[i].u.area->locidx])
    {
      printf("Explained refcnt %zu, actual %zu\n",
             (size_t)stackbase[abce->gcblockbase[i].u.area->locidx],
             (size_t)abce->gcblockbase[i].u.area->refcnt);
      printf("type: %d\n", abce->gcblockbase[i].typ);
      abort();
    }
    i++;
  }

  abce_free_refcs(stackbase, stackcap);
  stackbase = NULL;
  stackcap = 0;
}

void abce_gc(struct abce *abce)
{
  size_t i;
  const size_t safety_margin = 100;
  struct abce_gcqe *stackbase;
  size_t stackcap;
  size_t stackidx = 0;
  if (!abce->in_engine)
  {
    return;
  }
  if (abce->do_check_heap_on_gc)
  {
    abce_check_heap(abce);
  }
  stackcap = abce->gcblocksz + safety_margin;
  stackbase = abce_alloc_gcqueue(stackcap);
  if (abce->oneblock.typ != ABCE_T_N)
  {
    abce_mark_mb(abce, &abce->oneblock, stackbase, &stackidx, stackcap);
  }
  abce_mark_mb(abce, &abce->dynscope, stackbase, &stackidx, stackcap);
  abce_mark_mb(abce, &abce->err.mb, stackbase, &stackidx, stackcap);
  for (i = 0; i < abce->sp; i++)
  {
    abce_mark_mb(abce, &abce->stackbase[i], stackbase, &stackidx, stackcap);
  }
  for (i = 0; i < abce->cachesz; i++)
  {
    abce_mark_mb(abce, &abce->cachebase[i], stackbase, &stackidx, stackcap);
  }
  for (i = 0; i < abce->btsz; i++)
  {
    abce_mark_mb(abce, &abce->btbase[i], stackbase, &stackidx, stackcap);
  }
  abce_free_gcqueue(stackbase, stackcap);
  stackbase = NULL;
  stackcap = 0;

  // Set all references to 0 for unmarked objects and what they may refer
  i = 0;
  while (i < abce->gcblocksz)
  {
    if (abce->gcblockbase[i].u.area->locidx != (size_t)-1)
    {
      abce_mb_gc_refdn(abce, abce->gcblockbase[i].u.area, abce->gcblockbase[i].typ);
    }
    i++;
  }

  i = 0;
  while (i < abce->gcblocksz)
  {
    if (abce->gcblockbase[i].u.area->locidx != (size_t)-1)
    {
      enum abce_type typ = abce->gcblockbase[i].typ;
      struct abce_mb_area *mba = abce->gcblockbase[i].u.area;
      // All unmarked objects must be freed
      if (mba->refcnt != 0)
      {
        printf("refcnt is not 0: %p: %d\n", mba, (int)mba->refcnt);
        printf("type: %d\n", (int)abce->gcblockbase[i].typ);
        abort();
      }
      if (mba->locidx != i)
      {
        printf("locidx invalid %d %d\n", (int)mba->locidx, (int)i);
        abort();
      }
      abce->gcblockbase[i] = abce->gcblockbase[--abce->gcblocksz];
      if (abce->gcblockbase[i].u.area->locidx != (size_t)-1)
      {
        abce->gcblockbase[i].u.area->locidx = i;
      }
      switch (typ)
      {
        case ABCE_T_T:
          // Ok, this might be better (faster)
          while (mba->u.tree.tree.root != NULL)
          {
            struct abce_mb_rb_entry *mbe =
              ABCE_CONTAINER_OF(mba->u.tree.tree.root,
                           struct abce_mb_rb_entry, n);
            abce_rb_tree_nocmp_delete(&mba->u.tree.tree,
                                 mba->u.tree.tree.root);
            abce->alloc(mbe, sizeof(*mbe), 0, &abce->alloc_baton);
          }
          abce->alloc(mba, sizeof(*mba), 0, &abce->alloc_baton);
          break;
        case ABCE_T_IOS:
          fclose(mba->u.ios.f);
          abce->alloc(mba, sizeof(*mba), 0, &abce->alloc_baton);
          break;
        case ABCE_T_A:
          abce->alloc(mba->u.ar.mbs, mba->u.ar.capacity*sizeof(*mba->u.ar.mbs), 0, &abce->alloc_baton);
          abce->alloc(mba, sizeof(*mba), 0, &abce->alloc_baton);
          break;
        case ABCE_T_S:
          abce->alloc(mba, sizeof(*mba) + mba->u.str.size + 1, 0, &abce->alloc_baton);
          break;
        case ABCE_T_PB:
          abce->alloc(mba->u.pb.buf, mba->u.pb.capacity, 0, &abce->alloc_baton);
          abce->alloc(mba, sizeof(*mba), 0, &abce->alloc_baton);
          break;
        case ABCE_T_SC:
          for (i = 0; i < mba->u.sc.size; i++)
          {
            // Ok, this might be better (faster)
            while (mba->u.sc.heads[i].root != NULL)
            {
              struct abce_mb_rb_entry *mbe =
                ABCE_CONTAINER_OF(mba->u.sc.heads[i].root,
                             struct abce_mb_rb_entry, n);
              abce_rb_tree_nocmp_delete(&mba->u.sc.heads[i],
                                   mba->u.sc.heads[i].root);
              abce->alloc(mbe, sizeof(*mbe), 0, &abce->alloc_baton);
            }
          }
          abce->alloc(mba, sizeof(*mba) + mba->u.sc.size * sizeof(*mba->u.sc.heads), 0, &abce->alloc_baton);
          break;
        default:
          abort();
      }
      continue;
    }
    i++;
  }

  // Unmark
  for (i = 0; i < abce->gcblocksz; i++)
  {
    abce->gcblockbase[i].u.area->locidx = i;
  }
}

void abce_free_bt(struct abce *abce)
{
  size_t i;
  for (i = 0; i < abce->btsz; i++)
  {
    abce_mb_refdn(abce, &abce->btbase[i]);
  }
  abce->btsz = 0;
}

void abce_free(struct abce *abce)
{
  size_t i;
  if (abce->oneblock.typ != ABCE_T_N)
  {
    abort();
  }
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
  abce_free_bt(abce);
  for (i = 0; i < abce->cachesz; i++)
  {
    abce_mb_refdn(abce, &abce->cachebase[i]);
  }
  abce->cachesz = 0;
  for (i = 0; i < abce->sp; i++)
  {
    abce_mb_refdn(abce, &abce->stackbase[i]);
  }
  abce->sp = 0;

  abce_err_free(abce, &abce->err);

  abce->in_engine = 1; // to make GC work
  abce_gc(abce);

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
  abce_free_stack(abce->gcblockbase, abce->gcblockcap);
  abce->gcblockbase = NULL;
  abce->gcblockcap = 0;
}

struct abce_mb abce_mb_concat_string(struct abce *abce, const char *str1, size_t sz1,
                                     const char *str2, size_t sz2)
{
  struct abce_mb_area *mba;
  struct abce_mb mb = {};
  mba = (struct abce_mb_area*)abce->alloc(NULL, 0, sizeof(*mba) + sz1 + sz2 + 1, &abce->alloc_baton);
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
  abce_setup_mb_for_gc(abce, mba, ABCE_T_S);
  return mb;
}

struct abce_mb abce_mb_rep_string(struct abce *abce, const char *str, size_t sz, size_t rep)
{
  struct abce_mb_area *mba;
  struct abce_mb mb = {};
  size_t i;
  mba = (struct abce_mb_area*)abce->alloc(NULL, 0, sizeof(*mba) + sz*rep + 1, &abce->alloc_baton);
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
  abce_setup_mb_for_gc(abce, mba, ABCE_T_S);
  return mb;
}

struct abce_mb abce_mb_create_string(struct abce *abce, const char *str, size_t sz)
{
  struct abce_mb_area *mba;
  struct abce_mb mb = {};
  mba = (struct abce_mb_area*)abce->alloc(NULL, 0, sizeof(*mba) + sz + 1, &abce->alloc_baton);
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
  abce_setup_mb_for_gc(abce, mba, ABCE_T_S);
  return mb;
}

struct abce_mb abce_mb_create_pb(struct abce *abce)
{
  struct abce_mb_area *mba;
  struct abce_mb mb = {};
  mba = (struct abce_mb_area*)abce->alloc(NULL, 0, sizeof(*mba), &abce->alloc_baton);
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
    (char*)abce->alloc(NULL, 0, 128, &abce->alloc_baton);
  if (mba->u.pb.buf == NULL)
  {
    mba->u.pb.capacity = 0; // This is the simplest way forward.
  }
  mba->refcnt = 1;
  mb.typ = ABCE_T_PB;
  mb.u.area = mba;
  abce_setup_mb_for_gc(abce, mba, ABCE_T_PB);
  return mb;
}

struct abce_mb abce_mb_create_pb_from_buf(struct abce *abce, const void *buf, size_t sz)
{
  struct abce_mb_area *mba;
  struct abce_mb mb = {};
  mba = (struct abce_mb_area*)abce->alloc(NULL, 0, sizeof(*mba), &abce->alloc_baton);
  if (mba == NULL)
  {
    abce->err.code = ABCE_E_NO_MEM;
    abce->err.val2 = sizeof(*mba);
    mb.typ = ABCE_T_N;
    return mb;
  }
  mba->u.pb.size = sz;
  mba->u.pb.capacity = sz;
  mba->u.pb.buf =
    (char*)abce->alloc(NULL, 0, sz, &abce->alloc_baton);
  if (mba->u.pb.buf == NULL)
  {
    abce->err.code = ABCE_E_NO_MEM;
    abce->err.val2 = sz;
    abce->alloc(mba, sizeof(*mba), 0, &abce->alloc_baton);
    mb.typ = ABCE_T_N;
    return mb;
  }
  memcpy(mba->u.pb.buf, buf, sz);
  mba->refcnt = 1;
  mb.typ = ABCE_T_PB;
  mb.u.area = mba;
  abce_setup_mb_for_gc(abce, mba, ABCE_T_PB);
  return mb;
}

struct abce_mb abce_mb_create_tree(struct abce *abce)
{
  struct abce_mb_area *mba;
  struct abce_mb mb = {};
  mba = (struct abce_mb_area*)abce->alloc(NULL, 0, sizeof(*mba), &abce->alloc_baton);
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
  abce_setup_mb_for_gc(abce, mba, ABCE_T_T);
  return mb;
}

struct abce_mb abce_mb_create_array(struct abce *abce)
{
  struct abce_mb_area *mba;
  struct abce_mb mb = {};
  mba = (struct abce_mb_area*)abce->alloc(NULL, 0, sizeof(*mba), &abce->alloc_baton);
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
    (struct abce_mb*)abce->alloc(NULL, 0, 16*sizeof(*mba->u.ar.mbs), &abce->alloc_baton);
  if (mba->u.ar.mbs == NULL)
  {
    mba->u.ar.capacity = 0; // This is the simplest way forward.
  }
  mba->refcnt = 1;
  mb.typ = ABCE_T_A;
  mb.u.area = mba;
  abce_setup_mb_for_gc(abce, mba, ABCE_T_A);
  return mb;
}

int abce_mb_array_append_grow(struct abce *abce, struct abce_mb *mb)
{
  size_t new_cap = 2*mb->u.area->u.ar.size + 1;
  struct abce_mb *mbs2;
  mbs2 = (struct abce_mb*)abce->alloc(mb->u.area->u.ar.mbs,
                     sizeof(*mb->u.area->u.ar.mbs)*mb->u.area->u.ar.capacity,
                     sizeof(*mb->u.area->u.ar.mbs)*new_cap,
                     &abce->alloc_baton);
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
  new_buf = (char*)abce->alloc(mbpb->u.area->u.pb.buf, mbpb->u.area->u.pb.capacity, new_capacity, &abce->alloc_baton);
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

void abce_maybe_mv_obj_to_scratch_tail(struct abce *abce, const struct abce_mb *obj)
{
  struct abce_mb_area *mba;
  mba = obj->u.area;
  switch (obj->typ)
  {
    case ABCE_T_IOS:
      if (1)
      {
        fclose(mba->u.ios.f);
        abce->alloc(mba, sizeof(*mba), 0, &abce->alloc_baton);
      }
      return;
    case ABCE_T_S:
      if (1)
      {
        abce->alloc(mba, sizeof(*mba) + mba->u.str.size + 1, 0, &abce->alloc_baton);
      }
      return;
    case ABCE_T_PB:
      if (1)
      {
        abce->alloc(mba->u.pb.buf, mba->u.pb.capacity, 0, &abce->alloc_baton);
        abce->alloc(mba, sizeof(*mba), 0, &abce->alloc_baton);
      }
      return;
    case ABCE_T_T:
    case ABCE_T_A:
    case ABCE_T_SC:
    default:
      abort();
  }
}
