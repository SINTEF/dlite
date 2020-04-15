/* fileinfo.c -- cross-platform information about files
 *
 * Copyright (C) 2017 SINTEF
 *
 * Distributed under terms of the MIT license.
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#if defined(__unix__)
# ifndef POSIX
#  define POSIX
# endif
# include <sys/types.h>
# include <sys/stat.h>
# include <unistd.h>
#elif defined(_WIN32)
# ifndef WINDOWS
#  define WINDOWS
# endif
# include <Windows.h>
#else
# error "fileinfo supports only POSIX (__unix__) and Windows (_WIN32)"
#endif

#include "fileinfo.h"


/* Returns non-zero if `path` exists. */
int fileinfo_exists(const char *path)
{
#if defined(POSIX)
  struct stat statbuf;
  if (stat(path, &statbuf) == 0) return 1;
#elif defined(WINDOWS)
  DWORD dwAttrib = GetFileAttributes(path);
  if (dwAttrib != INVALID_FILE_ATTRIBUTES) return 1;
#endif
  return 0;
}

/* Returns non-zero if `path` is a directory. */
int fileinfo_isdir(const char *path)
{
#if defined(POSIX)
  struct stat statbuf;
  if (stat(path, &statbuf) == 0 &&
      statbuf.st_mode & S_IFDIR) return 1;
#elif defined(WINDOWS)
  DWORD dwAttrib = GetFileAttributes(path);
  if (dwAttrib != INVALID_FILE_ATTRIBUTES &&
      (dwAttrib & FILE_ATTRIBUTE_DIRECTORY)) return 1;
#endif
  return 0;
}

/* Returns non-zero if `path` is a normal file. */
int fileinfo_isnormal(const char *path)
{
#if defined(POSIX)
  struct stat statbuf;
  if (stat(path, &statbuf) == 0 &&
      statbuf.st_mode & S_IFREG) return 1;
#elif defined(WINDOWS)
  DWORD dwAttrib = GetFileAttributes(path);
  if (dwAttrib != INVALID_FILE_ATTRIBUTES &&
      (dwAttrib & FILE_ATTRIBUTE_NORMAL)) return 1;
#endif
  return 0;
}
