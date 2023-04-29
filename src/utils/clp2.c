#include "integers.h"


/* Returns x rounded up to next power of two. */
size_t clp2(size_t n)
{
  n--;
  n |= n >> 1;
  n |= n >> 2;
  n |= n >> 4;
#if SIZEOF_SIZE_TYPE > 1
  n |= n >> 8;
#endif
#if SIZEOF_SIZE_TYPE > 2
  n |= n >> 16;
#endif
#if SIZEOF_SIZE_TYPE > 4
  n |= n >> 32;
#endif
#if SIZEOF_SIZE_TYPE > 8
  n |= n >> 64;
#endif
  return ++n;
}


/* Returns x rounded down to previous power of two. */
size_t flp2(size_t n)
{
  n |= n >> 1;
  n |= n >> 2;
  n |= n >> 4;
#if SIZEOF_SIZE_TYPE > 1
  n |= n >> 8;
#endif
#if SIZEOF_SIZE_TYPE > 2
  n |= n >> 16;
#endif
#if SIZEOF_SIZE_TYPE > 4
  n |= n >> 32;
#endif
#if SIZEOF_SIZE_TYPE > 8
  n |= n >> 64;
#endif
  return n - (n >> 1);
}
