/* rng.h - pseudo random number generators
 *
 * Copyright (C) 2023 SINTEF
 *
 * Distributed under terms of the MIT license.
 */

/**
   @file rng.h
   @brief Pseudo random number generators


   MWC by George Marsaglia, 1994
   -----------------------------
   The MWC generator concatenates two 16-bit multiply-with-carry
   generators, x(n)=36969x(n-1)+carry, y(n)=18000y(n-1)+carry mod
   2^16, has period about 2^60 and seems to pass all tests of
   randomness.

   MSWS by Bernard Widynski, 2022
   ------------------------------
   A variation on John von Neumann's original middle-square method.
   This generator may be the fastest RNG that passes all the
   statistical tests.  It is provided in 32 and 64 bit variants.
   The 32 bit variant has periodicity of 2^64.  The periodicity of
   64 bit variant is not documented in Widynski (2022), but it should
   at least be 2^64, more likely 2^128.

   Ref: https://arxiv.org/abs/1704.00358
*/
#ifndef _RNG_H
#define _RNG_H

#include "integers.h"


/**
  Ask system to write `size` random bytes to `buf`.

  This function is intended to be used to seed the RNGs.  It is called
  when the seed functions are called with `seed=0`.

  It tries first to use the system random source.  If that doesn't
  work and macro `RNG_ONLY_HIGH_QUALITY_SEED` is not defined, it falls
  back to using the system clock.

  Returns non-zero on error.
*/
int random_seed(void *buf, size_t size);


/**
 * @name MWC by George Marsaglia, 1994
 */
/** @{ */

/** Internal state of the MWC RNG. */
typedef struct {
  uint32_t mwc_upper, mwc_lower;
} MWCState;


/** Return new random number using the MWC RNG. */
uint32_t rand_mwc();

/** Seed the rand_mws() RNG.
    If `seed` is zero, it is seeded with random_seed(). */
int srand_mwc(uint32_t seed);


/** Reentrant version of rand_mwc().
    The internal state should always be seeded with srand_mws_r(). */
uint32_t rand_mwc_r(MWCState *state);

/** Reentrant version of srand_mwc().
    If `seed` is zero, it is seeded with random_seed(). */
int srand_mwc_r(MWCState *state, uint32_t seed);



/** @} */
/**
 * @name MSWS by Bernard Widynski, 2022
 */
/** @{ */


/** Internal state of the MSWS32 RNG. */
typedef struct {
  uint64_t x, w, s;
} MSWS32State;

/** Return new 32 bit random number using the MSWS RNG. */
uint32_t rand_msws32(void);

/** Return a 32-bit floating point number in the range [0,1).*/
double drand_msws32(void);

/** Seed the rand_msws32() RNG.
    If `seed` is zero, it is seeded with random_seed(). */
int srand_msws32(uint64_t seed);


/** Reentrant version of rand_msws32(). Should always be seeded before use. */
uint32_t rand_msws32_r(MSWS32State *s);

/** Reentrant version of drand_msws32() */
double drand_msws32_r(MSWS32State *s);

/** Reentrant version of srand_msws32()
    If `seed` is zero, it is seeded with random_seed(). */
int srand_msws32_r(MSWS32State *s, uint64_t seed);



/** Internal state of the MSWS64 RNG. */
typedef struct {
  uint64_t x1, w1, s1, x2, w2, s2;
} MSWS64State;

/** Return new 64 bit random number using the MSWS RNG. */
uint64_t rand_msws64(void);

/** Return a 53-bit floating point number in the range [0,1).*/
double drand_msws64(void);

/** Seed the rand_msws64() RNG.
    If `seed` is zero, it is seeded with random_seed(). */
int srand_msws64(uint64_t seed);


/** Reentrant version of rand_msws64(). Should always be seeded before use. */
uint64_t rand_msws64_r(MSWS64State *s);

/** Reentrant version of drand_msws64(). Should always be seeded before use. */
double drand_msws64_r(MSWS64State *s);

/** Reentrant version of srand_msws64()
    If `seed` is zero, it is seeded with random_seed(). */
int srand_msws64_r(MSWS64State *s, uint64_t seed);

/** @} */


#endif  /* _RNG_H */
