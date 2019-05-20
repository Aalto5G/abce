#ifndef _DATATYPES_H_
#define _DATATYPES_H_

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <endian.h>
#include "abcerbtree.h"
#include "abcemurmur.h"
#include "abcecontainerof.h"
#include "abcelikely.h"
#include "abceopcodes.h"
#include "abce_err.h"

#define ABCE_DEFAULT_SCOPE_SIZE 8192
#define ABCE_DEFAULT_CACHE_SIZE 8192

#ifdef __cplusplus
extern "C" {
#endif

struct abce_const_str_len {
  const char *str;
  size_t len;
};

struct abce_mb;

struct abce_mb_tree {
  struct abce_rb_tree_nocmp tree;
  size_t sz;
};
struct abce_mb_scope {
  int holey;
  struct abce_mb_area *parent;
  size_t size;
  size_t locidx;
  struct abce_rb_tree_nocmp heads[0];
};
struct abce_mb_array {
  struct abce_mb *mbs;
  size_t capacity;
  size_t size;
};
struct abce_mb_pb {
  size_t size;
  size_t capacity;
  char *buf;
};
struct abce_mb_string {
  struct abce_rb_tree_node node;
  size_t size;
  size_t locidx;
  char buf[0];
};
struct abce_mb_ios {
  FILE *f;
};
struct abce_mb_area {
  size_t refcnt;
  size_t locidx;
  union {
    struct abce_mb_array ar;
    struct abce_mb_ios ios;
    struct abce_mb_scope sc;
    struct abce_mb_tree tree;
    struct abce_mb_string str;
    struct abce_mb_pb pb;
  } u;
};
// These must match error codes ABCE_E_EXPECT_*
enum abce_type {
  ABCE_T_T = 45,
  ABCE_T_IOS = 46,
  ABCE_T_A = 47,
  ABCE_T_S = 48,
  ABCE_T_PB = 49, // packet buffer
  ABCE_T_SC = 50,

  ABCE_T_D = 39,
  ABCE_T_B = 40,
  ABCE_T_F = 41,
  ABCE_T_BP = 42,
  ABCE_T_IP = 43,
  //ABCE_T_LP,
  ABCE_T_N = 44,
};
struct abce_mb {
  enum abce_type typ;
  union {
    double d;
    struct abce_mb_area *area;
  } u;
};
struct abce_mb_rb_entry {
  struct abce_rb_tree_node n;
  struct abce_mb key; // must be a string!
  struct abce_mb val;
};

struct abce_err {
  enum abce_errcode code;
  uint16_t opcode;
  struct abce_mb mb;
  double val2;
  //struct memblock mb2;
};

struct abce {
  int in_engine;
  struct abce_err err;
  void *(*alloc)(void *old, size_t oldsz, size_t newsz, void **pbaton);
  int (*trap)(void **pbaton, uint16_t ins, unsigned char *addcode, size_t addsz);
  void *alloc_baton;
  void *trap_baton;
  void *userdata;
  size_t lastbytes_alloced;
  size_t lastgcblocksz;
  int trusted;
  // Stack and registers
  int (*ins_budget_fn)(void **pbaton, uint16_t ins);
  void *ins_budget_baton;
  size_t bytes_alloced;
  size_t bytes_cap;
  struct abce_mb *stackbase;
  size_t stacklimit;
  size_t sp;
  size_t bp;
  int64_t ip;
  // Byte code
  unsigned char *bytecode;
  size_t bytecodesz;
  size_t bytecodecap;
  // Object cache
  struct abce_mb *cachebase;
  size_t cachesz;
  size_t cachecap;
  struct abce_rb_tree_nocmp strcache[ABCE_DEFAULT_CACHE_SIZE];
  // GC cache
  struct abce_mb *gcblockbase;
  size_t gcblocksz;
  size_t gcblockcap;
  size_t scratchstart;
  // Dynamic scope
  struct abce_mb dynscope;
  size_t btcap;
  size_t btsz;
  struct abce_mb *btbase;
};

#ifdef __cplusplus
};
#endif

#endif
