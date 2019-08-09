#include "abceopcodes.h"
#include "safemode.h"
#include <errno.h>

int noio_restrict_fn(void **pbaton, uint16_t ins)
{
  // a blacklist of disallowed instructions
  switch (ins)
  {
    case ABCE_OPCODE_FILE_OPEN:
      return -EPERM;
  }
  return 0;
}
