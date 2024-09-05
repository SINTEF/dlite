/* uri_encode.h -- C library for URI percent encoding/decoding
 *
 * This software is Copyright (c) 2016 by David Farrell
 *
 * Distributed under terms of the (two-clause) FreeBSD License
 *
 * See: https://github.com/dnmfarrell/URI-Encode-C
 *
 * Modified by Jesper Friis, 2024
 */
#ifndef _URI_ENCODE_H
#define _URI_ENCODE_H

#include <stdlib.h>

/**
  Percent-encode `src`, which is a buffer of length `len`, and write
  the result to `dst`.

  If `dst` is NULL, only return its expected length (minus one).

  Returns the number of bytes written to `dst`, not including the
  terminating NUL.
 */
size_t uri_encode(const char *src, const size_t len, char *dst);


/**
  Percent-decode `src`, which is a buffer of length `len`, and write
  the result to `dst`.

  If `dst` is NULL, only return its expected length (minus one).

  Returns the number of bytes written to `dst`, not including the
  terminating NUL.
 */
size_t uri_decode(const char *src, const size_t len, char *dst);

#endif  /* _URI_ENCODE_H */
