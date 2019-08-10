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
    case ABCE_OPCODE_STRGET: // we don't use STRSET at all using regular syntax
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

static inline uint16_t
get_corresponding_set(uint16_t get)
{
  switch (get)
  {
    case ABCE_OPCODE_PUSH_STACK:
      return ABCE_OPCODE_SET_STACK;
    case ABCE_OPCODE_SCOPEVAR:
      return ABCE_OPCODE_SCOPEVAR_SET;
    case ABCE_OPCODE_SCOPEVAR_NONRECURSIVE:
      return ABCE_OPCODE_SCOPEVAR_SET;
    case ABCE_OPCODE_LISTGET:
      return ABCE_OPCODE_LISTSET;
    case ABCE_OPCODE_DICTGET:
      return ABCE_OPCODE_DICTSET_MAINTAIN;
    case ABCE_OPCODE_PBGET:
      return ABCE_OPCODE_PBSET;
    case ABCE_OPCODE_PBLEN:
      return ABCE_OPCODE_PBSETLEN;
    case ABCE_OPCODE_STRGET:
      abort();
      return ABCE_OPCODE_STRGET;
    case ABCE_OPCODE_DICTHAS:
      abort();
      return ABCE_OPCODE_DICTHAS;
    case ABCE_OPCODE_SCOPE_HAS:
      abort();
      return ABCE_OPCODE_SCOPE_HAS;
    case ABCE_OPCODE_STRLEN:
      abort();
      return ABCE_OPCODE_STRLEN;
    case ABCE_OPCODE_LISTLEN:
      abort();
      return ABCE_OPCODE_LISTLEN;
    case ABCE_OPCODE_DICTLEN:
      abort();
      return ABCE_OPCODE_DICTLEN;
    case ABCE_OPCODE_PUSH_FROM_CACHE:
      abort();
    default:
      printf("FIXME default1\n");
      abort();
  }
}


#endif
