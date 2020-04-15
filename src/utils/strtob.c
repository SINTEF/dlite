/* strtob.c -- converts string to boolean
 *
 * Copyright (c) 2018-2020, SINTEF
 *
 * Distributed under terms of the MIT license.
 *
 */
#include "compat.h"
#include <stdlib.h>
#include <string.h>

/* Converts the initial part of the string `ptr` to a boolean. */
int strtob(const char *ptr, char **endptr)
{
  char *truevals[] = {"1", "true", ".true.", "yes", "on", NULL};
  char *falsevals[] = {"0", "false", ".false.", "no", "off", NULL};
  char **q, *p = (char *)ptr;

  if (!p || !*p) return 0;
  p += strspn(p, " \t\n\v\f\r");
  for (q=truevals; *q; q++) {
    size_t n = strlen(*q);
    if (strncasecmp(*q, p, n) == 0) {
      if (endptr) *endptr = p + n;
      return 1;
    }
  }
  for (q=falsevals; *q; q++) {
    size_t n = strlen(*q);
    if (strncasecmp(*q, p, n) == 0) {
      if (endptr) *endptr = p + n;
      return 0;
    }
  }
  if (*p) p++;
  if (endptr) *endptr = p + 1;
  return -1;
}


/* Converts a string to true (1) or false (0). */
int atob(const char *ptr)
{
  return strtob(ptr, NULL);
}
