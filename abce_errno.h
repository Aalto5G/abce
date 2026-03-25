#ifndef _ABCE_ERRNO_H_
#define _ABCE_ERRNO_H_

#if ABCE_NO_ERRNO

#define ENOMEM 12
#define EINVAL 22
#define EFAULT 14
#define ENOENT 2
#define EOVERFLOW 75
#define EEXIST 17
#define EINTR 4
#define EILSEQ 84
#define EINPROGRESS 115
#define EPERM 1
#define EACCES 13
#define EIO 5
#define ENOTSUP 95
#define EAGAIN 11
#define ERANGE 34
#define ECANCELED 125

#else

#include <errno.h>

#endif

#endif
