#ifndef _ABCE_LIKELY_H_
#define _ABCE_LIKELY_H_

#ifdef __clang__
  #define abce_likely(x)       __builtin_expect(!!(x),1)
  #define abce_unlikely(x)     __builtin_expect(!!(x),0)
#else
  #ifdef __GNUC__
    #define abce_likely(x)       __builtin_expect(!!(x),1)
    #define abce_unlikely(x)     __builtin_expect(!!(x),0)
  #else
    #define abce_likely(x)       (!!(x))
    #define abce_unlikely(x)     (!!(x))
  #endif
#endif

#endif
