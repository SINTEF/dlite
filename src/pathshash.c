#include <stdlib.h>
#include <string.h>

#include "utils/sha3.h"
#include "utils/err.h"
#include "pathshash.h"

/*
  Calculate sha3 hash of all files in `paths` and stores it in `hash`.
  `hashsize` is the size of `hash` in bytes. Should be 32, 48 or 64.

  If `pattern` is given, it should be a glob pattern for selecting what
  files to include.

  Returns non-zero on error.
*/
int pathshash(unsigned char *hash, int hashsize, const FUPaths *paths,
              const char *pattern)
{
  sha3_context c;
  FUIter *iter;
  unsigned bitsize = hashsize * 8;
  const unsigned char *buf;
  const char *path;
  if (!(iter = fu_startmatch(pattern, paths)))
    return err(1, "cannot initiate paths iterator (%d)", (paths) ? (int)paths->n : -1);
  if (sha3_Init(&c, bitsize))
    return err(1, "invalid hash size: %d bytes", hashsize);
  while ((path = fu_nextmatch(iter)))
    sha3_Update(&c, path, strlen(path));
  buf = sha3_Finalize(&c);
  fu_endmatch(iter);
  memcpy(hash, buf, hashsize);
  return 0;
}
