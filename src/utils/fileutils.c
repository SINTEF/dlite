/* fileutils.c -- cross-platform file utility functions */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "compat.h"
#include "err.h"
#include "globmatch.h"
#include "fileutils.h"


/* Iterator for */
struct _FUIter {
  const char *pattern;   /* File name glob pattern to match against. */
  size_t i;              /* Index of current position in `paths`. */
  const FUPaths *paths;  /* Paths */
  const char *filename;  /* Name of curren file (excluding directory path) */
  char *path;            /* Full path to current file */
  size_t pathsize;       /* Allocated size of `path` */
  FUDir *dir;            /* Currend directory corresponding to `p`. */
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

#if 0
/* Like fu_opendir(), but truncates `path` at first occation of PATHSEP. */
static FUDir *opendir_sep(const char *path)
{
  FUDir *dir;
  char *p;
  size_t len = strcspn(path, PATHSEP);
  if (!(p = malloc(len + 1))) return err(1, "allocation failure"), NULL;
  strncpy(p, path, len);
  p[len] = '\0';
  dir = opendir(p);  /* do not call err() if dir cannot be opened */
  free(p);
  return dir;
}
#endif


/*
  Initiates `paths`.  If `envvar` is not NULL, it should be the name
  of an environment variable with initial search paths separated by
  the PATHSEP charecter.

  Returns the initial number of paths or -1 on error.
 */
int fu_paths_init(FUPaths *paths, const char *envvar)
{
  char *p=NULL, *edup=NULL, *epath = (envvar) ? getenv(envvar) : NULL;
  memset(paths, 0, sizeof(FUPaths));
  if (epath) p = edup = strdup(epath);
  while (p) {
    size_t n = strcspn(p, PATHSEP);
    if (p[n]) {
      p[n] = '\0';
      fu_paths_append(paths, p);
      p += n+1;
    } else {
      fu_paths_append(paths, p);
      break;
    }
  }
  if (edup) free(edup);
  return 0;
}

/*
  Frees all memory allocated in `paths`.
 */
void fu_paths_deinit(FUPaths *paths)
{
  size_t n;
  if (paths->paths) {
    for (n=0; n<paths->n; n++) free((char *)paths->paths[n]);
    free(paths->paths);
  }
  memset(paths, 0, sizeof(FUPaths));
}

/*
  Returns a NULL-terminated array of pointers to paths or NULL if
  `paths` is empty.
 */
const char **fu_paths_get(FUPaths *paths)
{
  assert(paths);
  if (!paths->n) return NULL;
  return paths->paths;
}

/*
  Inserts `path` into `paths` before position `n`.  If `n` is negative, it
  counts from the end (like Python).

  Returns the index of the newly inserted element or -1 on error.
 */
int fu_paths_insert(FUPaths *paths, const char *path, int n)
{
  if (paths->n + 1 >= paths->size) {
    paths->size = paths->n + FU_PATHS_CHUNKSIZE;
    if (!(paths->paths = realloc(paths->paths, paths->size*sizeof(char *))))
      return err(-1, "allocation failure");
  }
  if (n < 0) n += paths->n;
  if (n < 0) n = 0;
  if (n > (int)paths->n) n = paths->n;
  if (n < (int)paths->n)
    memmove(paths->paths+n+1, paths->paths+n, (paths->n-n)*sizeof(char *));
  paths->paths[n] = strdup(path);
  paths->paths[++paths->n] = NULL;
  return n;
}

/*
  Appends `path` to `paths`.  Equivalent to

      fu_paths_insert(paths, path, paths->n)

  Returns the index of the newly appended element or -1 on error.
 */
int fu_paths_append(FUPaths *paths, const char *path)
{
  return fu_paths_insert(paths, path, paths->n);
}

/*
  Removes path index `n` from `paths`.  If `n` is negative, it
  counts from the end (like Python).

  Returns non-zero on error.
 */
int fu_paths_remove(FUPaths *paths, int n)
{
  if (n < 0) n += paths->n;
  if (n < 0) n = 0;
  if (n >= (int)paths->n) n = paths->n - 1;
  free((char *)paths->paths[n]);
  memmove(paths->paths+n, paths->paths+n+1, (paths->n-n)*sizeof(char *));
  paths->n--;
  return 0;
}


/*
  Returns a new iterator for finding files matching `pattern` in
  directories included in `paths`.

  Returns NULL on error.
 */
FUIter *fu_startmatch(const char *pattern, const FUPaths *paths)
{
  FUIter *iter;
  if (!(iter = calloc(1, sizeof(FUIter))))
    return err(1, "allocation failure"), NULL;
  iter->pattern = pattern;
  iter->i = 0;
  iter->paths = paths;
  iter->filename = NULL;
  iter->path = NULL;
  iter->pathsize = 0;
  iter->dir = NULL;
  return iter;
}

/*
  Returns name of the next file matching the pattern provided to
  fu_startmatch() or NULL if there are no more matches.
 */
const char *fu_nextmatch(FUIter *iter)
{
  const char *filename;
  const FUPaths *p = iter->paths;
  if (iter->i >= p->n) return NULL;

  while (iter->i < p->n) {
    const char *path = p->paths[iter->i];
    if (!iter->dir) {
      if (iter->i >= p->n) return NULL;
      if (!(iter->dir = opendir(path))) continue;
    }

    if ((filename = fu_nextfile(iter->dir))) {
      if (globmatch(iter->pattern, filename) == 0) {
        size_t n = strlen(path) + strlen(PATHSEP) + strlen(filename) + 1;
        if (n > iter->pathsize) {
          iter->pathsize = n;
          if (!(iter->path = realloc(iter->path, iter->pathsize)))
            return err(1, "allocation failure"), NULL;
        }
        iter->filename = filename;
        strcpy(iter->path, path);
        strcat(iter->path, DIRSEP);
        strcat(iter->path, filename);
        return iter->path;
      }
    } else {
      fu_closedir(iter->dir);
      iter->i++;
      iter->dir = NULL;
    }
  }
  return NULL;
}

/*
  Ends pattern matching iteration.
 */
int fu_endmatch(FUIter *iter)
{
  if (iter->path) free(iter->path);
  if (iter->dir) fu_closedir(iter->dir);
  free(iter);
  return 0;
}
