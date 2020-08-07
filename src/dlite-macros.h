#ifndef _MACROS_H
#define _MACROS_H
/**
  @file
  @brief A simple colledtion of convinient macros (internal use)
*/
#include "dlite-misc.h"


/** Macro for getting rid of unused parameter warnings... */
#define UNUSED(x) (void)(x)

/** Turns literal `s` into a C string */
#define STRINGIFY(s) _STRINGIFY(s)
#define _STRINGIFY(s) # s

/** Expands to number of elements of array `arr` */
#define countof(arr) (sizeof(arr) / sizeof(arr[0]))


/** Convenient macros for failing */
#define FAIL(msg) do { \
    dlite_err(1, msg); goto fail; } while (0)
#define FAIL1(msg, a1) do { \
    dlite_err(1, msg, a1); goto fail; } while (0)
#define FAIL2(msg, a1, a2) do { \
    dlite_err(1, msg, a1, a2); goto fail; } while (0)
#define FAIL3(msg, a1, a2, a3) do { \
    dlite_err(1, msg, a1, a2, a3); goto fail; } while (0)
#define FAIL4(msg, a1, a2, a3, a4) do { \
    dlite_err(1, msg, a1, a2, a3, a4); goto fail; } while (0)
#define FAIL5(msg, a1, a2, a3, a4, a5) do {		\
    dlite_err(1, msg, a1, a2, a3, a4, a5); goto fail; } while (0)

/** Convinient macros for warnings */
#define WARN(msg) do { \
    dlite_warn(1, msg); goto fail; } while (0)
#define WARN1(msg, a1) do { \
    dlite_warn(1, msg, a1); goto fail; } while (0)
#define WARN2(msg, a1, a2) do { \
    dlite_warn(1, msg, a1, a2); goto fail; } while (0)
#define WARN3(msg, a1, a2, a3) do { \
    dlite_warn(1, msg, a1, a2, a3); goto fail; } while (0)
#define WARN4(msg, a1, a2, a3, a4) do { \
    dlite_warn(1, msg, a1, a2, a3, a4); goto fail; } while (0)
#define WARN5(msg, a1, a2, a3, a4, a5) do {		\
    dlite_warn(1, msg, a1, a2, a3, a4, a5); goto fail; } while (0)



/** Debugging messages.  Printed if compiled with WITH_DEBUG */
#if defined(WITH_DEBUG) && defined(HAVE___VA_ARGS__)
# define DEBUG_LOG(msg, ...) fprintf(stderr, msg, __VA_ARGS__)
#else
# define DEBUG_LOG(msg, ...)
#endif


#endif /* _MACROS_H */
