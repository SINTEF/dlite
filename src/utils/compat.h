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

/** strdup */
#ifdef HAVE__STRDUP
# define strdup(s) _strdup(s)
#else
# ifndef HAVE_STRDUP
char *strdup(const char *s);
# endif
#endif

/** strndup */
#ifdef HAVE__STRNDUP
# define strndup(s, n) _strndup(s, n)
#else
# ifndef HAVE_STRNDUP
char *strndup(const char *s, size_t n);
# endif
#endif

/** strcasecmp */
#ifdef HAVE__STRICMP
# define strcasecmp(s1, s2) _stricmp(s1, s2)
#else
# ifndef HAVE_STRCASECMP
int strcasecmp(const char *s1, const char *s2);
# endif
#endif

/** strncasecmp */
#ifdef HAVE__STRNICMP
# define strncasecmp(s1, s2, n) _strnicmp(s1, s2, n)
#else
# ifndef HAVE_STRNCASECMP
int strncasecmp(const char *s1, const char *s2, size_t n);
# endif
#endif


/** asnprintf() - print to allocated string */
#ifndef HAVE_ASNPRINTF
int asnprintf(char **buf, size_t *size, const char *fmt, ...);
#endif

/** asnprintf() - print to allocated string using va_list */
#ifndef HAVE_VASNPRINTF
int vasnprintf(char **buf, size_t *size, const char *fmt, va_list ap);
#endif

/** asnprintf() - print to position `pos` in allocated string */
#ifndef HAVE_ASNPPRINTF
int asnpprintf(char **buf, size_t *size, size_t pos, const char *fmt, ...);
#endif

/** asnprintf() - print to position `pos` in allocated string using va_list */
#ifndef HAVE_VASNPPRINTF
int vasnpprintf(char **buf, size_t *size, size_t pos, const char *fmt,
                va_list ap);
#endif


#endif /* _COMPAT_H */
