/* strutils.c -- cross-platform string utility functions
 *
 * Copyright (C) 2021 SINTEF
 *
 * Distributed under terms of the MIT license.
 */
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include "integers.h"
#include "compat.h"
#include "clp2.h"
#include "strutils.h"


#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))


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


/********************************************************************
 *  Functions for writing characters to a buffer
 ********************************************************************/

/*
  Writes character `c` to buffer `dest` of size `size`.

  If `c` is larger than 127 and a valid UTF-8 code point, it will only
  be written if there is space enough to write out the code point fully.

  If there is space, the buffer will always be NUL-terminated.

  Returns always 1 (corresponding number of characters written to
  `dest`, or would have been written to `dest` if it had been large
  enough).
 */
int strsetc(char *dest, long size, int c)
{
  if (size >= 2) {
    if (c < 0) {
      /* This may happen if `c` is a character larger than 127 that is
         casted to char. */
      unsigned char v = (unsigned char)c;
      if (v < 127)  // how did this happen?
        dest[0] = v;
      else if ((v & 0xc0) == 0x80)
        dest[0] = v;
      else if ((v & 0xe0) == 0xc0)
        dest[0] = (size >= 3) ? v : '\0';
      else if ((v & 0xf0) == 0xe0)
        dest[0] = (size >= 4) ? v : '\0';
      else if ((v & 0xf8) == 0xf0)
        dest[0] = (size >= 5) ? v : '\0';
      else
        dest[0] = v;  // fallback - not utf-8 encoded
    } else {
      if (c <= 127)
        dest[0] = c;
      else if (c <= 255) {
        if ((c & 0xc0) == 0x80)
          dest[0] = c;
        else if ((c & 0xe0) == 0xc0)
          dest[0] = (size >= 3) ? c : '\0';
        else if ((c & 0xf0) == 0xe0)
          dest[0] = (size >= 4) ? c : '\0';
        else if ((c & 0xf8) == 0xf0)
          dest[0] = (size >= 5) ? c : '\0';
        else
          dest[0] = c;  // fallback - not utf-8 encoded
      } else if ((c & 0xffffffc0) == 0x80)
        ((unsigned char *)dest)[0] = c;
      else if ((c & 0xffffe0c0) == 0xc080)
        ((unsigned char *)dest)[0] = (size >= 3) ? (c >> 8) & 0xff : '\0';
      else if ((c & 0xffffc0c0) == 0x8080)
        ((unsigned char *)dest)[0] = c >> 8;
      else if ((c & 0xfff0c0c0) == 0xe08080)
        ((unsigned char *)dest)[0] = (size >= 4) ? (c >> 16) & 0xff : '\0';
      else if ((c & 0xffc0c0c0) == 0x808080)
        ((unsigned char *)dest)[0] = c >> 16;
      else if ((c & 0xf8c0c0c0) == 0xf0808080)
        ((unsigned char *)dest)[0] = (size >= 5) ? (c >> 24) & 0xff : '\0';
      else
        ((unsigned char *)dest)[0] = c;  // fallback - not utf-8 encoded
    }
    dest[1] = '\0';
  } else if (size >= 1)
    dest[0] = '\0';
  return 1;
}


/*
  Copies `src` to `dest`.

  At most `size` bytes will be written to `dest`.
  If `size` is larger than zero, `dest` will always be NUL-terminated.
  No, partly UTF-8 code point will be written to dest.

  Returns number of bytes written to `dest` or the number of bytes that
  would have been written to `dest` if it had been large enough.
 */
int strsets(char *dest, long size, const char *src)
{
  int n=0;
  while (*src)
    n += strsetc(dest+n, size-n, *(src++));
  return n;
}

/*
  Like strset(), but copies at most `len` bytes from `src`.
 */
int strsetn(char *dest, long size, const char *src, int len)
{
  int i, n=0;
  if (len < 0) len = strlen(src);
  for (i=0; i<len; i++)
    n += strsetc(dest+n, size-n, src[i]);
  return n;
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
  Like strput(), but at most `len` bytes from `src` will be copied.
  If `len` is negative, all of `src` will be copited.
 */
int strnput(char **destp, size_t *sizep, size_t pos, const char *src, int len)
{
  size_t size;
  char *p = *destp;
  if (len < 0) len = strlen(src);
  size = pos + len + 1;

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

  strncpy(p + pos, src, len);
  p[pos+len] = '\0';
  *destp = p;
  if (sizep) *sizep = size;
  return len;
}



/*
  Like strnput(), but escapes all characters in categories larger than
  `unescaped`, which should be less than `strcatOther`.

  Escaped characters are written as `escape` followed by 2-character hex
  representation of the character (byte) value.

  Returns -1 on error.
 */
int strnput_escape(char **destp, size_t *sizep, size_t pos,
                   const char *src, int len,
                   StrCategory unescaped, const char *escape)
{
  size_t size = (*destp) ? *sizep : 0;
  size_t n=pos, esclen=strlen(escape);
  char *p = *destp;
  int m, i=0;
  int escape_size = esclen + 3;
  if (unescaped >= strcatOther) return -1;
  if (len < 0) len = strlen(src);

  while (i < len && src[i]) {
    if (size < n+escape_size) {
      size = clp2(n+escape_size + len);
      if (!(p = realloc(p, size))) return -1;
      *destp = p;
      *sizep = size;
    }
    if (strcategory(src[i]) <= unescaped) {
      p[n++] = src[i++];
      p[n] = '\0';
    } else {
      if ((m = snprintf(p+n, size-n, "%s%02X", escape,
                        (unsigned char)(src[i++]))) < 0) return -1;
      n += m;
    }
  }
  assert(n < size);
  return n;
}


/********************************************************************
 *  Quoting/unquoting strings
 ********************************************************************/

/* Expands to `a - b` if `a > b` else to `0`. */
#define PDIFF(a, b) (((size_t)(a) > (size_t)(b)) ? (a) - (b) : 0)


/*
  Double-quote input string `s` and write it to `dest`.

  Embedded double-quotes are escaped with backslash. At most `size`
  characters are written to `dest` (including terminating NUL).

  If `s` is NULL, this function will behave similarly to snprintf(),
  except for the added quoting marks.

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

  /* Write initial quote sign to `dest`. */
  if (!(flags & strquoteNoQuote)) {
    if (size > i) dest[i] = '"';
    i++;
  }

  if (s) {

    /* Loop over each character in source `s` and copy it to `dest`.
       If `n` is positive, consume at most `n` characters from `s`. */
    while (s[j] && (n < 0 || (int)j < n)) {

      /* Add backslash (escape) in front of double-quote characters
         when copying to `dest` */
      if (s[j] == '"' && !(flags & strquoteNoEscape)) {
        if (size > i) dest[i] = '\\';
        i++;
      }
      if (size > i) dest[i] = s[j];
      i++;
      j++;
    }

  } else {

    /* If `s` is NULL, use system snprintf() to represent it in a standard
       way. */
    int m = snprintf(dest+i, PDIFF(size, i), "%s", s);
    if (m >= 0) i += j;
  }

  /* Write final quote sign to `dest`. */
  if (!(flags & strquoteNoQuote)) {
    if (dest && size > i) dest[i] = '"';
    i++;
  }

  /* Ensure that `dest` is NUL-terminated. */
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
  Like strunquote(), but if `n` is non-negative, at most `n` bytes are
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
  Like strnunquote(), but reallocates the destination and writes to
  position `pos`.

  On allocation error, -3 is returned.
 */
int strnput_unquote(char **destp, size_t *sizep, size_t pos, const char *s,
                    int n, int *consumed, StrquoteFlags flags)
{
  int m;
  /* Ensure consistency */
  if (!*destp) *sizep = 0;
  if (!*sizep) *destp = NULL;

  /* Use strnunquote() to get now much memory we need. */
  m = strnunquote(NULL, 0, s, n, consumed, flags);
  if (m < 0) return m;  // On error, pass it on...

  /* If the allocated size is not large enough, reallocate `*destp` to the
     needed size. */
  if (m + pos >= *sizep) {
    char *q;
    size_t size = m + pos + 1;
    if (!(q = realloc(*destp, size))) return -3;
    *destp = q;
    *sizep = size;
  }

  /* Use strnunquote() again to write the allocated buffer. */
  m = strnunquote(*destp+pos, PDIFF(*sizep, pos), s, n, consumed, flags);
  assert(m >= 0);            // we don't expect any errors now
  assert(m + pos < *sizep);  // the buffer should be large enough
  return m;
}


/********************************************************************
 *  Hexadecimal encoding/decoding
 ********************************************************************/

/*
  Writes binary data to hex-encoded string.

  `hex` destination string.  Will be NUL-terminated.
  `hexsize` size of memory poined to by `hex` (incl. NUL terminator).
  `data` points to the first byte of binary data of size `size`.
  `size` number of bytes to read from `data`.

  Returns number of bytes one wants to write to `hex` (not incl. NUL
  terminator), or -1 on error.
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
      else if (c >= 'a' && c <= 'f')
        v += c - 'a' + 10;
      else
        return -1;
    }
    data[i] = v;
  }
  return hexsize / 2;
}


/********************************************************************
 *  Character categorisation
 ********************************************************************/

/*
  Returns the category (from RFC 3986) of character `c`.
 */
StrCategory strcategory(int c)
{
  switch (c) {
  case 'A':
  case 'B':
  case 'C':
  case 'D':
  case 'E':
  case 'F':
  case 'G':
  case 'H':
  case 'I':
  case 'J':
  case 'K':
  case 'L':
  case 'M':
  case 'N':
  case 'O':
  case 'P':
  case 'Q':
  case 'R':
  case 'S':
  case 'T':
  case 'U':
  case 'V':
  case 'W':
  case 'X':
  case 'Y':
  case 'Z':
    return strcatUpper;
  case 'a':
  case 'b':
  case 'c':
  case 'd':
  case 'e':
  case 'f':
  case 'g':
  case 'h':
  case 'i':
  case 'j':
  case 'k':
  case 'l':
  case 'm':
  case 'n':
  case 'o':
  case 'p':
  case 'q':
  case 'r':
  case 's':
  case 't':
  case 'u':
  case 'v':
  case 'w':
  case 'x':
  case 'y':
  case 'z':
    return strcatLower;
  case '0':
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
  case '6':
  case '7':
  case '8':
  case '9':
    return strcatDigit;
  case '-':
  case '.':
  case '_':
  case '~':
    return strcatUnreserved;
  case '!':
  case '$':
  case '&':
  case '\'':
  case '(':
  case ')':
  case '*':
  case '+':
  case ',':
  case ';':
  case '=':
    return strcatSubDelims;  // reserved
  case ':':
  case '/':
  case '?':
  case '#':
  case '[':
  case ']':
  case '@':
    return strcatGenDelims;  // reserved
  case '%':
    return strcatPercent;
  case '"':
  case '\\':
  case '<':
  case '>':
  case '^':
  case '{':
  case '}':
  case '|':
    return strcatCExtra;  // characters in the C standard not listed above
  case ' ':
  case '\f':
  case '\n':
  case '\r':
  case '\t':
  case '\v':
    return strcatSpace;  // confirms to isspace()
  case 0:
    return strcatNul;
  default:
    return strcatOther;
  }
}

/*
  Returns the length of initial segment of `s` which concists entirely of
  bytes in category `cat`.
 */
int strcatspn(const char *s, StrCategory cat)
{
  int n=0;
  while (s[n] && strcategory(s[n]) == cat) n++;
  return n;
}

/*
  Returns the length of initial segment of `s` which concists entirely of
  bytes NOT in category `cat`.
 */
int strcatcspn(const char *s, StrCategory cat)
{
  int n=0;
  while (s[n] && strcategory(s[n]) != cat) n++;
  return n;
}

/*
  Returns the length of initial segment of `s` which concists entirely of
  bytes in all categories less or equal to `cat`.
 */
int strcatjspn(const char *s, StrCategory cat)
{
  int n=0;
  while (s[n] && strcategory(s[n]) <= cat) n++;
  return n;
}

/*
  Returns the length of initial segment of `s` which concists entirely of
  bytes NOT in all categories less or equal to `cat`.
 */
int strcatcjspn(const char *s, StrCategory cat)
{
  int n=0;
  while (s[n] && strcategory(s[n]) > cat) n++;
  return n;
}


/********************************************************************
 * Allocated string lists
 *
 * A string list is an allocated NULL-terminated array of pointers to
 * allocated strings.
 ********************************************************************/

#define CHUNK_SIZE 32


/* Help function. Like strlst_insert(), but takes `len`, the number of
   elements in the list as an additional argument. */
static char **_strlst_insert(char **strlst, size_t *n, const char *s, int i,
                             size_t len)
{
  char *str, **q=strlst;
  size_t m = (strlst) ? *n : 0;
  int j;

  if (!(str = strdup(s))) return NULL;

  if (!strlst || !*n) {
    m = CHUNK_SIZE;
    if (!(q = calloc(m, sizeof(char *)))) goto fail;
  } else if (m < len+1) {
    m += CHUNK_SIZE;
    if (!(q = realloc(strlst, m*sizeof(char *)))) goto fail;
  }
  assert(m > len+1);

  if (i < 0) i += len;
  if (i < 0 || i > (int)len) i = len;
  for (j=len; j>i; j--) q[j] = q[j-1];
  q[i] = str;
  q[++len] = NULL;

  *n = m;
  return q;
 fail:
  free(str);
  return NULL;
}


/*
  Insert string `s` before position `i` in NULL-terminated array of
  string pointers `strlst`.

  If `i` is negative count from the end of the string, like Python.
  Any `i` out of range correspond to appending.

  `n` is the allocated length of `strlst`.  If needed `strlst` will be
  reallocated and `n` updated.

  Returns a pointer to the new string list or NULL on allocation error.
 */
char **strlst_insert(char **strlst, size_t *n, const char *s, int i)
{
  return _strlst_insert(strlst, n, s, i, strlst_count(strlst));
}

/*
  Appends string `s` to NULL-terminated array of string pointers `strlst`.
  `n` is the allocated length of `strlst`.  If needed `strlst` will be
  reallocated and `n` updated.

  Returns a pointer to the new string list or NULL on allocation error.
 */
char **strlst_append(char **strlst, size_t *n, const char *s)
{
  size_t len = strlst_count(strlst);
    return _strlst_insert(strlst, n, s, len, len);
}

/* Return number of elements in string list. */
size_t strlst_count(char **strlst)
{
  char **p=strlst;
  size_t n;
  if (!p) return 0;
  for (n=0; *p; n++) p++;
  return n;
}

/* Free all memory in string list. */
void strlst_free(char **strlst)
{
  char **p=strlst;
  if (!strlst) return;
  while (*p) free(*(p++));
  free(strlst);
}

/*
  Returns a pointer to element `i` the string list. Like in Python,
  negative `i` counts from the back.

  The caller gets a borrowed reference to the string. Do not free it.

  Returns NULL if `i` is out of range.
*/
const char *strlst_get(char **strlst, int i)
{
  int j;
  if (i < 0) i += strlst_count(strlst);
  if (i < 0) return NULL;
  for (j=0; j<i; j++) if (!strlst[j]) return NULL;
  return strlst[i];
}

/*
  Remove element `i` from the string list. Like in Python, negative `i`
  counts from the back.

  Returns non-zero if `i` is out of range.
*/
int strlst_remove(char **strlst, int i)
{
  int j;
  if (i < 0) i += strlst_count(strlst);
  if (i < 0) return 1;
  for (j=0; j<=i; j++) if (!strlst[j]) return 1;
  free(strlst[i]);
  for (j=i; strlst[j]; j++) strlst[j] = strlst[j+1];
  return 0;
}

/*
  Remove and return element `i` from the string list. Like in Python,
  negative `i` counts from the back.

  The caller becomes the owner of the returned string and is
  responsible to free it.

  Returns NULL if `i` is out of range.
*/
char *strlst_pop(char **strlst, int i)
{
  int j;
  char *p;
  if (i < 0) i += strlst_count(strlst);
  if (i < 0) return NULL;
  for (j=0; j<=i; j++) if (!strlst[j]) return NULL;
  p = strlst[i];
  for (j=i; strlst[j]; j++) strlst[j] = strlst[j+1];
  return p;
}


/*
  A version of atoi() that reads at most `n` bytes.

  Returns zero if `n` is negative.
 */
int natoi(const char *s, int n)
{
  int x=0, sign=1;
  while (isspace(*s)) s++, n--;
  if (*s == '-') {
    sign = -1;
    s++, n--;
  }
  if (*s == '+') s++;
  while(isdigit(*s) && n-- > 0) {
    x = 10*x + (*s - '0');
    s++;
  }
  return sign*x;
}


/*
  Checks if `v` points to a valid semantic version 2.0.0 number.

  Returns -1 if `v` is not a valid semantic version number.
  Otherwise, the length of `v` is returned.
 */
int strchk_semver(const char *v)
{
  int n = strlen(v);
  int m = strnchk_semver(v, n);
  if (m != n) return -1;
  return m;
}

/*
  Check if the initial part of `v` is a valid semantic version 2.0.0 number.

  Only the first `n` bytes of `v` are checked.

  Returns the length of the semantic version number or -1 if `v` is
  not a valid semantic version number.
*/
int strnchk_semver(const char *v, size_t n)
{
  size_t m=0;
  /* Major number */
  if (n < 5) return -1;
  if (!isdigit(v[m])) return -1;
  if (v[0] == '0' && isdigit(v[1])) return -1;
  while (isdigit(v[m]) && m < n) m++;
  /* Minor number */
  if (v[m] != '.') return -1;
  if (m < n) m++;
  if (!isdigit(v[m])) return -1;
  if (n >= m+2 && v[m] == '0' && isdigit(v[m+1])) return -1;
  while (isdigit(v[m]) && m < n) m++;
  /* Patch number */
  if (v[m] != '.') return -1;
  if (m < n) m++;
  if (!isdigit(v[m])) return -1;
  if (n >= m+2 && v[m] == '0' && isdigit(v[m+1])) return -1;
  while (isdigit(v[m]) && m < n) m++;
  /* Pre-release */
  if (m == n) return m;
  if (v[m] == '-') {
    do {
      if (n <= ++m) return -1;
      if (!isalnum(v[m]) && v[m] != '-') return -1;
      while ((isalnum(v[m]) || v[m] == '-') && m < n) m++;
    } while (v[m] == '.');
  }
  /* Build */
  if (m == n) return m;
  if (v[m] == '+') {
    do {
      if (n <= ++m ) return -1;
      if (!isalnum(v[m]) && v[m] != '-') return -1;
      while ((isalnum(v[m]) || v[m] == '-') && m < n) m++;
    } while (v[m] == '.');
  }
  if (m < n && v[m]) return -1;
  return m;
}


/*
  Compare strings `v1` and `v2` using semantic versioning 2.0.0 order.

  Returns
      -1 if v1 < v2
       0 if v1 == v2
       1 if v1 > v2
  If `v1` or `v2` are not valid semantic version numbers, the return value
  is undefined.

  See also: https://semver.org/
 */
int strcmp_semver(const char *v1, const char *v2)
{
  int n1 = strlen(v1);
  int n2 = strlen(v2);
  return strncmp_semver(v1, v2, MAX(n1, n2));
}


/*
  Like strcmp_semver(), but compares only the first `n` bytes of `v1` and `v2`.
*/
int strncmp_semver(const char *v1, const char *v2, size_t n)
{
  int d, pr1=0, pr2=0;
  size_t m, n1=0, n2=0;
  /* Compare major number */
  d = natoi(v1, n) - natoi(v2, n);
  if (d) return (d < 0) ? -1 : 1;
  while (isdigit(v1[n1]) && n1 < n) n1++;
  while (isdigit(v2[n2]) && n2 < n) n2++;
  /* Compare minor number */
  if (n1 < n) n1++;
  if (n2 < n) n2++;
  d = natoi(v1+n1, n-n1) - natoi(v2+n2, n-n2);
  if (d) return (d < 0) ? -1 : 1;
  while (isdigit(v1[n1]) && n1 < n) n1++;
  while (isdigit(v2[n2]) && n2 < n) n2++;
  /* Compare patch number */
  if (n1 < n) n1++;
  if (n2 < n) n2++;
  d = natoi(v1+n1, n-n1) - natoi(v2+n2, n-n2);
  if (d) return (d < 0) ? -1 : 1;
  while (isdigit(v1[n1]) && n1 < n) n1++;
  while (isdigit(v2[n2]) && n2 < n) n2++;
  assert(n1 == n2);
  m = n1;
  /* Compare pre-release */
  if (m > n) return 0;
  if (v1[m] == '-') pr1 = 1;
  if (v2[m] == '-') pr2 = 1;
  if (!pr1 && pr2) return -1;
  if (pr1 && !pr2) return 1;
  if (pr1 && pr2) {
    do {
      size_t d1, d2;
      if (m < n) m++;
      for (n1=m; (isalnum(v1[n1]) || v1[n1] == '-') && n1 < n; n1++);
      for (n2=m; (isalnum(v2[n2]) || v2[n2] == '-') && n2 < n; n2++);
      for (d1=m; (isdigit(v1[d1])) && d1 < n; d1++);
      for (d2=m; (isdigit(v2[d2])) && d2 < n; d2++);
      if (d1 == n1 && d2 == n2) {
        d = natoi(v1+m, n-m) - natoi(v2+m, n-m);
      } else {
        d = strncmp(v1+m, v2+m, MIN(n1, n2)-m);
        if (d == 0) d = n1 - n2;
      }
      if (d) return (d < 0) ? -1 : 1;
      assert(n1 == n2);
      m = n1;
    } while (v1[n1] == '.' && v2[n2] == '.');
  }
  return 0;
}
