/* setenv.h -- environment related cross-platform compatibility functions
 *
 * Copyright (C) 2017 SINTEF
 *
 * 
 * Distributed under terms of the MIT license.
 */

#ifndef _COMPAT_SETENV_H
#define _COMPAT_SETENV_H

#include "config-paths.h"


/** setenv() - change or add an environment variable */
#if !defined(HAVE_SETENV) && defined(HAVE__PUTENV_S)
#define HAVE_SETENV
int setenv(const char *name, const char *value, int overwrite);
#endif


#endif  /* _COMPAT_SETENV_H */
