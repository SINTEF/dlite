#ifndef _PATHSHASH_H
#define _PATHSHASH_H

#include "utils/fileutils.h"

/**
  Calculate sha3 hash of all files in `paths` and stores it in `hash`.
  `hashsize` is the size of `hash` in bytes. Should be 32, 48 or 64.

  If `pattern` is given, it should be a glob pattern for selecting what
  files to include.

  Returns non-zero on error.
*/
int pathshash(unsigned char *hash, int hashsize, const FUPaths *paths,
              const char *pattern);


#endif  /* _PATHSHASH_H */
