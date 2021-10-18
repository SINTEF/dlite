/* fileutils.h -- cross-platform file utility functions
 *
 * Copyright (C) 2017 SINTEF
 *
 * Distributed under terms of the MIT license.
 */
#ifndef _FILEUTILS_H
#define _FILEUTILS_H

#include <stdio.h>
#include <stdarg.h>

/**
  @file
  @brief Cross-platform file utility functions
 */

/** Error codes */
enum {
 fu_PathError=5870,
 fu_OpenDirectoryError,
};


#if defined __unix__ || (defined __APPLE__ && defined __MARCH__)
/* POSIX */
# ifndef POSIX
#  define POSIX
# endif
# include <sys/types.h>
# include <dirent.h>
# ifndef PATHSEP
#  define PATHSEP ":"  /*!< Separator in PATH environment variable */
# endif
# ifndef DIRSEP
#  define DIRSEP "/"   /*!< Directory separator file paths */
# endif
#elif defined WIN32 || defined _WIN32 || defined __WIN32__
/* WINDOWS */
# ifndef WINDOWS
#  define WINDOWS
# endif
# include "compat/dirent.h"
# ifndef PATHSEP
#  define PATHSEP ";"
# endif
# ifndef DIRSEP
#  define DIRSEP "\\"
# endif
#else
# warning "Neither POSIX or Windows"
#endif

#ifndef FU_PATHS_CHUNKSIZE
# define FU_PATHS_CHUNKSIZE 16  /*!< Chunk size for path allocation */
#endif

#ifndef __GNUC__
# define __attribute__(x)
#endif

/** Platform */
typedef enum _FUPlatform {
  fuUnknownPlatform=-1,  /*!< Unknown platform */
  fuNative=0,            /*!< Platform we are compiling on */
  fuUnix,                /*!< Unix-like platforms; POSIX complient */
  fuWindows,             /*!< Windows */
  fuApple,               /*!< Apple - not yet supported... */
  fuLastPlatform         /*!< Must always be the last */
} FUPlatform;

/** Directory state. */
typedef DIR FUDir;

/** List of directory search pathes */
typedef struct _FUPaths {
  size_t n;             /*!< number of paths */
  size_t size;          /*!< allocated number of paths */
  const char **paths;   /*!< NULL-terminated array of pointers to paths */
  FUPlatform platform;  /*!< platform that returned paths should confirm to,
                             defaults to native */
} FUPaths;

/** File matching iterator. */
typedef struct _FUIter FUIter;


/**
  Returns native platform.
 */
FUPlatform fu_native_platform(void);

/**
  Returns non-zero if `platform` is supported.
*/
int fu_supported_platform(FUPlatform platform);

/**
  Returns a constant pointer to the name of `platform`.
 */
const char *fu_platform_name(FUPlatform platform);

/**
  Returns the platform number corresponding to `name` or -1 on error.
 */
FUPlatform fu_platform(const char *name);

/**
  Returns a pointer to directory separator for `platform` or NULL on error.
*/
const char *fu_dirsep(FUPlatform platform);

/**
  Returns a pointer to path separator for `platform` or NULL on error.
*/
const char *fu_pathsep(FUPlatform platform);

/**
  Returns a pointer to line separator for `platform` or NULL on error.
*/
const char *fu_linesep(FUPlatform platform);


/**
  Returns non-zero if `path` is an absolute path.
*/
int fu_isabs(const char *path);

/**
  Joins a set of pathname components, inserting '/' as needed.  The
  first path component is `a`.  The last argument must be NULL.

  If any component is an absolute path, all previous path components
  will be discarded.  An empty last part will result in a path that
  ends with a separator.

  Returns a pointer to a malloc'ed string or NULL on error.
 */
char *fu_join(const char *a, ...)
  __attribute__((sentinel));

/**
  Like fu_join(), but takes path separator as first argument.
*/
char *fu_join_sep(int sep, const char *a, ...)
  __attribute__((sentinel));

/**
  Like fu_join_sep(), but takes a va_list instead of a variable number
  of arguments.
*/
char *fu_vjoin_sep(int sep, const char *a, va_list ap);

/**
  Returns a pointer to the last directory separator in `path`.
 */
char *fu_lastsep(const char *path);

/**
  Returns the directory component of `path` as a newly allocated string.
*/
char *fu_dirname(const char *path);

/**
  Returns the final component of `path` as a newly allocated string.
*/
char *fu_basename(const char *path);

/**
  Returns a pointer to file extension of `path` (what follows after
  the last ".").
*/
const char *fu_fileext(const char *path);

/**
  Updates `path` to use more "user-friendly" directory separators.

  On Unix-like systems this function does nothing.

  On Windows, the following logic is applied:
    - path starts with "//" or "\\":                      '/' -> '\'
    - path starts with "C:" (where C is any character):   '\' -> '/'
    - otherwise                                           '\' -> '/'

  Returns a pointer to `path`.
 */
char *fu_friendly_dirsep(char *path);


/**
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
      character and followed by forward or backward slash. Ex: 'C:\xxx'.
    - it is preceded by only alphabetic characters and followed by
      two foreard slashes and a alphabetic character.  Ex: http://xxx...

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

  @note
  The length of a returned path can be obtained with `endptr-p`.
 */
const char *fu_nextpath(const char *paths, char **endptr, const char *pathsep);


/**
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
char *fu_winpath(const char *path, char *dest, size_t size,
                 const char *pathsep);


/**
  Converts `path` to a valid Unix path and return a pointer to the result.

  The string `path` may be a single path or several paths.  In the
  latter case they are separated with fu_nextpath().  The `pathsep` argument
  is passed to it.

  If `dest` is not NULL, the converted path is written to the memory it
  points to and `dest` is returned.  At most `size` bytes is written.

  If `dest` is NULL, sufficient memory is allocated for performing the
  full convertion and a pointer to it is returned.

  Returns NULL on error.
 */
char *fu_unixpath(const char *path, char *dest, size_t size,
                  const char *pathsep);


/**
  Converts `path` native platform.

  Calls fu_winpath() or fu_unixpath() depending on current platform.
 */
char *fu_nativepath(const char *path, char *dest, size_t size,
                    const char *pathsep);


/**
  Returns the canonicalized absolute pathname for `path`.  Resolves
  symbolic links and references to '/./', '/../' and extra '/'.  Note
  that `path` must exists.

  If `resolved_path` is NULL, the returned path is malloc()'ed.
  Otherwise, it must be a buffer of at least size PATH_MAX (on POSIX)
  or MAX_PATH (on Windows).

  Returns NULL on error.
 */
char *fu_realpath(const char *path, char *resolved_path);


/**
  Opens a directory and returns a handle to it.  Returns NULL on error.
*/
FUDir *fu_opendir(const char *path);

/**
  Iterates over all files in a directory.

  Returns the name of next file or NULL if there are no more files.
*/
const char *fu_nextfile(FUDir *dir);

/**
  Closes directory handle.
*/
int fu_closedir(FUDir *dir);


/**
  Initiates `paths`.  If `envvar` is not NULL, it should be the name
  of an environment variable with initial search paths separated by
  the PATHSEP charecter.

  Returns the initial number of paths or -1 on error.
 */
int fu_paths_init(FUPaths *paths, const char *envvar);

/**
  Like fu_paths_init(), but allow custom path separator `pathsep`.

  Note that any character in `pathsep` works as a path separator.
 */
int fu_paths_init_sep(FUPaths *paths, const char *envvar, const char *pathsep);

/**
  Sets platform that `paths` should confirm to.

  Returns the previous platform or -1 on error.
*/
FUPlatform fu_paths_set_platform(FUPaths *paths, FUPlatform platform);

/**
  Returns the current platform for `paths`.
 */
FUPlatform fu_paths_get_platform(FUPaths *paths);

/**
  Frees all memory allocated in `paths`.
 */
void fu_paths_deinit(FUPaths *paths);

/**
  Returns an allocated string with all paths in `paths` separated by
  the path separator of the platform specified by fu_paths_set_platform().

  Returns NULL on error.
 */
char *fu_paths_string(const FUPaths *paths);

/**
  Returns a NULL-terminated array of pointers to paths or NULL if
  `paths` is empty.

  The memory own by `paths` and should not be deallocated by the
  caller.
 */
const char **fu_paths_get(FUPaths *paths);

/**
  Inserts `path` into `paths` before position `n`.  If `n` is negative, it
  counts from the end (like Python).

  Returns the index of the newly inserted element or -1 on error.
 */
int fu_paths_insert(FUPaths *paths, const char *path, int n);

/**
  Like fu_paths_insert(), but allows that `path` is not NUL-terminated.

  Inserts the `len` first bytes of `path` into `paths` before position `n`.
  If `len` is zero, this is equivalent to fu_paths_insert().

  Returns the index of the newly inserted element or -1 on error.
 */
int fu_paths_insertn(FUPaths *paths, const char *path, size_t len, int n);

/**
  Appends `path` to `paths`.  Equivalent to

      fu_paths_insert(paths, path, paths->n)

  Returns the index of the newly appended element or -1 on error.
 */
int fu_paths_append(FUPaths *paths, const char *path);

/**
  Appends `path` to `paths`.  Equivalent to

      fu_paths_insertn(paths, path, len, paths->n)

  Returns the index of the newly inserted element or -1 on error.
 */
int fu_paths_appendn(FUPaths *paths, const char *path, size_t len);

/**
  Extends `paths` by appending all `pathsep`-separated paths in `s` to it.
  See fu_nextpath() for a description of `pathsep`.

  Returns the index of the last appended element or zero if nothing is appended.
  On error, -1 is returned.
*/
int fu_paths_extend(FUPaths *paths, const char *s, const char *pathsep);

/**
  Like fu_paths_extend(), but prefix all relative paths in `s` with `prefix`
  before appending them to `paths`.

  Returns the index of the last appended element or zero if nothing is appended.
  On error, -1 is returned.
*/
int fu_paths_extend_prefix(FUPaths *paths, const char *prefix, const char *s,
                           const char *pathsep);

/**
  Removes path index `n` from `paths`.  If `n` is negative, it
  counts from the end (like Python).

  Returns non-zero on error.
 */
int fu_paths_remove_index(FUPaths *paths, int index);

/**
  Removes path `path`.  Returns non-zero if there is no such path.
 */
int fu_paths_remove(FUPaths *paths, const char *path);

/**
  Returns index of path `path` or -1 if there is no such path in `paths`.
 */
int fu_paths_index(FUPaths *paths, const char *path);



/**
  Returns a new iterator for finding files matching `pattern` in
  directories included in `paths`.

  Returns NULL on error.
 */
FUIter *fu_startmatch(const char *pattern, const FUPaths *paths);

/**
  Returns name of the next file matching the pattern provided to
  fu_startmatch() or NULL if there are no more matches.

  @note
  The returned string is owned by the iterator. It will be overwritten
  by the next call to fu_nextmatch() and should not be changed.  Use
  strdup() or strncpy() if a copy is needed.

  @note
  The iterator will not be affected by adding or deleting paths during
  iteration.  Hence, it is safe to insert a path or delete the path
  returned by fu_nextmatch().  But if you delete a yet non-visited path
  it will still be visited.
 */
const char *fu_nextmatch(FUIter *iter);

/**
  Ends pattern matching iteration.
 */
int fu_endmatch(FUIter *iter);


/**
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
FUIter *fu_pathsiter_init(const FUPaths *paths, const char *pattern);

/**
  Returns the next file or directory in the iterator `iter` created
  with fu_paths_iter_init().  NULL is returned on error or if there
  are no more file names to iterate over.

  @note
  The iterator will not be affected by adding or deleting paths during
  iteration.  Hence, it is safe to insert a path or delete the path
  returned by fu_pathiter_next().  But if you delete a yet non-visited
  path it will still be visited.
 */
const char *fu_pathsiter_next(FUIter *iter);

/**
  Deallocates iterator created with fu_pathsiter_init().
  Returns non-zero on error.
 */
int fu_pathsiter_deinit(FUIter *iter);



/**
  Returns a new iterator over files matching `pattern`.
  Only the last component of `pattern` may contain wildcards.

  Use fu_globnext() and fu_globend() to iterate over matching files
  and end iteration, respectively.
 */
FUIter *fu_glob(const char *pattern);

/**
  Returns path to the next matching file.
 */
const char *fu_globnext(FUIter *iter);

/**
  Ends glob pattern iteration.  Returns non-zero on error.
 */
int fu_globend(FUIter *iter);

/**
  Sets the directory separator in returned by fu_nextmatch() and
  fu_globnext().  Defaults to DIRSEP.
*/
void fu_iter_set_dirsep(FUIter *iter, int dirsep);



/**
  Read `stream` into an malloc'ed buffer and return a pointer to the buffer.
  Returns NULL on error.
*/
char *fu_readfile(FILE *fp);


#endif  /*  _FILEUTILS_H */
