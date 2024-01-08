/**
 * Copyright (c) 2016 rxi
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 */

#include <stdio.h>
#include <time.h>

#ifdef HAVE_STDINT_H
# include <stdint.h>
#else
# include "integers.h"
#endif

#if defined(_WIN32)
#include <windows.h>
#include <wincrypt.h>
#endif

#include "rng.h"
#include "uuid4.h"


int uuid4_generate(char *dst) {
  static const char *template = "xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx";
  static const char *chars = "0123456789abcdef";
  static int seeded = 0;
  union { unsigned char b[16]; uint64_t word[2]; } s;
  const char *p;
  int i, n;

  /* seed? */
  if (!seeded) {
    srand_msws64(0);
    seeded = 1;
  }

  /* get random */
  s.word[0] = rand_msws64();
  s.word[1] = rand_msws64();

  /* build string */
  p = template;
  i = 0;
  while (*p) {
    n = s.b[i >> 1];
    n = (i & 1) ? (n >> 4) : (n & 0xf);
    switch (*p) {
      case 'x'  : *dst = chars[n];              i++;  break;
      case 'y'  : *dst = chars[(n & 0x3) + 8];  i++;  break;
      default   : *dst = *p;
    }
    dst++, p++;
  }
  *dst = '\0';

  /* return ok */
  return UUID4_ESUCCESS;
}
