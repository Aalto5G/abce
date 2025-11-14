#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <libgen.h>
#include "amyplanyy.h"
#include "amyplanyyutils.h"

typedef void *yyscan_t;
extern int amyplanyyparse(yyscan_t scanner, struct amyplanyy *amyplanyy);
extern int amyplanyylex_init(yyscan_t *scanner);
extern void amyplanyyset_in(FILE *in_str, yyscan_t yyscanner);
extern int amyplanyylex_destroy(yyscan_t yyscanner);

void amyplanyydoparse(FILE *filein, struct amyplanyy *amyplanyy)
{
  yyscan_t scanner;
  amyplanyylex_init(&scanner);
  amyplanyyset_in(filein, scanner);
  if (amyplanyyparse(scanner, amyplanyy) != 0)
  {
    fprintf(stderr, "parsing failed\n");
    exit(1);
  }
  amyplanyylex_destroy(scanner);
  if (!feof(filein))
  {
    fprintf(stderr, "error: additional data at end of amyplanyy data\n");
    exit(1);
  }
}

void amyplanyydomemparse(char *filedata, size_t filesize, struct amyplanyy *amyplanyy)
{
  FILE *myfile;
  myfile = fmemopen(filedata, filesize, "r");
  if (myfile == NULL)
  {
    fprintf(stderr, "can't open memory file\n");
    exit(1);
  }
  amyplanyydoparse(myfile, amyplanyy);
  if (fclose(myfile) != 0)
  {
    fprintf(stderr, "can't close memory file\n");
    exit(1);
  }
}

static void *memdup(const void *mem, size_t sz)
{
  void *result;
  result = malloc(sz);
  if (result == NULL)
  {
    return result;
  }
  memcpy(result, mem, sz);
  return result;
}

struct amyplan_escaped_string amyplanyy_escape_string(char *orig, char quote)
{
  char *buf = NULL;
  char *result = NULL;
  struct amyplan_escaped_string resultstruct;
  size_t j = 0;
  size_t capacity = 0;
  size_t i = 1;
  while (orig[i] != quote)
  {
    if (j+2 >= capacity)
    {
      char *buf2;
      capacity = 2*capacity+10;
      buf2 = realloc(buf, capacity);
      if (buf2 == NULL)
      {
        free(buf);
        resultstruct.str = NULL;
        return resultstruct;
      }
      buf = buf2;
    }
    if (orig[i] != '\\')
    {
      buf[j++] = orig[i++];
    }
    else if (orig[i+1] == 'x')
    {
      char hexbuf[3] = {0};
      char *endptr;
      hexbuf[0] = orig[i+2];
      hexbuf[1] = orig[i+3];
      buf[j++] = strtol(hexbuf, &endptr, 16);
      if (strlen(hexbuf) != 2 || *endptr != '\0')
      {
        fprintf(stderr, "Invalid string hex escape: \\x%s\n", hexbuf);
        exit(1);
      }
      i += 4;
    }
    else if (orig[i+1] == 'u')
    {
      char hexbuf[5] = {0};
      char *endptr;
      uint16_t unicode;
      hexbuf[0] = orig[i+2];
      hexbuf[1] = orig[i+3];
      hexbuf[2] = orig[i+4];
      hexbuf[3] = orig[i+5];
      unicode = strtol(hexbuf, &endptr, 16);
      if (strlen(hexbuf) != 4 || *endptr != '\0')
      {
        fprintf(stderr, "Invalid string unicode escape: \\u%s\n", hexbuf);
        exit(1);
      }
      if (unicode <= 0x7F)
      {
        buf[j++] = (uint8_t)unicode;
        i += 6;
        continue;
      }
      if (unicode <= 0x7FF)
      {
        buf[j++] = (uint8_t)(0xc0|(unicode>>6));
        buf[j++] = (uint8_t)(0x80|(unicode&0x3f));
        i += 6;
        continue;
      }
      if (unicode <= 0xFFFF)
      {
        buf[j++] = (uint8_t)(0xe0|(unicode>>12));
        buf[j++] = (uint8_t)(0x80|((unicode>>6)&0x3f));
        buf[j++] = (uint8_t)(0x80|(unicode&0x3f));
        i += 6;
        continue;
      }
      abort();
    }
    else if (orig[i+1] == 't')
    {
      buf[j++] = '\t';
      i += 2;
    }
    else if (orig[i+1] == 'r')
    {
      buf[j++] = '\r';
      i += 2;
    }
    else if (orig[i+1] == 'n')
    {
      buf[j++] = '\n';
      i += 2;
    }
    else
    {
      buf[j++] = orig[i+1];
      i += 2;
    }
  }
  if (j >= capacity)
  {
    char *buf2;
    capacity = 2*capacity+10;
    buf2 = realloc(buf, capacity);
    if (buf2 == NULL)
    {
      free(buf);
      resultstruct.str = NULL;
      return resultstruct;
    }
    buf = buf2;
  }
  resultstruct.sz = j;
  buf[j++] = '\0';
  result = memdup(buf, j);
  resultstruct.str = result;
  free(buf);
  return resultstruct;
}

void amyplanyynameparse(const char *fname, struct amyplanyy *amyplanyy, int require)
{
  FILE *amyplanyyfile;
  amyplanyyfile = fopen(fname, "r");
  if (amyplanyyfile == NULL)
  {
    if (require)
    {
      fprintf(stderr, "File %s cannot be opened\n", fname);
      exit(1);
    }
#if 0
    if (amyplanyy_postprocess(amyplanyy) != 0)
    {
      exit(1);
    }
#endif
    return;
  }
  amyplanyydoparse(amyplanyyfile, amyplanyy);
#if 0
  if (amyplanyy_postprocess(amyplanyy) != 0)
  {
    exit(1);
  }
#endif
  fclose(amyplanyyfile);
}

void amyplanyydirparse(
  const char *argv0, const char *fname, struct amyplanyy *amyplanyy, int require)
{
  const char *dir;
  char *copy = strdup(argv0);
  char pathbuf[PATH_MAX];
  dir = dirname(copy); // NB: not for multi-threaded operation!
  snprintf(pathbuf, sizeof(pathbuf), "%s/%s", dir, fname);
  free(copy);
  amyplanyynameparse(pathbuf, amyplanyy, require);
}
