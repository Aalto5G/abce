#include "abceopcodes.h"
#include "safemode.h"
#include "abcedatatypes.h"
#include "abce_err.h"
#include <errno.h>

int noio_restrict_fn(void **pbaton, uint16_t ins)
{
  struct abce *abce = ABCE_CONTAINER_OF(pbaton, struct abce, ins_budget_baton);
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
