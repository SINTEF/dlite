/* compat.h -- auxiliary compatibility functions */

#include "config.h"

/* strdup */
#ifndef HAVE_STRDUP
# ifndef HAVE__STRDUP
char *strdup(const char *s);
# endif
#endif

#ifndef HAVE_STRCASECMP
# ifndef HAVE__STRICMP
int strcasecmp(const char *s1, const char *s2);
# endif
#endif

#ifndef HAVE_STRNCASECMP
# ifndef HAVE__STRNICMP
int strncasecmp(const char *s1, const char *s2, size_t n);
# endif
#endif
