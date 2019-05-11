#ifndef _APLANYY_H_
#define _APLANYY_H_

#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#ifdef __cplusplus
extern "C" {
#endif

struct escaped_string {
  size_t sz;
  char *str;
};

struct CSnippet {
  char *data;
  size_t len;
  size_t capacity;
};

static inline void csadd(struct CSnippet *cs, char ch)
{
  if (cs->len + 2 >= cs->capacity)
  {
    size_t new_capacity = cs->capacity * 2 + 2;
    cs->data = (char*)realloc(cs->data, new_capacity);
    cs->capacity = new_capacity;
  }
  cs->data[cs->len] = ch;
  cs->data[cs->len + 1] = '\0';
  cs->len++;
}

static inline void csaddstr(struct CSnippet *cs, char *str)
{
  size_t len = strlen(str);
  if (cs->len + len + 1 >= cs->capacity)
  {
    size_t new_capacity = cs->capacity * 2 + 2;
    if (new_capacity < cs->len + len + 1)
    {
      new_capacity = cs->len + len + 1;
    }
    cs->data = (char*)realloc(cs->data, new_capacity);
    cs->capacity = new_capacity;
  }
  memcpy(cs->data + cs->len, str, len);
  cs->len += len;
  cs->data[cs->len] = '\0';
}

struct dep {
  char *name;
  int rec;
};

struct aplanyyrule {
  struct dep *deps;
  size_t depsz;
  size_t depcapacity;
  char **targets;
  size_t targetsz;
  size_t targetcapacity;
};

struct aplanyy {
  void *baton;
  uint8_t *bytecode;
  size_t bytecapacity;
  size_t bytesz;
};

size_t symbol_add(struct aplanyy *aplanyy, const char *symbol, size_t symlen);
size_t aplanyy_add_fun_sym(struct aplanyy *aplanyy, const char *symbol, int maybe, size_t loc);

static inline void aplanyy_add_byte(struct aplanyy *aplanyy, uint8_t byte)
{
  size_t newcapacity;
  if (aplanyy->bytesz >= aplanyy->bytecapacity)
  {
    newcapacity = 2*aplanyy->bytecapacity + 1;
    aplanyy->bytecode = (uint8_t*)realloc(aplanyy->bytecode, sizeof(*aplanyy->bytecode)*newcapacity);
    aplanyy->bytecapacity = newcapacity;
  }
  aplanyy->bytecode[aplanyy->bytesz++] = byte; 
}

static inline void aplanyy_add_double(struct aplanyy *aplanyy, double dbl)
{
  uint64_t val;
  memcpy(&val, &dbl, 8);
  aplanyy_add_byte(aplanyy, val>>56);
  aplanyy_add_byte(aplanyy, val>>48);
  aplanyy_add_byte(aplanyy, val>>40);
  aplanyy_add_byte(aplanyy, val>>32);
  aplanyy_add_byte(aplanyy, val>>24);
  aplanyy_add_byte(aplanyy, val>>16);
  aplanyy_add_byte(aplanyy, val>>8);
  aplanyy_add_byte(aplanyy, val);
}

static inline void aplanyy_free(struct aplanyy *aplanyy)
{
  free(aplanyy->bytecode);
  memset(aplanyy, 0, sizeof(*aplanyy));
}

#ifdef __cplusplus
};
#endif

#endif
