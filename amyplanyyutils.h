#ifndef _AMYPLAN_YYUTILS_H_
#define _AMYPLAN_YYUTILS_H_

#undef DIRPARSE
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#if defined (__unix__) || (defined (__APPLE__) && defined (__MACH__))
#include <unistd.h>
#endif
#include "amyplanyy.h"

#ifdef __cplusplus
extern "C" {
#endif

static inline char *abce_strdup(const char *x)
{
  size_t len = strlen(x);
  char *res = malloc(len+1);
  if (res == NULL)
  {
    return NULL;
  }
  memcpy(res, x, len+1);
  return res;
}

void amyplanyydoparse(FILE *filein, struct amyplanyy *amyplanyy);

#ifdef _POSIX_C_SOURCE
#if _POSIX_C_SOURCE < 200809L
static inline FILE *fmemopenwrap(void *buf, size_t size, const char *mode)
{
  return NULL;
}
#else
static inline FILE *fmemopenwrap(void *buf, size_t size, const char *mode)
{
  return fmemopen(buf, size, mode);
}
#endif
#else
#ifdef _POSIX_VERSION
#if _POSIX_VERSION < 200809L
static inline FILE *fmemopenwrap(void *buf, size_t size, const char *mode)
{
  return NULL;
}
#else
static inline FILE *fmemopenwrap(void *buf, size_t size, const char *mode)
{
  return fmemopen(buf, size, mode);
}
#endif
#else
static inline FILE *fmemopenwrap(void *buf, size_t size, const char *mode)
{
  return NULL;
}
#endif
#endif

#ifdef MEMPARSE
void amyplanyydomemparse(char *filedata, size_t filesize, struct amyplanyy *amyplanyy);
#endif

void amyplanyynameparse(const char *fname, struct amyplanyy *amyplanyy, int require);

#ifdef DIRPARSE
void amyplanyydirparse(
  const char *argv0, const char *fname, struct amyplanyy *amyplanyy, int require);
#endif

struct amyplan_escaped_string amyplanyy_escape_string(char *orig, char quote);

#ifdef __cplusplus
};
#endif

#endif

