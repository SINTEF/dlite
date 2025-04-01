/* fileutils.c -- cross-platform file utility functions
 *
 * Copyright (C) 2017 SINTEF
 *
 * Distributed under terms of the MIT license.
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* Whether we are on Windows */
#if defined WIN32 || defined _WIN32 || defined __WIN32__
# ifndef WINDOWS
#  define WINDOWS
# endif
#endif

#ifdef WINDOWS
#include "windows.h"
#include "shlwapi.h"
#include "fileapi.h"
#endif

#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#ifdef HAVE_ACCESS
#include <unistd.h>
#endif
#ifdef HAVE__ACCESS
#include <io.h>
#endif

#include "compat.h"
#include "err.h"
#include "globmatch.h"
#include "strutils.h"
#include "fileinfo.h"
#include "urlsplit.h"
#include "fileutils.h"

/* Convenient macros for failing */
#define FAIL(msg) do { \
    err(1, msg); goto fail; } while (0)
#define FAIL1(msg, a1) do { \
    err(1, msg, a1); goto fail; } while (0)

/* Expands to `a`, if `a` is positive, otherwise to zero. */
#define POS(a) (((a) > 0) ? (a) : 0)


/* Paths iterator */
struct _FUIter {
  const char *pattern;   /* File name glob pattern to match against. */
  size_t i;              /* Index of current position in `paths`. */
  const FUPaths *paths;  /* Paths */
  char **origpaths;      /* Copy of paths->paths to not be affected by
                            inserts and deletions during iteration */
  size_t origlen;        /* Copy of paths->n */
  const char *filename;  /* Name of current file (excluding directory path) */
  char *dirname;         /* Allocated name of current directory */
  char *path;            /* Full path to current file */
  size_t pathsize;       /* Allocated size of `path` */
  FUDir *dir;            /* Currend directory corresponding to `p`. */
  int dirsep;            /* Directory separator */
  struct _FUIter *globiter;  /* Sub-iterator over glob patterns */
};

/* Platform names */
static char *_platform_names[] = {"Native", "Unix", "Windows", "Apple", NULL};


/*
  Returns native platform.
*/
FUPlatform fu_native_platform(void)
{
#if defined POSIX && !defined(__APPLE__)
  return fuUnix;
#elif defined WINDOWS
  return fuWindows;
#elif (defined __APPLE__ && defined __MACH__)
  return fuApple;
#else
  return fuUnknownPlatform;
#endif
}

/*
  Returns non-zero if `platform` is supported.
*/
int fu_supported_platform(FUPlatform platform)
{
  if (platform == fuNative) platform = fu_native_platform();
  switch (platform) {
  case fuUnix:
  case fuWindows:
  case fuApple:
    return 1;
  default:
    return 0;
  }
}

/*
  Returns a constant pointer to the name of `platform`.
 */
const char *fu_platform_name(FUPlatform platform)
{
  if (platform < 0 || platform >= fuLastPlatform)
    return "Unknown";
  else
    return _platform_names[platform];
}

/*
  Returns the platform number corresponding to `name` or -1 on error.
 */
FUPlatform fu_platform(const char *name)
{
  int i;
  for (i=0; _platform_names[i]; i++)
    if (strcasecmp(_platform_names[i], name) == 0) return i;
  return errx(fuUnknownPlatform, "unknown platform: %s", name);
}


/*
  Returns a pointer to directory separator for `platform` or NULL on error.
*/
const char *fu_dirsep(FUPlatform platform)
{
  if (platform == fuNative) platform = fu_native_platform();
  switch (platform) {
  case fuUnix:
  case fuApple:
    return "/";
  case fuWindows:
    return "\\";
  default: return err(1, "unsupported platform: %d", platform), NULL;
  }
}

/*
  Returns a pointer to path separator for `platform` or NULL on error.
*/
const char *fu_pathsep(FUPlatform platform)
{
  if (platform == fuNative) platform = fu_native_platform();
  switch (platform) {
  case fuUnix:
  case fuApple:
    return ":";
  case fuWindows:
    return ";";
  default: return err(1, "unsupported platform: %d", platform), NULL;
  }
}

/*
  Returns a pointer to line separator for `platform` or NULL on error.
*/
const char *fu_linesep(FUPlatform platform)
{
  if (platform == fuNative) platform = fu_native_platform();
  switch (platform) {
  case fuUnix: return "\n";
  case fuWindows: return "\r\n";
  case fuApple: return "\r";
  default: return err(1, "unsupported platform: %d", platform), NULL;
  }
}

/*
  Returns non-zero if `path` is an absolute path.
*/
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
  Returns non-zero if `path`, of length `len`, is a Windows path.

  A Windows path must start with a drive letter and colon (ex "C:"),
  but not followed by two forward slashes (ex "c://", which is
  interpreted as an URL).

  Alternatively a Windows path can start with two backward slashes
  indicating a network (UNC) path.
 */
int fu_iswinpath(const char *path, int len)
{
  int n = (len >= 0) ? len : (int)strlen(path);
  if (n > 2 && path[0] == '\\' && path[1] == '\\' && path[2] != '\\')
    return 1;
  if (n < 2 || !isalpha(path[0]) || path[1] != ':') return 0;
  if (n >= 4 && path[2] == '/' && path[3] == '/') return 0;
  return 1;
}

/*
  Joins a set of pathname components, inserting '/' as needed.  The
  first path component is `a`.  The last argument must be NULL.

  If any component is an absolute path, all previous path components
  will be discarded.  An empty last part will result in a path that
  ends with a separator.

  Returns a pointer to a malloc'ed string or NULL on error.
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
    assert(len > n);
    strncpy(path, a, n);
    pos += n;
    path[pos++] = sep;
    arg0++;
  }
  for (i=0; i<arg0-1; i++)
    p = va_arg(aq, char *);
  for (i=arg0; i<nargs; i++) {
    p = va_arg(aq, char *);
    n = strlen(p);
    assert(len-pos > n);
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
  if ((p = fu_lastsep(path)) && *p) return strdup(p+1);
  return strdup(path);
}

/*
  Returns the final component of `path` without extension as a newly
  allocated string.
*/
char *fu_stem(const char *path)
{
  char *p;
  if ((p = fu_lastsep(path)) && *p) {
    p++;
    const char *ext = fu_fileext(p);
    if (ext && *ext) return strndup(p, ext - p - 1);
    return strdup(p);
  }
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
    - path starts with "C:" (where C is any character):   '/' -> '\'
    - otherwise                                           '\' -> '/'

  Returns a pointer to `path`.
 */
char *fu_friendly_dirsep(char *path)
{
#ifdef WINDOWS
  int from, to;
  char *c, *p=path;
  if (strlen(path) >= 2 &&
      ((path[0] == '/' && path[1] == '/') ||
       (path[0] == '\\' && path[1] == '\\')))
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
  Finds individual paths in `paths`, which should be a string of paths
  joined with `pathsep`.

  At the initial call, `*endptr` must point to NULL.  On return will
  `endptr` be updated to point to the first character after the
  returned path.  That is the next path separator or or the
  terminating NUL if there are no more paths left.

  If `pathsep` is not NULL, any character in `pathsep` will be used as
  a path separator.  NULL corresponds to ";:", except that the colon
  will not be considered as a path separator in the following two cases:
    - it is the second character in the path preceded by an alphabetic
      character and followed by forward or backward slash. Ex: C:\xxx..
    - it is preceded by only alphabetic characters and followed by
      two forward slashes and a alphabetic character.  Ex: http://xxx...

  Repeated path separators will be treated as a single path separator.

  Returns a pointer to the next path or NULL if there are no more paths left.
  On error is NULL returned and `*endptr` will not point to NUL.

  Example
  -------

  ```C
  char *endptr=NULL, *paths="C:\\aa\\bb.txt;/etc/fstab:http://example.com";
  const char *p;
  while ((p = fu_pathsep(paths, &endptr, NULL)))
    printf("%.*s\n", endptr - p, p);
  ```

  Should print

      C:\\aa\\bb.txt
      /etc/fstab
      http://example.com

  Note that the length of a returned path can be obtained with `endptr-p`.
 */
const char *fu_nextpath(const char *paths, char **endptr, const char *pathsep)
{
  char *p;
  if (!paths) return NULL;
  if (!*endptr)        /* first call */
    p = (char *)paths;
  else if (!**endptr)  /* exhausted, do not change endptr */
    return NULL;
  else                 /* proceeding */
    p = *endptr + 1;

  /* ignore repeated path separators */
  if (*p) {
    while (strchr((pathsep) ? pathsep : ";:", *p)) p++;
  }

  if (pathsep) {
    *endptr = p + strcspn(p, pathsep);
  } else {
    char *colon = strchr(p, ':'), *semicolon = strchr(p, ';');
    if (!colon && !semicolon) {
      *endptr = p + strcspn(p, ":");
    } else if (!colon) {
      *endptr = semicolon;
    } else {
      if (isalpha(p[0]) && p[1] == ':' && strchr("/\\", p[2])) {
        colon = strchr(p + 2, ':');
      } else if (isalpha(p[0]) && p[1] == ':' && !strchr("/\\", p[2])) {
        colon = strchr(p + 2, ':');
      } else {
        int i=0;
        while (isalpha(p[i])) i++;
        if (i > 0 && p[i] == ':' && p[i+1] == '/' && p[i+2] == '/' &&
            isalpha(p[i+3]))
          colon = strchr(p + i + 3, ':');
      }
      if (colon && semicolon)
        *endptr = (colon < semicolon) ? colon : semicolon;
      else if (colon)
        *endptr = colon;
      else if (semicolon)
        *endptr = semicolon;
      else
        *endptr = p + strlen(p);
    }
  }
  return p;
}


/*
  Converts `path` to a valid Windows path and return a pointer to the result.

  The string `path` may be a single path or several paths.  In the
  latter case they are separated with fu_nextpath().  The `pathsep` argument
  is passed to it.

  If `dest` is not NULL, the converted path is written to the memory it
  points to and `dest` is returned.  At most `size` bytes is written.

  If `dest` is NULL, sufficient memory is allocated for performing the
  full convertion and a pointer to it is returned.

  Returns NULL on error.
 */
char *fu_winpath(const char *path, char *dest, size_t size, const char *pathsep)
{
  char *q, *d, *endptr=NULL;
  const char *p;
  int n=0;
  if (!dest) {
    size = strlen(path) + 3;
    for (p=path; *p; p++) if (strchr(";:", *p)) size += 2;
    if (!(dest = malloc(size))) return err(1, "allocation failure"), NULL;
  }
  while ((p = fu_nextpath(path, &endptr, pathsep))) {
    int len = endptr - p;
    if (!fu_iswinpath(p, len) && isurln(p, len)) {
      n += snprintf(dest+n, POS(size-n), "%.*s", len, p);
    } else {
      if (globmatch("/[a-zA-Z]/*", p) == 0) {
        n += snprintf(dest+n, POS(size-n), "%c:\\%.*s", toupper(p[1]),
                      len-3, p+3);
      } else {
        n += snprintf(dest+n, POS(size-n), "%.*s", len, p);
      }
      if (*endptr) n += snprintf(dest+n, POS(size-n), ";");
    }
  }
  for (q=dest; *q; q++) if (*q == '/') *q = '\\';

  /* Remove repeated dirsep's */
  for (d=q=dest; *q; d++, q++) {
    while (*q == '\\' && *(q+1) == '\\') q++;
    *d = *q;
  }
  return dest;
}


/*
  Converts `path` to a valid Unix path and return a pointer to the result.

  The string `path` may be a single path or several paths.  In the
  latter case they are separated with fu_nextpath().  The `pathsep` argument
  is passed to it.

  If `dest` is not NULL, the converted path is written to the memory it
  points to and `dest` is returned.  At most `size` bytes is written.

  If `dest` is NULL, sufficient memory is allocated for performing the
  full convertion and a pointer to it is returned.

  Returns a pointer to `dest` or NULL on error.
 */
char *fu_unixpath(const char *path, char *dest, size_t size,
                  const char *pathsep)
{
  char *endptr=NULL;
  const char *p;
  char sep = (pathsep) ? (strchr(pathsep, ':')) ? ':' : pathsep[0] : ':';

  int n=0;
  if (!dest) {
    size = strlen(path) + 1;
    if (!(dest = malloc(size))) return err(1, "allocation failure"), NULL;
  }
  while ((p = fu_nextpath(path, &endptr, pathsep))) {
    int len = endptr - p;

    if (isurln(p, len)) {
      n += snprintf(dest+n, POS(size-n), "%.*s", len, p);
    } else {
      char *start=dest+n, *q, *d;
      if (len > 3 && isalpha(p[0]) && p[1] == ':' && strchr("\\/", p[2])) {
        n += snprintf(dest+n, POS(size-n), "/%c/%.*s", tolower(p[0]),
                      len-3, p+3);
      } else if (len > 2 && isalpha(p[0]) && p[1] == ':' &&
                 !strchr("\\/", p[2])) {
        warn("relative path prefixed with drive: '%s'. Drive is ignored, "
             "please use absolute paths in combination with drive", p);
        n +=snprintf(dest+n, POS(size-n), "%.*s", len-2, p+2);
      } else {
        n += snprintf(dest+n, POS(size-n), "%.*s", len, p);
      }
      /* Convert backward to forward slashes and remove repeated dirsep's */
      for (q=start; *q; q++) if (*q == '\\') *q = '/';
      for (d=q=dest; *q; d++, q++) {
        while (*q == '/' && *(q+1) == '/') q++, n--;
        *d = *q;
      }
    }
    /* add separator */
    if (*endptr) n += snprintf(dest+n, POS(size-n), "%c", sep);
  }

  return dest;
}

/*
  Converts `path` native platform.

  Calls fu_winpath() or fu_unixpath() depending on current platform.
 */
char *fu_nativepath(const char *path, char *dest, size_t size,
                    const char *pathsep)
{
  switch (fu_native_platform()) {
  case fuUnix:
  case fuApple:
    return fu_unixpath(path, dest, size, pathsep);
  case fuWindows:
    return fu_winpath(path, dest, size, pathsep);
  default:
    return err(1, "don't know how to convert path - current platform is "
               "neither Unix or Windows"), NULL;
  }
}


/* Returns zero if path exists. */
int fu_exists(const char *path)
{
#ifdef HAVE_ACCESS
  return access(path, F_OK);
#endif

#ifdef HAVE__ACCESS
  return _access(path, 0);
#endif

#if defined(HAVE_PathFileExistsW) && defined(HAVE_MBSTOWCS_S)
  size_t wlen;
  wchar_t wpath[MAX_PATH+1];
  if (mbstowcs_s(&wlen, wpath, MAX_PATH+1, path, MAX_PATH+1))
    return err(-1, "cannot convert path to wide char: %s", path);
  return (PathFileExistsW(wpath) == TRUE) ? 0 : 1;
#endif

  // Fallback
  FILE *fp;
  if (!(fp = fopen(path, "rb"))) return 1;
  fclose(fp);
  return 0;
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
#ifdef WINDOWS
#if defined(HAVE_GetFullPathNameW) && \
  defined(HAVE_MBSTOWCS_S) && defined(HAVE_WCSTOMBS_S)
  size_t wlen;
  wchar_t wpath[MAX_PATH + 1], *buf=NULL;
  int n;

  /* Convert path to wide characters */
  if (mbstowcs_s(&wlen, wpath, MAX_PATH + 1, path, MAX_PATH + 1)) {
    err(1, "cannot convert path '%s' to wide char", path);
    goto fail;
  }

  /* Check if path exists */
#ifdef HAVE_PathFileExistsW
  if (!PathFileExistsW(wpath)) {
    err(1, "no such file or directory: %s", path);
    goto fail;
  }
#endif

  /* Resolve path name */
  if ((n = GetFullPathNameW(wpath, 0, NULL, NULL)) <= 0) {
    err(1, "cannot determine length of canonical path for '%s'", path);
    goto fail;
  }
  if (!(buf = calloc(n+1, sizeof(wchar_t)))) {
    err(1, "allocation failure");
    goto fail;
  }
  if (GetFullPathNameW(wpath, n+1, buf, NULL) <= 0) {
    err(1, "cannot resolve canonical path for '%s'", path);
    goto fail;
  }

  /* Convert back from wide characters */
  if (!resolved_path) {
    size_t size;

    if (wcstombs_s(&size, NULL, 0, buf, 0)) {
      err(1, "cannot determine length of resolved path");
      goto fail;
    }
    if (!(resolved_path = malloc(size))) {
      err(1, "allocation failure");
      goto fail;
    }
    if (wcstombs_s(&size, resolved_path, size, buf, size)) {
      free(resolved_path);
      err(1, "cannot convert wide character to multibyte string");
      goto fail;
    }
  } else {
    if (wcstombs_s(NULL, resolved_path, MAX_PATH, buf, MAX_PATH)) {
      err(1, "cannot convert wide character to multibyte string");
      goto fail;
    }
  }
  free(buf);
  /*
#ifdef HAVE_GetFullPathNameW
  int n, size=MAX_PATH;
  if (!resolved_path) size=0;
  if (n = GetFullPathNameW(path, size, resolved_path, NULL) == 0)
    return err(fu_PathError, "cannot resolve canonical path for '%s'",
               path), NULL;
  if (!resolved_path) {
    size = n + 1;
    if (!(resolved_path = malloc(size)))
      return err(1, "allocation failure"), NULL;
    if (n = GetFullPathNameW(path, size, resolved_path, NULL) == 0)
      return err(fu_PathError, "cannot resolve canonical path for '%s'",
                 path), NULL;
  }
  if (n > size)
    return err(fu_PathError, "cannot create large enough buffer for "
               "canonicalize '%s' (limited to MAX_PATH=%d)", path, MAX_PATH);
  */
  return resolved_path;

 fail:
  if (buf) free(buf);
  return NULL;
#else
#error "Windows have no GetFullPathNameW()"
  /* Poor man's realpath - do nothing... */
  if (resolved_path) {
    if (!(resolved_path = strdup(path)))
      return err(1, "allocation failure"), NULL;
  } else {
    strncpy(resolved_path, path, MAX_PATH);
    resolved_path[MAX_PATH - 1] = '\0';
  }
  return resolved_path;
#endif
#else  /* ! WINDOWS */
  return realpath(path, resolved_path);
#endif
}


/*
  Opens a directory and returns a FUDir handle to it.  Returns NULL on error.
 */
FUDir *fu_opendir(const char *path)
{
  FUDir *dir;
  if (!(dir = (FUDir *)opendir(path)))
    return err(fu_OpenDirectoryError, "cannot open directory \"%s\"",
               path), NULL;
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

#if 0  // XXX
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
  const char *p;
  char *endptr=NULL, *s = (envvar) ? getenv(envvar) : NULL;
  memset(paths, 0, sizeof(FUPaths));
  if (pathsep) paths->pathsep = strdup(pathsep);
  paths->platform = fu_native_platform();
  while ((p = fu_nextpath(s, &endptr, pathsep)))
    fu_paths_appendn(paths, p, endptr - p);
  return 0;
}

/*
  Sets platform that `paths` should confirm to.

  Returns the previous platform or -1 on error.
*/
FUPlatform fu_paths_set_platform(FUPaths *paths, FUPlatform platform)
{
  size_t i;
  FUPlatform prev = paths->platform;
  if (platform < 0 || platform >= fuLastPlatform)
    return err(-1, "invalid platform number: %d", platform);
  if (platform == fuNative)
    platform = fu_native_platform();
  if (platform == paths->platform)
    return 0;
  paths->platform = platform;
  if (paths->pathsep) free((void *)paths->pathsep);
  paths->pathsep = strdup(fu_pathsep(platform));

  for (i=0; i < paths->n; i++) {
    const char *p = paths->paths[i];
    if (platform == fuUnix || platform == fuApple)
      paths->paths[i] = fu_unixpath(p, NULL, 0, ":");
    else if (platform == fuWindows)
      paths->paths[i] = fu_winpath(p, NULL, 0, ";");
    else {
      warn("unsupported platform: %s", fu_platform_name(platform));
      paths->paths[i] = strdup(p);
    }
    free((void *)p);
  }

  return prev;
}

/*
  Returns the current platform for `paths`.
 */
FUPlatform fu_paths_get_platform(FUPaths *paths)
{
  return paths->platform;
}

/*
  Frees all memory allocated in `paths`.
 */
void fu_paths_deinit(FUPaths *paths)
{
  size_t n;
  if (paths->paths) {
    for (n=0; n<paths->n; n++) free((char *)paths->paths[n]);
    free((void *)paths->paths);
  }
  if (paths->pathsep) free((void *)(paths->pathsep));
  memset(paths, 0, sizeof(FUPaths));
}


/*
  Returns an allocated string with all paths in `paths` separated by
  the path separator of the platform specified by fu_paths_set_platform().

  Returns NULL on error.
*/
char *fu_paths_string(const FUPaths *paths)
{
  size_t i, seplen, size=0;
  char *s, *string;
  if (!paths->n) return strdup("");
  const char *pathsep = (paths->pathsep) ?
    paths->pathsep : fu_pathsep(paths->platform);
  seplen = strlen(pathsep);
  for (i=0; i<paths->n; i++) size += strlen(paths->paths[i]);
  size += (paths->n - 1) * seplen;
  if (!(s = string = malloc(size + 1)))
    return err(1, "allocation failure"), NULL;
  for (i=0; i < paths->n; i++) {
    size_t n = strlen(paths->paths[i]);
    strncpy(s, paths->paths[i], n);
    s += n;
    if (i < paths->n - 1) {
      strncpy(s, pathsep, seplen);
      s += seplen;
    }
  }
  *s = '\0';
  assert((long)size >= s - string);
  return string;
}

/*
  Returns a NULL-terminated array of pointers to paths or NULL if
  `paths` is empty.

  The memory own by `paths` and should not be deallocated by the
  caller.
 */
const char **fu_paths_get(FUPaths *paths)
{
  assert(paths);
  if (!paths->n) return NULL;
  return paths->paths;
}


/*
  Inserts `path` into `paths` before position `n`.  If `n` is
  negative, it counts from the end (like Python).

  If `path` already exists in `paths`, it is moved to position `n`, but
  not duplicated.

  Returns the index of the newly inserted element or -1 on error.
 */
int fu_paths_insert(FUPaths *paths, const char *path, int n)
{
  return fu_paths_insertn(paths, path, 0, n);
}


/*
  Like fu_paths_insert(), but allows that `path` is not NUL-terminated.

  Inserts the `len` first bytes of `path` into `paths` before position `n`.
  If `len` is zero, this is equivalent to fu_paths_insert().

  If `n` is negative, it counts from the end (like Python).

  If `path` already exists in `paths`, it is moved to position `n`, but
  not duplicated.

  Returns the index of the newly inserted element or -1 on error.
 */
int fu_paths_insertn(FUPaths *paths, const char *path, size_t len, int n)
{
  int platform = paths->platform;
  char *p=NULL, *tmp=NULL;
  int index;

  if (n < -(int)(paths->n) || n >= (int)paths->n+1)
    FAIL1("path index out of range: %d", n);
  if (n < 0) n += paths->n;

  if (len) {
    if (!(tmp = strndup(path, len))) FAIL("allocation failure");
    path = tmp;
  }

  if (platform == fuNative) platform = fu_native_platform();
  if (!fu_supported_platform(platform))
    FAIL1("unsupported platform: %d", platform);
  switch (platform) {
  case fuUnix:
  case fuApple:
    p = fu_unixpath(path, NULL, 0, paths->pathsep);
    break;
  case fuWindows:  p = fu_winpath(path, NULL, 0, paths->pathsep);   break;
  default: assert(0);  // should never happen
  }
  if (!p) FAIL("allocation failure");

  if ((index = fu_paths_index(paths, p)) >= 0) {

    /* path already in paths - remove it if it is not at expected position */
    if (index == n || (n == (int)paths->n && index == (int)paths->n - 1)) {
      if (p) free(p);
      if (tmp) free(tmp);
      return index;  // path already at expected position
    }
    if (fu_paths_remove_index(paths, index)) goto fail;
    if (n > index) n--;
  }

  if (paths->n+1 >= paths->size) {
    const char **q;
    paths->size = paths->n + FU_PATHS_CHUNKSIZE;
    if (!(q = realloc((char **)paths->paths, paths->size*sizeof(char **))))
      FAIL("reallocation failure");
    paths->paths = q;
  }
  if (n < (int)paths->n)
    memmove((char **)paths->paths + n + 1, paths->paths + n,
            (paths->n-n)*sizeof(char **));
  paths->paths[n] = p;
  paths->paths[++paths->n] = NULL;
  if (tmp) free(tmp);
  return n;
 fail:
  if (p) free(p);
  if (tmp) free(tmp);
  return -1;
}

/*
  Appends `path` to `paths` (if it not already exists).  Equivalent to

      fu_paths_insert(paths, path, paths->n)

  Returns the index of the newly appended element or -1 on error.
 */
int fu_paths_append(FUPaths *paths, const char *path)
{
  return fu_paths_insert(paths, path, paths->n);
}

/*
  Appends `path` to `paths`.  Equivalent to

      fu_paths_insertn(paths, path, len, paths->n)

  Returns the index of the newly inserted element or -1 on error.
 */
int fu_paths_appendn(FUPaths *paths, const char *path, size_t len)
{
  return fu_paths_insertn(paths, path, len, paths->n);
}

/*
  Extends `paths` by appending all `pathsep`-separated paths in `s` to it.
  See fu_nextpath() for a description of `pathsep`.

  Returns the index of the last appended element or zero if nothing is appended.
  On error, -1 is returned.
*/
int fu_paths_extend(FUPaths *paths, const char *s, const char *pathsep)
{
  const char *p;
  char *endptr=NULL;
  int stat=0;
  while ((p = fu_nextpath(s, &endptr, pathsep)))
    if ((stat = fu_paths_appendn(paths, p, endptr - p)) < 0) return stat;
  return stat;
}

/*
  Like fu_paths_extend(), but prefix all relative paths in `s` with `prefix`
  before appending them to `paths`.

  Returns the index of the last appended element or zero if nothing is appended.
  On error, -1 is returned.
*/
int fu_paths_extend_prefix(FUPaths *paths, const char *prefix,
                           const char *s, const char *pathsep)
{
  const char *p;
  char *endptr=NULL;
  int stat=0;
  while ((p = fu_nextpath(s, &endptr, pathsep))) {
    int len = endptr - p;
    if (fu_isabs(p)) {
      if ((stat = fu_paths_appendn(paths, p, len)) < 0) return stat;
    } else {
      char buf[1024];
      int n = snprintf(buf, sizeof(buf), "%s/%.*s", prefix, len, p);
      if (n < 0) return err(-1, "unexpected error in snprintf()");
      if (n >= (int)sizeof(buf) - 1)
        return err(-1, "path exeeds buffer size: %s/%.*s", prefix, len, p);
      if ((stat = fu_paths_append(paths, buf)) < 0) return stat;
    }
  }
  return stat;
}

/*
  Removes path index `n` from `paths`.  If `n` is negative, it
  counts from the end (like Python).

  Returns non-zero on error.
 */
int fu_paths_remove_index(FUPaths *paths, int index)
{
  if (index < -(int)(paths->n) || index >= (int)paths->n+1)
    return err(-1, "path index out of range: %d", index);
  if (index < 0) index += paths->n;
  assert(paths->paths[index]);
  free((char *)paths->paths[index]);
  memmove((char **)paths->paths+index, paths->paths+index+1,
          (paths->n-index)*sizeof(char **));
  paths->n--;
  return 0;
}

/*
  Removes path `path`.  Returns non-zero if there is no such path.
 */
int fu_paths_remove(FUPaths *paths, const char *path)
{
  int i = fu_paths_index(paths, path);
  if (i < 0) return -1;
  if (fu_paths_remove_index(paths, i)) return 1;
  return 0;
}

/*
  Returns index of path `path` or -1 if there is no such path in `paths`.
 */
int fu_paths_index(FUPaths *paths, const char *path)
{
  size_t i;
  for (i=0; i < paths->n; i++)
    if (strcmp(paths->paths[i], path) == 0) return i;
  return -1;
}



/* ---------------------------------------------------------- */

/* Frees NULL-terminated list of malloc'ed strings.
   Returns non-zero on error. */
static int strlist_free(char **strlist)
{
  char **q;
  if (!strlist) return err(1, "string list is NULL");
  for (q=strlist; *q; q++) free(*q);
  free(strlist);
  return 0;
}

/* Returns a copy of NULL-terminated list of malloc'ed strings. */
static char **strlist_copy(const char **strlist)
{
  char **cpy;
  size_t i, n=0;
  if (!strlist) return NULL;
  while (strlist[n]) n++;
  if (!(cpy = calloc(n+1, sizeof(char *))))
    return err(1, "allocation failure"), NULL;
  for (i=0; i<n; i++) {
    if (!(cpy[i] = strdup(strlist[i]))) {
      strlist_free(cpy);
      return err(1, "allocation failure"), NULL;
    }
  }
  return cpy;
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
  FUIter *iter=NULL;
  if (!(iter = calloc(1, sizeof(FUIter)))) FAIL("allocation failure");
  iter->pattern = pattern;
  iter->i = 0;
  iter->paths = paths;
  iter->origlen = paths->n;
  if (!(iter->origpaths = strlist_copy(paths->paths))) goto fail;
  iter->filename = NULL;
  iter->path = NULL;
  iter->pathsize = 0;
  iter->dir = NULL;
  iter->dirsep = DIRSEP[0];
  return iter;
 fail:
  if (iter->origpaths) strlist_free(iter->origpaths);
  if (iter) free(iter);
  return NULL;
}

/*
  Returns name of the next file matching the pattern provided to
  fu_startmatch() or NULL if there are no more matches.

  Note:
  The returned string is owned by the iterator. It will be overwritten
  by the next call to fu_nextmatch() and should not be changed.  Use
  strdup() or strncpy() if a copy is needed.

  Note 2:
  The iterator will not be affected by adding or deleting paths during
  iteration.  Hence, it is safe to insert a path or delete the path
  returned by fu_nextmatch().  But if you delete a yet non-visited path
  it will still be visited.
 */
const char *fu_nextmatch(FUIter *iter)
{
  const char *filename;
  char *tmppath=NULL;
  char dirsep[2] = { iter->dirsep, 0 };
  int iswin = iter->paths->platform == fuWindows ||
    (iter->paths->platform == fuNative && fu_native_platform() == fuNative);
  if (iter->i >= iter->origlen) return NULL;

  while (iter->i < iter->origlen) {
    UrlComponents comp;
    const char *path = iter->origpaths[iter->i];
    tmppath = NULL;

    if (!(iswin && fu_iswinpath(path, -1)) && urlsplit(path, &comp)) {
      if (strncmp(comp.scheme, "file", comp.scheme_len) == 0) {
        if (!(tmppath = strndup(comp.path, comp.path_len)))
          FAIL("allocation failure");
        path = (const char *)tmppath;
      } else if (globmatch(iter->pattern, path) == 0) {
        iter->i++;
        return path;
      } else {
        iter->i++;
        continue;
      }
    }

    if (!iter->dir) {
      if (iter->i >= iter->origlen) return NULL;
      if (!path[0]) path = ".";

      ErrTry:
        iter->dir = fu_opendir(path);
      ErrCatch(fu_OpenDirectoryError):  // suppress open directory errors
        break;
      ErrEnd;

      if (!iter->dir) {
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
            FAIL("allocation failure");
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
    if (tmppath) free(tmppath);
    tmppath = NULL;
  }
 fail:
  if (tmppath) free(tmppath);
  return NULL;
}

/*
  Ends pattern matching iteration.
 */
int fu_endmatch(FUIter *iter)
{
  int status = 0;
  if (iter->path) free(iter->path);
  if (iter->dir) fu_closedir(iter->dir);
  status |= strlist_free(iter->origpaths);
  free(iter);
  return status;
}

/* ---------------------------------------------------------- */

/*
  Returns a new iterator over all files and directories in `paths`.

  An optional `pattern` can be provided to filter out file and
  directory names that doesn't matches it.  This pattern will only
  match against the base file/directory name, with the directory part
  stripped off.

  Returns NULL on error.

  This is very similar to fu_startmatch(), but allows paths to be a
  mixture of directories and files with glob patterns.  Is intended to
  be used together with fu_pathsiter_next() and fu_pathsiter_deinit().
 */
FUIter *fu_pathsiter_init(const FUPaths *paths, const char *pattern)
{
  FUIter *iter;
  if (!(iter = calloc(1, sizeof(FUIter))))
    return err(1, "Allocation failure"), NULL;
  iter->paths = paths;
  iter->pattern = pattern;
  iter->dirsep = DIRSEP[0];
  iter->origlen = paths->n;
  if (!(iter->origpaths = (paths->paths) ? strlist_copy(paths->paths) : NULL))
    goto fail;
  return iter;
 fail:
  if (iter) free(iter);
  return NULL;
}

/*
  Help function for fu_pathsiter_next(), which does not take `pattern`
  into account.
 */
const char *_fu_pathsiter_next(FUIter *iter)
{
  if (iter->i >= iter->origlen) return NULL;

  while (iter->i < iter->origlen) {
    const char *path;

    /* Check for ongoing iteration over a directory... */
    if (iter->dir) {
      if ((iter->filename = (char *)fu_nextfile(iter->dir))) {
        size_t n = strlen(iter->dirname);
        size_t m = strlen(iter->filename);
        size_t len = n + m + 2;
        if (!strcmp(iter->filename, ".") || !strcmp(iter->filename, ".."))
          continue;
        if (iter->pathsize < len) {
          iter->pathsize = len;
          if (!(iter->path = realloc((iter->path), iter->pathsize)))
            return err(1, "allocation failure"), NULL;
        }
        memcpy(iter->path, iter->dirname, n);
        iter->path[n] = iter->dirsep;
        memcpy(iter->path + n + 1, iter->filename, m + 1);
        iter->path[n+m+1] = '\0';
        return iter->path;
      } else {
        fu_closedir(iter->dir);
        free(iter->dirname);
        iter->dir = NULL;
        iter->dirname = NULL;
        iter->filename = NULL;
        if (++iter->i >= iter->origlen) break;
      }
    }

    /* Check for ongoing iteration over a glob path... */
    if (iter->globiter) {
      if ((path = (char *)fu_globnext(iter->globiter))) {
        char *p;
        size_t len = strlen(path) + 1;
        if (iter->pathsize < len) {
          iter->pathsize = len;
          if (!(iter->path = realloc((iter->path), iter->pathsize)))
            return err(1, "allocation failure"), NULL;
        }
        memcpy(iter->path, path, len);
        iter->filename = (p = strrchr(iter->path, iter->dirsep)) ?
          p + 1 : iter->path;
        return iter->path;
      } else {
        fu_globend(iter->globiter);
        iter->globiter = NULL;
        if (++iter->i >= iter->origlen) break;
      }
    }

    assert(!iter->dir && !iter->globiter);
    path = iter->origpaths[iter->i];

    /* Check if `path` is a directory...
       Note that we are calling opendir() directly, to avoid errors if
       `path` is not a directory. */
    if ((iter->dir = opendir(path))) {
      iter->dirname = strdup(path);
      continue;
    }

    /* Check if `path` is a glob pattern... */
    if ((iter->globiter = fu_glob(path, iter->paths->pathsep))) continue;
    assert(0);  /* this should never be reached */
  }
  return NULL;
}

/*
  Returns the next file or directory in the iterator `iter` created
  with fu_paths_iter_init().  NULL is returned on error or if there
  are no more file names to iterate over.

  Note:
  The iterator will not be affected by adding or deleting paths during
  iteration.  Hence, it is safe to insert a path or delete the path
  returned by fu_pathiter_next().  But if you delete a yet non-visited
  path it will still be visited.
 */
const char *fu_pathsiter_next(FUIter *iter)
{
  const char *path;

  if (!(path = _fu_pathsiter_next(iter))) return NULL;
  if (!iter->pattern) return path;

  while (globmatch(iter->pattern, iter->filename))
    if (!(path = _fu_pathsiter_next(iter))) return NULL;

  return path;
}

/*
  Deallocates iterator created with fu_pathsiter_init().
  Returns non-zero on error.
 */
int fu_pathsiter_deinit(FUIter *iter)
{
  int status=0;
  if (iter->dirname) free(iter->dirname);
  if (iter->path) free(iter->path);
  if (iter->dir) status |= fu_closedir(iter->dir);
  if (iter->globiter) status |= fu_globend(iter->globiter);
  status |= strlist_free(iter->origpaths);
  free(iter);
  return status;
}


/* ---------------------------------------------------------- */


/*
  Returns a new iterator over files matching `pattern`.
  Only the last component of `pattern` may contain wildcards.

  Use fu_globnext() and fu_globend() to iterate over matching files
  and end iteration, respectively.
 */
FUIter *fu_glob(const char *pattern, const char *pathsep)
{
  FUIter *iter=NULL;
  FUPaths *paths=NULL;
  char *dirname=NULL, *basename=NULL;
  UrlComponents comp;
  int len = strlen(pattern);
  if (!(paths = malloc(sizeof(FUPaths)))) FAIL("allocation failure");
  fu_paths_init_sep(paths, NULL, pathsep);
  if (!fu_iswinpath(pattern, len) && urlsplitn(pattern, len, &comp) == len) {
    if (strncmp(comp.scheme, "file", comp.scheme_len) == 0) {
      if (!(basename = fu_basename(comp.path))) goto fail;
      if (!(dirname = fu_dirname(comp.path))) goto fail;
    } else {
      if (!(basename = strdup(pattern))) goto fail;
      if (!(dirname = strdup(pattern))) goto fail;
    }
  } else {
    if (!(basename = fu_basename(pattern))) goto fail;
    if (!(dirname = fu_dirname(pattern))) goto fail;
  }
  if (!*dirname) {
    free(dirname);
    dirname = strdup(".");
  }
  fu_paths_append(paths, dirname);
  iter = fu_startmatch(basename, paths);
 fail:
  if (dirname) free(dirname);
  if (!iter) {
    fu_paths_deinit(paths);
    free(paths);
  }
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
  int status = fu_endmatch(iter);
  fu_paths_deinit(paths);
  free(paths);
  free(pattern);
  return status;
}

/*
  Sets the directory separator in returned by fu_nextmatch() and
  fu_globnext().  Defaults to DIRSEP.
*/
void fu_iter_set_dirsep(FUIter *iter, int dirsep)
{
  iter->dirsep = dirsep;
}



/*
  Read `stream` into an malloc'ed buffer and return a pointer to the buffer.
  Returns NULL on error.
*/
char *fu_readfile(FILE *fp)
{
  char *ptr, *buf=NULL;
  size_t n, newsize, size=0, pos=0, chunk_size=4096;
  while (1) {
    size += chunk_size;
    if (!(ptr = realloc(buf, size))) {
      if (buf) free(buf);
      return  err(1, "allocation failure"), NULL;
    }
    buf = ptr;
    if ((n = fread(buf+pos, 1, chunk_size, fp)) < chunk_size) break;
    pos += n;
  }
  if (ferror(fp)) {
    free(buf);
    return err(1, "error reading file"), NULL;
  }
  newsize = pos + n;
  assert(newsize < size);
  buf = realloc(buf, newsize + 1);
  buf[newsize] = '\0';
  return buf;
}
