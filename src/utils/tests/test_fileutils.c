#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

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

MU_TEST(test_fu_iswinpath)
{
  mu_assert_int_eq(0, fu_iswinpath("/", -1));
  mu_assert_int_eq(0, fu_iswinpath("/usr/bin/ls", -1));
  mu_assert_int_eq(1, fu_iswinpath("C:\\users\\file", -1));
  mu_assert_int_eq(0, fu_iswinpath("ls", -1));
  mu_assert_int_eq(0, fu_iswinpath("", -1));
  mu_assert_int_eq(0, fu_iswinpath("C://example.com/", -1));
  mu_assert_int_eq(1, fu_iswinpath("C://example.com/", 3));
  mu_assert_int_eq(0, fu_iswinpath("http://example.com/", -1));
  mu_assert_int_eq(1, fu_iswinpath("c:file.txt", -1));
  mu_assert_int_eq(1, fu_iswinpath("c:dir/file.txt", -1));
  mu_assert_int_eq(1, fu_iswinpath("c:/dir/file.txt", -1));
  mu_assert_int_eq(0, fu_iswinpath("c:/dir/file.txt", 1));
  mu_assert_int_eq(1, fu_iswinpath("\\\\server\\share\\foo.txt", -1));
}

MU_TEST(test_fu_join)
{
  char *s;

  s = fu_join("a", "bb", "ccc", NULL);
  mu_assert_string_eq("a/bb/ccc", s);
  free(s);

  s = fu_join_sep('/', "a", "bb", "ccc", NULL);
  mu_assert_string_eq("a/bb/ccc", s);
  free(s);

  s = fu_join("a", "/bb", "ccc", NULL);
  mu_assert_string_eq("/bb/ccc", s);
  free(s);

  s = fu_join("a", "bb", "/ccc", NULL);
  mu_assert_string_eq("/ccc", s);
  free(s);

  s = fu_join("a", "bb", "ccc", "", NULL);
  mu_assert_string_eq("a/bb/ccc/", s);
  free(s);
}


MU_TEST(test_fu_lastsep)
{
  mu_assert_string_eq("/ccc.txt", fu_lastsep("a/bb/ccc.txt"));
  mu_assert_string_eq("/ccc.txt", fu_lastsep("/a/bb/ccc.txt"));
  mu_assert_string_eq(NULL, fu_lastsep("ccc.txt"));
}


MU_TEST(test_fu_dirname)
{
  char *s;
  s = fu_dirname("a/bb/ccc.txt");
  mu_assert_string_eq("a/bb", s);
  free(s);

  s = fu_dirname("a" DIRSEP "bb" DIRSEP "ccc.txt");  // cppcheck-suppress unknownMacro
  mu_assert_string_eq("a" DIRSEP "bb", s);
  free(s);

  s = fu_dirname("a/bb/ccc/");
  mu_assert_string_eq("a/bb/ccc", s);
  free(s);

  s = fu_dirname("/");
  mu_assert_string_eq("/", s);
  free(s);

  s = fu_dirname("ccc.txt");
  mu_assert_string_eq("", s);
  free(s);
}

MU_TEST(test_fu_basename)
{
  char *s;
  s = fu_basename("a/bb/ccc.txt");
  mu_assert_string_eq("ccc.txt", s);
  free(s);

  s = fu_basename("a/bb/ccc/");
  mu_assert_string_eq("", s);
  free(s);
}

MU_TEST(test_fu_stem)
{
  char *s;
  s = fu_stem("a/bb/ccc.txt");
  mu_assert_string_eq("ccc", s);
  free(s);

  s = fu_stem("a/bb/ccc");
  mu_assert_string_eq("ccc", s);
  free(s);

  s = fu_stem("a/bb/ccc/");
  mu_assert_string_eq("", s);
  free(s);
}

MU_TEST(test_fu_fileext)
{
  mu_assert_string_eq("txt", fu_fileext("a/bb/ccc.txt"));
  mu_assert_string_eq("txt", fu_fileext("cc.c.txt"));
  mu_assert_string_eq("", fu_fileext("a/bb/ccc"));
  mu_assert_string_eq("", fu_fileext("a/b.b/ccc"));
  mu_assert_string_eq("", fu_fileext("a/bb/ccc."));
}

MU_TEST(test_fu_friendly_dirsep)
{
  char path[256];
  strcpy(path, "/etc/fstab");
  mu_assert_string_eq("/etc/fstab", fu_friendly_dirsep(path));

  strcpy(path, "\\\\drive:a/file");
#ifdef WINDOWS
  mu_assert_string_eq("\\\\drive:a\\file", fu_friendly_dirsep(path));
#else
  mu_assert_string_eq("\\\\drive:a/file", fu_friendly_dirsep(path));
#endif

  strcpy(path, "C:\\dir\\file");
#ifdef WINDOWS
  //mu_assert_string_eq("C:\\dir\\file", fu_friendly_dirsep(path));
#else
  mu_assert_string_eq("C:\\dir\\file", fu_friendly_dirsep(path));
#endif

  strcpy(path, "C:/dir/file");
#ifdef WINDOWS
  //mu_assert_string_eq("C:\\dir\\file", fu_friendly_dirsep(path));
#else
  mu_assert_string_eq("C:/dir/file", fu_friendly_dirsep(path));
#endif
}

MU_TEST(test_fu_nextpath)
{
  char *paths = "C:\\aa\\bb.txt;/etc/fstab:http://example.com";
  char *paths2 = "C:reldir/f.txt:bin/;/etc/fstab";
  char *paths3 = "/var///log::/dev/null";  // repeated dirsep and pathsep
  char *endptr = NULL;
  const char *p;
  p = fu_nextpath(paths, &endptr, NULL);
  mu_assert_string_eq(paths, p);
  mu_assert_int_eq(';', *endptr);

  p = fu_nextpath(paths, &endptr, NULL);
  mu_assert_string_eq("/etc/fstab:http://example.com", p);
  mu_assert_int_eq(':', *endptr);

  p = fu_nextpath(paths, &endptr, NULL);
  mu_assert_string_eq("http://example.com", p);
  mu_assert_int_eq('\0', *endptr);

  p = fu_nextpath(paths, &endptr, NULL);
  mu_assert_string_eq(NULL, p);
  mu_assert_int_eq('\0', *endptr);


  endptr = NULL;
  p = fu_nextpath(paths, &endptr, ";");
  mu_assert_string_eq(paths, p);
  mu_assert_int_eq(';', *endptr);

  p = fu_nextpath(paths, &endptr, ";");
  mu_assert_string_eq("/etc/fstab:http://example.com", p);
  mu_assert_int_eq('\0', *endptr);


  endptr = NULL;
  p = fu_nextpath(paths2, &endptr, NULL);
  mu_assert_string_eq(paths2, p);
  mu_assert_int_eq(':', *endptr);

  p = fu_nextpath(paths2, &endptr, NULL);
  mu_assert_string_eq("bin/;/etc/fstab", p);
  mu_assert_int_eq(';', *endptr);

  p = fu_nextpath(paths2, &endptr, NULL);
  mu_assert_string_eq("/etc/fstab", p);
  mu_assert_int_eq('\0', *endptr);


  endptr = NULL;
  p = fu_nextpath(paths3, &endptr, NULL);
  mu_assert_string_eq(paths3, p);

  p = fu_nextpath(paths3, &endptr, NULL);
  mu_assert_string_eq("/dev/null", p);
}

MU_TEST(test_fu_winpath)
{
  char buf[256], *s;
  mu_check(fu_winpath("/users/mrx/file.txt", buf, sizeof(buf), NULL));
  mu_assert_string_eq("\\users\\mrx\\file.txt", buf);

  mu_check(fu_winpath("/c/users/mrx/file.txt", buf, sizeof(buf), NULL));
  mu_assert_string_eq("C:\\users\\mrx\\file.txt", buf);

  mu_check(fu_winpath("../file.txt", buf, sizeof(buf), NULL));
  mu_assert_string_eq("..\\file.txt", buf);

  mu_check(fu_winpath("/d/users/mrx/bin:/c/bin:/users/mry/bin",
                      buf, sizeof(buf), NULL));
  mu_assert_string_eq("D:\\users\\mrx\\bin;C:\\bin;\\users\\mry\\bin", buf);

  mu_check((s = fu_winpath("C:\\dir\\file.txt", NULL, 0, NULL)));
  mu_assert_string_eq("C:\\dir\\file.txt", s);
  free(s);
}

MU_TEST(test_fu_unixpath)
{
  char buf[256], *s;
  mu_check(fu_unixpath("C:\\users\\mrx\\file.txt", buf, sizeof(buf), NULL));
  mu_assert_string_eq("/c/users/mrx/file.txt", buf);

  mu_check(fu_unixpath("D:\\users\\mrx\\file.txt", buf, sizeof(buf), NULL));
  mu_assert_string_eq("/d/users/mrx/file.txt", buf);

  mu_check(fu_unixpath("..\\file.txt", buf, sizeof(buf), NULL));
  mu_assert_string_eq("../file.txt", buf);

  mu_check(fu_unixpath("C:reldir/file.txt", buf, sizeof(buf), NULL));
  mu_assert_string_eq("reldir/file.txt", buf);

  mu_check(fu_unixpath("D:\\users\\mrx\\bin;C:\\bin;C:\\users\\mry\\bin",
                      buf, sizeof(buf), NULL));
  mu_assert_string_eq("/d/users/mrx/bin:/c/bin:/c/users/mry/bin", buf);

  mu_check((s = fu_unixpath("/users/mrx/file.txt", NULL, 0, NULL)));
  mu_assert_string_eq("/users/mrx/file.txt", s);
  free(s);
}

MU_TEST(test_fu_exists)
{
  char *testfile = fu_join(STRINGIFY(TESTDIR), "test_fileutils.c", NULL);
  mu_assert_int_eq(0, fu_exists(testfile));
  mu_check(fu_exists("_non_existing_file_.abc"));
}

MU_TEST(test_fu_realpath)
{
  char *testdir, *path, *realpath;
#ifdef WINDOWS
  char buff[MAX_PATH];
#else
  char buff[PATH_MAX];
#endif
  printf("\nfu_realpath()\n");
  fflush(stdout);

  testdir = fu_realpath(STRINGIFY(TESTDIR), NULL);

  realpath = fu_join(testdir, "test_fileutils.c", NULL);
  fu_friendly_dirsep(realpath);
  fu_realpath(realpath, buff);
  fu_friendly_dirsep(buff);
  mu_assert_string_eq(realpath, buff);

  path = fu_join(testdir, "..", ".", "tests", "test_fileutils.c", NULL);
  fu_realpath(path, buff);
  fu_friendly_dirsep(buff);
  mu_assert_string_eq(realpath, buff);
  free(path);

  path = fu_join(testdir, "..", "", "tests", "test_fileutils.c", NULL);
  fu_realpath(path, buff);
  fu_friendly_dirsep(buff);
  mu_assert_string_eq(realpath, buff);
  free(path);

  mu_assert_string_eq(NULL, fu_realpath("a_strange/non-existing_path...", buff));
  mu_check(fu_realpath("a_strange/non-existing_path...", buff) == NULL);
  printf("\n");
  fflush(stdout);
  free(realpath);
  free(testdir);
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
  fflush(stdout);

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
  char *s;
  fu_paths_init(&paths, NULL);
  fu_paths_set_platform(&paths, fuUnix);
  mu_assert_int_eq(0, paths.n);

  mu_assert_int_eq(0, fu_paths_append(&paths, "/var/path1"));
  mu_assert_int_eq(1, fu_paths_append(&paths, "\\c\\users\\path2"));
  mu_assert_int_eq(2, paths.n);
  mu_assert_int_eq(2, count_paths(&paths));
  mu_assert_string_eq("/var/path1", paths.paths[0]);
  mu_assert_string_eq("/c/users/path2", paths.paths[1]);
  mu_assert_string_eq(NULL,    paths.paths[2]);

  mu_assert_int_eq(0, fu_paths_remove_index(&paths, 1));
  mu_assert_int_eq(1, paths.n);
  mu_assert_int_eq(1, count_paths(&paths));
  mu_assert_string_eq(NULL,    paths.paths[1]);

  mu_assert_int_eq(1, fu_paths_append(&paths, "/c/users/path2"));
  mu_assert_int_eq(2, paths.n);

  mu_assert_int_eq(0, fu_paths_insert(&paths, "path0", 0));
  fprintf(stderr, "\n");
  fprintf(stderr, "*** paths[0]='%s'\n", paths.paths[0]);
  fprintf(stderr, "*** paths[1]='%s'\n", paths.paths[1]);
  fprintf(stderr, "*** paths[2]='%s'\n", paths.paths[2]);
  mu_assert_int_eq(3, paths.n);
  mu_assert_int_eq(3, count_paths(&paths));
  mu_assert_string_eq("path0", paths.paths[0]);
  mu_assert_string_eq("/var/path1", paths.paths[1]);
  mu_assert_string_eq("/c/users/path2", paths.paths[2]);
  mu_assert_string_eq(NULL,    paths.paths[3]);

  mu_assert_int_eq(0, fu_paths_index(&paths, "path0"));
  mu_assert_int_eq(1, fu_paths_index(&paths, "/var/path1"));
  mu_assert_int_eq(2, fu_paths_index(&paths, "/c/users/path2"));
  mu_assert_int_eq(-1, fu_paths_index(&paths, "non-existing-path"));
  mu_assert_int_eq(-1, fu_paths_index(&paths, ""));

  mu_assert_int_eq(1, fu_paths_insert(&paths, "new", -2));
  mu_assert_int_eq(4, paths.n);
  mu_assert_string_eq("path0", paths.paths[0]);
  mu_assert_string_eq("new",   paths.paths[1]);
  mu_assert_string_eq("/var/path1", paths.paths[2]);
  mu_assert_string_eq("/c/users/path2", paths.paths[3]);
  mu_assert_string_eq(NULL,    paths.paths[4]);

  mu_assert_int_eq(0, fu_paths_insert(&paths, "new2", -4));
  mu_assert_int_eq(5, paths.n);
  mu_assert_string_eq("new2",  paths.paths[0]);
  mu_assert_string_eq("path0", paths.paths[1]);
  mu_assert_string_eq("new",   paths.paths[2]);
  mu_assert_string_eq("/var/path1", paths.paths[3]);
  mu_assert_string_eq("/c/users/path2", paths.paths[4]);
  mu_assert_string_eq(NULL,    paths.paths[5]);

  mu_assert_int_eq(5, fu_paths_insert(&paths, "new3", 5));
  mu_assert_int_eq(6, paths.n);
  mu_assert_int_eq(6, count_paths(&paths));
  mu_assert_string_eq("/c/users/path2", paths.paths[4]);
  mu_assert_string_eq("new3",  paths.paths[5]);

  s = fu_paths_string(&paths);
  mu_assert_string_eq("new2:path0:new:/var/path1:/c/users/path2:new3", s);
  free(s);

  mu_assert_int_eq(8, fu_paths_extend(&paths, "aa:bb;cc", NULL));
  s = fu_paths_string(&paths);
  mu_assert_string_eq("new2:path0:new:/var/path1:/c/users/path2:new3:aa:bb:cc",
                      s);
  free(s);

  fu_paths_set_platform(&paths, fuWindows);
  s = fu_paths_string(&paths);
  mu_assert_string_eq("new2;path0;new;\\var\\path1;C:\\users\\path2;"
                      "new3;aa;bb;cc", s);
  free(s);

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
  while ((filename = fu_nextmatch(iter)))
    printf("  %s\n", filename);
  fu_endmatch(iter);
  fu_paths_deinit(&paths);
}

MU_TEST(test_fu_glob)
{
  const char *p;
  FUIter *iter = fu_glob("*", "|");
  printf("\nFiles:\n");
  while ((p = fu_globnext(iter)))
    printf("  %s\n", p);
  fu_globend(iter);

  p = "postgresql://localhost:5432?user=guest";
  iter = fu_glob(p, "|");
  mu_assert_string_eq(p, fu_globnext(iter));
  mu_assert_string_eq(NULL, fu_globnext(iter));
  mu_assert_string_eq(NULL, fu_globnext(iter));
  fu_globend(iter);

#define DIR STRINGIFY(TESTDIR) "/../"
  iter = fu_glob("file:" DIR "fileu*.c", "|");
  mu_assert_string_eq(DIR "fileutils.c", fu_globnext(iter));
  mu_assert_string_eq(NULL, fu_globnext(iter));
  fu_globend(iter);
}

MU_TEST(test_fu_pathsiter)
{
  const char *filename;
  FUIter *iter;
  FUPaths paths;
  fu_paths_init(&paths, NULL);
  fu_paths_append(&paths, "doc");
  fu_paths_append(&paths, "src/Makefile");
  fu_paths_append(&paths, "tools/c*");
  fu_paths_append(&paths, "d*");
  iter = fu_pathsiter_init(&paths, "*.cmake");
  printf("\nCMake files:\n");
  while ((filename = fu_pathsiter_next(iter)))
    printf("  %s\n", filename);
  fu_pathsiter_deinit(iter);
  fu_paths_deinit(&paths);
}


/***********************************************************************/

MU_TEST_SUITE(test_suite)
{
  MU_RUN_TEST(test_fu_isabs);
  MU_RUN_TEST(test_fu_iswinpath);
  MU_RUN_TEST(test_fu_join);
  MU_RUN_TEST(test_fu_lastsep);
  MU_RUN_TEST(test_fu_dirname);
  MU_RUN_TEST(test_fu_basename);
  MU_RUN_TEST(test_fu_stem);
  MU_RUN_TEST(test_fu_fileext);
  MU_RUN_TEST(test_fu_friendly_dirsep);
  MU_RUN_TEST(test_fu_nextpath);
  MU_RUN_TEST(test_fu_winpath);
  MU_RUN_TEST(test_fu_unixpath);
  MU_RUN_TEST(test_fu_exists);
  MU_RUN_TEST(test_fu_realpath);

  MU_RUN_TEST(test_fu_opendir);       /* setup */
  MU_RUN_TEST(test_fu_getfile);
  MU_RUN_TEST(test_fu_closedir);      /* tear down */

  MU_RUN_TEST(test_fu_paths);
  MU_RUN_TEST(test_fu_match);
  MU_RUN_TEST(test_fu_glob);
  MU_RUN_TEST(test_fu_pathsiter);
}



int main()
{
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return (minunit_fail) ? 1 : 0;
}
