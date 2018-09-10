/* error.c -- simple error reporting
 */
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

#include "compat.h"

/* Thread local storate */
#ifdef USE_THREAD_LOCAL_STORAGE
# ifdef __GNUC__
# define _tls __thread
# else
# define _tls __declspec(thread)
# endif
#else
# define _tls
#endif


/* Stream for print errors. Set to NULL to for silent. */
static FILE err_dummy_stream;  /* marker for ERR_STREAM environment variable */
static FILE *err_stream = &err_dummy_stream;

/* Prefix to append to all errors in this application.  Typically the
   program name. */
static const char *err_prefix = "";

/* Indicate wheter the error functions should return, exit or about.
 *   - err_fail_mode >= 2: abort
 *   - err_fail_mode == 1: exit (with error code)
 *   - err_fail_mode == 0: normal return
 *   - err_fail_mode < 0:  check ERR_FAIL_MODE environment variable (default) */
static int err_fail_mode = -1;

/* Tread-local variables */
static _tls int errcode = 0;       /* Error value of the latest error. */
static _tls char errmsg[256]= "";  /* Error message of the latest error. */


/* Reports the error and returns `eval`.  Args:
 *  errname : name of error, e.g. "Fatal" or "Error"
 *  eval    : error value that is returned or passed exit()
 *  errnum  : error number for system errors
 *  pos     : position in source file where the error occured
 *  msg     : error message
 *  ap      : printf()-like argument list for error message
 */
static int format_error(const char *errname, int eval, int errnum,
                        const char *pos, const char *msg, va_list ap)
{
  int n=0;

  (void)(pos);

  errcode = eval;
  if (err_prefix && *err_prefix)
    n += snprintf(errmsg + n, sizeof(errmsg) - n, "%s: ", err_prefix);
  if (errname && *errname)
    n += snprintf(errmsg + n, sizeof(errmsg) - n, "%s: ", errname);
  if (msg && *msg)
    n += vsnprintf(errmsg + n, sizeof(errmsg) - n, msg, ap);
  if (errnum)
    n += snprintf(errmsg + n, sizeof(errmsg) - n, ": %s", strerror(errnum));

  /* write to err_stream */
  if (err_stream == &err_dummy_stream) {
    char *errfile = getenv("ERR_STREAM");
    if (!errfile)
      err_stream = stderr;
    else if (!errfile[0])
      err_stream = NULL;
    else if (strcmp(errfile, "stderr") == 0)
      err_stream = stderr;
    else if (strcmp(errfile, "stdout") == 0)
      err_stream = stdout;
    else
      err_stream = fopen(errfile, "a");
  }
  if (err_stream)
    fprintf(err_stream, "%s\n", errmsg);

  /* check err_fail_mode */
  if (err_fail_mode < 0) {
    char *errmode = getenv("ERR_FAIL_MODE");
    if (!errmode || !errmode[0])
      err_fail_mode = 0;
    else if (strcasecmp(errmode, "exit") == 0)
      err_fail_mode = 1;
    else if (strcasecmp(errmode, "abort") == 0)
      err_fail_mode = 2;
    else
      err_fail_mode = atoi(errmode);
  }
  if (err_fail_mode == 1)
    exit(eval);
  else if (err_fail_mode > 1)
    abort();

  return eval;
}


#define BODY(errname, errnum, pos)                       \
  do {                                                   \
    va_list ap;                                          \
    va_start(ap, msg);                                   \
    format_error(errname, eval, errnum, pos, msg, ap);   \
    va_end(ap);                                          \
  } while (0)


void fatal(int eval, const char *msg,...)
{
  BODY("Fatal", errno, NULL);
  exit(eval);
}

void fatalx(int eval, const char *msg, ...)
{
  BODY("Fatal", 0, NULL);
  exit(eval);
}

int err(int eval, const char *msg, ...)
{
  BODY("Error", errno, NULL);
  return eval;
}

int errx(int eval, const char *msg, ...)
{
  BODY("Error", 0, NULL);
  return eval;
}


void vfatal(int eval, const char *pos, const char *msg, va_list ap)
{
  exit(format_error("Fatal", eval, errno, pos, msg, ap));
}

void vfatalx(int eval, const char *pos, const char *msg, va_list ap)
{
  exit(format_error("Fatal", eval, 0, pos, msg, ap));
}

int verr(int eval, const char *pos, const char *msg, va_list ap)
{
  return format_error("Error", eval, errno, pos, msg, ap);
}

int verrx(int eval, const char *pos, const char *msg, va_list ap)
{
  return format_error("Error", eval, 0, pos, msg, ap);
}


/* Associated functions */
int err_getcode()
{
  return errcode;
}

char *err_getmsg()
{
  return (errcode) ? errmsg : "";
}

void err_clear()
{
  errcode = 0;
}

void err_set_prefix(const char *prefix)
{
  err_prefix = prefix;
}

void err_set_fail_mode(int mode)
{
  err_fail_mode = mode;
}
