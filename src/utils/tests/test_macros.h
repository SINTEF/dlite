#ifndef _TEST_MACROS_H
#define _TEST_MACROS_H


/* Get rid of annoying warnings on Windows */
#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#pragma warning(disable: 4996 4267)
#endif


#define STRINGIFY(s) _STRINGIFY(s)
#define _STRINGIFY(s) # s

#define UNUSED(x) (void)(x)

#define countof(arr) (sizeof(arr) / sizeof(arr[0]))

#endif /*  _TEST_MACROS_H */
