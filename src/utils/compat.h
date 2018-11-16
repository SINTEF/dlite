/**
  @file
  @brief auxiliary compatibility functions

  Note that the declarations for the functions found in the compat/
  subdirectory are provided in config.h.in.
*/
#ifndef _COMPAT_H
#define _COMPAT_H

#include <stdlib.h>
#include "config.h"

/** strdup */
#ifndef HAVE_STRDUP
# ifdef HAVE__STRDUP
#  define strdup(s) _strdup(s)
# else
char *strdup(const char *s);
# endif
#endif

/** strdup */
#ifndef HAVE_STRNDUP
# ifdef HAVE__STRNDUP
#  define strndup(s, n) _strndup(s, n)
# else
char *strndup(const char *s, size_t n);
# endif
#endif

/** strcasecmp */
#ifndef HAVE_STRCASECMP
# ifdef HAVE__STRICMP
#  define strcasecmp(s1, s2) _stricmp(s1, s2)
# else
int strcasecmp(const char *s1, const char *s2);
# endif
#endif

/** strncasecmp */
#ifndef HAVE_STRNCASECMP
# ifdef HAVE__STRNICMP
#  define strncasecmp(s1, s2, n) _stricmp(s1, s2, n)
# else
int strncasecmp(const char *s1, const char *s2, size_t n);
# endif
#endif


#endif /* _COMPAT_H */
