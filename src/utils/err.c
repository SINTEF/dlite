/* err.c -- simple error reporting library
 *
 * Copyright (C) 2010-2020 SINTEF
 *
 * Distributed under terms of the MIT license.
 */

/*
 * TODO: Consider to reuse good ideas from https://github.com/rxi/log.c
 */

#include <assert.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "compat.h"
#include "err.h"

/* Thread local storage
 * https://stackoverflow.com/questions/18298280/how-to-declare-a-variable-as-thread-local-portab
 */
#if defined HAVE_THREADS && !defined thread_local
# if __STDC_VERSION__ >= 201112 && !defined __STDC_NO_THREADS__
#  define thread_local _Thread_local
# elif defined _WIN32 && ( \
       defined _MSC_VER || \
       defined __ICL || \
       defined __DMC__ || \
       defined __BORLANDC__ )
#  define thread_local __declspec(thread)
/* note that ICC (linux) and Clang are covered by __GNUC__ */
# elif defined __GNUC__ || \
       defined __SUNPRO_C || \
       defined __xlC__
#  define thread_local __thread
# else
#  error "Cannot define thread_local"
# endif
#else
# define thread_local
#endif


/* Global variables */
typedef struct {
  const char *err_prefix;  //!< Prefix to append to all errors in this
                           //!< application.  Typically the program name.
  FILE *err_stream;        //!< Stream for error printing. Set to NULL to silent
  ErrHandler err_handler;  //!< Error handler
  int err_stream_opened;   //!< Whether err_stream has been opened
  int err_stream_atexit_called;  //!< Whether atexit() has been installed
} Globals;

/* Thread local variables */
typedef struct {

  /* Indicate wheter the error functions should return, exit or about.
   * If negative (default), check the environment. */
  ErrAbortMode err_abort_mode;

  /* Indicate whether warnings should be turned into errors.
   * If negative (default), check the environment. */
  ErrWarnMode err_warn_mode;

  /* Indicates whether error messages should include debugging info.
   * If negative (default), check the environment. */
  ErrDebugMode err_debug_mode;

  /* How to handle overridden errors in  ErrTry clauses.
   * If negative (default), check the environment. */
  ErrOverrideMode err_override;

  /* Pointer to the top level record. */
  ErrRecord *err_record;

  /* Root of the linked list of error records. */
  ErrRecord err_root_record;

  /* Pointer to global state */
  Globals *globals;

} ThreadLocals;

/* Global state */
static Globals _globals = {"", err_default_stream, err_default_handler, 0, 0};

/* Thread local state */
static thread_local ThreadLocals _tls;

/* Whether _cls is initialised */
static thread_local int _tls_initialized = 0;


/* Reset thread local state */
static void reset_tls(void)
{
  memset(&_globals, 0, sizeof(_globals));
  _globals.err_prefix = "";
  _globals.err_stream = err_default_stream;
  _globals.err_handler = err_default_handler;
  _globals.err_stream_opened = 0;
  _globals.err_stream_atexit_called = 0;

  memset(&_tls, 0, sizeof(_tls));
  _tls.err_abort_mode = -1;
  _tls.err_warn_mode = -1;
  _tls.err_debug_mode = -1;
  _tls.err_override = -1;
  _tls.err_record = &_tls.err_root_record;
  _tls.globals = &_globals;
}

/* Returns a pointer to thread local state */
static ThreadLocals *get_tls(void)
{
  if (!_tls_initialized) {
    _tls_initialized = 1;
    memset(&_tls, 0, sizeof(_tls));
    _tls.err_abort_mode = -1;
    _tls.err_warn_mode = -1;
    _tls.err_debug_mode = -1;
    _tls.err_override = -1;
    _tls.err_record = &_tls.err_root_record;
    _tls.globals = &_globals;
  }
  return &_tls;
}

/* Return a pointer to (thread local) state for this module */
void *err_get_state(void)
{
  return (void *)get_tls();
}

/* Sets state from state returned by err_get_state().
   If `state` is NULL, the state is initialised to default values. */
void err_set_state(void *state)
{
  if (state) {
    ThreadLocals *src = state;
    memcpy(&_tls, src, sizeof(ThreadLocals));
    memcpy(&_globals, src->globals, sizeof(Globals));
  } else {
    reset_tls();
  }
}

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
 *  errlevel : error level
 *  eval     : error value that is returned or passed exit()
 *  errnum   : error number for system errors
 *  file     : file and line number in source file where the error occured
 *  func     : name of function in which the error occured
 *  msg      : error message
 *  ap       : printf()-like argument list for error message
 */
int _err_vformat(ErrLevel errlevel, int eval, int errnum, const char *file,
		 const char *func, const char *msg, va_list ap)
{
  ThreadLocals *tls = get_tls();
  int n=0;
  char *errname = error_names[errlevel];
  char *errmsg = tls->err_record->msg;
  size_t errsize = sizeof(tls->err_record->msg);
  FILE *stream = err_get_stream();
  ErrDebugMode debug_mode = err_get_debug_mode();
  ErrAbortMode abort_mode = err_get_abort_mode();
  ErrWarnMode warn_mode = err_get_warn_mode();
  ErrOverrideMode override = err_get_override_mode();
  int ignore_new_error = 0;
  ErrHandler handler = err_get_handler();
  int call_handler = handler && !tls->err_record->prev;

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
  if (tls->err_record->eval) {
    switch (override) {
    case errOverrideAppend:
      n = strlen(errmsg);
      n += snprintf(errmsg + n, errsize - n, "%s", err_append_sep);
      break;
    case errOverrideWarnOld:
      if (stream) fprintf(stream, "Warning: Overriding old error: '%s'\n",
                          tls->err_record->msg);
      break;
    case errOverrideWarnNew:
      ignore_new_error = 1;
      if (stream) fprintf(stream, "Warning: Ignoring new error %d\n",
                          tls->err_record->eval);
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
  tls->err_record->level = errlevel;
  tls->err_record->eval = eval;
  tls->err_record->errnum = errnum;

  /* Write error message */
  if (!ignore_new_error) {
    Globals *g = tls->globals;

    if (g->err_prefix && *g->err_prefix)
      n += snprintf(errmsg + n, errsize - n, "%s: ", g->err_prefix);

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
  if (errlevel >= errLevelError && tls->err_record->state)
    tls->err_record->reraise = eval;

  /* If we are not within a ErrTry...ErrEnd clause */
  if (call_handler) {

    /* ...call the error handler */
    handler(tls->err_record);

    /* ...check err_abort_mode */
    if (errlevel >= errLevelError) {
      if (abort_mode == errAbortExit) {
        if (!call_handler) handler(tls->err_record);
        exit(eval);
      } else if (abort_mode >= errAbortAbort) {
        if (!call_handler) handler(tls->err_record);
        abort();
      }
    }

    /* ...make sure that fatal errors always exit */
    if (errlevel >= errLevelFatal) {
      if (!call_handler) handler(tls->err_record);
      exit(eval);
    }
  }

  /* Clear errno */
  errno = 0;

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
  ThreadLocals *tls = get_tls();
  return tls->err_record->eval;
}

int err_update_eval(int eval)
{
  ThreadLocals *tls = get_tls();
  if (tls->err_record->eval) tls->err_record->eval = eval;
  return tls->err_record->eval;
}

const char *err_getmsg(void)
{
  ThreadLocals *tls = get_tls();
  return tls->err_record->msg;
}

void err_clear(void)
{
  ThreadLocals *tls = get_tls();
  errno = 0;
  tls->err_record->level = 0;
  tls->err_record->eval = 0;
  tls->err_record->errnum = 0;
  tls->err_record->msg[0] = '\0';
  tls->err_record->handled = 0;
  tls->err_record->reraise = 0;
  tls->err_record->state = 0;
}

const char *err_set_prefix(const char *prefix)
{
  ThreadLocals *tls = get_tls();
  Globals *g = tls->globals;
  const char *prev = g->err_prefix;
  g->err_prefix = prefix;
  return prev;
}

const char *err_get_prefix(void)
{
  ThreadLocals *tls = get_tls();
  Globals *g = tls->globals;
  return g->err_prefix;
}

/* whether the error stream has been opened with fopen() */
//static int err_stream_opened = 0;

/* whether `atexit(err_close_stream)` has been called */
//static int err_stream_atexit_called = 0;

static void err_close_stream(void)
{
  ThreadLocals *tls = get_tls();
  Globals *g = tls->globals;
  if (g->err_stream_opened) {
    fflush(g->err_stream);
    if (g->err_stream != stderr && g->err_stream != stdout)
      fclose(g->err_stream);
    g->err_stream_opened = 0;
  }
}

FILE *err_set_stream(FILE *stream)
{
  ThreadLocals *tls = get_tls();
  Globals *g = tls->globals;
  FILE *prev = g->err_stream;
  err_close_stream();
  g->err_stream = stream;
  return prev;
}

FILE *err_get_stream(void)
{
  ThreadLocals *tls = get_tls();
  Globals *g = tls->globals;
  if (g->err_stream == err_default_stream) {
    char *errfile = getenv("ERR_STREAM");
    if (!errfile) {
      g->err_stream = stderr;
    } else if (!errfile[0]) {
      g->err_stream = NULL;
    } else if (strcmp(errfile, "stderr") == 0) {
      g->err_stream = stderr;
    } else if (strcmp(errfile, "stdout") == 0) {
      g->err_stream = stdout;
    } else {
      g->err_stream = fopen(errfile, "a");
      if (!g->err_stream) { /* fallback to stderr if errfile cannot be opened */
        g->err_stream = stderr;
      } else {
        g->err_stream_opened = 1;
        if (!g->err_stream_atexit_called) {
          atexit(err_close_stream);
          g->err_stream_atexit_called = 1;
        }
      }
    }
  }
  return g->err_stream;
}

ErrAbortMode err_set_abort_mode(int mode)
{
  ThreadLocals *tls = get_tls();
  int prev = tls->err_abort_mode;
  tls->err_abort_mode = mode;
  return prev;
}

ErrAbortMode err_get_abort_mode(void)
{
  ThreadLocals *tls = get_tls();
  if (tls->err_abort_mode < 0) {
    char *mode = getenv("ERR_ABORT");
    if (!mode || !mode[0])
      tls->err_abort_mode = errAbortNormal;
    else if (strcasecmp(mode, "exit") == 0)
      tls->err_abort_mode = errAbortExit;
    else if (strcasecmp(mode, "abort") == 0)
      tls->err_abort_mode = errAbortAbort;
    else
      tls->err_abort_mode = atoi(mode);
    if (tls->err_abort_mode < 0) tls->err_abort_mode = 0;
    if (tls->err_abort_mode > errAbortAbort) tls->err_abort_mode = errAbortAbort;
  }
  return tls->err_abort_mode;
}

ErrWarnMode err_set_warn_mode(int mode)
{
  ThreadLocals *tls = get_tls();
  int prev = tls->err_warn_mode;
  tls->err_warn_mode = mode;
  return prev;
}

ErrWarnMode err_get_warn_mode()
{
  ThreadLocals *tls = get_tls();
  if (tls->err_warn_mode < 0) {
    char *mode = getenv("ERR_WARN");
    if (!mode || !mode[0])
      tls->err_warn_mode = errWarnNormal;
    else if (strcasecmp(mode, "ignore") == 0)
      tls->err_warn_mode = errWarnIgnore;
    else if (strcasecmp(mode, "error") == 0)
      tls->err_warn_mode = errWarnError;
    else
      tls->err_warn_mode = atoi(mode);
    if (tls->err_warn_mode < 0) tls->err_warn_mode = errWarnNormal;
    if (tls->err_warn_mode > errWarnError) tls->err_warn_mode = errWarnError;
  }
  return tls->err_warn_mode;
}

ErrDebugMode err_set_debug_mode(int mode)
{
  ThreadLocals *tls = get_tls();
  int prev = tls->err_debug_mode;
  tls->err_debug_mode = mode;
  return prev;
}

ErrDebugMode err_get_debug_mode()
{
  ThreadLocals *tls = get_tls();
  if (tls->err_debug_mode < 0) {
    char *mode = getenv("ERR_DEBUG");
    tls->err_debug_mode =
      (!mode || !*mode)             ? errDebugOff :
      (strcmp(mode, "debug") == 0)  ? errDebugSimple :
      (strcmp(mode, "full") == 0)   ? errDebugFull :
      atoi(mode);
    if (tls->err_debug_mode < 0) tls->err_debug_mode = errDebugOff;
    if (tls->err_debug_mode > errDebugFull) tls->err_debug_mode = errDebugFull;
  }
  return tls->err_debug_mode;
}

ErrOverrideMode err_set_override_mode(int mode)
{
  ThreadLocals *tls = get_tls();
  int prev = tls->err_override;
  tls->err_override = mode;
  return prev;
}

ErrOverrideMode err_get_override_mode()
{
  ThreadLocals *tls = get_tls();
  if (tls->err_override < 0) {
    char *mode = getenv("ERR_OVERRIDE");
    tls->err_override =
      (!mode || !*mode)                 ? errOverrideAppend :
      (strcmp(mode, "warn-old") == 0)   ? errOverrideWarnOld :
      (strcmp(mode, "warn-new") == 0)   ? errOverrideWarnNew :
      (strcmp(mode, "old") == 0)        ? errOverrideOld :
      (strcmp(mode, "ignore-new") == 0) ? errOverrideIgnoreNew :
      atoi(mode);
    if (tls->err_override < 0) tls->err_override = errOverrideAppend;
    if (tls->err_override > errOverrideIgnoreNew)
      tls->err_override = errOverrideIgnoreNew;
  }
  return tls->err_override;
}

/* Default error handler. */
static void _err_default_handler(const ErrRecord *record)
{
  ThreadLocals *tls = get_tls();
  Globals *g = tls->globals;
  if (g->err_stream) fprintf(g->err_stream, "** %s\n", record->msg);
}

ErrHandler err_set_handler(ErrHandler handler)
{
  ThreadLocals *tls = get_tls();
  Globals *g = tls->globals;
  ErrHandler prev = g->err_handler;
  g->err_handler = handler;
  return prev;
}

ErrHandler err_get_handler(void)
{
  ThreadLocals *tls = get_tls();
  Globals *g = tls->globals;
  if (g->err_handler == err_default_handler)
    g->err_handler = _err_default_handler;
  return g->err_handler;
}


/* Returns a pointer to the error record */
ErrRecord *_err_get_record()
{
  ThreadLocals *tls = get_tls();
  return tls->err_record;
}


/* ---------------------
 * ErrTry/ErrCatch block
 * ---------------------*/

void _err_link_record(ErrRecord *record)
{
  ThreadLocals *tls = get_tls();
  memset(record, 0, sizeof(ErrRecord));
  record->prev = tls->err_record;
  tls->err_record = record;
}

void _err_unlink_record(ErrRecord *record)
{
  ThreadLocals *tls = get_tls();
  Globals *g = tls->globals;
  assert(record == tls->err_record);
  assert(tls->err_record->prev);

  tls->err_record = record->prev;
  if (record->reraise || (record->eval && !record->handled)) {
    int eval = (record->reraise) ? record->reraise : record->eval;
    ErrAbortMode abort_mode = err_get_abort_mode();
    int ignore_new = 0;
    int n = 0;

    if (tls->err_record->eval) {
      switch (err_get_override_mode()) {
      case errOverrideEnv:
      case errOverrideAppend:
        n = strlen(tls->err_record->msg);
        strncat(tls->err_record->msg+n, err_append_sep, ERR_MSGSIZE-n);
        n += strlen(err_append_sep);
        break;
      case errOverrideWarnOld:
        fprintf(stderr, "** Warning: overwriting old error: %s\n",
                tls->err_record->msg);
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
    tls->err_record->level = record->level;
    tls->err_record->eval = eval;
    tls->err_record->errnum = record->errnum;
    if (!ignore_new) strncpy(tls->err_record->msg+n, record->msg, ERR_MSGSIZE-n);

    if (record->level == errLevelException && tls->err_record->prev)
      longjmp(tls->err_record->env, eval);

    if (!tls->err_record->prev) {
      ErrHandler handler = err_get_handler();
      if (handler) g->err_handler(tls->err_record);
    }

    if ((abort_mode && record->level >= errLevelError) ||
        record->level >= errLevelException) {
      if (abort_mode == errAbortAbort) abort();
      exit(eval);
    }
  }
}
