/* strutils.h -- cross-platform string utility functions
 *
 * Copyright (C) 2021 SINTEF
 *
 * Distributed under terms of the MIT license.
 */
#ifndef _STRUTILS_H
#define _STRUTILS_H


/** Flags for strquote() */
typedef enum _StrquoteFlags {
  strquoteInitialBlanks=1,  /*!< Do not skip initial blanks */
  strquoteNoQuote=2,        /*!< Input is not expected to start and end
                                 with double quote */
  strquoteNoEscape=4,       /*!< Do not escape embedded double quotes */
  strquoteRaw=7             /*!< Copy the input without conversions */
} StrquoteFlags;



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


#endif  /* _STRUTILS_H */
