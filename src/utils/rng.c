/* rng.c - pseudo random number generators
 *
 * Copyright (C) 2023 SINTEF
 *
 * Distributed under terms of the MIT license.
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <stdio.h>

#if defined(HAVE_BCryptGenRandom)
# include <bcrypt.h>
#elif defined(HAVE_GetSystemTime)
# include <windows.h>
#elif defined(HAVE_CLOCK) && defined(HAVE_CLOCK_T)
# include <time.h>
#endif

#include "rng.h"


#define MIN(a, b) (((a) < (b)) ? (a) : (b))


/* Fill `buf` of size `size` with random bytes using seed. */
static void random_bytes(void *buf, size_t size, uint32_t seed)
{
  uint32_t *p = buf;
  MSWS32State state;
  srand_msws32_r(&state, seed);

  do {
    uint32_t v = rand_msws32_r(&state);
    memcpy(p++, &v, MIN(sizeof(uint32_t), size));
    size -= sizeof(uint32_t);
  } while (size > 0);
}


/*
  Ask system to write `size` random bytes to `buf`.

  Intended to be used to seed the RNGs.

  It tries first to use the system random source.  If that doesn't
  work, it falls back to using the system clock.

  Returns non-zero on error.
*/
int random_seed(void *buf, size_t size)
{
  int success = 0;

  /* Windows should have BCryptGenRandom() */
#ifdef HAVE_BCryptGenRandom
  if (!success) {
    NTSTATUS stat = BCryptGenRandom(BCRYPT_RNG_ALG_HANDLE, buf, size, 0);
    success = (stat == STATUS_SUCCESS) ? 1 : 0;
  }
#endif

  /* On linux we can read from /dev/urandom */
  if (!success) {
    FILE *fp = fopen("/dev/urandom", "rb");
    if (fp) {
      if (fread(buf, size, 1, fp) == 1) success = 1;
      fclose(fp);
    }
  }

  /* Fallback to use system time. */
#ifndef RNG_ONLY_HIGH_QUALITY_SEED

  /* Since the clock() function is broken on Windows (has only second
     resolution) we try GetSystemTimeAsFileTime() first */
#ifdef HAVE_GetSystemTimeAsFileTime
  if (!success) {
    SYSTEMTIME systime;

    GetSystemTime(&systime);
    if (systime.wYear) {
      // XOR together all fields in the SYSTEMTIME structure
      WORD seed = systime.wYear ^ systime.wMonth ^ systime.wDayOfWeek
        ^ systime.wDay ^ systime.wHour ^ systime.wMinute
        ^ systime.wSecond ^ systime.wMillisecond;
      random_bytes(buf, size, seed);
      success = 1;
    }
  }
#endif

#if defined(HAVE_CLOCK) && defined(HAVE_CLOCK_T) && !defined(_WIN32)
  if (!success && sizeof(clock_t) >= sizeof(uint32_t)) {
    clock_t ticks = clock();
    random_bytes(buf, size, ticks);
    success = 1;
  }
#endif

#endif /* RNG_ONLY_HIGH_QUALITY_SEED */

  return (success) ? 0 : 1;
}




/*
 * MWC RNG by George Marsaglia
 */
static uint32_t mwc_upper = 362436069, mwc_lower = 521288629;


int srand_mwc(uint32_t seed)
{
  if (seed == 0) return random_seed(&seed, sizeof(seed));
  mwc_lower = seed | 521288629;   /* Can't seed with 0 */
  mwc_upper = seed | 362436069;
  return 0;
}

uint32_t rand_mwc(void)
{
  mwc_lower = 18000*(mwc_lower & 0xffff) + (mwc_lower>>16);
  mwc_upper = 36969*(mwc_upper & 0xffff) + (mwc_upper>>16);
  return (mwc_upper<<16) + mwc_lower;
}


int srand_mwc_r(MWCState *state, uint32_t seed)
{
  if (seed == 0) return random_seed(&seed, sizeof(seed));
  state->mwc_lower = seed | 521288629;   /* Can't seed with 0 */
  state->mwc_upper = seed | 362436069;
  return 0;
}

uint32_t rand_mwc_r(MWCState *state)
{
  state->mwc_lower = 18000*(state->mwc_lower & 0xffff) + (state->mwc_lower>>16);
  state->mwc_upper = 36969*(state->mwc_upper & 0xffff) + (state->mwc_upper>>16);
  return (state->mwc_upper<<16) + state->mwc_lower;
}


/*
 * Middle-Square Weyl Sequence RNG by Bernard Widynski
 */

static uint64_t rand_digits(uint64_t n)
{
  MSWS32State s;
  uint64_t c, i, j, k, m, r, t, u, v;

  uint64_t sconst[] = {
    0x37e1c9b5e1a2b843, 0x56e9d7a3d6234c87,
    0xc361be549a24e8c7, 0xd25b9768a1582d7b,
    0x18b2547d3de29b67, 0xc1752836875c29ad,
    0x4e85ba61e814cd25, 0x17489dc6729386c1,
    0x7c1563ad89c2a65d, 0xcdb798e4ed82c675,
    0xd98b72e4b4e682c1, 0xdacb7524e4b3927d,
    0x53a8e9d7d1b5c827, 0xe28459db142e98a7,
    0x72c1b3461e4569db, 0x1864e2d745e3b169,
    0x6a2c143bdec97213, 0xb5e1d923d741a985,
    0xb4875e967bc63d19, 0x92b64d5a82db4697,
    0x7cae812d896eb1a5, 0xb53827d41769542d,
    0x6d89b42c68a31db5, 0x75e26d434e2986d5,
    0x7c82643d293cb865, 0x64c3bd82e8637a95,
    0x2895c34d9dc83e61, 0xa7d58c34dea35721,
    0x3dbc5e687c8e61d5, 0xb468a235e6d2b193
  };

  /* initialize local msws state `s` */
  r = n / 100000000;
  t = n % 100000000;
  s.s = sconst[r%30];
  r /= 30;
  s.x = s.w = t*s.s + r*s.s*100000000;

  /* odd random number for low order digit */
  u = (rand_msws32_r(&s) % 8) * 2 + 1;
  v = (1LL<<u);

  /* get rest of digits */
  for (m=60,c=0;m>0;) {
    j = rand_msws32_r(&s);     /* get 8 digit 32-bit random word */
    for (i=0;i<32;i+=4) {
      k = (j>>i) & 0xf;        /* get a digit */
      if (k!=0 && (c & (1LL<<k)) == 0) { /* not 0 and not previous */
        c |= (1LL<<k);
        u |= (k<<m);           /* add digit to output */
        m -= 4;
        if (m==24 || m==28) c = (1LL<<k) | v;
        if (m==0) break;
      }
    }
  }

  return u;
}

/* Reentrant functions */
uint32_t rand_msws32_r(MSWS32State *s)
{
  s->x *= s->x;
  s->x += (s->w += s->s);
  return s->x = (s->x>>32) | (s->x<<32);
}

double drand_msws32_r(MSWS32State *s)
{
  return rand_msws32_r(s) / 4294967296.;
}

int srand_msws32_r(MSWS32State *s, uint64_t seed)
{
  if (seed == 0) return random_seed(&seed, sizeof(seed));
  s->x = s->w = s->s = rand_digits(seed);
  return 0;
}


uint64_t rand_msws64_r(MSWS64State *s)
{
  uint64_t xx;
  s->x1 *= s->x1;
  xx = s->x1 += (s->w1 += s->s1);
  s->x1 = (s->x1 >> 32) | (s->x1 << 32);
  s->x2 *= s->x2;
  s->x2 += (s->w2 += s->s2);
  s->x2 = (s->x2 >> 32) | (s->x2 << 32);
  return xx ^ s->x2;
}

double drand_msws64_r(MSWS64State *s)
{
  return (rand_msws64_r(s)>>11) / 9007199254740992.;
}

int srand_msws64_r(MSWS64State *s, uint64_t seed)
{
  int stat=0;
  if (seed == 0) {
    stat |= random_seed(&seed, sizeof(seed));
    s->x1 = s->w1 = s->s1 = rand_digits(seed);
    stat |= random_seed(&seed, sizeof(seed));
    s->x2 = s->w2 = s->s2 = rand_digits(seed);
  } else {
    s->x1 = s->w1 = s->s1 = rand_digits(seed);
    s->x2 = s->w2 = s->s2 = rand_digits(seed+2);
  }
  return stat;
}


/* Non-reentrant functions */
static MSWS32State msws32state = {0, 0, 0xb5ad4eceda1ce2a9};
static MSWS64State msws64state = {0, 0, 0xb5ad4eceda1ce2a9,
                                  0, 0, 0x278c5a4d8419fe6b};

uint32_t rand_msws32(void)
{
  return rand_msws32_r(&msws32state);
}

double drand_msws32(void)
{
  return drand_msws32_r(&msws32state);
}

int srand_msws32(uint64_t seed)
{
  return srand_msws32_r(&msws32state, seed);
}

uint64_t rand_msws64(void)
{
  return rand_msws64_r(&msws64state);
}

double drand_msws64(void)
{
  return drand_msws64_r(&msws64state);
}

int srand_msws64(uint64_t seed)
{
  return srand_msws64_r(&msws64state, seed);
}
