/* fileutils.c -- cross-platform file utility functions */

#include <stdlib.h>

#include "fileutils.h"


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
