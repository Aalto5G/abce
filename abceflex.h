#ifndef _ABCEFLEX_H_
#define _ABCEFLEX_H_

#if __STDC_VERSION__ >= 199901L
#define ABCE_FLEX
#else
#define ABCE_FLEX 0
#endif

// Both array[] and array[0] are nonstandard if used in union.
// Pick array[] if C99 in use, although that might fail too.
#if __STDC_VERSION__ >= 199901L
#define ABCE_FLEX_IN_UNION
#else
#define ABCE_FLEX_IN_UNION 0
#endif

#endif
