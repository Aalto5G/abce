#ifndef _ABCE_ERR_H_
#define _ABCE_ERR_H_

enum abce_errcode {
  ABCE_E_NONE = 0,
  ABCE_E_EXIT = 1, // value: unused
  ABCE_E_NOTSUP_INSTRUCTION = 2, // value: opcode
  ABCE_E_RUN_INTO_FUNC = 3,
  ABCE_E_INDEX_OOB = 4, // value: index, value2: sz
  ABCE_E_INDEX_NOT_INT = 5, // value: index
  ABCE_E_NO_MEM = 6, // value: unused (FIXME or alloc size?)
  ABCE_E_INVALID_CH = 7, // value: value
  ABCE_E_INVALID_STREAMIDX = 8, // value: value
  ABCE_E_IO_ERROR = 9, // value: unused
  ABCE_E_ERROR_EXIT = 10, // value: unused
  ABCE_E_REPCNT_NOT_UINT = 11, // value: value
  ABCE_E_NOT_A_NUMBER_STRING = 12, // value: mb
  ABCE_E_EXPECT_ARRAY_OR_TREE = 13, // FIXME split, value: mb
  ABCE_E_UNKNOWN_INSTRUCTION = 14, // value: opcode
  ABCE_E_ILLEGAL_INSTRUCTION = 15, // value: opcode
  ABCE_E_BYTECODE_FAULT = 16,
  ABCE_E_EXPECT_FUN_HEADER = 17,
  ABCE_E_INVALID_ARG_CNT = 18, // value: actual, value2: expected
  ABCE_E_STACK_UNDERFLOW = 19, // value: unused
  ABCE_E_STACK_OVERFLOW = 20, // value: mb
  ABCE_E_ARRAY_UNDERFLOW = 21, // value: unused
  ABCE_E_PB_NEW_LEN_NOT_UINT = 22, // value: value
  ABCE_E_PB_VAL_OOB = 23, // value: value
  ABCE_E_PB_OFF_NOT_UINT = 24, // value: value
  ABCE_E_PB_OPSZ_INVALID = 25, // value: value
  ABCE_E_PB_GET_OOB = 26, // value: index
  ABCE_E_PB_SET_OOB = 27, // value: index
  ABCE_E_RET_LOCVARCNT_NOT_UINT = 28, // value: argument
  ABCE_E_STACK_IDX_NOT_UINT = 29, // value: idx
  ABCE_E_STACK_IDX_OOB = 30, // value: idx
  ABCE_E_RET_ARGCNT_NOT_UINT = 31, // value: argument
  ABCE_E_CACHE_IDX_NOT_INT = 32, // value: value
  ABCE_E_CACHE_IDX_OOB = 33, // value: idx
  ABCE_E_SCOPEVAR_NOT_FOUND = 34, // value: mb
  ABCE_E_SCOPEVAR_NAME_NOT_STR = 35, // value: mb
  ABCE_E_TREE_ENTRY_NOT_FOUND = 36, // value: mb
  ABCE_E_TREE_KEY_NOT_STR = 37, // value: mb
  ABCE_E_TREE_ITER_NOT_STR_OR_NUL = 38, // value: mb
  ABCE_E_EXPECT_DBL = 39, // value: mb, value2: idx
  ABCE_E_EXPECT_BOOL = 40, // value: mb, value2: idx
  ABCE_E_EXPECT_FUNC = 41, // value: mb, value2: idx
  ABCE_E_EXPECT_BP = 42, // value: mb, value2: idx
  ABCE_E_EXPECT_IP = 43, // value: mb, value2: idx
  ABCE_E_EXPECT_NIL = 44, // value: mb, value2: idx
  ABCE_E_EXPECT_TREE = 45, // value: mb, value2: idx
  ABCE_E_EXPECT_IOS = 46, // value: mb, value2: idx
  ABCE_E_EXPECT_ARRAY = 47, // value: mb, value2: idx
  ABCE_E_EXPECT_STR = 48, // value: mb, value2: idx
  ABCE_E_EXPECT_PB = 49, // value: mb, value2: idx
  ABCE_E_EXPECT_SCOPE = 50, // value: mb, value2: idx
  ABCE_E_FUNADDR_NOT_INT = 51, // value: fun addr
  ABCE_E_REG_NOT_INT = 52, // value: mb, value2: idx
};

#endif
