#ifndef _YYUTILS_H_
#define _YYUTILS_H_

#include <stdio.h>
#include <stdint.h>
#include "aplanyy.h"

#ifdef __cplusplus
extern "C" {
#endif


void aplanyydoparse(FILE *filein, struct aplanyy *aplanyy);

void aplanyydomemparse(char *filedata, size_t filesize, struct aplanyy *aplanyy);

void aplanyynameparse(const char *fname, struct aplanyy *aplanyy, int require);

void aplanyydirparse(
  const char *argv0, const char *fname, struct aplanyy *aplanyy, int require);

struct escaped_string yy_escape_string(char *orig);

struct escaped_string yy_escape_string_single(char *orig);

uint32_t yy_get_ip(char *orig);

#ifdef __cplusplus
};
#endif

#endif

