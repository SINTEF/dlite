/* fileutils.c -- cross-platform file utility functions */

#include <stdlib.h>

#include "fileutils.h"


#if defined POSIX

fu_dir *fu_opendir(const char *path)
{
  return (fu_dir *)opendir(path);
}

const char *fu_nextfile(fu_dir *dir)
{
  struct dirent *ent = readdir((DIR *)dir);
  return (ent) ? ent->d_name : NULL;
}

int fu_closedir(fu_dir *dir)
{
  return closedir((DIR *)dir);
}


//#ifndef _POSIX_C_SOURCE
//#define _POSIX_C_SOURCE
//#endif
//
//#include <stdio.h>
//
//FILE *fu_mkstemp(char *template)
//{
//  int fd = mkstemp(template);
//  if (fd == -1) return NULL;
//  return fdopen(fd, "w+");
//}
//
//char *fu_mkdtemp(char *template)
//{
//  return mkdtemp(template);
//}




#elif defined WINDOWS

typedef struct {
  HANDLE handle;
  WIN32_FIND_DATA ffd;
} fu_dir;

fu_dir *fu_opendir(const char *path)
{
  fu_dir *dir = calloc(sizeof(1, fu_dir));
  dir->handle = FindFirstFile(path, &dir->ffd);
  if (dir->handle == INVALID_HANDLE_VALUE) {
    free(dir);
    return NULL;
  }
  return dir;
}

const char *fu_nextfile(fu_dir *dir)
{
  if (!FindNextFile(dir->handle, &dir->ffd)) return NULL;
  return dir->ffd.cFileName;
}

int fu_closedir(fu_dir *dir)
{
  BOOL success = FindClose(dir->handle);
  free(dir);
  return (success) ? 0 : 1;
}


//#include <stdio.h>
//
//FILE *fu_mkstemp(char *template)
//{
//  int fd = mkstemp(template);
//  if (fd == -1) return NULL;
//  return fdopen(fd, "w+");
//}
//
//char *fu_mkdtemp(char *template)
//{
//  return mkdtemp(template);
//}


#endif
