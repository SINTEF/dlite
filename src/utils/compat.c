/* compat.c -- auxiliary compatibility functions */

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* Ensure non-empty translation unit */
typedef int make_iso_compilers_happy;


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
char *strndup(const char s, size_t n)
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
