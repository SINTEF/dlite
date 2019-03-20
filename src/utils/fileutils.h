/* fileutils.h -- cross-platform file utility functions */
#ifndef _FILEUTILS_H
#define _FILEUTILS_H

/**
  @file
  @brief Cross-platform file utility functions
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

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
typedef DIR FUDir;
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


/** Directory state. */
typedef DIR FUDir;

/** List of directory search pathes */
typedef struct _FUPaths {
  size_t n;            /*!< number of paths */
  size_t size;         /*!< allocated number of paths */
  const char **paths;  /*!< NULL-terminated array of pointers to paths */
} FUPaths;

/** File matching iterator. */
typedef struct _FUIter FUIter;


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
 */
char *fu_join(const char *a, ...);

/**
  Like fu_join(), but takes path separator as first argument.
*/
char *fu_join_sep(int sep, const char *a, ...);

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
  Frees all memory allocated in `paths`.
 */
void fu_paths_deinit(FUPaths *paths);

/**
  Returns a NULL-terminated array of pointers to paths or NULL if
  `paths` is empty.
 */
const char **fu_paths_get(FUPaths *paths);

/**
  Inserts `path` into `paths` before position `n`.  If `n` is negative, it
  counts from the end (like Python).

  Returns the index of the newly inserted element or -1 on error.
 */
int fu_paths_insert(FUPaths *paths, const char *path, int n);

/**
  Appends `path` to `paths`.  Equivalent to

      fu_paths_insert(paths, path, paths->n)

  Returns the index of the newly appended element or -1 on error.
 */
int fu_paths_append(FUPaths *paths, const char *path);

/**
  Removes path index `n` from `paths`.  If `n` is negative, it
  counts from the end (like Python).

  Returns non-zero on error.
 */
int fu_paths_remove(FUPaths *paths, int n);


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
 */
const char *fu_nextmatch(FUIter *iter);

/**
  Ends pattern matching iteration.
 */
int fu_endmatch(FUIter *iter);



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

#endif  /*  _FILEUTILS_H */
