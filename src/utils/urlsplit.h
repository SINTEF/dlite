/* urlsplit.h -- splits an URL into its components according to RFC 3986
 *
 * Copyright (C) 2023 SINTEF
 *
 * Distributed under terms of the MIT license.
 */
#ifndef _URLSPLIT_H
#define _URLSPLIT_H

/**
  Following [RFC 3986], an URL has the general structure:

      URL = scheme ":" ["//" authority] path ["?" query] ["#" fragment]

      authority = [userinfo "@"] host [":" port]


  References
  ----------
  [RFC 3986]: https://datatracker.ietf.org/doc/html/rfc3986
  [wikipedia]: https://en.wikipedia.org/wiki/URL
*/

#include "strutils.h"

/**
  Struct filled in by urlsplit() with the start position and length of
  the different components of an url.

  The scheme always start at position zero.
 */
typedef struct {
  const char *scheme;
  int scheme_len;
  const char *authority;
  int authority_len;
  const char *userinfo;
  int userinfo_len;
  const char *host;
  int host_len;
  const char *port;
  int port_len;
  const char *path;
  int path_len;
  const char *query;
  int query_len;
  const char *fragment;
  int fragment_len;
} UrlComponents;


/**
  Returns the length of the initial segment of `url` that correspond
  to a valid URL.

  Hence, non-zero is returned if `url` corresponds to a valid URL.
 */
int isurl(const char *url);


/**
  Returns the length of the initial segment of `url` that correspond
  to a valid URL.  Hence, zero is returned if `url` is not a valid URL.

  If `components` is not NULL, the memory it points to is filled in with
  start and length information of the different components of `url`.
 */
int urlsplit(const char *url, UrlComponents *components);


/**
  Like urlsplit(), but takes an extra argument `len`.

  If `len` is positive, the `len` first characters of `url` must be a
  valid URL (in which case `len` is returned), otherwise zero is returned.

  A negative `len` means that the whole length of `url` is parsed
  (equivalent to ``len=strlen(url)``).
 */
int urlsplitn(const char *url, int len, UrlComponents *components);


/**
  Join URL components `components` and write them to buffer `buf`.

  At most `size` bytes are written to `buf` (incl. terminating NUL.

  If the 'host' field of `components` is not NULL, the authority will
  be derived from the 'username', 'host' and 'port' fields, otherwise
  the `authority` field will be used.

  Return number of bytes written to `buf`, or the number of bytes that would
  have been written to `buf` if `size` would be big enough.
  On error, a negative value is returned.
 */
int urljoin(char *buf, long size, UrlComponents *components);


/**
  Write percent-encoded copy of `src` to `buf`.

  At most `size` bytes are written (including NUL-termination).

  Return the number of bytes that would have been written to `buf`
  (excluding NUL-terminator), or would have been written to `buf` if
  it would have been large enough.
  On error a negative number is returned.
 */
int pct_encode(char *buf, long size, const char *src);

/**
  Like pct_encode(), but at most `len` bytes are read from `src`.
  If `len` is negative, all of `src` is read.
 */
int pct_nencode(char *buf, long size, const char *src, long len);

/**
  Write percent-encoded copy of the first `len` bytes of `src` to `buf`.

  At most `size` bytes are written (including NUL-termination).
  Character in categories larger than `maxcat` and not in `accepted`,
  will be percent-encoded.

  Return the number of bytes that would have been written to `buf`
  (excluding NUL-terminator), or would have been written to `buf` if
  it would have been large enough.
  On error a negative number is returned.
 */
int pct_xencode(char *buf, long size, const char *src, long len,
                StrCategory maxcat, const char *accepted);

/**
  Write percent-decoded copy of `encoded` to `buf`.

  At most `size` bytes are written (including NUL-termination).

  Return the number of bytes that would have been written to `buf`
  (excluding NUL-terminator), or would have been written to `buf` if
  it would have been large enough.
  On error a negative number is returned.
 */
int pct_decode(char *buf, long size, const char *encoded);

/**
  Like pct_decode(), but at most `len` bytes are read from `encoded`.
  If `len` is negative, all of `encoded` is read.
 */
int pct_ndecode(char *buf, long size, const char *encoded, long len);


#endif  /* _URLSPLIT_H */
