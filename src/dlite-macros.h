#ifndef _MACROS_H
#define _MACROS_H
/**
  @file
  @brief A simple colledtion of convinient macros (internal use)
*/

#include "config.h"


/* Macro for getting rid of unused parameter warnings... */
#define UNUSED(x) (void)(x)

/* Convenient macros for failing */
#define FAIL(msg) do { \
    err(1, msg); goto fail; } while (0)
#define FAIL1(msg, a1) do { \
    err(1, msg, a1); goto fail; } while (0)
#define FAIL2(msg, a1, a2) do { \
    err(1, msg, a1, a2); goto fail; } while (0)
#define FAIL3(msg, a1, a2, a3) do { \
    err(1, msg, a1, a2, a3); goto fail; } while (0)
#define FAIL4(msg, a1, a2, a3, a4) do { \
    err(1, msg, a1, a2, a3, a4); goto fail; } while (0)


#if defined(WITH_DEBUG) && defined(HAVE_VA_ARGS)
# define DEBUG(msg, ...) fprintf(stderr, msg, __VA_ARGS__)
#else
# define DEBUG(msg, ...)
#endif


#endif /* _MACROS_H */
