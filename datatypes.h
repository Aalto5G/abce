#ifndef _DATATYPES_H_
#define _DATATYPES_H_

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <endian.h>
#include "rbtree.h"
#include "murmur.h"
#include "containerof.h"
#include "likely.h"
#include "abceopcodes.h"

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
  struct rb_tree_nocmp tree;
  size_t sz;
};
struct abce_mb_scope {
  int holey;
  struct abce_mb_area *parent;
  size_t size;
  size_t locidx;
  struct rb_tree_nocmp heads[0];
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
  struct rb_tree_node node;
  size_t size;
  size_t locidx;
  char buf[0];
};
struct abce_mb_ios {
  FILE *f;
};
struct abce_mb_area {
  size_t refcnt;
  union {
    struct abce_mb_array ar;
    struct abce_mb_ios ios;
    struct abce_mb_scope sc;
    struct abce_mb_tree tree;
    struct abce_mb_string str;
    struct abce_mb_pb pb;
  } u;
};
enum abce_type {
  ABCE_T_T,
  ABCE_T_D,
  ABCE_T_B,
  ABCE_T_F,
  ABCE_T_S,
  ABCE_T_PB, // packet buffer
  ABCE_T_IOS,
  ABCE_T_BP,
  ABCE_T_IP,
  //ABCE_T_LP,
  ABCE_T_A,
  ABCE_T_SC,
  ABCE_T_N,
};
struct abce_mb {
  enum abce_type typ;
  union {
    double d;
    struct abce_mb_area *area;
  } u;
};
struct abce_mb_rb_entry {
  struct rb_tree_node n;
  struct abce_mb key; // must be a string!
  struct abce_mb val;
};

struct abce {
  void *(*alloc)(void *old, size_t newsz, void *alloc_baton);
  int (*trap)(struct abce*, uint16_t ins, unsigned char *addcode, size_t addsz);
  void *alloc_baton;
  void *userdata;
  // Stack and registers
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
  struct rb_tree_nocmp strcache[ABCE_DEFAULT_CACHE_SIZE];
  // Dynamic scope
  struct abce_mb dynscope;
};

#ifdef __cplusplus
};
#endif

#endif
