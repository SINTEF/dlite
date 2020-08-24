/* execprocess.c -- execute process in a user-defined environment
 *
 * Copyright (C) 2020 SINTEF
 *
 * Distributed under terms of the MIT license.
 */
#ifdef HAVE_CONFIG
#include "config.h"
#endif

#ifdef HAVE_EXEC
#include <sys/types.h>
#include <unistd.h>
#else
#include <string.h>
#include <windows.h>
#include <processthreadsapi.h>
#include <processenv.h>
#endif

#include "err.h"
#include "execprocess.h"


/** Convenient macros for failing */
#define FAIL(msg) do { \
    err(1, msg); goto fail; } while (0)
#define FAIL1(msg, a1) do { \
    err(1, msg, a1); goto fail; } while (0)
#define FAIL2(msg, a1, a2) do { \
    err(1, msg, a1, a2); goto fail; } while (0)

#define BUFSIZE  32768

/*
  Execute `pathname` in a new process.

  Arguments:
    - pathname: path to executable.
    - argv: arguments passed to the process.  It is a NULL-terminated
          array of pointers to NUL-terminated strings.  The first argument
          should point to `pathname`.
    - env: environment variables passed to the process.  It is a
          NULL-terminated array of pointers to NUL-terminated
          strings.

  Returns non-zero on error.
*/
int exec_process(const char *pathname, const char *argv[], const char *env[])
{
#ifdef HAVE_EXEC
  int status=1;
  pid_t pid = fork();
  if (pid < 0)
    return err(-1 "error forking: %s", pathname);
  else if (pid > 0) {
    /* parent process */
    wait(&status);
  } else {
    /* child process */
    if (execvpe(pathname, argv, env) < 0)
      fatal(1, "cannot execute pathname %s", pathname);
  }
  return status;

#else
  STARTUPINFO si;
  PROCESS_INFORMATION pi;
  CHAR cmdline[4096];
  char *p, envbuf[BUFSIZE];
  int i, n;

  n = snprintf(cmdline, sizeof(cmdline), "%s", pathname);
  if (argv) {
    for (i=1; argv[i] && n < sizeof(cmdline); i++)
      n += snprintf(cmdline+n, sizeof(cmdline)-n, " \"%s\"", argv[i]);
  }

  envbuf[0] = envbuf[1] = '\0';
  if (env) {
    for (p=(char *)env[0], i=n=0; env[i] && n < BUFSIZE; p=(char *)env[++i]) {
      int len = strlen(env[i]);
      strncpy(envbuf+n, env[i], BUFSIZE-n);
      //if (FAILED(StringCchCopy(envbuf+n, BUFSIZE-n, env[i])))
      //  return err(1, "error creating new environment");
      if (n+len < BUFSIZE-2) envbuf[n+len] = envbuf[n+len+1] = '\0';
      n += len+1;
    }
    envbuf[BUFSIZE-2] = envbuf[BUFSIZE-1] = '\0';
  }

  SecureZeroMemory(&si, sizeof(STARTUPINFO));
  si.cb = sizeof(STARTUPINFO);

  /* Start the child process. */
  if(!CreateProcess(NULL,     // No module name (use command line)
                    cmdline,  // Command line
                    NULL,     // Process handle not inheritable
                    NULL,     // Thread handle not inheritable
                    FALSE,    // Set handle inheritance to FALSE
                    0,        // No creation flags
                    envbuf,   // Environment block
                    NULL,     // Use parent's starting directory
                    &si,      // Pointer to STARTUPINFO structure
                    &pi))     // Pointer to PROCESS_INFORMATION structure
    return err(1, "error executing pathname %s", pathname);
  WaitForSingleObject(pi.hProcess, INFINITE);
  CloseHandle(pi.hThread);
  CloseHandle(pi.hProcess);
  return 0;
#endif
}


/*
  Returns a pointer to the environment of the current process.

  The returned value is a pointer to a NULL-terminated array of pointers
  to NUL-terminated strings, where each string is of the form ``NAME=VALUE``.

  The returned environment must be free'ed with strlist_free().

  Returns NULL on error.
 */
char **get_environment(void)
{
#ifdef HAVE_EXEC
  return copy_environment(environ);
#endif
  size_t size=0, n=0;
  char *p, **env=NULL;
  LPCH envbuf = GetEnvironmentStrings();
  if (!envbuf) return err(1, "cannot get environment"), NULL;
  for (p=envbuf; *p; p+=strlen(p)+1) {
    if (n+1 >= size) {
      size += 64*sizeof(char *);
      env = realloc(env, size);
    }
    env[n++] = strdup(p);
  }
  env[n] = NULL;
  FreeEnvironmentStrings(envbuf);
  return env;
}

/*
  Returns a pointer to the ``NAME=VALUE`` item in environment `env`
  corresponding to environment variable `name`, or NULL if `name`
  isn't in the environment.

  Note that `name` can either be terminated with NUL or '='.
*/
char **get_envitem(char **env, const char *name)
{
  char **q;
  for (q=env; *q; q++) {
    int n, len=strcspn(name, "=");
    char *p = strchr(*q, '=');
    if (!p) return err(1, "no equal sign in environment item: %s", *q), NULL;
    n = *q -p;
    if (n == len && strncmp(*q, name, n) == 0) return q;
  }
  return NULL;
}

/*
  Returns value of environment variable `name` or NULL on error.
*/
char *get_envvar(char **env, const char *name)
{
  char **q = get_envitem(env, name);
  if (!q) return NULL;
  return *q + strcspn(*q, "=") + 1;
}

/*
  Sets environment variable NAME to `item`, where `item` is a
  NAME=VALUE pair.  If it exists from before, it is overwritten.  Only
  call this on an malloc'ed environment, like the one returned by
  get_environment().

  If `env` is NULL, a new environment is allocated.

  Returns a pointer to the (possible reallocated) environment or NULL
  on error.
 */
char **set_envitem(char **env, const char *item)
{
  char **retval=NULL;
  char **q;
  if (!strchr(item, '='))
    return err(1, "no equal sign in environment item: %s", item), NULL;
  if ((q = get_envitem(env, item))) {
    free(*q);
    *q = strdup(item);
  } else {
    env = strlist_add(env, item);
  }
  return env;
}

/*
  Sets environment variable `name` to `value`.  If it exists from
  before, it is overwritten.  Only call this on an malloc'ed
  environment, like the one returned by get_environment().

  If `env` is NULL, a new environment is allocated.

  Returns a pointer to the (possible reallocated) environment or NULL
  on error.
 */
char **set_envvar(char **env, const char *name, const char *value)
{
  char *item=NULL, **retval=NULL;
  int len = strlen(name) + strlen(value) + 2;
  if (!(item = malloc(len))) FAIL("allocation failure");
  if (snprintf(item, len, "%s=%s", name, value) < 0)
    FAIL1("error setting environment variable: %s", name);
  retval = set_envitem(env, item);
 fail:
  if (item) free(item);
  return retval;
}

/*
  Returns a copy of `strlist` or NULL on error.
*/
char **strlist_copy(char **strlist)
{
  int i, n=0;
  char **copy;
  while (strlist[n]) n++;
  if (!(copy = malloc(n*sizeof(char *))))
    return err(1, "allocation failure"), NULL;
  for (i=0; i<n; i++)
    copy[i] = strdup(strlist[i]);
  return copy;
}

/*
  Appends `s` to string list `strlist` and return a pointer to the
  reallocated list.  If `strlist` is NULL, a new string list is created.

  Returns NULL on error.
 */
char **strlist_add(char **strlist, const char *s)
{
  int n=0;
  if (strlist)
    while (strlist[n]) n++;
  if (!(strlist = realloc(strlist, (n+2)*sizeof(char **))))
    return err(1, "allocation failure"), NULL;
  if (!(strlist[n] = strdup(s)))
    return err(1, "allocation failure"), NULL;
  strlist[n+1] = NULL;
  return strlist;
}

/*
  Frees a string list.
*/
void strlist_free(char **strlist)
{
  char **p = strlist;
  while (*p) free(*(p++));
  free(strlist);
}
