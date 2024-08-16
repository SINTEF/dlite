/* strutils.h -- cross-platform string utility functions
 *
 * Copyright (C) 2021-2023 SINTEF
 *
 * Distributed under terms of the MIT license.
 */
/**
  @file
  @brief Cross-platform string utility functions
*/
#ifndef _STRUTILS_H
#define _STRUTILS_H

#include <stdlib.h>

/* Remove __attribute__ when we are not compiling with gcc */
#ifndef __GNUC__
# define __attribute__(x)
#endif

/**
 * @name Typedefs and structs
 */
/** @{ */

/** Flags for strquote() */
typedef enum _StrquoteFlags {
  strquoteInitialBlanks=1,  /*!< Do not skip initial blanks */
  strquoteNoQuote=2,        /*!< Input is not expected to start and end
                                 with double quote */
  strquoteNoEscape=4,       /*!< Do not escape embedded double quotes */
  strquoteRaw=7             /*!< Copy the input without conversions */
} StrquoteFlags;


/** Character categories, from, RFC 3986 */
typedef enum {
  strcatUpper,       //!< A-Z
  strcatLower,       //!< a-z
  strcatDigit,       //!< 0-9
  strcatUnreserved,  //!< "-._~"  (in addition to upper + lower + digit)
  strcatSubDelims,   //!< "!$&'()*+,;="
  strcatGenDelims,   //!< ":/?#[]@"
  strcatReserved,    //!< strcatSubDelims | strcatGenDelims
  strcatPercent,     //!< "%"
  strcatCExtra,      //!< "\"\\<>^{}|" (extra characters in the C standard)
  strcatSpace,       //!< " \f\n\r\t\v"
  strcatOther,       //!< anything else, except NUL
  strcatNul,         //!< NUL
} StrCategory;


/** @} */
/**
 * @name Print allocated string
 */
/** @{ */

/**
  A convinient variant of asprintf() that returns the allocated string,
  or NULL on error.
 */
char *aprintf(const char *fmt, ...)
  __attribute__ ((__malloc__, __format__ (__printf__, 1, 2)));

/** @} */
/**
 * @name Functions for writing characters to a buffer
 */
/** @{ */

/**
  Writes character `c` to buffer `dest` of size `size`.  If there is
  space, the buffer will always be NUL-terminated.

  Returns always 1 (number of characters written to `dest`, or would
  have been written to `dest` if it had been large enough).
 */
int strsetc(char *dest, long size, int c);

/**
  Copies `src` to `dest`.

  At most `size` bytes will be written to `dest`.
  If `size` is larger than zero, `dest` will always be NUL-terminated.
  No, partly UTF-8 code point will be written to dest.

  Returns number of bytes written to `dest` or the number of bytes that
  would have been written to `dest` if it had been large enough.
 */
int strsets(char *dest, long size, const char *src);

/**
  Like strset(), but copies at most `len` bytes from `src`.
 */
int strsetn(char *dest, long size, const char *src, int len);


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
  Like strnput(), but escapes all characters in categories larger than
  `unescaped`, which should be less than `strcatOther`.

  Escaped characters are written as `escape` followed by 2-character hex
  representation of the character (byte) value.

  Returns -1 on error.
 */
int strnput_escape(char **destp, size_t *sizep, size_t pos,
                   const char *src, int len,
                   StrCategory unescaped, const char *escape);


/** @} */
/**
 * @name Quoting/unquoting strings
 */
/** @{ */

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
  Like strnunquote(), but reallocates the destination and writes to
  position `pos`.

  On allocation error, -3 is returned.
 */
int strnput_unquote(char **destp, size_t *sizep, size_t pos, const char *s,
                    int n, int *consumed, StrquoteFlags flags);


/** @} */
/**
 * @name Hexadecimal encoding/decoding
 */
/** @{ */

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


/** @} */
/**
 * @name Character categorisation
 */
/** @{ */


/**
  Returns the category of character `c`.
 */
StrCategory strcategory(int c);

/**
  Returns the length of initial segment of `s` which concists entirely of
  bytes in category `cat`.
 */
int strcatspn(const char *s, StrCategory cat);

/**
  Returns the length of initial segment of `s` which concists entirely of
  bytes NOT in category `cat`.
 */
int strcatcspn(const char *s, StrCategory cat);

/**
  Returns the length of initial segment of `s` which concists entirely of
  bytes in all categories less or equal to `cat`.
 */
int strcatjspn(const char *s, StrCategory cat);

/**
  Returns the length of initial segment of `s` which concists entirely of
  bytes NOT in all categories less or equal to `cat`.
 */
int strcatcjspn(const char *s, StrCategory cat);


/** @} */
/**
 * @name Allocated string list
 *
 * A string list is an allocated NULL-terminated array of pointers to
 * allocated strings.
 */
/** @{ */

/**
  Insert string `s` before position `i` in NULL-terminated array of
  string pointers `strlst`.

  If `i` is negative count from the end of the string, like Python.
  Any `i` out of range correspond to appending.

  `n` is the allocated length of `strlst`.  If needed `strlst` will be
  reallocated and `n` updated.

  Returns a pointer to the new string list or NULL on allocation error.
 */
char **strlst_insert(char **strlst, size_t *n, const char *s, int i);

/**
  Appends string `s` to NULL-terminated array of string pointers `strlst`.
  `n` is the allocated length of `strlst`.  If needed `strlst` will be
  reallocated and `n` updated.

  Returns a pointer to the new string list or NULL on allocation error.
 */
char **strlst_append(char **strlst, size_t *n, const char *s);

/** Return number of elements in string list. */
size_t strlst_count(char **strlst);

/** Free all memory in string list. */
void strlst_free(char **strlst);

/**
  Returns a pointer to element `i` the string list. Like in Python,
  negative `i` counts from the back.

  The caller gets a borrowed reference to the string. Do not free it.

  Returns NULL if `i` is out of range.
*/
const char *strlst_get(char **strlst, int i);

/**
  Remove element `i` from the string list. Like in Python, negative `i`
  counts from the back.

  Returns non-zero if `i` is out of range.
*/
int strlst_remove(char **strlst, int i);

/**
  Remove and return element `i` from the string list. Like in Python,
  negative `i` counts from the back.

  The caller becomes the owner of the returned string and is
  responsible to free it.

  Returns NULL if `i` is out of range.
*/
char *strlst_pop(char **strlst, int i);

/**
  A version of atoi() that reads at most `n` bytes.
 */
int natoi(const char *s, int n);

/**
  Checks if `v` points to a valid semantic version 2.0.0 number.

  Returns -1 if `v` is not a valid semantic version number.
  Otherwise, the length of the version number is returned.
 */
int strchk_semver(const char *v);

/**
  Check if the initial part of `v` is a valid semantic version 2.0.0 number.

  Only the first `n` bytes of `v` are checked.

  Returns the length of the semantic version number or -1 if `v` is
  not a valid semantic version number.
*/
int strnchk_semver(const char *v, size_t n);

/**
  Compare strings `v1` and `v2` using semantic versioning 2.0.0 order.

  Returns -1 if v1 < v2
           0 if v1 == v2
           1 if v1 > v2

  See also: https://semver.org/
 */
int strcmp_semver(const char *v1, const char *v2);

/**
  Like strcmp_version(), but compares only the first `n` bytes of `v1` and `v2`.
*/
int strncmp_semver(const char *v1, const char *v2, size_t n);


/** @} */

#endif  /* _STRUTILS_H */
