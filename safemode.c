#include "abceopcodes.h"
#include "safemode.h"
#include "abcedatatypes.h"
#include "abce_err.h"
#include <errno.h>

int noio_restrict_fn(struct abce *abce, void **pbaton, uint16_t ins)
{
  // a blacklist of disallowed instructions
  switch (ins)
  {
    case ABCE_OPCODE_FILE_OPEN:
      abce->err.code = ABCE_E_INS_NOT_PERMITTED;
      abce->err.mb.typ = ABCE_T_D;
      abce->err.mb.u.d = ins;
      return -EPERM;
  }
  return 0;
}
