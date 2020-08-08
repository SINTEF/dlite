/* err.c -- simple error reporting library
 *
 * Copyright (C) 2010-2020 SINTEF
 *
 * Distributed under terms of the MIT license.
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
 * If negative (default), check the environment. */
static _tls ErrAbortMode err_abort_mode = -1;

/* Indicate whether warnings should be turned into errors.
 * If negative (default), check the environment. */
static _tls ErrWarnMode err_warn_mode = -1;

/* Indicates whether error messages should include debugging info.
 * If negative (default), check the environment. */
static _tls ErrDebugMode err_debug_mode = -1;

/* How to handle overridden errors in  ErrTry clauses.
 * If negative (default), check the environment. */
static _tls ErrOverrideMode err_override = -1;

/* Error handler */
static ErrHandler err_handler = err_default_handler;

/* Error records
 * Hold the latest error and message. These are thread-local to ensure
 * that errors in different threads doesn't mess up with each other. */

/* Root of the linked list of error records. */
static _tls ErrRecord err_root_record;

/* Pointer to the top level record. */
static _tls ErrRecord *err_record = &err_root_record;

/* Separator between appended errors */
static char *err_append_sep = "\n - ";

/* Error names */
static char *error_names[] = {
  "Success",
  "Warning",
  "Error",
  "Exception",
  "Fatal"
};


/* Reports the error and returns `eval`.  Args:
 *  errname : name of error, e.g. "Fatal" or "Error"
 *  eval    : error value that is returned or passed exit()
 *  errnum  : error number for system errors
 *  file    : file and line number in source file where the error occured
 *  func    : name of function in which the error occured
 *  msg     : error message
 *  ap      : printf()-like argument list for error message
 */
int _err_vformat(ErrLevel errlevel, int eval, int errnum, const char *file,
		 const char *func, const char *msg, va_list ap)
{
  int n=0;
  char *errname = error_names[errlevel];
  char *errmsg = err_record->msg;
  size_t errsize = sizeof(err_record->msg);
  FILE *stream = err_get_stream();
  ErrDebugMode debug_mode = err_get_debug_mode();
  ErrAbortMode abort_mode = err_get_abort_mode();
  ErrWarnMode warn_mode = err_get_warn_mode();
  ErrOverrideMode override = err_get_override_mode();
  int ignore_new_error = 0;
  ErrHandler handler = err_get_handler();
  int call_handler = handler && !err_record->prev;

  /* Check warning mode */
  if (errlevel == errLevelWarn) {
    switch (warn_mode) {
    case errWarnNormal:
      break;
    case errWarnIgnore:
      return 0;
    case errWarnError:
      errlevel = errLevelError;
      errname = error_names[errlevel];
      break;
    default:  // should never be reached
      assert(0);
    }
  }

  /* Handle overridden errors */
  if (err_record->eval) {
    switch (override) {
    case errOverrideAppend:
      n = strlen(errmsg);
      n += snprintf(errmsg + n, errsize - n, err_append_sep);
      break;
    case errOverrideWarnOld:
      if (stream) fprintf(stream, "Warning: Overriding old error: '%s'\n",
                          err_record->msg);
      break;
    case errOverrideWarnNew:
      ignore_new_error = 1;
      if (stream) fprintf(stream, "Warning: Ignoring new error %d\n",
                          err_record->eval);
      break;
    case errOverrideOld:
      break;
    case errOverrideIgnoreNew:
      ignore_new_error = 1;
      break;
    default:  // should never be reached
      assert(0);
    }
  }

  /* Update the current error record */
  err_record->level = errlevel;
  err_record->eval = eval;
  err_record->errnum = errnum;

  /* Write error message */
  if (!ignore_new_error) {

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
  }

  /* If this error occured after the try clause in an ErrTry handler,
     we mark this error to be reraised after leaving the handler. */
  if (errlevel >= errLevelError && err_record->state)
    err_record->reraise = eval;

  /* call the error handler if we are not within a ErrTry...ErrEnd clause */
  if (call_handler) handler(err_record);

  /* check err_abort_mode */
  if (errlevel >= errLevelError) {
    if (abort_mode == errAbortExit) {
      if (!call_handler) handler(err_record);
      exit(eval);
    } else if (abort_mode >= errAbortAbort) {
      if (!call_handler) handler(err_record);
      abort();
    }
  }

  /* fatal errors should never exit */
  if (errlevel >= errLevelFatal) {
    if (!call_handler) handler(err_record);
    exit(eval);
  }

  return eval;
}

int _err_format(ErrLevel errlevel, int eval, int errnum, const char *file,
                const char *func, const char *msg, ...)
{
  va_list ap;
  va_start(ap, msg);
  _err_vformat(errlevel, eval, errnum, file, func, msg, ap);
  va_end(ap);
  return eval;
}

#ifndef HAVE___VA_ARGS__

#define BODY(errlevel, eval, errnum)                            \
  do {								\
    va_list ap;							\
    va_start(ap, msg);						\
    _err_vformat(errlevel, eval, errnum, NULL, NULL, msg, ap);	\
    va_end(ap);							\
  } while (0)


void fatal(int eval, const char *msg,...)
{
  BODY(errLevelFatal, eval, errno);
  exit(eval);
}

void fatalx(int eval, const char *msg, ...)
{
  BODY(errLevelFatal, eval, 0);
  exit(eval);
}

int err(int eval, const char *msg, ...)
{
  BODY(errLevelError, eval, errno);
  return eval;
}

int errx(int eval, const char *msg, ...)
{
  BODY(errLevelError, eval, 0);
  return eval;
}

int warn(const char *msg, ...)
{
  BODY(errLevelWarn, 0, errno);
  return 0;
}

int warnx(const char *msg, ...)
{
  BODY(errLevelWarn, 0, 0);
  return 0;
}

int err_generic(int errlevel, int eval, int errnum, const char *msg, ...)
{
  BODY(errlevel, eval, errnum);
  return eval;
}


void vfatal(int eval, const char *msg, va_list ap)
{
  exit(_err_vformat(errLevelFatal, eval, errno, NULL, NULL, msg, ap));
}

void vfatalx(int eval, const char *msg, va_list ap)
{
  exit(_err_vformat(errLevelFatal, eval, 0, NULL, NULL, msg, ap));
}

int verr(int eval, const char *msg, va_list ap)
{
  return _err_vformat(errLevelError, eval, errno, NULL, NULL, msg, ap);
}

int verrx(int eval, const char *msg, va_list ap)
{
  return _err_vformat(errLevelError, eval, 0, NULL, NULL, msg, ap);
}

int vwarn(const char *msg, va_list ap)
{
  return _err_vformat(errLevelWarn, 0, errno, NULL, NULL, msg, ap);
}

int vwarnx(const char *msg, va_list ap)
{
  return _err_vformat(errLevelWarn, 0, 0, NULL, NULL, msg, ap);
}

int verr_generic(int errlevel, int eval, int errnum, const char *msg, va_list ap)
{
  return _err_vformat(errlevel, eval, errnum, NULL, NULL, msg, ap);
}

#endif /* HAVE___VA_ARGS__ */



/* Associated functions */
int err_geteval(void)
{
  return err_record->eval;
}

int err_update_eval(int eval)
{
  if (err_record->eval) err_record->eval = eval;
  return err_record->eval;
}

const char *err_getmsg(void)
{
  return err_record->msg;
}

void err_clear(void)
{
  errno = 0;
  err_record->level = 0;
  err_record->eval = 0;
  err_record->errnum = 0;
  err_record->msg[0] = '\0';
  err_record->handled = 0;
  err_record->reraise = 0;
  err_record->state = 0;
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
static int err_stream_opened = 0;

/* whether `atexit(err_close_stream)` has been called */
static int err_stream_atexit_called = 0;

static void err_close_stream(void)
{
  if (err_stream_opened) {
    fflush(err_stream);
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

ErrAbortMode err_set_abort_mode(int mode)
{
  int prev = err_abort_mode;
  err_abort_mode = mode;
  return prev;
}

ErrAbortMode err_get_abort_mode(void)
{
  if (err_abort_mode < 0) {
    char *mode = getenv("ERR_ABORT");
    if (!mode || !mode[0])
      err_abort_mode = errAbortNormal;
    else if (strcasecmp(mode, "exit") == 0)
      err_abort_mode = errAbortExit;
    else if (strcasecmp(mode, "abort") == 0)
      err_abort_mode = errAbortAbort;
    else
      err_abort_mode = atoi(mode);
    if (err_abort_mode < 0) err_abort_mode = 0;
    if (err_abort_mode > errAbortAbort) err_abort_mode = errAbortAbort;
  }
  return err_abort_mode;
}

ErrWarnMode err_set_warn_mode(int mode)
{
  int prev = err_warn_mode;
  err_warn_mode = mode;
  return prev;
}

ErrWarnMode err_get_warn_mode()
{
  if (err_warn_mode < 0) {
    char *mode = getenv("ERR_WARN");
    if (!mode || !mode[0])
      err_warn_mode = errWarnNormal;
    else if (strcasecmp(mode, "ignore") == 0)
      err_warn_mode = errWarnIgnore;
    else if (strcasecmp(mode, "error") == 0)
      err_warn_mode = errWarnError;
    else
      err_warn_mode = atoi(mode);
    if (err_warn_mode < 0) err_warn_mode = errWarnNormal;
    if (err_warn_mode > errWarnError) err_warn_mode = errWarnError;
  }
  return err_warn_mode;
}

ErrDebugMode err_set_debug_mode(int mode)
{
  int prev = err_debug_mode;
  err_debug_mode = mode;
  return prev;
}

ErrDebugMode err_get_debug_mode()
{
  if (err_debug_mode < 0) {
    char *mode = getenv("ERR_DEBUG");
    err_debug_mode =
      (!mode || !*mode)             ? errDebugOff :
      (strcmp(mode, "debug") == 0)  ? errDebugSimple :
      (strcmp(mode, "full") == 0)   ? errDebugFull :
      atoi(mode);
    if (err_debug_mode < 0) err_debug_mode = errDebugOff;
    if (err_debug_mode > errDebugFull) err_debug_mode = errDebugFull;
  }
  return err_debug_mode;
}

ErrOverrideMode err_set_override_mode(int mode)
{
  int prev = err_override;
  err_override = mode;
  return prev;
}

ErrOverrideMode err_get_override_mode()
{
  if (err_override < 0) {
    char *mode = getenv("ERR_OVERRIDE");
    err_override =
      (!mode || !*mode)                 ? errOverrideAppend :
      (strcmp(mode, "warn-old") == 0)   ? errOverrideWarnOld :
      (strcmp(mode, "warn-new") == 0)   ? errOverrideWarnNew :
      (strcmp(mode, "old") == 0)        ? errOverrideOld :
      (strcmp(mode, "ignore-new") == 0) ? errOverrideIgnoreNew :
      atoi(mode);
    if (err_override < 0) err_override = errOverrideAppend;
    if (err_override > errOverrideIgnoreNew)
      err_override = errOverrideIgnoreNew;
  }
  return err_override;
}

/* Default error handler. */
static void _err_default_handler(const ErrRecord *record)
{
  if (err_stream) fprintf(err_stream, "** %s\n", record->msg);
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
  if (record->reraise || (record->eval && !record->handled)) {
    int eval = (record->reraise) ? record->reraise : record->eval;
    ErrAbortMode abort_mode = err_get_abort_mode();
    int ignore_new = 0;
    int n = 0;

    if (err_record->eval) {
      switch (err_get_override_mode()) {
      case errOverrideEnv:
      case errOverrideAppend:
        n = strlen(err_record->msg);
        strncat(err_record->msg+n, err_append_sep, ERR_MSGSIZE-n);
        n += strlen(err_append_sep);
        break;
      case errOverrideWarnOld:
        fprintf(stderr, "** Warning: overwriting old error: %s\n",
                err_record->msg);
        break;
      case errOverrideWarnNew:
        ignore_new = 1;
        fprintf(stderr, "** Warning: ignoring error: %s\n", record->msg);
        break;
      case errOverrideOld:
        break;
      case errOverrideIgnoreNew:
        ignore_new = 1;
        break;
      }
    }
    err_record->level = record->level;
    err_record->eval = eval;
    err_record->errnum = record->errnum;
    if (!ignore_new) strncpy(err_record->msg+n, record->msg, ERR_MSGSIZE-n);

    if (record->level == errLevelException && err_record->prev)
      longjmp(err_record->env, eval);

    if (!err_record->prev) {
      ErrHandler handler = err_get_handler();
      if (handler) err_handler(err_record);
    }

    if ((abort_mode && record->level >= errLevelError) ||
        record->level >= errLevelException) {
      if (abort_mode == errAbortAbort) abort();
      exit(eval);
    }
  }
}
