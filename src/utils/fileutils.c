/* fileutils.c -- cross-platform file utility functions */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>
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



/* Returns non-zero if `path` is an absolute path. */
int fu_isabs(const char *path)
{
  if (!path) return 0;
  if (strlen(path) >= 1 && (path[0] == '/' || path[0] == '\\')) return 1;
  if (strlen(path) >= 2 && path[1] == ':' &&
      (('a' <= path[0] && path[0] <= 'z') ||
       ('A' <= path[0] && path[0] <= 'Z'))) return 1;
  return 0;
}


/*
  Joins a set of pathname components, inserting '/' as needed.  The
  last argument must be NULL.

  If any component is an absolute path, all previous path components
  will be discarded.  An empty last part will result in a path that
  ends with a separator.
 */
char *fu_join(const char *a, ...)
{
  va_list ap, aq;
  char *p, *path;
  int i, n, pos=0, nargs=1, arg0=0, len=strlen(a)+1;
  va_start(ap, a);
  va_copy(aq, ap);

  while ((p = va_arg(ap, char *))) {
    int n = strlen(p);
    if (fu_isabs(p)) {
      arg0 = nargs;
      len = n+1;
    } else {
      len += n+1;
    }
    nargs++;
  }

  if (!(path = malloc(len))) return err(1, "allocation failure"), NULL;

  if (arg0 == 0) {
    n = strlen(a);
    strncpy(path, a, n);
    pos += n;
    path[pos++] = '/';
    arg0++;
  }
  for (i=0; i<arg0-1; i++)
    va_arg(aq, char *);
  for (i=arg0; i<nargs; i++) {
    p = va_arg(aq, char *);
    n = strlen(p);
    strncpy(path+pos, p, n);
    pos += n;
    path[pos++] = '/';
  }
  path[len-1] = '\0';

  va_end(ap);
  va_end(aq);
  return path;
}


/*
  Updates `path` to use more "user-friendly" directory separators.

  On Unix-like systems this function does nothing.

  On Windows, the following logic is applied:
    - path starts with "//" or "\\":                      '/' -> '\'
    - path starts with "C:" (where C is any character):   '\' -> '/'
    - otherwise                                           '\' -> '/'

  Returns a pointer to `path`.
 */
char *fu_friendly_dirsep(char *path)
{
#if defined WIN32 || defined _WIN32 || defined __WIN32__
  int from, to;
  char *c, *p=path;
  if (strlen(path) >= 2 &&
      (path[0] == path[1] == '/' || path[0] == path[1] == '\\'))
    to='\\';
  else if (strlen(path) >= 2 && path[1] == ':' &&
           (('a' <= path[0] && path[0] <= 'z') ||
            ('A' <= path[0] && path[0] <= 'Z')))
    to='/';
  else
    to='/';

  from = (to == '/') ? '\\' : '/';
  while ((c = strchr(p, from))) {
    *c = to;
    p = c+1;
  }
#endif
  return path;
}


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
      if (n) fu_paths_append(paths, p);
      p += n+1;
    } else {
      if (n) fu_paths_append(paths, p);
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

  Note:
  The returned string is owned by the iterator. It will be overwritten
  by the next call to fu_nextmatch() and should not be changed.  Use
  strdup() or strncpy() if a copy is needed.
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
      if (!(iter->dir = opendir(path))) {
        iter->i++;
        continue;
      }
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
        fu_friendly_dirsep(iter->path);
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
