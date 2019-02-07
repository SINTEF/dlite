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


#endif  /*  _FILEUTILS_H */
