/* strutils.h -- cross-platform string utility functions
 *
 * Copyright (C) 2021 SINTEF
 *
 * Distributed under terms of the MIT license.
 */
#ifndef _STRUTILS_H
#define _STRUTILS_H

#include <stdlib.h>

/* Remove __attribute__ when we are not compiling with gcc */
#ifndef __GNUC__
# define __attribute__(x)
#endif


/** Flags for strquote() */
typedef enum _StrquoteFlags {
  strquoteInitialBlanks=1,  /*!< Do not skip initial blanks */
  strquoteNoQuote=2,        /*!< Input is not expected to start and end
                                 with double quote */
  strquoteNoEscape=4,       /*!< Do not escape embedded double quotes */
  strquoteRaw=7             /*!< Copy the input without conversions */
} StrquoteFlags;


/**
  A convinient variant of asnprintf() that returns the allocated string,
  or NULL on error.
 */
char *aprintf(const char *fmt, ...)
  __attribute__ ((__malloc__, __format__ (__printf__, 1, 2)));


/**
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
int strput(char **destp, size_t *sizep, size_t pos, const char *src);


/**
  Like strput(), but at most `n` bytes from `src` will be copied.
  If `n` is negative, all of `src` will be copited.
 */
int strnput(char **destp, size_t *sizep, size_t pos, const char *src, int n);


/**
  Double-quote input string `s` and write it to `dest`.

  Embedded double-quotes are escaped with backslash. At most size
  characters are written to `dest` (including terminating NUL).

  Returns number of characters written to `dest` (excluding
  terminating NUL).  If the output is truncated, the number of
  characters which should have been written is returned.
*/
int strquote(char *dest, size_t size, const char *s);


/**
  Like strquote(), but reads at most `n` bytes from `s`.
*/
int strnquote(char *dest, size_t size, const char *s, int n,
              StrquoteFlags flags);


/**
  Strip double-quotes from `s` and write the result to `dest`.

  At most `size` characters are written to `dest` (including
  terminating NUL).  The input `s` may optionally starts with a
  sequence of blanks.  It should then be followed by a double quote.
  The scanning stops at the next unescaped double quote.

  Returns number of characters written to `dest` (excluding
  terminating NUL).  If the output is truncated, the number of
  characters which should have been written is returned.

  Returns a negative value on error (-1 if the first non-blank
  character in `s` is not a double quote and -2 if no terminating
  double quote is found).
 */
int strunquote(char *dest, size_t size, const char *s,
               int *consumed, StrquoteFlags flags);

/**
  Like strunquote, but if `n` is non-negative, at most `n` bytes are
  read from `s`.

  This mostly make sense in combination when `flags & strquoteNoEscape`
  is true.
 */
int strnunquote(char *dest, size_t size, const char *s, int n,
               int *consumed, StrquoteFlags flags);


/**
  Writes binary data to hex-encoded string.

  `hex` destination string.  Will be NUL-terminated.
  `hexsize` size of memory poined to by `hex` (incl. NUL terminator).
  `data` points to the first byte of binary data of size `size`.
  `size` number of bytes to read from `data`.

  Returns number of bytes one wants to write to `hex` (not incl. NUL
  terminator), or -1 on error.
*/
int strhex_encode(char *hex, size_t hexsize, const unsigned char *data,
                  size_t size);



/**
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
                  int hexsize);

#endif  /* _STRUTILS_H */
