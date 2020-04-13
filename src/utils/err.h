/* err.h -- simple error reporting library */
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
 *   * `ERR_DEBUG`: Wheter debugging information (source file, line number
 *     and function name) should be included in the error message.
 *       - "0" | unset | empty  : no debugging info
 *       - "1" | "debug"        : print file and line number
 *       - "2" | "full"         : print file, line number and function
 *       - otherwise            : no debugging info
 *
 *   * `ERR_OVERRIDE`: How to handle overridden errors in  ErrTry clauses.
 *       - unset | empty       : append new error message
 *       - "0" | "append"      : append new error message
 *       - "1" | "warn-old"    : ignore old error and write warning
 *       - "2" | "warn-new"    : ignore new error and write warning
 *       - "3" | "ignore-old"  : ignore old error
 *       - "4" | "ignore-new"  : ignore new error
 *       - otherwise           : append new error message
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

/* The functions that actually handles the errors */
int _err_format(const char *errname, int eval, int errnum, const char *file,
		const char *func, const char *msg, ...)
  __attribute__ ((__format__ (__printf__, 6, 7)));

int _err_vformat(const char *errname, int eval, int errnum, const char *file,
		 const char *func, const char *msg, va_list ap)
  __attribute__ ((__format__ (__printf__, 6, 0)));

/** @endcond */

/** @brief Buffer size for error messages. */
#ifndef ERR_MSGSIZE
#define ERR_MSGSIZE 1024
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
/** @} */

#else /* HAVE___VA_ARGS__ */
/** @cond private */

/* Note that `...` include the `msg` macro argument to ensure that `...`
 * always corresponds to at least one argument as required by ISO C99. */
#define fatal(eval, ...) \
  exit(_err_format("Fatal", eval, errno, ERR_FILEPOS, _err_func, __VA_ARGS__))
#define fatalx(eval, ...) \
  exit(_err_format("Fatal", eval, 0, ERR_FILEPOS, _err_func, __VA_ARGS__))
#define err(eval, ...) \
  _err_format("Error", eval, errno, ERR_FILEPOS, _err_func, __VA_ARGS__)
#define errx(eval, ...) \
  _err_format("Error", eval, 0, ERR_FILEPOS, _err_func, __VA_ARGS__)
#define warn(...) \
  _err_format("Warning", 0, errno, ERR_FILEPOS, _err_func, __VA_ARGS__)
#define warnx(...) \
  _err_format("Warning", 0, 0, ERR_FILEPOS, _err_func, __VA_ARGS__)

#define vfatal(eval, msg, ap) \
  exit(_err_vformat("Fatal", eval, errno, ERR_FILEPOS, _err_func, msg, ap))
#define vfatalx(eval, msg, ap) \
  exit(_err_vformat("Fatal", eval, 0, ERR_FILEPOS, _err_func, msg, ap))
#define verr(eval, msg, ap) \
  _err_vformat("Error", eval, errno, ERR_FILEPOS, _err_func, msg, ap)
#define verrx(eval, msg, ap) \
  _err_vformat("Error", eval, 0, ERR_FILEPOS, _err_func, msg, ap)
#define vwarn(msg, ap) \
  _err_vformat("Error", 0, errno, ERR_FILEPOS, _err_func, msg, ap)
#define vwarnx(msg, ap) \
  _err_vformat("Error", 0, 0, ERR_FILEPOS, _err_func, msg, ap)

/** @endcond */
#endif /* HAVE___VA_ARGS__ */


/**
 * @name Associated functions
 * @{
 */

/**
 * @brief Returns the error value of the last error.
 */
int err_geteval(void);

/**
 * @brief Returns the error message of the last error.
 */
char *err_getmsg(void);

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
int err_set_abort_mode(int mode);

/**
 * @brief Returns the current abort mode.
 */
int err_get_abort_mode(void);

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
int err_set_debug_mode(int mode);

/**
 * @brief Returns the current debugging mode.
 */
int err_get_debug_mode(void);

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
int err_set_override_mode(int mode);

/**
 * @brief Returns the current override mode.
 */
int err_get_override_mode(void);


/**
 * @brief Error record, describing the last error.
 */
typedef struct ErrRecord {
  int eval;               /*!< @brief Error value. */
  char msg[ERR_MSGSIZE];  /*!< @brief Error message. */
  int handled;            /*!< @brief Whether the error has been handled. */
  int exception;          /*!< @brief Whether error is an exception */
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
 *       break;
 *     ErrCatch(eval):
 *       error_handling...;
 *       break;
 *     ErrOther:
 *       fallback_error_handling...;
 *       break;
 *     ErrElse:
 *       code_on_no_errors...;
 *       break;
 *     ErrFinally:
 *       always_executed...;
 *       break;
 *     ErrEnd;
 *
 * Except for `ErrTry` and `ErrEnd`, all clauses are optional.  But if
 * they are given, they should occour in the above order.  Like
 * for `switch`, the statements should end with `break` to avoid
 * unintended fall through (only strictly required in the `ErrTry` and
 * `ErrCatch` clauses).
 *
 * The `ErrTry` clause is evaluated as normal, but errors that occurs
 * will only be reported if they are not caught by the `ErrCatch` or
 * `ErrOther` clauses.
 *
 * Multiple `ErrCatch` clauses may be provided, to handle different errors.
 * It is possible to handle more than one error value by fall-trhough. For
 * example in
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
 * @note
 * Errors occuring in the `ErrTry` clause (including any function called
 * from within the clause) will first be reported when execution
 * leaves the ErrEnd clause if they are not caught.  This opens for
 * the possibility that an error might be missed if another error occurs
 * within the same clause.  How to handle this, is controlled by
 * err_set_override_mode() and the environment variable `ERR_OVERRIDE`.
 */

/** @{ */


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
    int _jmpstat;                                    \
    _err_link_record(&_record);                      \
    switch ((_jmpstat = setjmp(_record.env))) {      \
    case 0

/**
 * @brief Catches an error with the given value.
 */
#define ErrCatch(eval)                       \
      /* if (!_record.handled) break; */     \
    case eval:                               \
      _record.handled = 1;                   \
      goto _errlabel ## eval;                \
     _errlabel ## eval

/**
 * @brief Handles uncaught errors.
 */
#define ErrOther                             \
      /* if (!_record.handled) break; */     \
    default:                                 \
      _record.handled = 1;                   \
      goto _errlabel_default;                \
     _errlabel_default

/**
 * @brief Code to run on no errors.
 */
#define ErrElse			\
    }				\
    switch (_record.eval) {	\
    case 0

/**
 * @brief Final (cleanup) code to always run.
 */
#define ErrFinally                           \
  }                                          \
    switch (_jmpstat == 0 && _record.eval) { \
    case 0

/**
 * @brief Ends an ErrTry block.
 */
#define ErrEnd                               \
    }                                        \
    if (!_jmpstat && _record.eval)           \
      longjmp(_record.env, _record.eval);    \
    _err_unlink_record(&_record);            \
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
#define err_raise(eval, ...)                              \
  do {                                                    \
    ErrRecord *record = _err_get_record();                \
    record->exception = 1;                                \
    if (record->prev)                                     \
      longjmp(record->env, err(eval, __VA_ARGS__));       \
    else                                                  \
      fatal(eval, __VA_ARGS__);                           \
  } while (0)

/**
 * @brief Like raise(), but does not append a system error message to the
 * exception message.
 */
#define err_raisex(eval, ...)                             \
  do {                                                    \
    ErrRecord *record = _err_get_record();                \
    record->exception = 1;                                \
    if (record->prev)                                     \
      longjmp(record->env, errx(eval, __VA_ARGS__));      \
    else                                                  \
      fatalx(eval, __VA_ARGS__);                          \
  } while (0)

/** @} */


#endif /* ERR_H */
