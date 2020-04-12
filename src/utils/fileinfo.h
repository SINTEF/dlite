/* fileinfo.h -- cross-platform information about files */
#ifndef _FILEINFO_H
#define _FILEINFO_H

/* Returns non-zero if `path` exists. */
int fileinfo_exists(const char *path);

/* Returns non-zero if `path` is a directory. */
int fileinfo_isdir(const char *path);

/* Returns non-zero if `path` is a normal file. */
int fileinfo_isnormal(const char *path);

///* Returns non-zero if `path` is readable. */
//int fileinfo_readable(const char *path);
//
///* Returns non-zero if `path` is readable. */
//int fileinfo_writable(const char *path);

#endif  /* _FILEINFO_H */
