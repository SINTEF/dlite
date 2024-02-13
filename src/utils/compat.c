/* compat.c -- auxiliary cross-platform compatibility functions
 *
 * Copyright (C) 2017 SINTEF
 *
 * Distributed under terms of the MIT license.
 */
#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

#include "compat.h"

/* Ensure non-empty translation unit */
typedef int make_iso_compilers_happy;


#ifdef HAVE_VASPRINTF
#include <stdarg.h>
#endif


/* strdup() - duplicate a string */
#if !defined(HAVE_STRDUP) && !defined(HAVE__STRDUP)
char *strdup(const char *s)
{
  size_t n = strlen(s) + 1;
  char *p = malloc(n);
  if (!p) return NULL;
  if (p) memcpy(p, s, n);
  return p;
}
#endif

/* strndup() - duplicates a string, copying at most n bytes and adds a
   terminating NUL */
#if !defined(HAVE_STRNDUP) && !defined(HAVE__STRNDUP)
char *strndup(const char *s, size_t n)
{
  size_t slen = strlen(s);
  size_t len = (slen > n) ? n : slen;
  char *p = malloc(len + 1);
  if (!p) return NULL;
  memcpy(p, s, len);
  p[len] = '\0';
  return p;
}
#endif


/* strcasecmp() - case insensitive, string comparison */
#if !defined(HAVE_STRCASECMP) && !defined(HAVE__STRICMP)
int strcasecmp(const char *s1, const char *s2)
{
  int c1, c2;
  do {
    c1 = tolower(*s1++);
    c2 = tolower(*s2++);
  } while (c1 == c2 && c1 != 0);
  return c1 - c2;
}
#endif

/* strncasecmp() - case insensitive, length-limited string comparison */
#if !defined(HAVE_STRNCASECMP) && !defined(HAVE__STRNICMP)
int strncasecmp(const char *s1, const char *s2, size_t len)
{
  unsigned char c1, c2;
  if (!len) return 0;
  do {
    c1 = *s1++;
    c2 = *s2++;
    if (!c1 || !c2) break;
    if (c1 == c2) continue;
    c1 = tolower(c1);
    c2 = tolower(c2);
    if (c1 != c2) break;
  } while (--len);
  return (int)c1 - (int)c2;
}
#endif

/* strlcpy() - like strncpy(), but guarantees that `dst` is NUL-terminated */
#if !defined(HAVE_STRLCPY)
size_t strlcpy(char *dst, const char *src, size_t size)
{
  strncpy(dst, src, size);
  dst[size -1] = '\0';
  return strlen(dst);
}
#endif

/* strlcat() - like strncat(), but guarantees that `dst` is NUL-terminated */
#if !defined(HAVE_STRLCPY)
size_t strlcat(char *dst, const char *src, size_t size)
{
  size_t m = strlen(dst);
  size_t n = strlen(src);
  strncpy(dst + m, src, size - m);
  dst[size -1] = '\0';
  return m + n;
}
#endif



/* asnprintf() - print to allocated string */
#if !defined(HAVE_ASNPRINTF)
int rpl_asnprintf(char **buf, size_t *size, const char *fmt, ...)
{
  int n;
  va_list ap;
  va_start(ap, fmt);
  n = vasnprintf(buf, size, fmt, ap);
  va_end(ap);
  return n;
}
#endif

/* asnprintf() - print to allocated string using va_list */
#if !defined(HAVE_VASNPRINTF)
int rpl_vasnprintf(char **buf, size_t *size, const char *fmt, va_list ap)
{
  return vasnpprintf(buf, size, 0, fmt, ap);
}
#endif

/* asnprintf() - print to position in allocated string */
#if !defined(HAVE_ASNPPRINTF)
int rpl_asnpprintf(char **buf, size_t *size, size_t pos, const char *fmt, ...)
{
  int n;
  va_list ap;
  va_start(ap, fmt);
  n = vasnpprintf(buf, size, pos, fmt, ap);
  va_end(ap);
  return n;
}
#endif

/* Returns the number of the most significant bit. */
static inline int msb(int v)
{
  int n=0;
  while (v >>= 1) n++;
  return n;
}

/* Expands to `a - b` if `a > b` else to `0`. */
#define PDIFF(a, b) (((size_t)(a) > (size_t)(b)) ? (a) - (b) : 0)

/* asnprintf() - print to position in allocated string using va_list */
#if !defined(HAVE_VASNPPRINTF)
int rpl_vasnpprintf(char **buf, size_t *size, size_t pos, const char *fmt,
                    va_list ap)
{
  void *p;
  int n;
  size_t newsize;
  va_list aq;
  if (!*buf) *size = 0;
  va_copy(aq, ap);
  n = vsnprintf(*buf + pos, PDIFF(*size, pos), fmt, aq);
  va_end(aq);

  if (n < 0) return n;  /* failure */
  if (n < (int)PDIFF(*size, pos)) return n;  // success, buffer is large enough

  /* Reallocate buffer. Round up the size to the next power of two. */
  newsize = ((size_t)1) << (msb(n + pos) + 1);

  if (!(p = realloc(*buf, newsize))) return -1;
  *buf = p;
  *size = newsize;
  n = vsnprintf(*buf + pos, PDIFF(*size, pos), fmt, ap);
  return n;
}
#endif
