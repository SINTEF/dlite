/* execprocess.c -- execute process in a user-defined environment
 *
 * Copyright (C) 2020 SINTEF
 *
 * Distributed under terms of the MIT license.
 */
#ifndef _EXECPROCESS_H
#define _EXECPROCESS_H


/**
  Execute `pathname` in a new process.

  Arguments:
    - pathname: path to executable.
    - argv: arguments passed to the process.  It is a NULL-terminated
          array of pointers to NUL-terminated strings.  The first argument
          should point to `pathname`.
    - env: environment variables passed to the process.  It is a
          NULL-terminated array of pointers to NUL-terminated
          strings.
*/
int exec_process(const char *pathname, char *const argv[], char *const env[]);


/**
  Returns a pointer to the environment of the current process.

  The returned value is a pointer to a NULL-terminated array of pointers
  to NUL-terminated strings, where each string is of the form "name=value".

  The returned environment must be free'ed with strlist_free().

  Returns NULL on error.
 */
char **get_environment(void);

/**
  Returns a pointer to the ``name=value`` item in environment `env`
  corresponding to environment variable `name`, or NULL if `name`
  isn't in the environment.
*/
char **get_envitem(char **env, const char *name);

/**
  Returns value of environment variable `name` or NULL if it doesn't exists.
*/
char *get_envvar(char **env, const char *name);

/**
  Sets environment variable NAME to `item`, where `item` is a
  NAME=VALUE pair.  If it exists from before, it is overwritten.  Only
  call this on an malloc'ed environment, like the one returned by
  get_environment().

  If `env` is NULL, a new environment is allocated.

  Returns a pointer to the (possible reallocated) environment or NULL
  on error.
 */
char **set_envitem(char **env, const char *item);

/**
  Sets environment variable `name` to `value`.  If it exists from before, it
  is overwritten.  Only call this on an malloc'ed environment, like the one
  returned by get_environment().

  If `env` is NULL, a new environment is allocated;

  Returns a pointer to the (possible reallocated) environment or NULL on error.
 */
char **set_envvar(char **env, const char *name, const char *value);


/**
  Returns a copy of `strlist` or NULL on error.
*/
char **strlist_copy(char **strlist);

/**
  Appends `s` to string list `strlist` and return a pointer to the
  reallocated list.  If `strlist` is NULL, a new string list is created.

  Returns NULL on error.
 */
char **strlist_add(char **strlist, const char *s);

/**
  Frees a string list.
*/
void strlist_free(char **strlist);


#endif /* _EXECPROCESS
_H_ */
