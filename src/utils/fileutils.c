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


/** Convenient macros for failing */
#define FAIL(msg) do { \
    err(1, msg); goto fail; } while (0)
#define FAIL1(msg, a1) do { \
    err(1, msg, a1); goto fail; } while (0)


/* Whether we are on Windows */
#if defined WIN32 || defined _WIN32 || defined __WIN32__
# ifndef WINDOWS
#  define WINDOWS
# endif
#endif


/* Paths iterator */
struct _FUIter {
  const char *pattern;   /* File name glob pattern to match against. */
  size_t i;              /* Index of current position in `paths`. */
  const FUPaths *paths;  /* Paths */
  const char *filename;  /* Name of curren file (excluding directory path) */
  char *path;            /* Full path to current file */
  size_t pathsize;       /* Allocated size of `path` */
  FUDir *dir;            /* Currend directory corresponding to `p`. */
  int dirsep;            /* Directory separator */
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
  first path component is `a`.  The last argument must be NULL.

  If any component is an absolute path, all previous path components
  will be discarded.  An empty last part will result in a path that
  ends with a separator.
 */
char *fu_join(const char *a, ...)
{
  va_list ap;
  char *path;
  va_start(ap, a);
  path = fu_vjoin_sep('/', a, ap);
  va_end(ap);
  return path;
}

/*
  Like fu_join(), but takes path separator as first argument.
*/
char *fu_join_sep(int sep, const char *a, ...)
{
  va_list ap;
  char *path;
  va_start(ap, a);
  path = fu_vjoin_sep(sep, a, ap);
  va_end(ap);
  return path;
}

/*
  Like fu_join_sep(), but takes a va_list instead of a variable number
  of arguments.
*/
char *fu_vjoin_sep(int sep, const char *a, va_list ap)
{
  va_list aq;
  char *p, *path;
  int i, n, pos=0, nargs=1, arg0=0, len=strlen(a)+1;
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

  if (!(path = malloc(len))) {
    va_end(aq);
    return err(1, "allocation failure"), NULL;
  }

  if (arg0 == 0) {
    n = strlen(a);
    strncpy(path, a, n);
    pos += n;
    path[pos++] = sep;
    arg0++;
  }
  for (i=0; i<arg0-1; i++)
    va_arg(aq, char *);
  for (i=arg0; i<nargs; i++) {
    p = va_arg(aq, char *);
    n = strlen(p);
    strncpy(path+pos, p, n);
    pos += n;
    path[pos++] = sep;
  }
  path[len-1] = '\0';

  va_end(aq);
  return path;
}


/*
  Returns a pointer to the last directory separator in `path`.
 */
char *fu_lastsep(const char *path)
{
#ifdef WINDOWS
  char *p = strrchr(path, '/');
  char *q = strrchr(path, '\\');
  return (p > q) ? p : q;
#else
  return strrchr(path, DIRSEP[0]);
#endif
}

/*
  Returns the directory component of `path` as a newly allocated string.
*/
char *fu_dirname(const char *path)
{
  char *p, *q;
  assert(path);
  if (!(p = strdup(path))) return err(1, "allocation failure"), NULL;
  if ((q = fu_lastsep(p)) && q > p) *q = '\0';
  if (!q) *p = '\0';
  return p;
}

/*
  Returns the final component of `path` as a newly allocated string.
*/
char *fu_basename(const char *path)
{
  char *p;
  if ((p = fu_lastsep(path))) return strdup(p+1);
  return strdup(path);
}

/*
  Returns a pointer to file extension of `path` (what follows after
  the last ".").
*/
const char *fu_fileext(const char *path)
{
  char *q, *p = strrchr(path, '.');
  if (!p || (q = fu_lastsep(path)) > p) return path + strlen(path);
  return p+1;
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
#ifdef WINDOWS
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
  Returns the canonicalized absolute pathname for `path`.  Resolves
  symbolic links and references to '/./', '/../' and extra '/'.  Note
  that `path` must exists.

  If `resolved_path` is NULL, the returned path is malloc()'ed.
  Otherwise, it must be a buffer of at least size PATH_MAX (on POSIX)
  or MAX_PATH (on Windows).

  Returns NULL on error.
 */
char *fu_realpath(const char *path, char *resolved_path)
{
#if defined(HAVE_REALPATH)
  return realpath(path, resolved_path);
#elif defined(HAVE_GetFullPathNameW)
  int n, size=MAX_PATH;
  if (!resolved_path) size=0;
  if (n = GetFullPathNameW(path, size, resolved_path, NULL) == 0)
    return err(1, "cannot resolve canonical path for '%s'", path), NULL;
  if (!resolved_path) {
    size = n + 1;
    if (!(resolved_path = malloc(size)))
      return err(1, "allocation failure"), NULL;
    if (n = GetFullPathNameW(path, size, resolved_path, NULL) == 0)
      return err(1, "cannot resolve canonical path for '%s'", path), NULL;
  }
  if (n > size)
    return err(1, "cannot create large enough buffer for canonicalize '%s' "
               "(limited to MAX_PATH=%d)", path, MAX_PATH);
  return resolved_path;
#else
#pragma message ( "Neither realpath() nor GetFullPathNameW() exists" )
  if (!resolved_path) return strdup(path);
# ifdef WINDOWS
  return strncpy(resolved_path, path, MAX_PATH);
# else
  return strncpy(resolved_path, path, PATH_MAX);
# endif
#endif
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
  return fu_paths_init_sep(paths, envvar, PATHSEP);
}

/*
  Like fu_paths_init(), but allow custom path separator `pathsep`.

  Note that any character in `pathsep` works as a path separator.
 */
int fu_paths_init_sep(FUPaths *paths, const char *envvar, const char *pathsep)
{
  char *p=NULL, *edup=NULL, *epath = (envvar) ? getenv(envvar) : NULL;
  memset(paths, 0, sizeof(FUPaths));
  if (epath) p = edup = strdup(epath);
  while (p) {
    size_t n = strcspn(p, pathsep);
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
  Returns an allocated string with all paths in `paths` separated by `pathsep`.

  If `pathsep` is NULL, the system path separator is used.  Note that unlike
  fu_paths_init_sep(), if `pathsep` is more than one character (in addition to
  the terminating NUL), the full string is inserted as path separator.
 */
char *fu_paths_string(const FUPaths *paths, const char *pathsep)
{
  size_t i, seplen, size=0;
  char *s, *string;
  if (!pathsep) pathsep = PATHSEP;
  seplen = strlen(pathsep);
  for (i=0; i<paths->n; i++) size += strlen(paths->paths[i]);
  size += (paths->n - 1) * seplen;
  s = string = malloc(size + 1);
  for (i=0; i < paths->n; i++) {
    int n = strlen(paths->paths[i]);
    strncpy(s, paths->paths[i], n);
    s += n;
    if (i < paths->n - 1) {
      strncpy(s, pathsep, seplen);
      s += seplen;
    }
  }
  *s = '\0';
  return string;
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

  Use fu_nextmatch() and fu_endmatch() to iterate over matching files
  and end iteration, respectively.

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
  iter->dirsep = DIRSEP[0];
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
  char dirsep[2] = { iter->dirsep, 0 };
  if (iter->i >= p->n) return NULL;

  while (iter->i < p->n) {
    const char *path = p->paths[iter->i];
    if (!iter->dir) {
      if (iter->i >= p->n) return NULL;
      if (!path[0]) path = ".";
      if (!(iter->dir = opendir(path))) {
        iter->i++;
        continue;
      }
    }

    if ((filename = fu_nextfile(iter->dir))) {
      if (globmatch(iter->pattern, filename) == 0) {
        size_t n = strlen(path) + strlen(filename) + 2;
        if (n > iter->pathsize) {
          iter->pathsize = n;
          if (!(iter->path = realloc(iter->path, iter->pathsize)))
            return err(1, "allocation failure"), NULL;
        }
        iter->filename = filename;
        strcpy(iter->path, path);
        strcat(iter->path, dirsep);
        strcat(iter->path, filename);
        fu_friendly_dirsep(iter->path);
        if (iter->path[0] == '.' && iter->path[1] == iter->dirsep)
          return iter->path+2;
        else
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


/*
  Returns a new iterator over files matching `pattern`.
  Only the last component of `pattern` may contain wildcards.

  Use fu_globnext() and fu_globend() to iterate over matching files
  and end iteration, respectively.
 */
FUIter *fu_glob(const char *pattern)
{
  FUIter *iter=NULL;
  FUPaths *paths=NULL;
  char *dirname=NULL, *basename=NULL;
  if (!(paths = malloc(sizeof(FUPaths)))) FAIL("allocation failure");
  if (!(dirname = fu_dirname(pattern))) goto fail;
  if (!(basename = fu_basename(pattern))) goto fail;
  if (!*dirname) {
    free(dirname);
    dirname = strdup(".");
  }
  fu_paths_init(paths, NULL);
  fu_paths_append(paths, dirname);
  iter = fu_startmatch(basename, paths);
 fail:
  if (dirname) free(dirname);
  return iter;
}

/*
  Returns path to the next matching file.
 */
const char *fu_globnext(FUIter *iter)
{
  return fu_nextmatch(iter);
}

/*
  Ends glob pattern iteration.  Returns non-zero on error.
 */
int fu_globend(FUIter *iter)
{
  FUPaths *paths = (FUPaths *)iter->paths;
  char *pattern = (char *)iter->pattern;
  int stat = fu_endmatch(iter);
  fu_paths_deinit(paths);
  free(paths);
  free(pattern);
  return stat;
}

/*
  Sets the directory separator in returned by fu_nextmatch() and
  fu_globnext().  Defaults to DIRSEP.
*/
void fu_iter_set_dirsep(FUIter *iter, int dirsep)
{
  iter->dirsep = dirsep;
}
