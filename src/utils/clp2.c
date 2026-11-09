#include <limits.h>

#include "integers.h"


/* Returns x rounded up to next power of two. */
size_t clp2(size_t n)
{
  n--;
  /* Loop instead of fixed shifts, so we never shift by >= the width of
     size_t (undefined behaviour).  Compilers unroll this. */
  for (unsigned s=1; s < sizeof(size_t)*CHAR_BIT; s <<= 1) n |= n >> s;
  return ++n;
}


/* Returns x rounded down to previous power of two. */
size_t flp2(size_t n)
{
  /* Loop instead of fixed shifts, so we never shift by >= the width of
     size_t (undefined behaviour).  Compilers unroll this. */
  for (unsigned s=1; s < sizeof(size_t)*CHAR_BIT; s <<= 1) n |= n >> s;
  return n - (n >> 1);
}
