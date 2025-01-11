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
#include <errno.h>
#include <stdio.h>

#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
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
# include <windows.h>
# include <shlwapi.h>
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
#if defined(HAVE_GetFullPathNameW) && defined(HAVE_MBSTOWCS_S)
  size_t wlen;
  wchar_t wpath[MAX_PATH + 1];

  /* Convert path to wide characters */
  if (mbstowcs_s(&wlen, wpath, MAX_PATH + 1, path, MAX_PATH + 1)) return 0;
  return PathFileExistsW(wpath);
#else
  DWORD dwAttrib = GetFileAttributes(path);
  if (dwAttrib != INVALID_FILE_ATTRIBUTES) return 1;
#endif
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
  if (fileinfo_exists(path) &&
      dwAttrib != INVALID_FILE_ATTRIBUTES &&
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
  /* Why does Windows attribute normal files as archives? */
  if (fileinfo_exists(path) &&
      dwAttrib != INVALID_FILE_ATTRIBUTES &&
      ((dwAttrib & FILE_ATTRIBUTE_NORMAL) ||
       (dwAttrib & FILE_ATTRIBUTE_ARCHIVE))) return 1;
#endif
  return 0;
}

/* Returns non-zero if `path` is a normal file and readable. */
int fileinfo_isreadable(const char *path)
{
  int errno_orig = errno;
  FILE *fp;
  int readable;
  if (fileinfo_isdir(path)) return 0;
  if (!fileinfo_isnormal(path)) return 0;
  fp = fopen(path, "r");
  readable = (fp) ? 1 : 0;
  if (fp) fclose(fp);
  errno = errno_orig;
  return readable;
}
