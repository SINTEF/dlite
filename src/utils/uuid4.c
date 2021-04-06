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

#include "uuid4.h"


static int seeded = 0;
static uint64_t seed[2];


static uint64_t xorshift128plus(uint64_t *s) {
  /* http://xorshift.di.unimi.it/xorshift128plus.c */
  uint64_t s1 = s[0];
  const uint64_t s0 = s[1];
  s[0] = s0;
  s1 ^= s1 << 23;
  s[1] = s1 ^ s0 ^ (s1 >> 18) ^ (s0 >> 5);
  return s[1] + s0;
}


#if RAND_MAX/256 >= 0xFFFFFFFFFFFFFF
  #define LOOP_COUNT 1
#elif RAND_MAX/256 >= 0xFFFFFF
  #define LOOP_COUNT 2
#elif RAND_MAX/256 >= 0x3FFFF
  #define LOOP_COUNT 3
#elif RAND_MAX/256 >= 0x1FF
  #define LOOP_COUNT 4
#else
  #define LOOP_COUNT 5
#endif
uint64_t rand_uint64(void) {
  uint64_t r = 0;
  for (int i=LOOP_COUNT; i > 0; i--) {
    r = r*(RAND_MAX + (uint64_t)1) + rand();
  }
  return r;
}


static int simple_seed(void) {
  unsigned int i, timeseed;
#ifdef _WIN32
  FILETIME ft;
  GetSystemTimeAsFileTime(&ft);
  timeseed = (unsigned int)(ft.dwLowDateTime + ft.dwHighDateTime);
#else
  /* FIXME - clock() returns the approximate CPU time used by the
     program (typically in ms resolution), which might be similar
     between invocations on fast CPUs. */
  timeseed = (unsigned int)clock();
#endif
  srand(timeseed);
  for (i=0; i<2; i++)
    seed[i] = rand_uint64();
  return UUID4_ESUCCESS;
}


static int init_seed(void) {
#if defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__)
  int res;
  FILE *fp = fopen("/dev/urandom", "rb");
  if (!fp) {
    return UUID4_EFAILURE;
  }
  res = fread(seed, 1, sizeof(seed), fp);
  fclose(fp);
  if ( res != sizeof(seed) ) {
#ifdef UUID4_ONLY_HIGH_QUALITY_SEED
    return UUID4_EFAILURE;
#else
    return simple_seed();
#endif
  }

#elif defined(_WIN32)
  int res;
  HCRYPTPROV hCryptProv;
  res = CryptAcquireContext(
    &hCryptProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT);
  if (!res) {
    return simple_seed();
  }
  res = CryptGenRandom(hCryptProv, (DWORD) sizeof(seed), (PBYTE) seed);
  CryptReleaseContext(hCryptProv, 0);
  if (!res) {
#ifdef UUID4_ONLY_HIGH_QUALITY_SEED
    return UUID4_EFAILURE;
#else
    return simple_seed();
#endif
  }

#else
  #error "unsupported platform"
#endif
  return UUID4_ESUCCESS;
}


int uuid4_generate(char *dst) {
  static const char *template = "xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx";
  static const char *chars = "0123456789abcdef";
  union { unsigned char b[16]; uint64_t word[2]; } s;
  const char *p;
  int i, n;
  /* seed? */
  if (!seeded) {
    do {
      int err = init_seed();
      if (err != UUID4_ESUCCESS) {
        return err;
      }
    } while (seed[0] == 0 && seed[1] == 0);
    seeded = 1;
  }
  /* get random */
  s.word[0] = xorshift128plus(seed);
  s.word[1] = xorshift128plus(seed);
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
