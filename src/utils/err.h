/* err.h -- simple error reporting library
 *
 * Copyright (C) 2010-2020 SINTEF
 *
 * Distributed under terms of the MIT license.
 */
#ifndef ERR_H
#define ERR_H

/**
 * @file
 * @brief Simple error reporting library
 *
 * This library is modelled after the BSD error reporting family of
 * functions with a few extensions.  These functions mostly follows
 * the documented behaviour in the unix err(3) man page, with the major
 * exception that err() and errx() does not call exit() (unless the
 * environment variable ERR_ABORT is set to "exit").
 *
 * ## Used environment variables
 *
 *   * `ERR_STREAM`: Error stream to write messages to.
 *       - empty             : do not write anything
 *       - "stderr" | unset  : write to stderr
 *       - "stdout"          : write to stdout
 *       - otherwise         : open the given file and append to it
 *
 *   * `ERR_ABORT`: Whether errors should should return normally,
 *     exit or about.
 *       - "0" | "normal" | unset : return normally
 *       - "1" | "exit"           : exit
 *       - "2" | "abort" | empty  : abort
 *       - otherwise              : return normally
 *
 *   * `ERR_WARN`: Whether warnings should be ignored or turned into errors.
 *       - "0" | "normal" | unset : report normally
 *       - "1" | "ignore"         : ignore
 *       - "2" | "error"          : turn into error
 *       - otherwise              : report normally
 *
 *   * `ERR_DEBUG`: Wheter debugging information (source file, line number
 *     and function name) should be included in the error message.
 *       - "0" | unset | empty  : no debugging info
 *       - "1" | "debug"        : print file and line number
 *       - "2" | "full"         : print file, line number and function
 *       - otherwise            : no debugging info
 *
 *   * `ERR_OVERRIDE`: How to handle error messages when there already is a
 *      message in the error message buffer.  Note that these options will
 *      only affect the error message, not the error value.
 *       - unset | empty       : append new error message to the old one
 *       - "0" | "append"      : append new error message
 *       - "1" | "warn-old"    : overwrite old error message and write warning
 *       - "2" | "warn-new"    : ignore new error message and write warning
 *       - "3" | "old"         : overwrite old error message
 *       - "4" | "ignore-new"  : ignore new error message
 *       - otherwise           : append new error message to the old one
 */

/** @cond private */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <setjmp.h>

/* FIXME - temporary workaround for MSVC */
#ifdef _MSC_VER
# ifdef HAVE___VA_ARGS__
#  undef HAVE___VA_ARGS__
# endif
#endif

/* Remove __attribute__ when we are not compiling with gcc */
#ifndef __GNUC__
# define __attribute__(x)
#endif

/* Stringnification */
#define _ERR_STRINGNIFY(x) #x
#define ERR_STRINGNIFY(x) _ERR_STRINGNIFY(x)

/* Concatenated source file and line position */
#define ERR_FILEPOS __FILE__ ":" ERR_STRINGNIFY(__LINE__)

#if defined HAVE___FUNC__
# define _err_func __func__
#elif defined HAVE___FUNCTION__
# define _err_func __FUNCTION__
#else
# define _err_func
#endif

/** Error levels */
typedef enum {
  errLevelSuccess,         /*!< success */
  errLevelWarn,            /*!< warning */
  errLevelError,           /*!< error */
  errLevelException,       /*!< exception (never returns) */
  errLevelFatal            /*!< fatal error (never returns) */
} ErrLevel;

/** Error abort mode */
typedef enum {
  errAbortEnv=-1,          /*!< set from ERR_ABORT environment variable */
  errAbortNormal=0,        /*!< return normally */
  errAbortExit,            /*!< call exit() with error value */
  errAbortAbort            /*!< call abort() */
} ErrAbortMode;

/** Error warn mode */
typedef enum {
  errWarnEnv=-1,           /*!< set from ERR_WARN environment variable */
  errWarnNormal=0,         /*!< report warnings as normal */
  errWarnIgnore,           /*!< ignore warnings */
  errWarnError             /*!< turn warnings into errors */
} ErrWarnMode;

/** Error debug mode */
typedef enum {
  errDebugEnv=-1,          /*!< set from ERR_DEBUG environment variable */
  errDebugOff=0,           /*!< no debugging info in error messages */
  errDebugSimple,          /*!< add file and line number to error messages */
  errDebugFull             /*!< add also function name to error messages */
} ErrDebugMode;

/** Error override mode */
typedef enum {
  errOverrideEnv=-1,       /*!< set from ERR_OVERRIDE environment variable */
  errOverrideAppend=0,     /*!< append new error to old error msg */
  errOverrideWarnOld,      /*!< overwrite old error message and warn */
  errOverrideWarnNew,      /*!< ignore new error message and warn */
  errOverrideOld,          /*!< overwrite old error message */
  errOverrideIgnoreNew     /*!< ignore new error message */
} ErrOverrideMode;


/* The functions that actually handles the errors */
int _err_format(ErrLevel errlevel, int eval, int errnum, const char *file,
		const char *func, const char *msg, ...)
  __attribute__ ((__format__ (__printf__, 6, 7)));

int _err_vformat(ErrLevel errlevel, int eval, int errnum, const char *file,
		 const char *func, const char *msg, va_list ap)
  __attribute__ ((__format__ (__printf__, 6, 0)));

/** @endcond */

/** @brief Buffer size for error messages. */
#ifndef ERR_MSGSIZE
#define ERR_MSGSIZE 4096
#endif

/** @brief Default error stream (checks the ERR_STREAM environment variable) */
#define err_default_stream ((FILE *)1)

/** @brief Default error handle */
#define err_default_handler ((ErrHandler)1)


/**
 * @brief Reports a (system) error and returns `eval`.
 *
 * @name Error functions
 *
 * The error functions below takes (some of) the following arguments:
 * @param eval    Error value.
 * @param msg     Error message.  This is a printf()-like format string.
 *                If NULL, the message from last error will be reused.
 *
 * The fatal() function creates an error record, passes it to the
 * current error handle for reporting and calls exit() with `eval` as
 * exit with `eval` as exit status.  The default handle simply prints
 * the error message to standard error.  The system error string
 * corresponding to the current value of `errno` is appended to the
 * error message.
 *
 * The err() function is similar to fatal(), but returns `eval` instead of
 * calling exit().
 *
 * The warn() function is similar to err(), but returns always zero
 * (indicating no error).
 *
 * The fatalx(), errx() and warnx() functions are similar to fatal(),
 * err() and warn(), respectively, but do not append the system error
 * message.
 *
 * The behaviour of these function can be altered using the associated
 * function described below or with environment variables.
 * @{
 */

#ifndef HAVE___VA_ARGS__

/** @cond private */
/* Rename to avoid intermixing with BSD error functions */
#define fatal  err_fatal
#define fatalx err_fatalx
#define err    err_err
#define errx   err_errx
#define warn   err_warn
#define warnx  err_warnx
/** @} */

/**
 * @brief Reports fatal error and exit the program with error value `eval`.
 */
void fatal(int eval, const char *msg, ...)
  __attribute__ ((__noreturn__, __format__ (__printf__, 2, 3)));

/**
 * @brief Reports fatal error (excluding output from strerror()) and exit
 * the program with error value `eval`.
 */
void fatalx(int eval, const char *msg, ...)
  __attribute__ ((__noreturn__, __format__ (__printf__, 2, 3)));

/**
 * @brief Reports an error and returns `eval`.
 */
int err(int eval, const char *msg, ...)
  __attribute__ ((__format__ (__printf__, 2, 3)));

/**
 * @brief Reports an error (excluding output from strerror()) and returns
 * `eval`.
 */
int errx(int eval, const char *msg, ...)
  __attribute__ ((__format__ (__printf__, 2, 3)));

/**
 * @brief Reports a warning and returns 0.
 */
int warn(const char *msg, ...)
  __attribute__ ((__format__ (__printf__, 1, 2)));

/**
 * @brief Reports a warning (excluding output from strerror()) and returns 0.
 */
int warnx(const char *msg, ...)
  __attribute__ ((__format__ (__printf__, 1, 2)));

/**
 * @brief Generic error function with explicit `errlevel` and `errnum`.
 * Returns `eval`.
 */
int err_generic(int errlevel, int eval, int errnum, const char *msg, ...)
  __attribute__ ((__format__ (__printf__, 4, 5)));

/** @} */

/**
 * @name Variants using `va_list`
 * These function are equivalent to their counterpart without the
 * prefix "v", except that they take a `va_list` instead of a variable
 * number of arguments.
 *
 * @{
 */
void vfatal(int eval, const char *msg, va_list ap)
  __attribute__ ((__noreturn__, __format__ (__printf__, 2, 0)));
void vfatalx(int eval, const char *msg, va_list ap)
  __attribute__ ((__noreturn__, __format__ (__printf__, 2, 0)));
int verr(int eval, const char *msg, va_list ap)
  __attribute__ ((__format__ (__printf__, 2, 0)));
int verrx(int eval, const char *msg, va_list ap)
  __attribute__ ((__format__ (__printf__, 2, 0)));
int vwarn(const char *msg, va_list ap)
  __attribute__ ((__format__ (__printf__, 1, 0)));
int vwarnx(const char *msg, va_list ap)
  __attribute__ ((__format__ (__printf__, 1, 0)));
int verr_generic(int errlevel, int eval, int errnum, const char *msg, va_list ap)
  __attribute__ ((__format__ (__printf__, 4, 0)));
/** @} */

#else /* HAVE___VA_ARGS__ */
/** @cond private */

/* Note that `...` include the `msg` macro argument to ensure that `...`
 * always corresponds to at least one argument as required by ISO C99. */
#define fatal(eval, ...)                                                \
  exit(_err_format(errLevelFatal, eval, errno, ERR_FILEPOS, _err_func,  \
                   __VA_ARGS__))
#define fatalx(eval, ...)                                               \
  exit(_err_format(errLevelFatal, eval, 0, ERR_FILEPOS, _err_func,      \
                   __VA_ARGS__))
#define err(eval, ...)                                                  \
  _err_format(errLevelError, eval, errno, ERR_FILEPOS, _err_func,         \
              __VA_ARGS__)
#define errx(eval, ...)                                                 \
  _err_format(errLevelError, eval, 0, ERR_FILEPOS, _err_func, __VA_ARGS__)
#define warn(...)                                                       \
  _err_format(errLevelWarn, 0, errno, ERR_FILEPOS, _err_func, __VA_ARGS__)
#define warnx(...)                                                      \
  _err_format(errLevelWarn, 0, 0, ERR_FILEPOS, _err_func, __VA_ARGS__)
#define err_generic(errlevel, eval, errnum, ...)                        \
  _err_format(errlevel, eval, errnum, ERR_FILEPOS, _err_func, __VA_ARGS__)

#define vfatal(eval, msg, ap) \
  exit(_err_vformat(errLevelFatal, eval, errno, ERR_FILEPOS, _err_func, msg, ap))
#define vfatalx(eval, msg, ap) \
  exit(_err_vformat(errLevelFatal, eval, 0, ERR_FILEPOS, _err_func, msg, ap))
#define verr(eval, msg, ap) \
  _err_vformat(errLevelError, eval, errno, ERR_FILEPOS, _err_func, msg, ap)
#define verrx(eval, msg, ap) \
  _err_vformat(errLevelError, eval, 0, ERR_FILEPOS, _err_func, msg, ap)
#define vwarn(msg, ap) \
  _err_vformat(errLevelWarn, 0, errno, ERR_FILEPOS, _err_func, msg, ap)
#define vwarnx(msg, ap) \
  _err_vformat(errLevelWarn, 0, 0, ERR_FILEPOS, _err_func, msg, ap)
#define verr_generic(errlevel, eval, errnum, msg, ap) \
  _err_format(errlevel, eval, errnum, ERR_FILEPOS, _err_func, msg, ap)

/** @endcond */
#endif /* HAVE___VA_ARGS__ */


/**
 * @name Associated functions
 * @{
 */



/**
 * Return a pointer to (thread local) state for this module
 */
void *err_get_state(void);

/**
 * Sets state from state returned by err_get_state().
 * If `state` is NULL, the state is initialised to default values.
 */
void err_set_state(void *state);

/**
 * @brief Returns the error value of the last error.
 */
int err_geteval(void);

/**
 * @brief If the current error value is non-zero, set it to `eval`.
 * Nothing is done if there is no current error.
 *
 * Returns the updated error value.
 */
int err_update_eval(int eval);

/**
 * @brief Returns a pointer the error message of the last error.
 *
 * Note that the memory pointed to will be overridded by the next error.
 * Do not use the returned pointer as input to a new error message -
 * this may result in an overflow of the error buffer (depending of the
 * implementation of the underlying snprintf() function).
 */
const char *err_getmsg(void);

/**
 * @brief Clear the last error (setting error value to zero).
 */
void err_clear(void);

/**
 * @brief Set prefix to prepend to all errors in this application.
 * Typically this is the program name.
 *
 *  Returns the current prefix.
 */
const char *err_set_prefix(const char *prefix);

/**
 * @brief Set stream that error messages are printed to.
 *
 * If `stream` is `err_default_stream` (default) the error
 * stream determined by the ERR_STREAM environment variable.
 *
 * If `stream` is NULL, no output will be written.
 *
 * Returns the previous error stream.
 */
FILE *err_set_stream(FILE *stream);

/**
 * @brief Returns the current error stream.
 */
FILE *err_get_stream(void);

/**
 * @brief Set wheter the error functions should return normally, exit or about.
 * Interpretation of `mode` argument:
 *   - mode >= 2:  abort
 *   - mode == 1:  exit (with error value)
 *   - mode == 0:  normal return
 *   - mode < 0:   check ERR_ABORT environment variable (default)
 *
 * Returns the previous abort mode.
 */
ErrAbortMode err_set_abort_mode(int mode);

/**
 * @brief Returns the current abort mode.
 */
ErrAbortMode err_get_abort_mode(void);

/**
 * @brief Set whether warnings should be turned to errors.
 * Interpretation of `mode` argument:
 *   - err_warn_mode >= 1: turn warnings into errors
 *   - err_warn_mode == 0: default
 *   - err_warn_mode < 0:  check ERR_WARN environment variable (default)
 *                         if it is set, warnings are turned into errors
 */
ErrWarnMode err_set_warn_mode(int mode);

/**
 * @brief Returns the current warning mode.
 */
ErrWarnMode err_get_warn_mode(void);

/**
 * @brief Sets whether error messages should include debugging info.
 * Interpretation of `mode` argument:
 *   - mode >= 2:  include file and line number and function
 *   - mode == 1:  include file and line number
 *   - mode == 0:  no debugging info
 *   - mode < 0:   check ERR_DEBUG environment variable (default)
 *
 * Returns the current debugging mode.
 *
 * @note Debugging info requeres that the compiler supports `__VA_ARGS__`.
 */
ErrDebugMode err_set_debug_mode(int mode);

/**
 * @brief Returns the current debugging mode.
 */
ErrDebugMode err_get_debug_mode(void);

/**
 * @brief Sets how to handle overridden error in a Try/Catch block.
 * Interpretation of `mode` argument:
 *   - mode >= 4:  ignore new error
 *   - mode == 3:  ignore old error
 *   - mode == 2:  ignore new error and write warning to error stream
 *   - mode == 1:  ignore old error and write warning to error stream
 *   - mode == 0:  append to previous error
 *   - mode < 0:   check ERR_OVERRIDE environment variable
 *
 * Returns the current debugging mode.
 */
ErrOverrideMode err_set_override_mode(int mode);

/**
 * @brief Returns the current override mode.
 */
ErrOverrideMode err_get_override_mode(void);

/* Where in an ErrTry.. ErrEnd clause we are */
typedef enum {
  errTryNormal,
  errTryCatch,
  errTryElse,
  errTryFinally
} ErrTryState;

/**
 * @brief Error record, describing the last error.
 */
typedef struct ErrRecord {
  ErrLevel level;         /*!< @brief Error level. */
  int eval;               /*!< @brief Error value. */
  int errnum;             /*!< @brief System error number. */
  char msg[ERR_MSGSIZE];  /*!< @brief Error message. */
  int handled;            /*!< @brief Whether the error has been handled. */
  int reraise;            /*!< @brief Error value to reraise. */
  ErrTryState state;      /*!< @brief Where we are in ErrTry.. ErrEnd. */
  struct ErrRecord *prev; /*!< @brief Pointer to previous record in the
			       stack-allocated list of error records. */
  jmp_buf env;            /*!< @brief Buffer for longjmp(). */
} ErrRecord;

/**
 * @brief Prototype for error handler.
 * The `eval` and `msg` fields of the error record are probably the most
 * relevant.  In addition it might get the current error stream with
 * err_get_stream().
 */
typedef void (*ErrHandler)(const ErrRecord *record);

/**
 * @brief Sets a new error handler.
 *
 * Returns the previous error handler.
 */
ErrHandler err_set_handler(ErrHandler handler);

/**
 * @brief Sets new error handler.
 *
 * If `handler` is `err_default_handler` the default handler will be used,
 * which simply prints the error message to the curent error stream.
 *
 * If `handler` is NULL, no handler will be called.
 *
 * Returns the current error handler.
 */
ErrHandler err_set_handler(ErrHandler handler);

/**
 * @brief Returns the current error handler.
 */
ErrHandler err_get_handler(void);

/** @} */


/**
 * @name ErrTry block
 * ErrTry blocks allows to selectively handel errors based on their
 * error value.  They are of the form:
 *
 *     ErrTry:
 *       statements...;
 *     ErrCatch(eval1):
 *       error_handling...;
 *       break;  // do not fall-through
 *     ErrCatch(eval2):
 *       error_handling...;
 *       break;  // not strictly needed since next clause is not an ErrCatch
 *     ErrOther:
 *       fallback_error_handling...;
 *     ErrElse:
 *       code_on_no_errors...;
 *     ErrFinally:
 *       always_executed...;
 *     ErrEnd;
 *
 * Except for `ErrTry` and `ErrEnd`, all clauses are optional.  But if
 * they are given, they should occour in the above order.
 *
 * The `ErrTry` clause is evaluated as normal, but errors that occurs
 * will only be reported if they are not caught by the `ErrCatch` or
 * `ErrOther` clauses.
 *
 * Multiple `ErrCatch` clauses may be provided, to handle different errors.
 * They should normally end with `break`.  If they don't, the evaluation
 * will fall-through to the next `ErrCatch` clause. For example, in:
 *
 *     ErrCatch(1):
 *     ErrCatch(2):
 *       statements...;
 *       break;
 *
 * will `statements...` be called for errors with value 1 and 2.
 *
 * The `ErrOther` clause will handle any error that has not been catched
 * by an earlier `ErrCatch` clause.
 *
 * The `ErrElse` clause will, if it is provided, be evaluated, if no errors
 * occured during evaluation of the `ErrTry` clause.
 *
 * The `ErrFinally` clause will, if it is provided, always be evaluated.
 * It is typically used for cleanup-code.
 *
 * It is possible to use the `break` statement to end the execution of
 * any clause.
 *
 * @note
 * Errors occuring in the `ErrTry` clause (including any function called
 * from within the clause) will first be reported when execution
 * leaves the ErrEnd clause if they are not caught.  This opens for
 * the possibility that an error might be missed if another error occurs
 * within the same clause.  How to handle this, is controlled by
 * err_set_override_mode() and the environment variable `ERR_OVERRIDE`.
 */

/** @{ */
#include <assert.h>

/** Adds link to new exception handler. Called internally by the
    ErrTry macro. Don't call this function directly. */
void _err_link_record(ErrRecord *errrecord);

/** Unlinks exception handler. Called internally by the ErrEnd
    macro. Don't call this function directly. */
void _err_unlink_record(ErrRecord *errrecord);

/** Returns pointer to current error record.  Called internally by
    the raise() macro.  Don't call this function directly. */
ErrRecord *_err_get_record();


/**
 * @brief Starts a ErrTry block.
*/
#define ErrTry                                       \
  do {                                               \
    ErrRecord _record;                               \
    int _fallthrough=0, _ft, _jmpeval, _last;        \
    (void)_fallthrough;                              \
    (void)_ft;                                       \
    (void)_jmpeval;                                  \
    (void)_last;                                     \
    _err_link_record(&_record);                      \
    _jmpeval = setjmp(_record.env);                  \
    _last = _jmpeval;                                \
    assert(_jmpeval == _record.eval);                \
    switch (_record.eval) {                          \
    case 0

/**
 * @brief Catches an error with the given value.
 */
#define ErrCatch(errval)                             \
      _fallthrough = _last;                          \
    }                                                \
    _ft = _fallthrough;                              \
    _fallthrough = 0;                                \
    _last = _record.eval == errval || _ft;           \
    _record.handled |= _last;                        \
    if (_last) _record.state = errTryCatch;          \
    switch (_last ? 1 : 0) {                         \
    case 1

/**
 * @brief Handles uncaught errors.
 */
#define ErrOther                                     \
    }                                                \
    _last = _record.eval && !_record.handled;        \
    _record.handled |= _last;                        \
    if (_last) _record.state = errTryCatch;          \
    switch (_last ? 1 : 0) {                         \
    case 1

/**
 * @brief Code to run on no errors.
 */
#define ErrElse			                     \
    }				                     \
   if (!_record.eval) _record.state = errTryElse;    \
   switch (_record.eval == 0 ? 1 : 0) {              \
    case 1

/**
 * @brief Final (cleanup) code to always run.
 */
#define ErrFinally                                   \
    }                                                \
    _record.state = errTryFinally;                   \
    switch (0) {                                     \
    case 0

/**
 * @brief Ends an ErrTry block.
 */
#define ErrEnd                                       \
    }                                                \
    _err_unlink_record(&_record);                    \
  } while (0)

/** @} */


/**
 * @name Macros for emulating exceptions
 *
 * @warning
 * Be careful when using raise() and raisex(), since they may skip clean-up
 * code and leave the program in an inconsistent state or leak resources.
 * @{
 */

/**
 * @brief Raises an exception
 * This transfers the execution to the nearest enclosing ErrTry block.
 * If no such block exists, fatal() is called.
 */
#define err_raise(eval, ...)                                    \
  do {                                                          \
    ErrRecord *record = _err_get_record();                      \
    if (record->prev) {                                         \
      err_generic(errLevelException, eval, errno, __VA_ARGS__); \
      longjmp(record->env, eval);                               \
    } else                                                      \
      fatal(eval, __VA_ARGS__);                                 \
  } while (0)

/**
 * @brief Like raise(), but does not append a system error message to the
 * exception message.
 */
#define err_raisex(eval, ...)                                   \
  do {                                                          \
    ErrRecord *record = _err_get_record();                      \
    if (record->prev) {                                         \
      err_generic(errLevelException, eval, 0, __VA_ARGS__);     \
      longjmp(record->env, eval);                               \
    } else                                                      \
      fatalx(eval, __VA_ARGS__);                                \
  } while (0)

/**
 * @brief Reraises current error or exception
 * This macro can only be called within `ErrCatch`, `ErrOther` and
 * `ErrFinally` clauses.
 */
#define err_reraise()                                           \
  do {                                                          \
    _record.reraise = _record.eval;                             \
  } while (0)




/** @} */


#endif /* ERR_H */
