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
 * environment variable ERR_FAIL_MODE is set to "exit").
 *
 * ## Used environment variables
 *
 *   * ERR_STREAM
 *         Error stream to write messages to.
 *           - if not set         : write to stderr
 *           - if set, but empty  : do not write anything
 *           - if set to "stderr" : write to stderr
 *           - if set to "stdout" : write to stdout
 *           - otherwise          : open the given file and append to it
 *   * ERR_FAIL_MODE
 *         Indicate wheter the error functions should return normally,
 *         exit or about.
 *           - if not set or empty     : return normally
 *           - if set to "exit"        : exit
 *           - if set to "abort"       : abort
 *           - if set to "0"           : return normally
 *           - if set to "1"           : exit
 *           - if set to "2" or larger : abort
 *           - otherwise               : return normally
 *   * ERR_DEBUG_MODE
 *         Indicate wheter debugging information (source file, line number
 *         and function name) should be included in the error message.
 *           - if not set or empty     : no debugging info
 *           - if set to "0"           : no debugging info
 *           - if set to "1"           : print file and line number
 *           - if set to "2" or larger : print file, line number and function
 *           - otherwise               : no debugging info
 */

/** @cond private */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

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

/**
 * @brief Reports a (system) error and returns `eval`.
 *
 * @name Error functions
 *
 * The error functions below takes the following arguments:
 * @param eval    Return value.
 * @param msg     Error message.  This is a printf()-like format string.
 *                If NULL, the message from last error will be reused.
 *
 * The fatal() and err() functions writes an error message to the standard
 * error, appending the system error message to it corresponding to the
 * current value of `errno`.
 *
 * The fatalx() and errx() functions are similar, but do not append the
 * system error message.
 *
 * The behaviour of these function can be altered using the associated
 * function described below or with environment variables.
 * @{
 */

#if !defined HAVE___VA_OPT__ || !defined HAVE_EXT__VA_ARGS__
/**
 * @brief Reports fatal error and exit the program with error code `eval`.
 */
int fatal(int eval, const char *msg, ...)
  __attribute__ ((__noreturn__, __format__ (__printf__, 2, 3)));

/**
 * @brief Reports fatal error (excluding output from strerror()) and exit
 * the program with error code `eval`.
 */
int fatalx(int eval, const char *msg, ...)
  __attribute__ ((__noreturn__, __format__ (__printf__, 2, 3)));

/**
 * @brief Reports error and returns `eval`.
 */
int err(int eval, const char *msg, ...)
  __attribute__ ((__format__ (__printf__, 2, 3)));

/**
 * @brief Reports error (excluding output from strerror()) and returns
 * `eval`.
 */
int errx(int eval, const char *msg, ...)
  __attribute__ ((__format__ (__printf__, 2, 3)));

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
void verr(int eval, const char *msg, va_list ap)
  __attribute__ ((__format__ (__printf__, 2, 0)));
void verrx(int eval, const char *msg, va_list ap)
  __attribute__ ((__format__ (__printf__, 2, 0)));
/** @} */

#else /* !defined HAVE___VA_OPT__ || !defined HAVE_EXT__VA_ARGS__ */
/** @cond private */
# if defined HAVE___VA_OPT__

#define fatal(eval, msg, ...) \
  exit(_err_format("Fatal", eval, errno, ERR_FILEPOS, _err_func, \
		   msg __VA_OPT__(,) __VA_ARGS__))
#define fatalx(eval, msg, ...) \
  exit(_err_format("Fatal", eval, 0, ERR_FILEPOS, _err_func, \
		   msg __VA_OPT__(,) __VA_ARGS__))
#define err(eval, msg, ...) \
  _err_format("Error", eval, errno, ERR_FILEPOS, _err_func, \
	      msg __VA_OPT__(,) __VA_ARGS__)
#define errx(eval, msg, ...) \
  _err_format("Error", eval, 0, ERR_FILEPOS, _err_func, \
	      msg __VA_OPT__(,) __VA_ARGS__)

#elif defined HAVE_EXT__VA_ARGS__

#define fatal(eval, msg, ...) \
  exit(_err_format("Fatal", eval, errno, ERR_FILEPOS, _err_func, \
		   msg, ##__VA_ARGS__))
#define fatalx(eval, msg, ...) \
  exit(_err_format("Fatal", eval, 0, ERR_FILEPOS, _err_func, \
		   msg, ##__VA_ARGS__))
#define err(eval, msg, ...) \
  _err_format("Error", eval, errno, ERR_FILEPOS, _err_func, \
	      msg, ##__VA_ARGS__)
#define errx(eval, msg, ...) \
  _err_format("Error", eval, 0, ERR_FILEPOS, _err_func, \
	      msg, ##__VA_ARGS__)

#else
# error "should never be reached..."
#endif

#define vfatal(eval, msg, ap) \
  exit(_err_vformat("Fatal", eval, errno, ERR_FILEPOS, msg, _err_func, ap))
#define vfatalx(eval, msg, ap) \
  exit(_err_vformat("Fatal", eval, 0, ERR_FILEPOS, msg, _err_func, ap))
#define verr(eval, msg, ap) \
  _err_vformat("Error", eval, errno, ERR_FILEPOS, msg, _err_func, ap)
#define verrx(eval, msg, ap) \
  _err_vformat("Error", eval, 0, ERR_FILEPOS, msg, _err_func, ap)

/** @endcond */
#endif /* !defined HAVE___VA_OPT__ || !defined HAVE_EXT__VA_ARGS__ */


/**
 * @name Associated functions
 *
 * @{
 */

/**
 * @brief Returns the error code of the last error.
 */
int err_getcode();

/**
 * @brief Returns the error message of the last error.
 */
char *err_getmsg();

/**
 * @brief Clear the last error (setting code to zero).
 */
void err_clear();

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
 *  If NULL, no output will be written and abort() and exit() will not
 *  be called for positive `mode`.  The default is stderr.
 *
 *  By default, the error stream can also be set with the ERR_STREAM
 *  environment variable.
 *
 *  Returns the current error stream.
 */
FILE *err_set_stream(FILE *stream);

/**
 * @brief Set wheter the error functions should return normally, exit or about.
 *   - mode >= 2: abort
 *   - mode == 1: exit (with error code)
 *   - mode == 0: normal return
 *   - mode < 0:  check ERR_FAIL_MODE environment variable (default)
 *
 * Returns the current fail mode.
 */
int err_set_fail_mode(int mode);

/**
 * @brief Sets whether error messages should include debugging info.
 *   - mode >= 2: include file and line number and function
 *   - mode == 1: include file and line number
 *   - mode == 0: no debugging info
 *   - mode < 0:  check ERR_DEBUG_MODE environment variable (default)
 *
 * Returns the current debugging mode.
 *
 * @note Debugging info requeres that the compiler supports `__VA_OPT__`
 * or `##__VA_ARGS__`.
 */
int err_set_debug_mode(int mode);

/** @} */


/**
 * @name Try/Catch block
 * Try/Catch blocks allows to selectively handel errors based on their error
 * code.  They are of the form:
 *
 *     ErrTry:
 *       statements...;
 *     ErrCatch code:
 *       error_handling...;
 *     ErrOther:
 *       fallback_error_handling...;
 *     ErrElse:
 *       code_on_no_errors...;
 *     ErrEnd;
 *
 * @{
 */

/** @cond priate */

/* Data record in the linked list of */
typedef struct _ErrRecord {
  int code;                /* error code */
  int handled;             /* whether the error has been handled */
  char msg[256];           /* error message */
  const char *func;        /* function in which the error was called */
  const char *file;        /* file nad line in which the error was called */
  struct _ErrRecord *prev; /* pointer to previous record in the stack-allocated
			      list of records */
} ErrRecord;

/* Declarations */
void _err_link_record(ErrRecord *errrecord);
void _err_unlink_record(ErrRecord *errrecord);

/** @endcond */

/**
 * @brief Starts a ErrTry/ErrCatch block.
*/
#define ErrTry					\
  do {						\
    ErrRecord errrecord;			\
    _err_link_record(&errrecord);		\
    switch (0) {				\
    case 0

/**
 * @brief Catches an error with the given code.
 */
#define ErrCatch				\
    }						\
    errrecord.handled++;			\
    switch (errrecord.code) {			\
    default:					\
      errrecord.handled--;			\
    case

/**
 * @brief Handles uncaught errors.
 */
#define ErrOther				\
    }						\
    switch (errrecord.handled++) {		\
    case 0

/**
 * @brief Code to run on no errors.
 */
#define ErrElse					\
    }						\
    switch (errrecord.code) {			\
    case 0

/**
 * @brief Ends an ErrTry/ErrCatch block.
 */
#define ErrEnd					\
    }						\
    _err_unlink_record(&errrecord);		\
  } while (0)



/** @} */

#endif /* ERR_H */
