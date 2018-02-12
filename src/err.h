#ifndef ERR_H
#define ERR_H

/* err.h -- simple error reporting
 */



/**
 * @file
 *
 * @ingroup err
 *
 * @brief Simple error reporting
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>


/* Remove __attribute__ when we are not compiling with gcc */
#ifndef __GNUC__
# define __attribute__(x)
#endif


/** @brief Reports a system error and returns `eval`.
 *
 * @name Error functions
 *
 * The error functions below takes (some of) the following arguments:
 * @param eval    Return value.
 * @param msg     Error message.  This is a printf()-like format string.
 *                If NULL, the message from last error will be reused.
 *
 * The functions prefixed with a "v" are equivalent to their corresponding
 * functions, but takes a `va_list` instead of a variable number of arguments.
 * @{
 */

/** @brief Reports fatal error and exit the program with error code `eval`. */
int fatal(int eval, const char *msg, ...)
  __attribute__ ((__noreturn__, __format__ (__printf__, 2, 3)));

/** @brief Reports fatal error (excluding output from strerror()) and exit
 *  the program with error code `eval`. */
int fatalx(int eval, const char *msg, ...)
  __attribute__ ((__noreturn__, __format__ (__printf__, 2, 3)));

/** @brief Reports error and returns `eval`. */
int err(int eval, const char *msg, ...)
  __attribute__ ((__format__ (__printf__, 2, 3)));

/** @brief Reports error (excluding output from strerror()) and returns
 *  `eval`. */
int errx(int eval, const char *msg, ...)
  __attribute__ ((__format__ (__printf__, 2, 3)));



void vfatal(int eval, const char *msg, va_list ap)
  __attribute__ ((__noreturn__, __format__ (__printf__, 2, 0)));

void vfatalx(int eval, const char *msg, va_list ap)
  __attribute__ ((__noreturn__, __format__ (__printf__, 2, 0)));

void verr(int eval, const char *msg, va_list ap)
  __attribute__ ((__format__ (__printf__, 2, 0)));

void verrx(int eval, const char *msg, va_list ap)
  __attribute__ ((__format__ (__printf__, 2, 0)));

/** @} */  /* end of group "Error functions" */



/** @brief Associated functions
 *
 * @{
 */

/** Returns the error code of the last error. */
int err_getcode();

/** Returns the error message of the last error. */
char *err_getmsg();

/** Clear the last error (setting code to zero). */
void err_clear();

/** Set prefix to prepend to all errors in this application.
 *  Typically this is the program name. */
void err_set_prefix(const char *prefix);

/** Set stream that error messages are printed to.  The default is stderr.
 *  Set to NULL for silence.
 *
 *  The error stream can also be set with the ERR_STREAM environment
 *  variable. */
void err_set_stream(FILE *stream);

/* Indicate wheter the error functions should return normally, exit or about.
 *   - mode >= 2: abort
 *   - mode == 1: exit (with error code)
 *   - mode == 0: normal return
 *   - mode < 0:  check ERR_FAIL_MODE environment variable */
void err_set_fail_mode(int mode);


/** @} */


/** @brief Environment variables
 *
 *   * ERR_STREAM
 *         Error stream to write messages to.
 *           - if not set         : write to stdout
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
 */

#endif /* ERR_H */

