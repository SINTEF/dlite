/* error.c -- simple error reporting
 */
#ifdef HAVE_CONFIG
#include "config.h"
#endif

#include <assert.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "compat.h"
#include "err.h"

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

/* Indicates whether error messages should include debugging info.
 *   - err_debug_mode >= 2: include file and line number and function
 *   - err_debug_mode == 1: include file and line number
 *   - err_debug_mode == 0: no debugging info
 *   - err_debug_mode < 0:  check ERR_DEBUG_MODE environment variable */
static int err_debug_mode = -1;

/* Tread-local variables */
static _tls ErrRecord base_errrecord = {0, 0, "", NULL, NULL, NULL};
static _tls ErrRecord *errrecord_ptr = &base_errrecord;


/* Reports the error and returns `eval`.  Args:
 *  errname : name of error, e.g. "Fatal" or "Error"
 *  eval    : error value that is returned or passed exit()
 *  errnum  : error number for system errors
 *  file    : file and line number in source file where the error occured
 *  func    : name of function in which the error occured
 *  msg     : error message
 *  ap      : printf()-like argument list for error message
 */
int _err_vformat(const char *errname, int eval, int errnum, const char *file,
		 const char *func, const char *msg, va_list ap)
{
  int n=0;
  char *errmsg = errrecord_ptr->msg;
  size_t errsize = sizeof(errrecord_ptr->msg);
  int debug_mode = err_debug_mode;

  errrecord_ptr->code = eval;
  if (debug_mode < 0) {
    char *mode = getenv("ERR_DEBUG_MODE");
    debug_mode = (mode) ? atoi(mode) : 0;
  }

  if (err_prefix && *err_prefix)
    n += snprintf(errmsg + n, errsize - n, "%s: ", err_prefix);

  if (debug_mode >= 1)
    n += snprintf(errmsg + n, errsize - n, "%s: ", file);
  if (debug_mode >= 2)
    n += snprintf(errmsg + n, errsize - n, "in %s(): ", func);

  if (errname && *errname)
    n += snprintf(errmsg + n, errsize - n, "%s: ", errname);
  if (msg && *msg)
    n += vsnprintf(errmsg + n, errsize - n, msg, ap);
  if (errnum)
    n += snprintf(errmsg + n, errsize - n, ": %s", strerror(errnum));

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

  if (err_stream && !errrecord_ptr->prev)
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
  if (err_fail_mode == 1 && err_stream)
    exit(eval);
  else if (err_fail_mode > 1 && err_stream)
    abort();

  return eval;
}

int _err_format(const char *errname, int eval, int errnum, const char *file,
                const char *func, const char *msg, ...)
{
  va_list ap;
  va_start(ap, msg);
  _err_vformat(errname, eval, errnum, file, func, msg, ap);
  va_end(ap);
  return eval;
}

#ifndef HAVE___VA_ARGS__

#define BODY(errname, errnum)					\
  do {								\
    va_list ap;							\
    va_start(ap, msg);						\
    _err_vformat(errname, eval, errnum, NULL, NULL,, msg, ap);	\
    va_end(ap);							\
  } while (0)


void fatal(int eval, const char *msg,...)
{
  BODY("Fatal", errno);
  exit(eval);
}

void fatalx(int eval, const char *msg, ...)
{
  BODY("Fatal", 0);
  exit(eval);
}

int err(int eval, const char *msg, ...)
{
  BODY("Error", errno);
  return eval;
}

int errx(int eval, const char *msg, ...)
{
  BODY("Error", 0);
  return eval;
}


void vfatal(int eval, const char *msg, va_list ap)
{
  exit(_err_format("Fatal", eval, errno, NULL, NULL, msg, ap));
}

void vfatalx(int eval, const char *msg, va_list ap)
{
  exit(format_error("Fatal", eval, 0, NULL, NULL, msg, ap));
}

int verr(int eval, const char *msg, va_list ap)
{
  return format_error("Error", eval, errno, NULL, NULL, msg, ap);
}

int verrx(int eval, const char *msg, va_list ap)
{
  return format_error("Error", eval, 0, NULL, NULL, msg, ap);
}

#endif /* HAVE___VA_ARGS__ */



/* Associated functions */
int err_getcode()
{
  return errrecord_ptr->code;
}

char *err_getmsg()
{
  return (errrecord_ptr->code) ? errrecord_ptr->msg : "";
}

void err_clear()
{
  errrecord_ptr->code = 0;
}

const char *err_set_prefix(const char *prefix)
{
  const char *current = err_prefix;
  err_prefix = prefix;
  return current;
}

FILE *err_set_stream(FILE *stream)
{
  FILE *current = err_stream;
  err_stream = stream;
  return current;
}

int err_set_fail_mode(int mode)
{
  int current = err_fail_mode;
  err_fail_mode = mode;
  return current;
}

int err_set_debug_mode(int mode)
{
  int current = err_debug_mode;
  err_debug_mode = mode;
  return current;
}


/* ---------------------
 * ErrTry/ErrCatch block
 * ---------------------*/

void _err_link_record(ErrRecord *errrecord)
{
  memset(errrecord, 0, sizeof(ErrRecord));
  errrecord->prev = errrecord_ptr;
  errrecord_ptr = errrecord;
}

void _err_unlink_record(ErrRecord *errrecord)
{
  assert(errrecord == errrecord_ptr);
  assert(errrecord_ptr->prev);
  errrecord_ptr = errrecord->prev;
  if (!errrecord->handled) {

    if (errrecord->code) {
      int debug_mode = err_debug_mode;
      if (debug_mode < 0) {
	char *mode = getenv("ERR_DEBUG_MODE");
	debug_mode = (mode) ? atoi(mode) : 0;
      }
      if (debug_mode > 0)
	fprintf(err_stream, "Warning: overriding unhandled error: %s",
		errrecord->msg);
    }

    /* reemit the error */
    errrecord_ptr->code = errrecord->code;
    strcpy(errrecord_ptr->msg, errrecord->msg);
    errrecord_ptr->func = errrecord->func;
    errrecord_ptr->file = errrecord->file;

    if (err_stream && !errrecord_ptr->prev)
      fprintf(err_stream, "%s\n", errrecord_ptr->msg);
  }
}
