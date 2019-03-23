/* error.c -- simple error reporting
 */
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


/* Prefix to append to all errors in this application.  Typically the
   program name. */
static const char *err_prefix = "";

/* Stream for print errors. Set to NULL to for silent. */
static FILE *err_stream = err_default_stream;

/* Indicate wheter the error functions should return, exit or about.
 *   - err_abort_mode >= 2: abort
 *   - err_abort_mode == 1: exit (with error value)
 *   - err_abort_mode == 0: normal return
 *   - err_abort_mode < 0:  check ERR_ABORT environment variable (default)*/
static int err_abort_mode = -1;

/* Indicates whether error messages should include debugging info.
 *   - err_debug_mode >= 2:  include file and line number and function
 *   - err_debug_mode == 1:  include file and line number
 *   - err_debug_mode == 0:  no debugging info
 *   - err_debug_mode < 0:   check ERR_DEBUG environment variable */
static int err_debug_mode = -1;

/* How to handle overridden errors in  ErrTry clauses.
 *   - err_override >= 4:  ignore new error
 *   - err_override == 3:  ignore old error
 *   - err_override == 2:  ignore new error and write warning to error stream
 *   - err_override == 1:  ignore old error and write warning to error stream
 *   - err_override == 0:  append to previous error
 *   - err_override < 0:   check ERR_OVERRIDE environment variable */
static int err_override = -1;

/* Error handler */
//static void err_default_handler(const ErrRecord *record);
static ErrHandler err_handler = err_default_handler;

/* Tread-local variables */
static _tls ErrRecord err_root_record;
static _tls ErrRecord *err_record = &err_root_record;


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
  char *errmsg = err_record->msg;
  size_t errsize = sizeof(err_record->msg);
  FILE *stream = err_get_stream();
  int debug_mode = err_get_debug_mode();
  int abort_mode = err_get_abort_mode();
  int override = err_get_override_mode();
  int ignore_new_error = 0;
  ErrHandler handler = err_get_handler();

  /* Handle overridden errors */
  if (err_record->eval && err_record->prev && !err_record->handled) {
    switch (override) {
    case 0:
      n = strlen(errmsg);
      n += snprintf(errmsg + n, errsize - n, "\n");
      break;
    case 1:
      if (stream) fprintf(stream, "Warning: Overriding old error: '%s'\n",
                          err_record->msg);
      break;
    case 2:
      ignore_new_error = 1;
      if (stream) fprintf(stream, "Warning: Ignoring new error %d\n",
                          err_record->eval);
      break;
    case 3:
      break;
    case 4:
    default:
      ignore_new_error = 1;
    }
  }

  /* Update the current error record */
  if (!ignore_new_error) {
    err_record->eval = eval;

    if (err_prefix && *err_prefix)
      n += snprintf(errmsg + n, errsize - n, "%s: ", err_prefix);

    if (debug_mode >= 1)
      n += snprintf(errmsg + n, errsize - n, "%s: ", file);
    if (debug_mode >= 2)
      n += snprintf(errmsg + n, errsize - n, "in %s(): ", func);

    if (eval) {
      n += snprintf(errmsg + n, errsize - n, "%s %d: ",
                    (errname && *errname) ? errname : "Errval", eval);
    } else if (errname && *errname) {
      n += snprintf(errmsg + n, errsize - n, "%s: ", errname);
    }
    if (msg && *msg)
      n += vsnprintf(errmsg + n, errsize - n, msg, ap);
    if (errnum)
      n += snprintf(errmsg + n, errsize - n, ": %s", strerror(errnum));

    if (n >= (int)errsize && stream)
      fprintf(stream, "Warning: error %d truncated due to full message buffer",
              eval);

    /* call the error handler */
    if ((!err_record->prev || err_record->handled) && handler)
      handler(err_record);
  }

  /* check err_abort_mode */
  if (abort_mode == 1)
    exit(eval);
  else if (abort_mode >= 2)
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

#define BODY(errname, eval, errnum)                             \
  do {								\
    va_list ap;							\
    va_start(ap, msg);						\
    _err_vformat(errname, eval, errnum, NULL, NULL, msg, ap);	\
    va_end(ap);							\
  } while (0)


void fatal(int eval, const char *msg,...)
{
  BODY("Fatal", eval, errno);
  exit(eval);
}

void fatalx(int eval, const char *msg, ...)
{
  BODY("Fatal", eval, 0);
  exit(eval);
}

int err(int eval, const char *msg, ...)
{
  BODY("Error", eval, errno);
  return eval;
}

int errx(int eval, const char *msg, ...)
{
  BODY("Error", eval, 0);
  return eval;
}

int warn(const char *msg, ...)
{
  BODY("Warning", 0, errno);
  return 0;
}

int warnx(const char *msg, ...)
{
  BODY("Warning", 0, 0);
  return 0;
}


void vfatal(int eval, const char *msg, va_list ap)
{
  exit(_err_vformat("Fatal", eval, errno, NULL, NULL, msg, ap));
}

void vfatalx(int eval, const char *msg, va_list ap)
{
  exit(_err_vformat("Fatal", eval, 0, NULL, NULL, msg, ap));
}

int verr(int eval, const char *msg, va_list ap)
{
  return _err_vformat("Error", eval, errno, NULL, NULL, msg, ap);
}

int verrx(int eval, const char *msg, va_list ap)
{
  return _err_vformat("Error", eval, 0, NULL, NULL, msg, ap);
}

int vwarn(const char *msg, va_list ap)
{
  return _err_vformat("Warning", 0, errno, NULL, NULL, msg, ap);
}

int vwarnx(const char *msg, va_list ap)
{
  return _err_vformat("Warning", 0, 0, NULL, NULL, msg, ap);
}

#endif /* HAVE___VA_ARGS__ */



/* Associated functions */
int err_geteval(void)
{
  return err_record->eval;
}

char *err_getmsg(void)
{
  return err_record->msg;
}

void err_clear(void)
{
  errno = 0;
  err_record->eval = 0;
  err_record->msg[0] = '\0';
}

const char *err_set_prefix(const char *prefix)
{
  const char *prev = err_prefix;
  err_prefix = prefix;
  return prev;
}

const char *err_get_prefix(void)
{
  return err_prefix;
}

/* whether the error stream has been opened with fopen() */
static int err_stream_opened;

/* whether `atexit(err_close_stream)` has been called */
static int err_stream_atexit_called;

static void err_close_stream(void)
{
  if (err_stream_opened) {
    fclose(err_stream);
    err_stream_opened = 0;
  }
}

FILE *err_set_stream(FILE *stream)
{
  FILE *prev = err_stream;
  err_close_stream();
  err_stream = stream;
  return prev;
}

FILE *err_get_stream(void)
{
  if (err_stream == err_default_stream) {
    char *errfile = getenv("ERR_STREAM");
    if (!errfile) {
      err_stream = stderr;
    } else if (!errfile[0]) {
      err_stream = NULL;
    } else if (strcmp(errfile, "stderr") == 0) {
      err_stream = stderr;
    } else if (strcmp(errfile, "stdout") == 0) {
      err_stream = stdout;
    } else {
      err_stream = fopen(errfile, "a");
      if (!err_stream) { /* fallback to stderr if errfile cannot be opened */
        err_stream = stderr;
      } else {
        err_stream_opened = 1;
        if (!err_stream_atexit_called) {
          atexit(err_close_stream);
          err_stream_atexit_called = 1;
        }
      }
    }
  }
  return err_stream;
}

int err_set_abort_mode(int mode)
{
  int prev = err_abort_mode;
  err_abort_mode = mode;
  return prev;
}

int err_get_abort_mode(void)
{
  if (err_abort_mode < 0) {
    char *mode = getenv("ERR_ABORT");
    if (!mode || !mode[0])
      err_abort_mode = 0;
    else if (strcasecmp(mode, "exit") == 0)
      err_abort_mode = 1;
    else if (strcasecmp(mode, "abort") == 0)
      err_abort_mode = 2;
    else
      err_abort_mode = atoi(mode);
    if (err_abort_mode < 0) err_abort_mode = 0;
  }
  return err_abort_mode;
}

int err_set_debug_mode(int mode)
{
  int prev = err_debug_mode;
  err_debug_mode = mode;
  return prev;
}

int err_get_debug_mode()
{
  if (err_debug_mode < 0) {
    char *mode = getenv("ERR_DEBUG");
    err_debug_mode =
      (!mode || !*mode) ? 0 :
      (strcmp(mode, "debug") == 0) ? 1 :
      (strcmp(mode, "full") == 0) ? 2 :
      atoi(mode);
    if (err_debug_mode < 0) err_debug_mode = 0;
  }
  return err_debug_mode;
}

int err_set_override_mode(int mode)
{
  int prev = err_override;
  err_override = mode;
  return prev;
}

int err_get_override_mode()
{
  if (err_override < 0) {
    char *mode = getenv("ERR_OVERRIDE");
    err_override =
      (!mode || !*mode) ? 0 :
      (strcmp(mode, "warn-old") == 0) ? 1 :
      (strcmp(mode, "warn-new") == 0) ? 2 :
      (strcmp(mode, "ignore-old") == 0) ? 3 :
      (strcmp(mode, "ignore-new") == 0) ? 4 :
      atoi(mode);
    if (err_override < 0) err_override = 0;
  }
  return err_override;
}

/* Default error handler. */
static void _err_default_handler(const ErrRecord *record)
{
  if (err_stream) fprintf(err_stream, "%s\n", record->msg);
}

ErrHandler err_set_handler(ErrHandler handler)
{
  ErrHandler prev = err_handler;
  err_handler = handler;
  return prev;
}

ErrHandler err_get_handler(void)
{
  if (err_handler == err_default_handler)
    err_handler = _err_default_handler;
  return err_handler;
}


/* Returns a pointer to the error record */
ErrRecord *_err_get_record()
{
  return err_record;
}


/* ---------------------
 * ErrTry/ErrCatch block
 * ---------------------*/

void _err_link_record(ErrRecord *record)
{
  memset(record, 0, sizeof(ErrRecord));
  record->prev = err_record;
  err_record = record;
}

void _err_unlink_record(ErrRecord *record)
{
  assert(record == err_record);
  assert(err_record->prev);
  err_record = record->prev;
  if (record->eval && !record->handled) {

    if (record->eval) {
      int debug_mode = err_debug_mode;
      if (debug_mode < 0) {
	char *mode = getenv("ERR_DEBUG_MODE");
	debug_mode = (mode) ? atoi(mode) : 0;
      }
      if (debug_mode > 0)
	fprintf(err_stream, "Warning: overriding unhandled error: %s",
		record->msg);
    }

    err_record->eval = record->eval;
    strcpy(err_record->msg, record->msg);

    /* reemit unhandled exeption */
    if (record->exception) {
      if (err_record->prev) {
        longjmp(err_record->env, err_record->eval);
      } else {
        if (err_handler)
          err_handler(err_record);
        exit(err_record->eval);
      }
    }

    /* reemit unhandled error */
    if (!err_record->prev && err_handler)
      err_handler(err_record);
  }
}
