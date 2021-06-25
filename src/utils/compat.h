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
#ifdef WIN32
/* https://stackoverflow.com/questions/42536311/msvc-1900-and-define-vsnprintf */
#include <stdio.h>
#endif
#include "config.h"

/* Remove __attribute__ when we are not compiling with gcc */
#ifndef __GNUC__
# define __attribute__(x)
#endif

/* Make sure that we have size_t defined */
#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#elif !defined HAVE_SIZE_T
#error "system is missing definition of size_t"
#endif

/*
 * compat/realpath.c
 */

/** realpath() - return the canonicalized absolute pathname */
#ifndef HAVE_REALPATH
char *realpath(const char *path, char *resolved);
#endif


/*
 * compat/snprintf.c
 */

/** snprintf() - write formatted output to sized buffer */
#ifndef HAVE_SNPRINTF
# ifdef HAVE__SNPRINTF
#  define snprintf _snprintf
# else
#  define snprintf rpl_snprintf
int rpl_snprintf(char *str, size_t size, const char *fmt, ...)
  __attribute__ ((__format__ (__printf__, 3, 4)));
# endif
#endif

/** asprintf() - write formatted output to allocated buffer */
#ifndef HAVE_ASPRINTF
# ifdef HAVE__ASPRINTF
#  define asprintf _asprintf
# else
#  define asprintf rpl_asprintf
int rpl_asprintf(char **buf, const char *fmt, ...)
  __attribute__ ((__format__ (__printf__, 2, 3)));
# endif
#endif


#ifdef HAVE_STDARG_H
#include <stdarg.h>

/** vsnprintf() - write formatted output to sized buffer */
#ifndef HAVE_VSNPRINTF
# define vsnprintf rpl_vsnprintf
int rpl_vsnprintf(char *str, size_t size, const char *fmt, va_list ap)
  __attribute__ ((__format__ (__printf__, 3, 0)));
#endif

/** vasprintf() - write formatted output to allocated buffer */
#ifndef HAVE_VASPRINTF
# define vasprintf rpl_vasprintf
int rpl_vasprintf(char **buf, const char *fmt, va_list ap)
  __attribute__ ((__format__ (__printf__, 2, 0)));
#endif

#endif  /* HAVE_STDARG_H */



/** strdup() - duplicate a string */
#ifdef HAVE__STRDUP
# define strdup(s) _strdup(s)
#else
# ifndef HAVE_STRDUP
char *strdup(const char *s)
  __attribute__ ((__malloc__));
# endif
#endif

/** strndup() - duplicate a string with a maximal size */
#ifdef HAVE__STRNDUP
# define strndup(s, n) _strndup(s, n)
#else
# ifndef HAVE_STRNDUP
char *strndup(const char *s, size_t n)
  __attribute__ ((__malloc__));
# endif
#endif

/** strcasecmp() - case insensitive string comparison */
#ifndef strcasecmp
# ifdef HAVE__STRICMP
#  define strcasecmp(s1, s2) _stricmp(s1, s2)
# else
#  ifndef HAVE_STRCASECMP
int strcasecmp(const char *s1, const char *s2);
#  endif
# endif
#endif

/** strncasecmp() - case insensitive, length-limited string comparison */
#ifndef strncasecmp
# ifdef HAVE__STRNICMP
#  define strncasecmp(s1, s2, n) _strnicmp(s1, s2, n)
# else
#  ifndef HAVE_STRNCASECMP
int strncasecmp(const char *s1, const char *s2, size_t n);
#  endif
# endif
#endif


/*
 * compat.c
 */

/** strlcpy() - like strncpy(), but guarantees that `dst` is NUL-terminated */
#ifndef HAVE_STRLCPY
size_t strlcpy(char *dst, const char *src, size_t size);
#endif

/** strlcat() - like strncat(), but guarantees that `dst` is NUL-terminated */
#ifndef HAVE_STRLCAT
size_t strlcat(char *dst, const char *src, size_t size);
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
