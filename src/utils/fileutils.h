/* fileutils.h -- cross-platform file utility functions */
#ifndef _FILEUTILS_H
#define _FILEUTILS_H

/**
  @file
  @brief Cross-platform file utility functions
 */


#if defined __unix__ || (defined __APPLE__ && defined __MARCH__)
/* POSIX */
# ifndef POSIX
# define POSIX
# endif

#include <sys/types.h>
#include <dirent.h>

#ifndef PATHSEP
#define PATHSEP ':'
#endif

#elif defined WIN32 || defined _WIN32 || defined __WIN32__
/* WINDOWS */
# ifndef WINDOWS
# define WINDOWS
# endif

#include "compat/dirent.h"

#ifndef PATHSEP
#define PATHSEP ';'
#endif

#else
# warning "Neither POSIX or Windows"
#endif

/** Directory state. */
typedef DIR FUDir;

/** File matching iterator. */
typedef struct _FUPaths FUPaths;


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
  Returns a new iterator for finding files matching `pattern`.
  `paths` is a PATHSEP-separated list of directory paths to search.
  If `envvar` is not NULL, it should be the name of an environment
  variable containing additional paths to search.
 */
FUPaths *fu_startmatch(const char *pattern, const char *paths,
                       const char *envvar);

/**
  Returns name of the next file matching the pattern provided to
  fu_startmatch() or NULL if there are no more matches.
 */
const char *fu_nextmatch(FUPaths *iter);

/**
  Ends pattern matching iteration.
 */
int fu_endmatch(FUPaths *iter);


#endif  /*  _FILEUTILS_H */
