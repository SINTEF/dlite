/* fileinfo.c -- cross-platform information about files */
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
# include <pwd.h>
# include <grp.h>
# include <unistd.h>
# include <string.h>
#elif defined(_WIN32)
# ifndef WINDOWS
#  define WINDOWS
# endif
# include <Windows.h>
#else
# error "fileinfo supports only POSIX (__unix__) and Windows (_WIN32)"
#endif

#include "fileinfo.h"


#ifdef POSIX
/* Help function.  Returns true if the current user is member of
   `statbuf`s group. */
int ismember(struct stat *statbuf) {
  char **p;
  struct passwd *pw = getpwuid(geteuid());
  struct group *gr = getgrgid(statbuf->st_gid);
  if (!pw || !gr) return 0;
  for (p=gr->gr_mem; *p; p++)
    if (strcmp(*p, pw->pw_name) == 0) return 1;
  return 0;
}
#endif


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

///* Returns non-zero if `path` is readable.
//
//   Seems not to be supported on Windows...
//*/
//int fileinfo_readable(const char *path)
//{
//#if defined(POSIX)
//  struct stat statbuf;
//  uid_t uid = geteuid();
//  gid_t gid = getegid();
//  if (stat(path, &statbuf)) return 0;
//  if (statbuf.st_uid == uid) {
//    if (statbuf.st_mode & S_IRUSR) return 1;
//  } else if (ismember(&statbuf)) {
//    if (statbuf.st_mode & S_IRGRP) return 1;
//  } else {
//    if (statbuf.st_mode & S_IRGRP) return 1;
//  }
//#elif defined(WINDOWS)
//  DWORD dwAttrib = GetFileAttributes(path);
//#endif
//  return 0;
//}
//
///* Returns non-zero if `path` is readable. */
//int fileinfo_writable(const char *path)
//{
//#if defined(POSIX)
//  struct stat statbuf;
//  uid_t uid = geteuid();
//  gid_t gid = getegid();
//  if (stat(path, &statbuf)) return 0;
//  if (statbuf.st_uid == uid) {
//    if (statbuf.st_mode & S_IWUSR) return 1;
//  } else if (ismember(&statbuf)) {
//    if (statbuf.st_mode & S_IWGRP) return 1;
//  } else {
//    if (statbuf.st_mode & S_IWGRP) return 1;
//  }
//#elif defined(WINDOWS)
//  DWORD dwAttrib = GetFileAttributes(path);
//  /* hmm, it seems that Windows doesn't honor FILE_ATTRIBUTE_READONLY
//     for directories... */
//  if (dwAttrib != INVALID_FILE_ATTRIBUTES &&
//      !(dwAttrib & FILE_ATTRIBUTE_READONLY)) return 1;
//#endif
//  return 0;
//}
