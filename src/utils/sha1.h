/*
 * sha1.h
 * Copyright (C) 2016 Kurten Chan <chinkurten@gmail.com>
 *
 * Distributed under terms of the MIT license.
 */
/*
 * Minor modifications by Jesper Friis (2017, 2020)
 */
#ifndef __sha1_h__
#define __sha1_h__

#ifdef HAVE_STDINT_H
# include <stdint.h>
#else
# include "integers.h"
#endif

/*
   SHA-1 in C
   By Steve Reid <steve@edmweb.com>
   100% Public Domain
 */

typedef struct
{
    uint32_t state[5];
    uint32_t count[2];
    unsigned char buffer[64];
} SHA1_CTX;


/** Initialize new context */
void SHA1Init(
    SHA1_CTX * context
    );

/** Updates `context` with `data` or length `len`. */
void SHA1Update(
    SHA1_CTX * context,
    const unsigned char *data,
    uint32_t len
    );

/** Writes digest to `digest`. */
void SHA1Final(
    unsigned char digest[20],
    SHA1_CTX * context
    );

/** Convenience function. Writes digest of `str` (of size `len`) to
    `hash_out`.  `hash_out` must be a string with length of at least 21
    bytes. */
void SHA1(
    char *hash_out,
    const char *str,
    int len);

/** Convenience function. Returns malloc'ed digest as a NUL-terminated
    hex string. */
char *SHA1String(
     SHA1_CTX *context);

#endif /* !__sha1_h__ */
