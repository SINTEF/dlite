#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "fileutils.h"
#include "test_macros.h"

#include "minunit/minunit.h"


FUDir *dir;

MU_TEST(test_fu_isabs)
{
  mu_assert_int_eq(1, fu_isabs("/"));
  mu_assert_int_eq(1, fu_isabs("/usr/bin/ls"));
  mu_assert_int_eq(1, fu_isabs("C:\\users\\file"));
  mu_assert_int_eq(0, fu_isabs("ls"));
  mu_assert_int_eq(0, fu_isabs(""));
}

MU_TEST(test_fu_join)
{
  char *s;

  s = fu_join("a", "bb", "ccc", NULL);
  mu_assert_string_eq("a/bb/ccc", s);
  free(s);

  s = fu_join("a", "/bb", "ccc", NULL);
  mu_assert_string_eq("/bb/ccc", s);
  free(s);

  s = fu_join("a", "bb", "/ccc", NULL);
  mu_assert_string_eq("/ccc", s);
  free(s);
}


MU_TEST(test_fu_opendir)
{
  char *path = STRINGIFY(TESTDIR);
  printf("\n*** path='%s'\n", path);

  dir = fu_opendir(path);
  mu_check(dir);
}


MU_TEST(test_fu_getfile)
{
  const char *fname;
  int found_self=0;
  int found_xyz=0;

  printf("\ndir list:\n");
  while ((fname = fu_nextfile(dir))) {
    printf("  %s\n", fname);
    if (strcmp(fname, "test_fileutils.c") == 0) found_self=1;
    if (strcmp(fname, "xyz") == 0) found_xyz=1;
  }

  mu_assert_int_eq(1, found_self);
  mu_assert_int_eq(0, found_xyz);
}


MU_TEST(test_fu_closedir)
{
  mu_assert_int_eq(0, fu_closedir(dir));
}


int count_paths(FUPaths *paths)
{
  int n=0;
  const char **p = fu_paths_get(paths);
  if (!p) return 0;
  while (p[n]) n++;
  return n;
}

MU_TEST(test_fu_paths)
{
  FUPaths paths;
  fu_paths_init(&paths, NULL);
  mu_assert_int_eq(0, paths.n);

  mu_assert_int_eq(0, fu_paths_append(&paths, "path1"));
  mu_assert_int_eq(1, fu_paths_append(&paths, "path2"));
  mu_assert_int_eq(2, paths.n);
  mu_assert_int_eq(2, count_paths(&paths));
  mu_assert_string_eq("path1", paths.paths[0]);
  mu_assert_string_eq("path2", paths.paths[1]);
  mu_assert_string_eq(NULL,    paths.paths[2]);

  mu_assert_int_eq(0, fu_paths_remove(&paths, 1));
  mu_assert_int_eq(1, paths.n);
  mu_assert_int_eq(1, count_paths(&paths));
  mu_assert_string_eq(NULL,    paths.paths[1]);

  mu_assert_int_eq(1, fu_paths_append(&paths, "path2"));
  mu_assert_int_eq(2, paths.n);

  mu_assert_int_eq(0, fu_paths_insert(&paths, "path0", 0));
  mu_assert_int_eq(3, paths.n);
  mu_assert_int_eq(3, count_paths(&paths));
  mu_assert_string_eq("path0", paths.paths[0]);
  mu_assert_string_eq("path1", paths.paths[1]);
  mu_assert_string_eq("path2", paths.paths[2]);

  mu_assert_int_eq(1, fu_paths_insert(&paths, "new", -2));
  mu_assert_int_eq(4, paths.n);
  mu_assert_string_eq("path0", paths.paths[0]);
  mu_assert_string_eq("new",   paths.paths[1]);
  mu_assert_string_eq("path1", paths.paths[2]);
  mu_assert_string_eq("path2", paths.paths[3]);

  mu_assert_int_eq(0, fu_paths_insert(&paths, "new2", -10));
  mu_assert_int_eq(5, paths.n);
  mu_assert_string_eq("new2",  paths.paths[0]);
  mu_assert_string_eq("path0", paths.paths[1]);

  mu_assert_int_eq(5, fu_paths_insert(&paths, "new3", 10));
  mu_assert_int_eq(6, paths.n);
  mu_assert_int_eq(6, count_paths(&paths));
  mu_assert_string_eq("path2", paths.paths[4]);
  mu_assert_string_eq("new3",  paths.paths[5]);

  fu_paths_deinit(&paths);

#if defined(HAVE_SETENV) && defined(HAVE_UNSETENV)
  setenv("XXX", "aa" PATHSEP "bb" PATHSEP "cc", 1);
  fu_paths_init(&paths, "XXX");
  mu_assert_int_eq(3, paths.n);
  mu_assert_string_eq("aa", paths.paths[0]);
  mu_assert_string_eq("bb", paths.paths[1]);
  mu_assert_string_eq("cc", paths.paths[2]);
  fu_paths_deinit(&paths);

  unsetenv("XXX");
  fu_paths_init(&paths, "XXX");
  mu_assert_int_eq(0, paths.n);
  fu_paths_deinit(&paths);
#endif
}



MU_TEST(test_fu_match)
{
  const char *filename;
  FUIter *iter;
  FUPaths paths;
  fu_paths_init(&paths, NULL);
  fu_paths_append(&paths, "..");
  iter = fu_startmatch("*.h", &paths);
  printf("\nHeaders:\n");
  while ((filename = fu_nextmatch(iter))) printf("  %s\n", filename);
  fu_endmatch(iter);
  fu_paths_deinit(&paths);
}


/***********************************************************************/

MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_fu_isabs);
  MU_RUN_TEST(test_fu_join);

  MU_RUN_TEST(test_fu_opendir);       /* setup */
  MU_RUN_TEST(test_fu_getfile);
  MU_RUN_TEST(test_fu_closedir);      /* tear down */

  MU_RUN_TEST(test_fu_paths);
  MU_RUN_TEST(test_fu_match);
}



int main()
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
