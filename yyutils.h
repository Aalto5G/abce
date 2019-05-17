#ifndef _YYUTILS_H_
#define _YYUTILS_H_

#include <stdio.h>
#include <stdint.h>
#include "amyplanyy.h"

#ifdef __cplusplus
extern "C" {
#endif


void amyplanyydoparse(FILE *filein, struct amyplanyy *amyplanyy);

void amyplanyydomemparse(char *filedata, size_t filesize, struct amyplanyy *amyplanyy);

void amyplanyynameparse(const char *fname, struct amyplanyy *amyplanyy, int require);

void amyplanyydirparse(
  const char *argv0, const char *fname, struct amyplanyy *amyplanyy, int require);

struct escaped_string yy_escape_string(char *orig);

struct escaped_string yy_escape_string_single(char *orig);

uint32_t yy_get_ip(char *orig);

#ifdef __cplusplus
};
#endif

#endif

