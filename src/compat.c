/* compat.c -- auxiliary compatibility functions */

#include "config.h"

#include <stdlib.h>
#include <string.h>

/* ensure non-empty translation unit */
typedef int make_iso_compilers_happy;


/* strdup */
#ifndef HAVE_STRDUP
# ifdef HAVE__STRDUP
#  define strdup _strdup
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
