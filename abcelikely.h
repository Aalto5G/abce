#ifndef _ABCE_LIKELY_H_
#define _ABCE_LIKELY_H_
#define abce_likely(x)       __builtin_expect((x),1)
#define abce_unlikely(x)     __builtin_expect((x),0)
#endif
