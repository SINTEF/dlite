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
#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "compat.h"
#include "err.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#if defined(HAVE_IO_H) && defined(HAVE__ISATTY)
#include <io.h>
#endif


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
  ErrNameConv err_nameconv;//!< Error name converter function
  int err_stream_opened;   //!< Whether err_stream has been opened
  int err_stream_atexit_called;  //!< Whether atexit() has been installed
} Globals;

/* Thread local variables */
typedef struct {

  /* Lowest error level to report.
   * If negative (default), check the environment. */
  ErrLevel err_level;

  /* Indicate wheter the error functions should return, exit or about.
   * If negative (default), check the environment. */
  ErrAbortMode err_abort_mode;

  /* Indicate whether warnings should be turned into errors.
   * If negative (default), check the environment. */
  ErrWarnMode err_warn_mode;

  /* Indicates whether error messages should include debugging info.
   * If negative (default), check the environment. */
  ErrDebugMode err_debug_mode;

  /* Whether to write errors messages color-coded. */
  ErrColorMode err_color_mode;

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
static Globals _globals = {"", err_default_stream, err_default_handler,
                           NULL, 0, 0};

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
  _globals.err_nameconv = NULL;
  _globals.err_stream_opened = 0;
  _globals.err_stream_atexit_called = 0;

  memset(&_tls, 0, sizeof(_tls));
  _tls.err_level = -1;
  _tls.err_abort_mode = -1;
  _tls.err_warn_mode = -1;
  _tls.err_debug_mode = -1;
  _tls.err_color_mode = -1;
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
    _tls.err_level = -1;
    _tls.err_abort_mode = -1;
    _tls.err_warn_mode = -1;
    _tls.err_debug_mode = -1;
    _tls.err_color_mode = -1;
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
static char *errlevel_names[] = {
  "Success",
  "Debug",
  "Info",
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
  const char *errlevel_name = err_getlevelname(errlevel);
  char *errmsg = tls->err_record->msg;
  size_t errsize = sizeof(tls->err_record->msg);
  FILE *stream = err_get_stream();
  ErrDebugMode debug_mode = err_get_debug_mode();
  ErrAbortMode abort_mode = err_get_abort_mode();
  ErrWarnMode warn_mode = err_get_warn_mode();
  ErrOverrideMode override = err_get_override_mode();
  int ignore_new_error = 0;
  ErrHandler handler = err_get_handler();
  ErrNameConv nameconv = err_get_nameconv();

  /* Skip low error levels */
  if (errlevel < err_get_level())
    return 0;

  /* Check warning mode */
  if (errlevel == errLevelWarn) {
    switch (warn_mode) {
    case errWarnNormal:
      break;
    case errWarnIgnore:
      return 0;
    case errWarnError:
      errlevel = errLevelError;
      errlevel_name = errlevel_names[errlevel];
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
      tls->err_record->pos = n;
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
      if (nameconv)
        n += snprintf(errmsg + n, errsize - n, "%s%s: ", nameconv(eval),
                      (errlevel_name && *errlevel_name) ?
                      errlevel_name : "");
      else
        n += snprintf(errmsg + n, errsize - n, "%s %d: ",
                      (errlevel_name && *errlevel_name) ?
                      errlevel_name : "Errval", eval);
    } else if (errlevel_name && *errlevel_name) {
      n += snprintf(errmsg + n, errsize - n, "%s: ", errlevel_name);
    }
    if (msg && *msg)
      n += vsnprintf(errmsg + n, errsize - n, msg, ap);
    if (errnum)
      n += snprintf(errmsg + n, errsize - n, ": %s", strerror(errnum));
    if (n >= (int)errsize && stream)
      fprintf(stream,
              "Warning: error %d truncated due to full message buffer: %s",
              eval, errmsg);
  }

  /* If this error occured after the try clause in an ErrTry handler,
     we mark this error to be reraised after leaving the handler. */
  if (errlevel >= errLevelError && tls->err_record->state)
    tls->err_record->reraise = eval;

  /* Call error handler */
  if (!tls->err_record->prev) {
    /* If we are not within a ErrTry...ErrEnd clause */

    /* ...call the error handler */
    if (handler) handler(tls->err_record);

    /* ...check err_abort_mode */
    if (errlevel >= errLevelError) {
      if (abort_mode == errAbortExit) {
        if (!handler) err_default_handler(tls->err_record);
        exit(eval);
      } else if (abort_mode >= errAbortAbort) {
        if (!handler) err_default_handler(tls->err_record);
        abort();
      }
    }

    /* ...make sure that fatal errors always exit */
    if (errlevel >= errLevelFatal) {
      if (!handler) err_default_handler(tls->err_record);
      exit(eval);
    }

  } else if (errlevel == errLevelWarn) {
    /* Handle warnings within a ErrTry...ErrEnd clause */

    if (handler) handler(tls->err_record);
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
int err_getlevel(void)
{
  ThreadLocals *tls = get_tls();
  return tls->err_record->level;
}

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
  tls->err_record->pos = 0;
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

ErrLevel err_set_level(int level)
{
  ThreadLocals *tls = get_tls();
  int prev = tls->err_level;
  tls->err_level = level;
  return prev;
}

ErrLevel err_get_level(void)
{
  ThreadLocals *tls = get_tls();
  int level = tls->err_level;
  if (level < 0) {
    char *s = getenv("ERR_LEVEL");
    if (!s || !s[0]) {
      level = 0;
    } else if (isdigit(s[0])) {
      level = atoi(s);
      if (level < 0) level = 0;
      if (level > errLevelFatal) level = errLevelFatal;
    } else {
      size_t i;
      level = 0;
      for (i=0; i<sizeof(errlevel_names)/sizeof(char *); i++)
        if (strcasecmp(s, errlevel_names[i]) == 0) {
          level = i;
          break;
        }
    }
    tls->err_level = level;  // cache level for next time
  }
  return (level == 0) ? errLevelWarn : level;
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

ErrColorMode err_set_color_mode(ErrColorMode mode)
{
  ThreadLocals *tls = get_tls();
  ErrColorMode prev = tls->err_color_mode;
  tls->err_color_mode = mode;
  return prev;
}

int err_get_color_coded()
{
  ThreadLocals *tls = get_tls();
  if (tls->err_color_mode < 0) {
    char *mode = getenv("ERR_COLOR");
    tls->err_color_mode =
      (!mode || !*mode)                                      ? errColorAuto :
      (strcmp(mode, "never") == 0 || strcmp(mode, "0") == 0) ? errColorNever :
      (strcmp(mode, "always") == 0 || strcmp(mode,"1") == 0) ? errColorAlways :
      errColorAuto;
  }

  switch (tls->err_color_mode) {
  case errColorAuto:
    {
      int terminal = 0;
#if defined(HAVE_UNISTD_H) && defined(HAVE_ISATTY)
      int fno = (tls->globals->err_stream) ?
        fileno(tls->globals->err_stream) : -1;
      if (fno >= 0) terminal = (isatty(fno) == 1);
#elif defined(HAVE__FILENO) && defined(HAVE__ISATTY)
      int fno = (tls->globals->err_stream) ?
        _fileno(tls->globals->err_stream) : -1;
      if (fno >= 0) terminal = _isatty(fno);
#endif
      return (terminal) ? 1 : 0;
    }
  case errColorAlways:
    return 1;
  default:
    return 0;
  }
}

/* Default error handler. */
void err_default_handler(const ErrRecord *record)
{
  FILE *stream = err_get_stream();
  const char *msg = record->msg + record->pos;
  char *errmark = (record->pos) ? "" : "** ";
  if (record->pos >= ERR_MSGSIZE) return;
  if (record->pos) {
    int m = strspn(msg, "\n");
    int n = strlen(err_append_sep) - m;
    fprintf(stream, "%.*s", n, msg+m);
    msg += m+n;
  }
  if (stream && err_get_color_coded()) {
    /* Print error message in colour. */
    int n;
    ThreadLocals *tls = get_tls();
    Globals *g = tls->globals;
    ErrDebugMode debug_mode = err_get_debug_mode();
    if (g->err_prefix && *g->err_prefix) {
      n = strlen(g->err_prefix) + 2;
      if (!record->pos) fprintf(stream, "\033[02;31m%.*s", n, msg);
      msg += n;
    }
    if (debug_mode >= 1) {
      n = strcspn(msg, ":") + 1;
      n += (msg[0] == '(') ? 1 : strcspn(msg+n, ":") + 2;
      fprintf(stream, "\033[00;34m%.*s", n, msg);
      msg += n;
    }
    if (debug_mode >= 2) {
      n = strcspn(msg, ":") + 2;
      fprintf(stream, "\033[02;32m%.*s", n, msg);
      msg += n;
    }
    n = strcspn(msg, ": ");
    fprintf(stream, "\033[00;31m%.*s\033[02;35m%s\033[0m\n", n, msg, msg+n);
  } else if (stream) {
    /* Print error message with error marker prepended. */
    fprintf(stream, "%s%s\n", errmark, msg);
  }
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
  return g->err_handler;
}

ErrNameConv err_set_nameconv(ErrNameConv nameconv)
{
  ThreadLocals *tls = get_tls();
  Globals *g = tls->globals;
  ErrNameConv prev = g->err_nameconv;
  g->err_nameconv = nameconv;
  return prev;
}

ErrNameConv err_get_nameconv(void)
{
  ThreadLocals *tls = get_tls();
  Globals *g = tls->globals;
  return g->err_nameconv;
}

const char *err_getname(int eval)
{
  ThreadLocals *tls = get_tls();
  Globals *g = tls->globals;
  if (g->err_nameconv) return g->err_nameconv(eval);
  return NULL;
}

const char *err_getlevelname(int errlevel)
{
  int n = sizeof(errlevel_names) / sizeof(char *);
  if (0 <= errlevel && errlevel < n) return errlevel_names[errlevel];
  return NULL;
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
      if (handler) handler(tls->err_record);
    }

    if ((abort_mode && record->level >= errLevelError) ||
        record->level >= errLevelException) {
      if (abort_mode == errAbortAbort) abort();
      exit(eval);
    }
  }
}
