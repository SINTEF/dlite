/* compat.h -- auxiliary compatibility functions */

#include "config.h"

/* strdup */
#ifndef HAVE_STRDUP
# ifndef HAVE__STRDUP
char *strdup(const char *s);
# endif
#endif
