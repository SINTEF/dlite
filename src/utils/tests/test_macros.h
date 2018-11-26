#ifndef _TEST_MACROS_H
#define _TEST_MACROS_H

#define STRINGIFY(s) _STRINGIFY(s)
#define _STRINGIFY(s) # s

#define UNUSED(x) (void)(x)

#define countof(arr) (sizeof(arr) / sizeof(arr[0]))

#endif /*  _TEST_MACROS_H */
