/* -*- C -*-  (not really, but good for syntax highlighting) */
/*
   Convinient macros
*/

%{

/** Macro for getting rid of unused parameter warnings... */
#define UNUSED(x) (void)(x)

/** Turns macro literal `s` into a C string */
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

/** Failure macros with explicit error codes */
#define FAILCODE(code, msg) do { \
    dlite_err(code, msg); goto fail; } while (0)
#define FAILCODE1(code, msg, a1) do { \
    dlite_err(code, msg, a1); goto fail; } while (0)
#define FAILCODE2(code, msg, a1, a2) do { \
    dlite_err(code, msg, a1, a2); goto fail; } while (0)
#define FAILCODE3(code, msg, a1, a2, a3) do { \
    dlite_err(code, msg, a1, a2, a3); goto fail; } while (0)
#define FAILCODE4(code, msg, a1, a2, a3, a4) do { \
    dlite_err(code, msg, a1, a2, a3, a4); goto fail; } while (0)

%}
