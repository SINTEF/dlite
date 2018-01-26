/* compat.c -- auxiliary compatibility functions */

#include "config.h"

#include <stdlib.h>
#include <string.h>

/* Ensure non-empty translation unit */
typedef int make_iso_compilers_happy;


/* strdup() - duplicate a string */
#ifndef HAVE_STRDUP
# ifdef HAVE__STRDUP
#  define strdup(s) _strdup(s)
# else
char *strdup(const char *s)
{
  size_t n = strlen(s) + 1;
  char *p = malloc(n);
  if (p) memcpy(p, s, n);
  return p;
}
# endif
#endif

/* strcasecmp() - case insensitive, string comparison */
#ifndef HAVE_STRCASECMP
# ifdef HAVE__STRICMP
#  define strcasecmp(s1, s2) _stricmp(s1, s2)
# else
int strcasecmp(const char *s1, const char *s2)
{
  int c1, c2;
  do {
    c1 = tolower(*s1++);
    c2 = tolower(*s2++);
  } while (c1 == c2 && c1 != 0);
  return c1 - c2;
}
# endif
#endif

/* strncasecmp() - case insensitive, length-limited string comparison */
#ifndef HAVE_STRNCASECMP
# ifdef HAVE__STRNICMP
#  define strncasecmp(s1, s2, n) _stricmp(s1, s2, n)
# else
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
# endif
#endif
