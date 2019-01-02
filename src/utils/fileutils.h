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

typedef DIR fu_dir;
//typedef int fu_file;


#elif defined WIN32 || defined _WIN32 || defined __WIN32__
/* WINDOWS */
# ifndef WINDOWS
# define WINDOWS
# endif

#include "compat/dirent.h"
typedef DIR fu_dir;


#else
# warning "Neither POSIX or Windows"
#endif


/**
  Opens a directory and returns a handle to it.  Returns NULL on error.
*/
fu_dir *fu_opendir(const char *path);

/**
  Iterates over all files in a directory.

  Returns the name of next file or NULL if there are no more files.
*/
const char *fu_nextfile(fu_dir *dir);

/**
  Closes directory handle.
*/
int fu_closedir(fu_dir *dir);




#endif  /*  _FILEUTILS_H */
