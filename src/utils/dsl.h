/* dsl -- dynamic shared libraries */
#ifndef _DSL_H
#define _DSL_H

/**
  @file
  @brief Portable macros for dynamic shared libraries.

  This header declares some macros for working with dynamic shared
  libraries in a portable way (POSIX and Windows).  On POSIX systems
  one should link with `-ldl`.

  ### Macro prototypes

      dsl_handle dsl_open(const char *filename);

  Opens shared library `filename` and returns a new handle or NULL on
  error.


      void *dsl_sym(dsl_handle handle, const char *symbol);

  Returns a pointer to symbol in a shared library or NULL on error.


      const char *dsl_error(void);

  Returns a pointer to a human-readable string describing the most
  recent error or NULL if no errors have occurred since the last call
  to dsl_error().


      int dsl_close(dsl_handle handle);

  Closes a handle.  Returns non-zero on error.
 */

#ifdef HAVE_CONFIG_H
 #include "config.h"
#endif

#define DSL_Posix     0
#define DSL_Windows   1


/* Determine platform */
#if defined __APPLE__ && defined __MARCH__
# define DSL_PLATFORM DSL_Posix
# ifndef DSL_PREFIX
#  define DSL_PREFIX ""
# endif
# ifndef DSL_EXT
#  define DSL_EXT ".dylib"
# endif
#elif defined unix        ||                            \
      defined __unix      ||                            \
      defined __unix__    ||                            \
      defined __linux__   ||                            \
      defined __FreeBSD__ ||                            \
      defined __CYGWIN__
# define DSL_PLATFORM DSL_Posix
# ifndef DSL_PREFIX
#  define DSL_PREFIX "lib"
# endif
# ifndef DSL_EXT
#  define DSL_EXT ".so"
# endif
#elif defined WIN32     ||                              \
      defined _WIN32    ||                              \
      defined __WIN32__
# define DSL_PLATFORM DSL_Windows
# ifndef DSL_PREFIX
#  define DSL_PREFIX ""
# endif
# ifndef DSL_EXT
#  define DSL_EXT ".dll"
# endif
#else
# error "Unsupported platform"
#endif

/* Compiler-specific definitions */
#if defined _MSC_VER
# if defined(__cplusplus)
#  define DSL_EXPORT extern "C" __declspec(dllexport)
#  define DSL_IMPORT extern "C" __declspec(dllimport)
# else
#  define DSL_EXPORT __declspec(dllexport)
#  define DSL_IMPORT __declspec(dllimport)
# endif
#else
//#if defined(__cplusplus)
//#define DSL_EXPORT extern "C" __attribute__((visibility("default")))
//#else
//#define DSL_EXPORT __attribute__((visibility("default")))
//#endif
# define DSL_EXPORT
# define DSL_IMPORT
#endif


/* POSIX */
#if DSL_PLATFORM == DSL_Posix

#include <dlfcn.h>

typedef void * dsl_handle;

#define dsl_open(filename)         ((dsl_handle)dlopen(filename, RTLD_LAZY))
#define dsl_sym(handle, symbol)    ((void *)dlsym((void *)(handle), symbol))
#define dsl_error()                ((const char *)dlerror())
#define dsl_close(handle)          ((int)dlclose((void *)(handle)))


/* Windows */
#elif DSL_PLATFORM == DSL_Windows

#include <windows.h>

typedef HMODULE dsl_handle;

#define dsl_open(filename) \
  (dsl_handle)LoadLibrary((LPCTSTR)(filename))
#define dsl_sym(handle, symbol) \
  (void *)GetProcAddress((HMODULE)(handle), (LPCSTR)(symbol))
#define dsl_error() \
  (const char *)strerror(GetLastError())
#define dsl_close(handle) \
  (int)FreeLibrary((HMODULE)(handle))

#endif  /* DSL_PLATFORM */


#endif /* _DSL_H */
