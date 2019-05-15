#ifndef _OPCODESONLY_H_
#define _OPCODESONLY_H_

enum abce_opcode {
  ABCE_OPCODE_NOP = 0,
  ABCE_OPCODE_PUSH_DBL = 1, // followed by double
  ABCE_OPCODE_PUSH_TRUE = 2,
  ABCE_OPCODE_PUSH_FALSE = 3,
  ABCE_OPCODE_PUSH_NIL = 4,
  ABCE_OPCODE_BOOLEANIFY = 5,
  ABCE_OPCODE_FUNIFY = 6,
  ABCE_OPCODE_EQ = 7,
  ABCE_OPCODE_NE = 8,
  ABCE_OPCODE_LOGICAL_AND = 9,
  ABCE_OPCODE_LOGICAL_OR = 10,
  ABCE_OPCODE_LOGICAL_NOT = 11,
  ABCE_OPCODE_BITWISE_AND = 12,
  ABCE_OPCODE_BITWISE_OR = 13,
  ABCE_OPCODE_BITWISE_XOR = 14,
  ABCE_OPCODE_BITWISE_NOT = 15,
  ABCE_OPCODE_LT = 16,
  ABCE_OPCODE_GT = 17,
  ABCE_OPCODE_LE = 18,
  ABCE_OPCODE_GE = 19,
  ABCE_OPCODE_SHL = 20,
  ABCE_OPCODE_SHR = 21,
  ABCE_OPCODE_ADD = 22,
  ABCE_OPCODE_SUB = 23,
  ABCE_OPCODE_MUL = 24,
  ABCE_OPCODE_DIV = 25,
  ABCE_OPCODE_MOD = 26,
  ABCE_OPCODE_UNARY_MINUS = 27,
  ABCE_OPCODE_IF_NOT_JMP = 28,
  ABCE_OPCODE_JMP = 29,
  ABCE_OPCODE_CALL = 30,
  ABCE_OPCODE_RET = 31,
  ABCE_OPCODE_RETEX2 = 32,
  ABCE_OPCODE_PUSH_STACK = 33,
  ABCE_OPCODE_SET_STACK = 34,
  ABCE_OPCODE_UNUSED0 = 35,
  ABCE_OPCODE_UNUSED1 = 36,
  ABCE_OPCODE_PUSH_FROM_CACHE = 37,
  ABCE_OPCODE_POP = 38,
  ABCE_OPCODE_POP_MANY = 39,
  ABCE_OPCODE_STRGET = 40,
  ABCE_OPCODE_STRLEN = 41,
  ABCE_OPCODE_APPEND_MAINTAIN = 42,
  ABCE_OPCODE_PBSETLEN = 43,
  ABCE_OPCODE_DICTSET_MAINTAIN = 44,
  ABCE_OPCODE_LISTPOP = 45,
  ABCE_OPCODE_LISTGET = 46,
  ABCE_OPCODE_LISTSET = 47,
  ABCE_OPCODE_LISTLEN = 48,
  ABCE_OPCODE_DICTDEL = 49,
  ABCE_OPCODE_DICTGET = 50,
  ABCE_OPCODE_GETSCOPE_DYN = 51,
  ABCE_OPCODE_SCOPEVAR = 52,
  ABCE_OPCODE_SCOPEVAR_SET = 53,
  ABCE_OPCODE_DICTNEXT_SAFE = 54,
  ABCE_OPCODE_TYPE = 55,
  ABCE_OPCODE_DICTHAS = 56,
  ABCE_OPCODE_SCOPE_HAS = 57,
  ABCE_OPCODE_CALL_IF_FUN = 58,
  ABCE_OPCODE_DICTLEN = 59,
  ABCE_OPCODE_PBGET = 60,
  ABCE_OPCODE_PBLEN = 61,
  ABCE_OPCODE_PBSET = 62,
  ABCE_OPCODE_FUN_HEADER = 63,

  ABCE_OPCODE_STRSUB = 128,
  ABCE_OPCODE_STR_FROMCHR = 129,
  ABCE_OPCODE_STRAPPEND = 130,
  ABCE_OPCODE_STR_CMP = 131,
  ABCE_OPCODE_STRLISTJOIN = 132,
  ABCE_OPCODE_STR_LOWER = 133,
  ABCE_OPCODE_STR_UPPER = 134,
  ABCE_OPCODE_STR_REVERSE = 135,
  ABCE_OPCODE_STRSTR = 136,
  ABCE_OPCODE_STRGSUB = 137,
  ABCE_OPCODE_STRREP = 138,
  ABCE_OPCODE_STRFMT = 139,
  ABCE_OPCODE_STRSET = 140,
  ABCE_OPCODE_STRSTRIP = 141, // specify strip charset as string to this!!!!
  ABCE_OPCODE_STRWORD = 142, // specify delim to this!!!!
  ABCE_OPCODE_STRWORDLIST = 143, // specify delim to this!!!!
  ABCE_OPCODE_STRWORDCNT = 144, // specify delim to this!!!!
  ABCE_OPCODE_TOSTRING = 145,
  ABCE_OPCODE_TONUMBER = 146,
  // math
  ABCE_OPCODE_ABS = 147,
  ABCE_OPCODE_ACOS = 148,
  ABCE_OPCODE_ASIN = 149,
  ABCE_OPCODE_ATAN = 150,
  ABCE_OPCODE_CEIL = 151,
  ABCE_OPCODE_FLOOR = 152,
  ABCE_OPCODE_COS = 153,
  ABCE_OPCODE_SIN = 154,
  ABCE_OPCODE_TAN = 155,
  ABCE_OPCODE_EXP = 156,
  ABCE_OPCODE_LOG = 157,
  ABCE_OPCODE_SQRT = 158,
  // utils
  ABCE_OPCODE_EXIT = 159,
  ABCE_OPCODE_DUMP = 160,
  ABCE_OPCODE_DUP_NONRECURSIVE = 161,
  ABCE_OPCODE_ERROR = 162,
  ABCE_OPCODE_OUT = 163, // warn or info, depending on arg
  // function header & trailer
  ABCE_OPCODE_HI_UNUSED0 = 164,
  ABCE_OPCODE_FUN_TRAILER = 165,
  ABCE_OPCODE_LISTSPLICE = 166,
  //ABCE_OPCODE_LUAPUSH = 167,
  //ABCE_OPCODE_LUAEVAL = 168,
  // modularity
  ABCE_OPCODE_IMPORT = 169,
  //ABCE_OPCODE_LUA_IMPORT = 170,
  ABCE_OPCODE_SCOPE_NEW = 171,
  // I/O
  ABCE_OPCODE_FILE_OPEN = 172,
  ABCE_OPCODE_FILE_CLOSE = 173,
  ABCE_OPCODE_FILE_GET = 174, // GET(delim, maxcnt), GET(NaN, maxcnt), GET(delim, Inf), GET(NaN, Inf)
  ABCE_OPCODE_FILE_SEEK_TELL = 175,
  ABCE_OPCODE_FILE_FLUSH = 176,
  ABCE_OPCODE_FILE_WRITE = 177,
  ABCE_OPCODE_MEMFILE_IOPEN = 178,
  // misc
  ABCE_OPCODE_FP_CLASSIFY = 179, // isnan, isfinite, isinf handled all by this
  ABCE_OPCODE_JSON_ENCODE = 180,
  ABCE_OPCODE_JSON_DECODE = 181,
  ABCE_OPCODE_PUSH_NEW_ARRAY = 182,
  ABCE_OPCODE_PUSH_NEW_DICT = 183,
  ABCE_OPCODE_APPENDALL_MAINTAIN = 184,
};
// FIXME hypot? round? erf? signum? log1p? expm1? cbrt? pow?

#endif
