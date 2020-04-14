/* fileinfo.h -- cross-platform information about files
 *
 * Copyright (C) 2017 SINTEF Materials and Chemistry
 * By Jesper Friis <jesper.friis@sintef.no>
 *
 * Distributed under terms of the MIT license.
 */
#ifndef _FILEINFO_H
#define _FILEINFO_H

/* Returns non-zero if `path` exists. */
int fileinfo_exists(const char *path);

/* Returns non-zero if `path` is a directory. */
int fileinfo_isdir(const char *path);

/* Returns non-zero if `path` is a normal file. */
int fileinfo_isnormal(const char *path);

#endif  /* _FILEINFO_H */
