/*
 * sha1.h
 * Copyright (C) 2016 Kurten Chan <chinkurten@gmail.com>
 *
 * Distributed under terms of the MIT license.
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

void SHA1Transform(
    uint32_t state[5],
    const unsigned char buffer[64]
    );

void SHA1Init(
    SHA1_CTX * context
    );

void SHA1Update(
    SHA1_CTX * context,
    const unsigned char *data,
    uint32_t len
    );

void SHA1Final(
    unsigned char digest[20],
    SHA1_CTX * context
    );

void SHA1(
    char *hash_out,
    const char *str,
    int len);

#endif /* !__sha1_h__ */
