/* strutils.c -- cross-platform string utility functions
 *
 * Copyright (C) 2021 SINTEF
 *
 * Distributed under terms of the MIT license.
 */
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include "strutils.h"


/* Expands to `a - b` if `a > b` else to `0`. */
#define PDIFF(a, b) (((size_t)(a) > (size_t)(b)) ? (a) - (b) : 0)


/*
  Double-quote input string `s` and write it to `dest`.

  Embedded double-quotes are escaped with backslash. At most `size`
  characters are written to `dest` (including terminating NUL).

  Returns number of characters written to `dest` (excluding
  terminating NUL).  If the output is truncated, the number of
  characters which should have been written is returned.
*/
int strquote(char *dest, size_t size, const char *s)
{
  return strnquote(dest, size, s, -1, 0);
}


/*
  Like strquote(), but reads at most `n` bytes from `s`.
  If `n` is negative, this function works like strquote().
*/
int strnquote(char *dest, size_t size, const char *s, int n,
              StrquoteFlags flags)
{
  size_t i=0, j=0;
  if (!(flags & strquoteNoQuote)) {
    if (size > i) dest[i] = '"';
    i++;
  }
  while (s[j] && (n < 0 || (int)j < n)) {
    if (s[j] == '"' && !(flags & strquoteNoEscape)) {
      if (size > i) dest[i] = '\\';
      i++;
    }
    if (size > i) dest[i] = s[j];
    i++;
    j++;
  }
  if (!(flags & strquoteNoQuote)) {
    if (dest && size > i) dest[i] = '"';
    i++;
  }
  if (dest) dest[(size > i) ? i : size-1] = '\0';
  return i;
}


/*
  Strip double-quotes from `s` and write the result to `dest`.

  At most `size` characters are written to `dest` (including
  terminating NUL).  The input `s` may optionally starts with a
  sequence of blanks.  It should then be followed by a double quote.
  The scanning stops at the next unescaped double quote.

  If `consumed` is not NULL, the number of consumed characters are
  stored in the interger it points to.

  Returns number of characters written to `dest` (excluding
  terminating NUL).  If the output is truncated, the number of
  characters which should have been written is returned.

  Returns a negative value on error (-1 if the first non-blank
  character in `s` is not a double quote and -2 if no terminating
  double quote is found).
 */
int strunquote(char *dest, size_t size, const char *s,
               int *consumed, StrquoteFlags flags)
{
  size_t i=0, j=0;
  if (!dest) size = 0;
  if (!size) dest = NULL;
  if (!(flags && strquoteInitialBlanks))
    while (isspace(s[j])) j++;
  if (!(flags & strquoteNoQuote) && s[j++] != '"') return -1;
  while (s[j] && ((flags & strquoteNoQuote) || s[j] != '"')) {
    if (!(flags & strquoteNoEscape) && s[j] == '\\' && s[j+1] == '"') j++;
    if (dest && size > i) dest[i] = s[j];
    i++;
    j++;
  }
  if (dest) dest[(size > i) ? i : size-1] = '\0';
  if (!(flags & strquoteNoQuote) && !s[j++]) return -2;
  if (consumed) *consumed = j;
  return i;
}
