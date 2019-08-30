#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include "abcerbtree.h"
#include "abcemurmur.h"
#include "abcecontainerof.h"
#include "abcelikely.h"
#include "abceopcodes.h"
#include "abce.h"
#include "abcetrees.h"
#include "abcejmalloc.h"

void *abce_jm_alloc(void *old, size_t oldsz, size_t newsz, void **pbaton)
{
  struct abce *abce = ABCE_CONTAINER_OF(pbaton, struct abce, alloc_baton);
  void *result;
  if (old == NULL)
  {
    if (newsz == 0)
    {
      newsz = 1;
    }
    if (oldsz != 0)
    {
      abort();
    }
    if (newsz > abce->bytes_cap)
    {
      return NULL;
    }
    if ((   (abce->gcblocksz*1.0 > abce->lastgcblocksz*2.0
            && abce->gcblocksz - abce->lastgcblocksz > 16*100*1000)
         || (abce->bytes_alloced*1.0 > abce->lastbytes_alloced*2.0
            && abce->bytes_alloced - abce->lastbytes_alloced > 16*1000*1000)
         || abce->gcblocksz >= abce->gcblockcap
         || abce->bytes_alloced > (abce->bytes_cap - newsz))
        && abce->in_engine)
    {
      //printf("B: gcblocksz %zu\n", abce->gcblocksz);
      //printf("B: lastgcblocksz %zu\n", abce->lastgcblocksz);
      //printf("B: bytes_alloced %zu\n", abce->bytes_alloced);
      //printf("B: lastbytes_alloced %zu\n", abce->lastbytes_alloced);
      abce_gc(abce);
      abce->lastgcblocksz = abce->gcblocksz;
      abce->lastbytes_alloced = abce->bytes_alloced;
      //printf("A: gcblocksz %zu\n", abce->gcblocksz);
      //printf("A: lastgcblocksz %zu\n", abce->lastgcblocksz);
      //printf("A: bytes_alloced %zu\n", abce->bytes_alloced);
      //printf("A: lastbytes_alloced %zu\n", abce->lastbytes_alloced);
    }
    if (abce->bytes_alloced > (abce->bytes_cap - newsz))
    {
      return NULL;
    }
    if (abce->gcblocksz >= abce->gcblockcap)
    {
      return NULL;
    }
    result = abce_jmalloc(newsz);
    if (result)
    {
      abce->bytes_alloced += newsz;
    }
    return result;
  }
  else if (newsz == 0)
  {
    if (oldsz > abce->bytes_alloced)
    {
      abort();
    }
    abce_jmfree(old, oldsz);
    abce->bytes_alloced -= oldsz;
    return NULL;
  }
  else
  {
    if (oldsz > abce->bytes_alloced)
    {
      abort();
    }
    if (newsz > abce->bytes_cap)
    {
      return NULL;
    }
    if ((   (abce->gcblocksz*1.0 > abce->lastgcblocksz*2.0
            && abce->gcblocksz - abce->lastgcblocksz > 16*100*1000)
         || (abce->bytes_alloced*1.0 > abce->lastbytes_alloced*2.0
            && abce->bytes_alloced - abce->lastbytes_alloced > 16*1000*1000)
         || (abce->bytes_alloced - oldsz) > (abce->bytes_cap - newsz))
        && abce->in_engine)
    {
      //printf("B: gcblocksz %zu\n", abce->gcblocksz);
      //printf("B: lastgcblocksz %zu\n", abce->lastgcblocksz);
      //printf("B: bytes_alloced %zu\n", abce->bytes_alloced);
      //printf("B: lastbytes_alloced %zu\n", abce->lastbytes_alloced);
      abce_gc(abce);
      abce->lastgcblocksz = abce->gcblocksz;
      abce->lastbytes_alloced = abce->bytes_alloced;
      //printf("A: gcblocksz %zu\n", abce->gcblocksz);
      //printf("A: lastgcblocksz %zu\n", abce->lastgcblocksz);
      //printf("A: bytes_alloced %zu\n", abce->bytes_alloced);
      //printf("A: lastbytes_alloced %zu\n", abce->lastbytes_alloced);
    }
    if ((abce->bytes_alloced - oldsz) > (abce->bytes_cap - newsz))
    {
      return NULL;
    }
    result = abce_jmrealloc(old, oldsz, newsz);
    if (result)
    {
      abce->bytes_alloced -= oldsz;
      abce->bytes_alloced += newsz;
    }
    return result;
  }
}

void *abce_std_alloc(void *old, size_t oldsz, size_t newsz, void **pbaton)
{
  struct abce *abce = ABCE_CONTAINER_OF(pbaton, struct abce, alloc_baton);
  void *result;
  if (old == NULL)
  {
    if (newsz == 0)
    {
      newsz = 1;
    }
    if (oldsz != 0)
    {
      abort();
    }
    if (newsz > abce->bytes_cap)
    {
      return NULL;
    }
    if ((   (abce->gcblocksz*1.0 > abce->lastgcblocksz*2.0
            && abce->gcblocksz - abce->lastgcblocksz > 16*100*1000)
         || (abce->bytes_alloced*1.0 > abce->lastbytes_alloced*2.0
            && abce->bytes_alloced - abce->lastbytes_alloced > 16*1000*1000)
         || abce->gcblocksz >= abce->gcblockcap
         || abce->bytes_alloced > (abce->bytes_cap - newsz))
        && abce->in_engine)
    {
      //printf("B: gcblocksz %zu\n", abce->gcblocksz);
      //printf("B: lastgcblocksz %zu\n", abce->lastgcblocksz);
      //printf("B: bytes_alloced %zu\n", abce->bytes_alloced);
      //printf("B: lastbytes_alloced %zu\n", abce->lastbytes_alloced);
      abce_gc(abce);
      abce->lastgcblocksz = abce->gcblocksz;
      abce->lastbytes_alloced = abce->bytes_alloced;
      //printf("A: gcblocksz %zu\n", abce->gcblocksz);
      //printf("A: lastgcblocksz %zu\n", abce->lastgcblocksz);
      //printf("A: bytes_alloced %zu\n", abce->bytes_alloced);
      //printf("A: lastbytes_alloced %zu\n", abce->lastbytes_alloced);
    }
    if (abce->bytes_alloced > (abce->bytes_cap - newsz))
    {
      return NULL;
    }
    if (abce->gcblocksz >= abce->gcblockcap)
    {
      return NULL;
    }
    result = malloc(newsz);
    if (result)
    {
      abce->bytes_alloced += newsz;
    }
    return result;
  }
  else if (newsz == 0)
  {
    if (oldsz > abce->bytes_alloced)
    {
      abort();
    }
    free(old);
    abce->bytes_alloced -= oldsz;
    return NULL;
  }
  else
  {
    if (oldsz > abce->bytes_alloced)
    {
      abort();
    }
    if (newsz > abce->bytes_cap)
    {
      return NULL;
    }
    if ((   (abce->gcblocksz*1.0 > abce->lastgcblocksz*2.0
            && abce->gcblocksz - abce->lastgcblocksz > 16*100*1000)
         || (abce->bytes_alloced*1.0 > abce->lastbytes_alloced*2.0
            && abce->bytes_alloced - abce->lastbytes_alloced > 16*1000*1000)
         || (abce->bytes_alloced - oldsz) > (abce->bytes_cap - newsz))
        && abce->in_engine)
    {
      //printf("B: gcblocksz %zu\n", abce->gcblocksz);
      //printf("B: lastgcblocksz %zu\n", abce->lastgcblocksz);
      //printf("B: bytes_alloced %zu\n", abce->bytes_alloced);
      //printf("B: lastbytes_alloced %zu\n", abce->lastbytes_alloced);
      abce_gc(abce);
      abce->lastgcblocksz = abce->gcblocksz;
      abce->lastbytes_alloced = abce->bytes_alloced;
      //printf("A: gcblocksz %zu\n", abce->gcblocksz);
      //printf("A: lastgcblocksz %zu\n", abce->lastgcblocksz);
      //printf("A: bytes_alloced %zu\n", abce->bytes_alloced);
      //printf("A: lastbytes_alloced %zu\n", abce->lastbytes_alloced);
    }
    if ((abce->bytes_alloced - oldsz) > (abce->bytes_cap - newsz))
    {
      return NULL;
    }
    result = realloc(old, newsz);
    if (result)
    {
      abce->bytes_alloced -= oldsz;
      abce->bytes_alloced += newsz;
    }
    return result;
  }
}

void abce_mb_do_arearefdn(struct abce *abce, struct abce_mb_area **mba, enum abce_type typ);

int64_t abce_cache_add_str(struct abce *abce, const char *str, size_t len)
{
  struct abce_mb mb;
  uint32_t hashval, hashloc;
  struct abce_const_str_len key = {.str = str, .len = len};
  struct abce_rb_tree_node *n;

  hashval = abce_str_len_hash(&key);
  hashloc = hashval % (sizeof(abce->strcache)/sizeof(*abce->strcache));
  n = ABCE_RB_TREE_NOCMP_FIND(&abce->strcache[hashloc], abce_str_cache_cmp_asymlen, NULL, &key);
  if (n != NULL)
  {
    return ABCE_CONTAINER_OF(n, struct abce_mb_string, node)->locidx;
  }
  if (abce->cachesz >= abce->cachecap)
  {
    return -EOVERFLOW;
  }
  mb = abce_mb_create_string(abce, str, len);
  if (mb.typ == ABCE_T_N)
  {
    return -ENOMEM;
  }
  mb.u.area->u.str.locidx = abce->cachesz;
  abce->cachebase[abce->cachesz++] = mb;
  if (abce_rb_tree_nocmp_insert_nonexist(&abce->strcache[hashloc], abce_str_cache_cmp_sym, NULL, &mb.u.area->u.str.node) != 0)
  {
    abort();
  }
  return mb.u.area->u.str.locidx;
}

#ifdef WITH_LUA

void mb_to_lua(lua_State *lua, const struct abce_mb *mb);

void tree_to_lua(lua_State *lua, const struct abce_rb_tree_node *n)
{
  struct abce_mb_rb_entry *e = ABCE_CONTAINER_OF(n, struct abce_mb_rb_entry, n);
  if (n == NULL)
  {
    return;
  }
  tree_to_lua(lua, n->left);
  mb_to_lua(lua, &e->key);
  mb_to_lua(lua, &e->val);
  lua_settable(lua, -3);
  tree_to_lua(lua, n->right);
}

void mb_to_lua(lua_State *lua, const struct abce_mb *mb)
{
  size_t i;
  switch (mb->typ)
  {
    case ABCE_T_B:
      lua_pushboolean(lua, !!mb->u.d);
      return;
    case ABCE_T_D:
      lua_pushnumber(lua, mb->u.d);
      return;
    case ABCE_T_S:
      lua_pushlstring(lua, mb->u.area->u.str.buf, mb->u.area->u.str.size);
      return;
    case ABCE_T_A:
      lua_newtable(lua);
      for (i = 0; i < mb->u.area->u.ar.size; i++)
      {
        mb_to_lua(lua, &mb->u.area->u.ar.mbs[i]);
        lua_rawseti(lua, -2, i+1); // this pops item from stack
      }
      return;
    case ABCE_T_T:
      lua_newtable(lua);
      tree_to_lua(lua, mb->u.area->u.tree.tree.root);
      return;
    case ABCE_T_N:
      lua_pushnil(lua);
      return;
    default:
      abort();
      return;
  }
}

void mb_from_lua(lua_State *lua, struct abce *abce, int idx)
{
  size_t i, len;
  int typ = lua_type(lua, idx);
  const char *str;
  struct abce_mb mb;
  switch (typ)
  {
    case LUA_TNIL:
      if (abce_push_nil(abce) != 0)
      {
        abort();
      }
      return;
    case LUA_TBOOLEAN:
      if (abce_push_boolean(abce, lua_toboolean(lua, idx)) != 0)
      {
        abort();
      }
      return;
    case LUA_TNUMBER:
      if (abce_push_double(abce, lua_tonumber(lua, idx)) != 0)
      {
        abort();
      }
      return;
    case LUA_TSTRING:
      str = lua_tolstring(lua, idx, &len);
      mb = abce_mb_create_string(abce, str, len);
      if (mb.typ == ABCE_T_N)
      {
        abort();
      }
      if (abce_push_mb(abce, &mb) != 0)
      {
        abort();
      }
      abce_mb_refdn(abce, &mb);
      return;
    case LUA_TTABLE:
      len = lua_objlen(lua, idx);
      if (len)
      {
        mb = abce_mb_create_array(abce);
        if (mb.typ == ABCE_T_N)
        {
          abort();
        }
        if (abce_push_mb(abce, &mb) != 0)
        {
          abort();
        }
        for (i = 0; i < len; i++)
        {
          struct abce_mb mb2;
          lua_pushnumber(lua, i + 1);
          lua_gettable(lua, (idx>=0)?idx:(idx-1));
          mb_from_lua(lua, abce, -1);
          if (abce_getmb(&mb2, abce, -1) != 0)
          {
            abort();
          }
          if (abce_mb_array_append(abce, &mb, &mb2) != 0)
          {
            abort();
          }
          abce_mb_refdn(abce, &mb2);
          abce_pop(abce);
          lua_pop(lua, 1);
        }
        abce_mb_refdn(abce, &mb);
        return;
      }
      else
      {
        int empty = 1;
        mb = abce_mb_create_tree(abce);
        if (mb.typ == ABCE_T_N)
        {
          abort();
        }
        if (abce_push_mb(abce, &mb) != 0)
        {
          abort();
        }
        lua_pushnil(lua);
        while (lua_next(lua, (idx>=0?idx:(idx-1))) != 0)
        {
          size_t l;
          const char *s;
          struct abce_mb mbkey, mbval;

          // Need to push it temporarily to stack top so that lua_tolstring
          // doesn't modify it in-place.
          lua_pushvalue(lua, -2);
          s = lua_tolstring(lua, -1, &l);
          mbkey = abce_mb_create_string(abce, s, l);
          lua_pop(lua, 1);

          empty = 0;
          if (mbkey.typ == ABCE_T_N)
          {
            abort();
          }
          if (abce_push_mb(abce, &mbkey) != 0)
          {
            abort();
          }
          mb_from_lua(lua, abce, -1);
          if (abce_getmb(&mbval, abce, -1) != 0)
          {
            abort();
          }
          if (abce_tree_set_str(abce, &mb, &mbkey, &mbval) != 0)
          {
            abort();
          }
          abce_mb_refdn(abce, &mbkey);
          abce_pop(abce);
          abce_mb_refdn(abce, &mbval);
          abce_pop(abce);
          lua_pop(lua, 1);
        }
        lua_pop(lua, 1); // FIXME needed?
        abce_mb_refdn(abce, &mb);
        if (empty)
        {
          abce_pop(abce);
          mb = abce_mb_create_array(abce);
          if (mb.typ == ABCE_T_N)
          {
            abort();
          }
          if (abce_push_mb(abce, &mb) != 0)
          {
            abort();
          }
        }
        return;
      }
      return;
    default:
      abort();
      return;
  }
}

int lua_makelexcall(lua_State *lua)
{
  struct abce_mb scop = {.typ = ABCE_T_SC};
  struct abce *abce;
  const struct abce_mb *res;
  unsigned char tmpbuf[64] = {0};
  size_t tmpsiz = 0;
  size_t i;
  if (lua_gettop(lua) == 0)
  {
    abort();
  }
  const char *str = luaL_checkstring(lua, 1);
  int args = lua_gettop(lua) - 1;

  lua_getglobal(lua, "__abcelua_abce");
  abce = lua_touserdata(lua, -1);
  lua_pop(lua, 1);
  lua_getglobal(lua, "__abcelua_scope");
  scop.u.area = lua_touserdata(lua, -1); // FIXME lua may mix pointers => crash!
  lua_pop(lua, 1);

  res = abce_sc_get_rec_str(&scop, str, 1);
  if (res == NULL)
  {
    abort();
  }
  if (abce_push_mb(abce, res) != 0)
  {
    abort();
  }

  for (i = 1; i <= args; i++)
  {
    mb_from_lua(lua, abce, i + 1);
  }

  if (abce_push_double(abce, args) != 0)
  {
    abort();
  }

  abce_add_ins_alt(tmpbuf, &tmpsiz, sizeof(tmpbuf), ABCE_OPCODE_CALL);
  abce_add_ins_alt(tmpbuf, &tmpsiz, sizeof(tmpbuf), ABCE_OPCODE_EXIT);

  if (abce_engine(abce, tmpbuf, tmpsiz) != 0)
  {
    abort();
  }

  struct abce_mb mbres;

  abce_getmb(&mbres, abce, -1);

  mb_to_lua(lua, &mbres);

  abce_mb_refdn(abce, &mbres);

  abce_pop(abce);

  return 1;
}

int lua_getlexval(lua_State *lua)
{
  struct abce_mb scop = {.typ = ABCE_T_SC};
  struct abce *abce;
  const struct abce_mb *res;
  if (lua_gettop(lua) == 0)
  {
    abort();
  }
  const char *str = luaL_checkstring(lua, 1);
  int args = lua_gettop(lua) - 1;
  if (args != 0)
  {
    abort();
  }
  lua_getglobal(lua, "__abcelua_abce");
  abce = lua_touserdata(lua, -1);
  lua_pop(lua, 1);
  lua_getglobal(lua, "__abcelua_scope");
  scop.u.area = lua_touserdata(lua, -1);
  lua_pop(lua, 1);
  res = abce_sc_get_rec_str(&scop, str, 1);
  if (res == NULL)
  {
    abort();
  }
  mb_to_lua(lua, res);
  return 1;
}


int luaopen_abce(lua_State *lua)
{
        static const luaL_Reg abce_lib[] = {
                {"makelexcall", lua_makelexcall},
                {"getlexval", lua_getlexval},
                {NULL, NULL}};

        luaL_newlib(lua, abce_lib);
        return 1;
}

#endif

struct abce_mb abce_mb_create_scope(struct abce *abce, size_t capacity,
                                      const struct abce_mb *parent, int holey)
{
  struct abce_mb_area *mba;
  struct abce_mb mb = {};
  size_t i;
#ifdef WITH_LUA
  lua_State *lua;

  lua = lua_open();
  if (lua == NULL)
  {
    abce->err.code = ABCE_E_NO_MEM;
    abce->err.val2 = 0;
    mb.typ = ABCE_T_N;
    return mb;
  }
  luaL_openlibs(lua);
  lua_pushcfunction(lua, luaopen_abce);
  lua_pushstring(lua, "Abce");
  lua_call(lua, 1, 1);
  lua_pushvalue(lua, -1);
  lua_setglobal(lua, "Abce");
  lua_pop(lua, 1);
#endif

  capacity = abce_next_highest_power_of_2(capacity);

  mba = abce->alloc(NULL, 0, sizeof(*mba) + capacity * sizeof(*mba->u.sc.heads),
                    &abce->alloc_baton);
  if (mba == NULL)
  {
    abce->err.code = ABCE_E_NO_MEM;
    abce->err.val2 = sizeof(*mba) + capacity * sizeof(*mba->u.sc.heads);
    mb.typ = ABCE_T_N;
    return mb;
  }
  mba->u.sc.size = capacity;
  mba->u.sc.holey = holey;
  mba->u.sc.userdata = NULL;
  if (parent)
  {
    if (parent->typ == ABCE_T_N)
    {
      mba->u.sc.parent = NULL;
    }
    else if (parent->typ != ABCE_T_SC)
    {
      abort();
    }
    else
    {
      mba->u.sc.parent = abce_mb_arearefup(abce, parent);
    }
  }
  else
  {
    mba->u.sc.parent = NULL;
  }
  for (i = 0; i < capacity; i++)
  {
    abce_rb_tree_nocmp_init(&mba->u.sc.heads[i]);
  }
  mba->refcnt = 1;
  mb.typ = ABCE_T_SC;
  mb.u.area = mba;

  // Add it to cache
  mb.u.area->u.sc.locidx = abce->cachesz;
  abce->cachebase[abce->cachesz++] = abce_mb_refup(abce, &mb);

  abce_setup_mb_for_gc(abce, mba, ABCE_T_SC);

#ifdef WITH_LUA
  mba->u.sc.lua = lua;
  lua_pushlightuserdata(lua, mba);
  lua_setglobal(lua, "__abcelua_scope");
  lua_pushlightuserdata(lua, abce);
  lua_setglobal(lua, "__abcelua_abce");
#endif

  return mb;
}

void abce_mb_gc_refdn2(struct abce *abce, struct abce_mb_area *mba, enum abce_type typ)
{
  if (!abce_is_dynamic_type(typ))
  {
    abort();
  }
  if (mba->refcnt == 0)
  {
    abort();
  }
  mba->refcnt--;
}

// Down-reference all objects pointed to by this object without freeing anything
void abce_mb_gc_refdn(struct abce *abce, struct abce_mb_area *mba, enum abce_type typ)
{
  struct abce_mb obj = {.u = {.area = mba}, .typ = typ};
  const struct abce_mb *key, *val;
  const struct abce_mb nil = {.typ = ABCE_T_N};
  size_t i;
  if (!abce_is_dynamic_type(typ))
  {
    abort();
  }
  switch (typ)
  {
    case ABCE_T_T:
      key = &nil;
      while (abce_tree_get_next(abce, &key, &val, &obj, key) == 0)
      {
        abce_mb_gc_refdn2(abce, key->u.area, key->typ);
        if (abce_is_dynamic_type(val->typ))
        {
          abce_mb_gc_refdn2(abce, val->u.area, val->typ);
        }
      }
      break;
    case ABCE_T_A:
      for (i = 0; i < mba->u.ar.size; i++)
      {
        if (abce_is_dynamic_type(mba->u.ar.mbs[i].typ))
        {
          abce_mb_gc_refdn2(abce, mba->u.ar.mbs[i].u.area, mba->u.ar.mbs[i].typ);
        }
      }
      break;

    case ABCE_T_SC:
      if (mba->u.sc.parent)
      {
        abce_mb_gc_refdn2(abce, mba->u.sc.parent, ABCE_T_SC);
      }
      for (i = 0; i < mba->u.sc.size; i++)
      {
        key = &nil;
        while (abce_rbtree_get_next(&key, &val, &mba->u.sc.heads[i], key) == 0)
        {
          abce_mb_gc_refdn2(abce, key->u.area, key->typ);
          if (abce_is_dynamic_type(val->typ))
          {
            abce_mb_gc_refdn2(abce, val->u.area, val->typ);
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
  //abce_mb_do_arearefdn(abce, mbap, typ);
}

void abce_mb_gc_free(struct abce *abce, struct abce_mb_area *mba, enum abce_type typ)
{
  size_t i;
  struct abce_mb obj = {.u = {.area = mba}, .typ = typ};

  mba->refcnt = 1;
  abce_maybe_mv_obj_to_scratch(abce, &obj);

  while (abce->scratchstart < abce->gcblockcap)
  {
    obj = abce->gcblockbase[abce->scratchstart++];
    mba = obj.u.area;
    switch (obj.typ)
    {
      case ABCE_T_T:
        if (1)
        {
          // Ok, this might be better (faster)
          while (mba->u.tree.tree.root != NULL)
          {
            struct abce_mb_rb_entry *mbe =
              ABCE_CONTAINER_OF(mba->u.tree.tree.root,
                           struct abce_mb_rb_entry, n);
            abce_maybe_mv_obj_to_scratch(abce, &mbe->key);
            abce_maybe_mv_obj_to_scratch(abce, &mbe->val);
            abce_rb_tree_nocmp_delete(&mba->u.tree.tree,
                                 mba->u.tree.tree.root);
            abce->alloc(mbe, sizeof(*mbe), 0, &abce->alloc_baton);
          }
          abce->alloc(mba, sizeof(*mba), 0, &abce->alloc_baton);
        }
        break;
      case ABCE_T_IOS:
        if (1)
        {
          fclose(mba->u.ios.f);
          abce->alloc(mba, sizeof(*mba), 0, &abce->alloc_baton);
        }
        break;
      case ABCE_T_A:
        if (1)
        {
          for (i = 0; i < mba->u.ar.size; i++)
          {
            abce_maybe_mv_obj_to_scratch(abce, &mba->u.ar.mbs[i]);
          }
          abce->alloc(mba->u.ar.mbs, mba->u.ar.capacity*sizeof(*mba->u.ar.mbs), 0, &abce->alloc_baton);
          abce->alloc(mba, sizeof(*mba), 0, &abce->alloc_baton);
        }
        break;
      case ABCE_T_S:
        if (1)
        {
          abce->alloc(mba, sizeof(*mba) + mba->u.str.size + 1, 0, &abce->alloc_baton);
        }
        break;
      case ABCE_T_PB:
        if (1)
        {
          abce->alloc(mba->u.pb.buf, mba->u.pb.capacity, 0, &abce->alloc_baton);
          abce->alloc(mba, sizeof(*mba), 0, &abce->alloc_baton);
        }
        break;
      case ABCE_T_SC:
        if (1)
        {
#if 1
          struct abce_mb objparent = {.u = {.area = mba->u.sc.parent}, .typ = ABCE_T_SC};
          if (mba->u.sc.parent)
          {
            abce_maybe_mv_obj_to_scratch(abce, &objparent);
          }
#else
          abce_mb_arearefdn(abce, &mba->u.sc.parent, ABCE_T_SC); // FIXME very suspicious!
#endif
          for (i = 0; i < mba->u.sc.size; i++)
          {
            // Ok, this might be better (faster)
            while (mba->u.sc.heads[i].root != NULL)
            {
              struct abce_mb_rb_entry *mbe =
                ABCE_CONTAINER_OF(mba->u.sc.heads[i].root,
                             struct abce_mb_rb_entry, n);
              abce_maybe_mv_obj_to_scratch(abce, &mbe->key);
              abce_maybe_mv_obj_to_scratch(abce, &mbe->val);
              abce_rb_tree_nocmp_delete(&mba->u.sc.heads[i],
                                   mba->u.sc.heads[i].root);
              abce->alloc(mbe, sizeof(*mbe), 0, &abce->alloc_baton);
            }
          }
#ifdef WITH_LUA
          lua_close(mba->u.sc.lua);
#endif
          abce->alloc(mba, sizeof(*mba) + mba->u.sc.size * sizeof(*mba->u.sc.heads), 0, &abce->alloc_baton);
        }
        break;
      default:
        break;
    }
  }
}

void abce_mb_do_arearefdn(struct abce *abce, struct abce_mb_area **mbap, enum abce_type typ)
{
  struct abce_mb_area *mba = *mbap;
  if (mba == NULL)
  {
    return;
  }
  if (mba->refcnt != 0)
  {
    abort();
  }
  //abce_free_gcblock_one(abce, mba->locidx);
  //mba->locidx = (size_t)-1;
  abce_mb_gc_free(abce, mba, typ);
  *mbap = NULL;
}

void abce_mb_treedump(const struct abce_rb_tree_node *n, int *first,
                      struct abce_dump_list *ll)
{
  struct abce_mb_rb_entry *e = ABCE_CONTAINER_OF(n, struct abce_mb_rb_entry, n);
  if (n == NULL)
  {
    return;
  }

  abce_mb_treedump(n->left, first, ll);
  if (*first)
  {
    *first = 0;
  }
  else
  {
    printf(", ");
  }

  abce_mb_dump_impl(&e->key, ll);
  printf(": ");
  abce_mb_dump_impl(&e->val, ll);
  abce_mb_treedump(n->right, first, ll);
}

void abce_dump_str(const char *str, size_t sz)
{
  size_t i;
  printf("\"");
  for (i = 0; i < sz; i++)
  {
    unsigned char uch = (unsigned char)str[i];
    if (uch == '\n')
    {
      printf("\\n");
    }
    else if (uch == '\r')
    {
      printf("\\r");
    }
    else if (uch == '\t')
    {
      printf("\\t");
    }
    else if (uch == '\b')
    {
      printf("\\b");
    }
    else if (uch == '\f')
    {
      printf("\\f");
    }
    else if (uch == '\\')
    {
      printf("\\\\");
    }
    else if (uch == '"')
    {
      printf("\"");
    }
    else if (uch < 0x20)
    {
      printf("\\u%.4X", uch);
    }
    else
    {
      putchar(uch);
    }
  }
  printf("\"");
}

#define DUMP_NEST_LIMIT 100

void abce_mb_dump_impl(const struct abce_mb *mb, struct abce_dump_list *ll)
{
  size_t i;
  int first = 1;
  size_t cnt = 0;
  struct abce_dump_list ll3 = {
    .parent = ll,
  };
  if (abce_is_dynamic_type(mb->typ))
  {
    struct abce_dump_list *ll2 = ll;
    ll3.area = mb->u.area;
    while (ll2)
    {
      if (ll2->area == ll3.area)
      {
        switch (mb->typ)
        {
          case ABCE_T_SC:
            printf("sc(%zu){...}", mb->u.area->u.sc.size);
            return;
          case ABCE_T_T:
            printf("{...}");
            return;
          case ABCE_T_A:
            printf("[...]");
            return;
          default:
            abort();
        }
      }
      ll2 = ll2->parent;
      cnt++;
    }
    if (cnt > DUMP_NEST_LIMIT)
    {
      switch (mb->typ)
      {
        case ABCE_T_SC:
          printf("sc(%zu){...}", mb->u.area->u.sc.size);
          return;
        case ABCE_T_T:
          printf("{...}");
          return;
        case ABCE_T_A:
          printf("[...]");
          return;
        default:
          break;
      }
    }
  }
  switch (mb->typ)
  {
    case ABCE_T_PB:
      printf("pb");
      break;
    case ABCE_T_N:
      printf("null");
      break;
    case ABCE_T_D:
      printf("%.20g", mb->u.d);
      break;
    case ABCE_T_B:
      printf("%s", mb->u.d ? "true" : "false");
      break;
    case ABCE_T_F:
      printf("fun(%lld)", (long long)mb->u.d);
      break;
    case ABCE_T_BP:
      printf("bp(%lld)", (long long)mb->u.d);
      break;
    case ABCE_T_IP:
      printf("ip(%lld)", (long long)mb->u.d);
      break;
    case ABCE_T_IOS:
      printf("ios(%p)", mb->u.area);
      break;
    case ABCE_T_A:
      printf("[");
      for (i = 0; i < mb->u.area->u.ar.size; i++)
      {
        if (i != 0)
        {
          printf(", ");
        }
        abce_mb_dump_impl(&mb->u.area->u.ar.mbs[i], &ll3);
      }
      printf("]");
      break;
    case ABCE_T_SC:
      printf("sc(%zu){", mb->u.area->u.sc.size);
      for (i = 0; i < mb->u.area->u.sc.size; i++)
      {
        abce_mb_treedump(mb->u.area->u.sc.heads[i].root, &first, &ll3);
      }
      printf("}");
      break;
    case ABCE_T_T:
      printf("{");
      abce_mb_treedump(mb->u.area->u.tree.tree.root, &first, &ll3);
      printf("}");
      break;
    case ABCE_T_S:
      abce_dump_str(mb->u.area->u.str.buf, mb->u.area->u.str.size);
      break;
  }
}
