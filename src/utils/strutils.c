/* strutils.c -- cross-platform string utility functions
 *
 * Copyright (C) 2021 SINTEF
 *
 * Distributed under terms of the MIT license.
 */
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include "compat.h"
#include "strutils.h"


/*
  A convinient variant of asnprintf() that returns the allocated string,
  or NULL on error.
 */
char *aprintf(const char *fmt, ...)
{
  char *buf=NULL;
  size_t size=0;
  va_list ap;
  va_start(ap, fmt);
  vasnprintf(&buf, &size, fmt, ap);
  va_end(ap);
  return buf;
}

/*
  Copies `src` string to malloc'ed memory pointed to by `*destp`.

  The string pointed to by `*destp` may be reallocated.  It will always
  be NUL-terminated.

  If `sizep` is not NULL, the value it points to should be the allocated
  size of `*destp`.  It will be updated on return.

  `pos` is the position of `*destp` that `src` will be copied to.

  Returns number of characters written (excluding terminating NUL).
  On allocation error, a negative number is returned and `destp` and
  `sizep` will not be touched.
 */
int strput(char **destp, size_t *sizep, size_t pos, const char *src)
{
  return strnput(destp, sizep, pos, src, -1);
}

/*
  Like strput(), but at most `n` bytes from `src` will be copied.
  If `n` is negative, all of `src` will be copited.
 */
int strnput(char **destp, size_t *sizep, size_t pos, const char *src, int n)
{
  size_t size;
  char *p = *destp;
  if (n < 0) n = strlen(src);
  size = pos + n + 1;

  if (sizep) {
    size_t m = (*destp) ? *sizep : 0;
    if (!sizep) p = NULL;
    if (m < size || m > 2*size) {
      p = realloc(p, size);
    } else {
      size = *sizep;
      p = *destp;
    }
  } else {
    p = realloc(*destp, size);
  }
  if (!p) return -1;

  strncpy(p + pos, src, n);
  p[pos+n] = '\0';
  *destp = p;
  if (sizep) *sizep = size;
  return n;
}


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
  if (!size) dest = NULL;
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
  return strnunquote(dest, size, s, -1, consumed, flags);
}


/*
  Like strunquote, but if `n` is non-negative, at most `n` bytes are
  read from `s`.

  This mostly make sense in combination when `flags & strquoteNoEscape`
  is true.
 */
int strnunquote(char *dest, size_t size, const char *s, int n,
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
    if (n >= 0 && (int)j >= n) break;
    if (dest && size > i) dest[i] = s[j];
    i++;
    j++;
  }
  if (dest) dest[(size > i) ? i : size-1] = '\0';
  if (!(flags & strquoteNoQuote) && s[j++] != '"') return -2;
  if (consumed) *consumed = (n >= 0 && (int)j >= n) ? n : (int)j;
  return i;
}


/*
  Writes binary data to hex-encoded string.

  `hex` destination string.  Will be NUL-terminated.
  `hexsize` size of memory poined to by `hex` (incl. NUL terminator).
  `data` points to the first byte of binary data of size `size`.
  `size` number of bytes to read from `data`.

  Returns number of bytes written to `hex`, assuming `hexsize` is
  sufficiently large, or -1 on error.
*/
int strhex_encode(char *hex, size_t hexsize, const unsigned char *data,
                  size_t size)
{
  int n, m=0;
  size_t i;
  for (i=0; i<size; i++) {
    if ((n = snprintf(hex+m, PDIFF(hexsize, m), "%02x", data[i])) < 0) return n;
    if (n == 2 && m == (int)hexsize-2) hex[m] = '\0';
    m += n;
  }
  return m;
}


/*
  Read binary data from hex-encoded string.

  `data` pointer to buffer to write to.  No more than `size` bytes
      are written.
  `size` size of `data` in bytes.
  `hex` hex-encoded string to read from.
  `hexsize` number of bytes to read from `hex`.  If negative, `hex` is
      assumed to be NUL-terminated and the whole string is read.

  Returns number of bytes written to `data`, assuming `size` is
  sufficiently large, or -1 on error.
*/
int strhex_decode(unsigned char *data, size_t size, const char *hex,
                  int hexsize)
{
  size_t i, j;
  if (hexsize < 0) hexsize = strlen(hex);
  if (hexsize % 2) return -1;
  for (i=0; i < size && i < (size_t)hexsize/2; i++) {
    int v = 0;
    for (j=0; j<2; j++) {
      int c = tolower(hex[2*i + j]);
      v *= 16;
      if (c >= '0' && c <= '9')
        v += c - '0';
      else if (c >= 'a' && c <= 'z')
        v += c - 'a' + 10;
      else
        return -1;
    }
    data[i] = v;
  }
  return hexsize / 2;
}
