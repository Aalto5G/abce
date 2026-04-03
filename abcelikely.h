#ifndef _ABCE_LIKELY_H_
#define _ABCE_LIKELY_H_

#ifdef __has_builtin
  #if __has_builtin (__builtin_expect)
    #define abce_likely(x)       __builtin_expect(!!(x),1)
    #define abce_unlikely(x)     __builtin_expect(!!(x),0)
  #else
    #define abce_likely(x)       (!!(x))
    #define abce_unlikely(x)     (!!(x))
  #endif
#else
  #ifdef __clang__ // LLVM 3.0 is the first to use this, no-op on LLVM 1.0
    #define abce_likely(x)       __builtin_expect(!!(x),1)
    #define abce_unlikely(x)     __builtin_expect(!!(x),0)
  #else
    #ifdef __GNUC__
      #if __GNUC__ >= 3
        #define abce_likely(x)       __builtin_expect(!!(x),1)
        #define abce_unlikely(x)     __builtin_expect(!!(x),0)
      #else
        #define abce_likely(x)       (!!(x))
        #define abce_unlikely(x)     (!!(x))
      #endif
    #else
      #define abce_likely(x)       (!!(x))
      #define abce_unlikely(x)     (!!(x))
    #endif
  #endif
#endif

#endif
