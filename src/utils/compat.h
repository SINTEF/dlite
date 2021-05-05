/* compat.h -- auxiliary cross-platform compatibility functions
 *
 * Copyright (C) 2017 SINTEF
 *
 * Distributed under terms of the MIT license.
 */

/**
  @file
  @brief auxiliary compatibility functions

  Note that the declarations for the functions found in the compat/
  subdirectory are provided in config.h.in.
*/
#ifndef _COMPAT_H
#define _COMPAT_H

#include <stdlib.h>
#include "config.h"

/* Remove __attribute__ when we are not compiling with gcc */
#ifndef __GNUC__
# define __attribute__(x)
#endif



/** strdup */
#ifdef HAVE__STRDUP
# define strdup(s) _strdup(s)
#else
# ifndef HAVE_STRDUP
char *strdup(const char *s)
  __attribute__ ((__malloc__));
# endif
#endif

/** strndup */
#ifdef HAVE__STRNDUP
# define strndup(s, n) _strndup(s, n)
#else
# ifndef HAVE_STRNDUP
char *strndup(const char *s, size_t n)
  __attribute__ ((__malloc__));
# endif
#endif

/** strcasecmp */
#ifndef strcasecmp
# ifdef HAVE__STRICMP
#  define strcasecmp(s1, s2) _stricmp(s1, s2)
# else
#  ifndef HAVE_STRCASECMP
int strcasecmp(const char *s1, const char *s2);
#  endif
# endif
#endif

/** strncasecmp */
#ifndef strncasecmp
# ifdef HAVE__STRNICMP
#  define strncasecmp(s1, s2, n) _strnicmp(s1, s2, n)
# else
#  ifndef HAVE_STRNCASECMP
int strncasecmp(const char *s1, const char *s2, size_t n);
#  endif
# endif
#endif


/** asnprintf() - print to allocated string */
#ifndef HAVE_ASNPRINTF
#define asnprintf rpl_asnprintf
int rpl_asnprintf(char **buf, size_t *size, const char *fmt, ...)
  __attribute__ ((__format__ (__printf__, 3, 4)));
#endif

/** asnprintf() - print to allocated string using va_list */
#ifndef HAVE_VASNPRINTF
#define vasnprintf rpl_vasnprintf
int rpl_vasnprintf(char **buf, size_t *size, const char *fmt, va_list ap)
  __attribute__ ((__format__ (__printf__, 3, 0)));
#endif

/** asnprintf() - print to position `pos` in allocated string */
#ifndef HAVE_ASNPPRINTF
#define asnpprintf rpl_asnpprintf
int rpl_asnpprintf(char **buf, size_t *size, size_t pos, const char *fmt, ...)
  __attribute__ ((__format__ (__printf__, 4, 5)));
#endif

/** asnprintf() - print to position `pos` in allocated string using va_list */
#ifndef HAVE_VASNPPRINTF
#define vasnpprintf rpl_vasnpprintf
int rpl_vasnpprintf(char **buf, size_t *size, size_t pos, const char *fmt,
                    va_list ap)
  __attribute__ ((__format__ (__printf__, 4, 0)));
#endif


#endif /* _COMPAT_H */
