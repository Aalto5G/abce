#ifndef _AMYPLAN_H_
#define _AMYPLAN_H_

static inline uint16_t
get_corresponding_get(uint16_t set)
{
  switch (set)
  {
    case ABCE_OPCODE_SET_STACK:
      return ABCE_OPCODE_PUSH_STACK;
    case ABCE_OPCODE_SCOPEVAR_SET:
      return ABCE_OPCODE_SCOPEVAR;
    case ABCE_OPCODE_LISTSET:
      return ABCE_OPCODE_LISTGET;
    case ABCE_OPCODE_DICTSET_MAINTAIN:
      return ABCE_OPCODE_DICTGET;
    case ABCE_OPCODE_PBSET:
      return ABCE_OPCODE_PBGET;
    case ABCE_OPCODE_PBSETLEN:
      return ABCE_OPCODE_PBLEN;
    case ABCE_OPCODE_STRGET:
      return ABCE_OPCODE_STRGET;
    case ABCE_OPCODE_DICTHAS:
      return ABCE_OPCODE_DICTHAS;
    case ABCE_OPCODE_SCOPE_HAS:
      return ABCE_OPCODE_SCOPE_HAS;
    case ABCE_OPCODE_STRLEN:
      return ABCE_OPCODE_STRLEN;
    case ABCE_OPCODE_LISTLEN:
      return ABCE_OPCODE_LISTLEN;
    case ABCE_OPCODE_DICTLEN:
      return ABCE_OPCODE_DICTLEN;
    default:
      printf("FIXME default1\n");
      abort();
  }
}


#endif
