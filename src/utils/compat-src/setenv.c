/* setenv.h -- environment related cross-platform compatibility functions
 *
 * Copyright (C) 2017 SINTEF
 *
 * 
 * Distributed under terms of the MIT license.
 */

#include <stdlib.h>
#include "utils/compat-src/setenv.h"

/* setenv() - change or add an environment variable */
#if defined(HAVE_SETENV) && defined(HAVE__PUTENV_S)
int setenv(const char *name, const char *value, int overwrite)
{
  if (!overwrite && getenv(name))
    return 0;
  return _putenv_s(name, value);
}
#endif

typedef int avoid_empty_translation_unit;
