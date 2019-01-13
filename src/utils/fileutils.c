/* fileutils.c -- cross-platform file utility functions */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>

#include "compat.h"
#include "err.h"
#include "globmatch.h"
#include "fileutils.h"


/* Iterator for */
struct _FUPaths {
  const char *pattern;  /* File name glob pattern to match against. */
  const char *paths;    /* Semicolon-separated list of paths. */
  const char *envvar;   /* Name of environment variable with paths to check. */
  const char *p;        /* Pointer to current position in `paths`. */
  FUDir *dir;     /* Current position in directory corresponding to `p`. */
};



/*
  Opens a directory and returns a FUDir handle to it.  Returns NULL on error.
 */
FUDir *fu_opendir(const char *path)
{
  FUDir *dir;
  if (!(dir = (FUDir *)opendir(path)))
    return err(1, "cannot open directory \"%s\"", path), NULL;
  return dir;
}

/*
  Returns a pointer to the name of the next file in directory referred to by
  `dir` or NULL if no more files are available in the directory.
 */
const char *fu_nextfile(FUDir *dir)
{
  struct dirent *ent = readdir((DIR *)dir);
  return (ent) ? ent->d_name : NULL;
}

/*
  Closes a directory opened with fu_opendir().  Returns non-zero on errir.
 */
int fu_closedir(FUDir *dir)
{
  return closedir((DIR *)dir);
}


/* Like fu_opendir(), but truncates `path` at first occation of PATHSEP. */
static FUDir *opendir_sep(const char *path)
{
  FUDir *dir;
  char sep[2] = {PATHSEP, '\0'}, *p;
  size_t len = strcspn(path, sep);
  if (!(p = strndup(path, len))) return err(1, "allocation failure"), NULL;
  dir = opendir(p);  /* do not call err() if dir cannot be opened */
  free(p);
  return dir;
}

/*
  Returns a new iterator for finding files matching `pattern`.
  `paths` is a PATHSEP-separated list of directory paths to search.
  If `envvar` is not NULL, it should be the name of an environment
  variable containing additional paths to search.
 */
FUPaths *fu_startmatch(const char *pattern, const char *paths,
                       const char *envvar)
{
  FUPaths *iter;
  if (!(iter = calloc(1, sizeof(FUPaths))))
    return err(1, "allocation failure"), NULL;
  iter->pattern = pattern;
  iter->paths = paths;
  iter->envvar = envvar;
  iter->p = iter->paths;
  iter->dir = NULL;
  return iter;
}

/*
  Returns name of the next file matching the pattern provided to
  fu_startmatch() or NULL if there are no more matches.
 */
const char *fu_nextmatch(FUPaths *iter)
{
  const char *filename;
  while (1) {
    if (!iter->p) {
      if (iter->envvar) {
        if (!(iter->paths = getenv(iter->envvar))) return NULL;
        iter->envvar = NULL;
        iter->p = iter->paths;
      } else {
        return NULL;
      }
    }

    if (!iter->dir && !(iter->dir = opendir_sep(iter->p))) {
      if ((iter->p = strchr(iter->p, PATHSEP))) iter->p++;
    }
    if (!iter->dir) continue;

    if ((filename = fu_nextfile(iter->dir))) {
      if (globmatch(iter->pattern, filename) == 0) return filename;
    } else {
      fu_closedir(iter->dir);
      iter->dir = NULL;
      if ((iter->p = strchr(iter->p, PATHSEP))) iter->p++;
    }
  }
}

/*
  Ends pattern matching iteration.
 */
int fu_endmatch(FUPaths *iter)
{
  free(iter);
  return 0;
}
