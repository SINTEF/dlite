#ifndef _DLITE_MISC_H
#define _DLITE_MISC_H

/**
  @file
  @brief Main header file for dlite
*/
#include <stdio.h>

#include "utils/fileutils.h"
#include "dlite-errors.h"
#include "dlite-type.h"

/** length of an uuid (excl. NUL-termination) */
#define DLITE_UUID_LENGTH 36

/** Fallback namespace for data instances */
#define DLITE_DATA_NS "http://onto-ns.com/data"


/**
  @name General dlite utility functions
  @{
 */

/**
  Returns static pointer to a string with the current version of DLite.
*/
const char *dlite_get_version(void);

/**
  Returns current platform based on the DLITE_PLATFORM environment
  variable.  Used when initiating paths.
 */
FUPlatform dlite_get_platform(void);


/**
  Returns current platform based on the DLITE_PLATFORM environment
  variable.  Used when initiating paths.
 */
FUPlatform dlite_get_platform(void);



/** Ways to express an instance ID. */
typedef enum {
  dliteIdRandom,  /*!< new random version 4 UUID */
  dliteIdHash,    /*!< new version 5 UUID with DNS namespace */
  dliteIdCopy     /*!< copy UUID */
} DLiteIdType;

/**
  Returns non-zero if `id` is a valid UUID.
*/
int dlite_isuuid(const char *id);

/**
  Returns the ID type of `id`.
 */
DLiteIdType dlite_idtype(const char *id);

/**
  Like dlite_idtype(), but takes the the length of `id` as an
  additional argument.
*/
DLiteIdType dlite_idtypen(const char *id, int len);

/**
  Write normalised `id` to `buff`, which is a buffer of size `n`.

  The normalisation is done according to the following table:

  | ID             | Normalised ID  |
  |----------------|----------------|
  | NULL           | NULL           |
  | *uuid*         | *ns* / *uuid*  |
  | *uri* / *uuid* | *uri* / *uuid* |
  | *uri*          | *uri*          |
  | *name*         | *ns* / *name*  |

  where;

  - *uuid* is a valid UUID. Ex: "0a46cacf-ce65-5b5b-a7d7-ad32deaed748"
  - *ns* is the predefined namespace string "http://onto-ns.com/data"
  - *uri* is a valid URI with no query or fragment parts.
    Ex: "http://onto-ns.com/meta/0.1/MyDatamodel"
  - *name* is a string that is neither a UUID or a URL. Ex: "aa6060"

  A final hash or slash in `id` is stripped off.

  Return the number of bytes written `buff` (excluding the terminating
  NUL) or would have been written to `buff` if it is not large enough.
  A negative number is returned on error.
 */
int dlite_normalise_id(char *buff, size_t n, const char *id, const char *uri);

/**
  Like dlite_normalise_id(), but takes `len`, the the length of `id` as an
  additional argument.
*/
int dlite_normalise_idn(char *buff, size_t n,
                        const char *id, size_t len,
                        const char *uri);

/**
  Writes instance UUID to `buff` based on `id`.

  Length of `buff` must at least (DLITE_UUID_LENGTH + 1) bytes (36 bytes
  for UUID + NUL termination).

  The UUID is calculated according to this table.

  | ID             | Corresponding UUID    | ID type       | Note      |
  |----------------|-----------------------|------.....----|-----------|
  | NULL           | random UUID           | dliteIdRandom |           |
  | *uuid*         | *uuid*                | dliteIdCopy   |           |
  | *uri* / *uuid* | *uuid*                | dliteIdCopy   |           |
  | *uri*          | hash of *uri*         | dliteIdHash   |           |
  | *name*         | hash of *name*        | dliteIdHash   |  < v0.6.0 |
  | *name*         | hash of *ns* / *name* | dliteIdHash   | >= v0.6.0 |

  where:

  - *uuid* is a valid UUID. Ex: "0a46cacf-ce65-5b5b-a7d7-ad32deaed748"
  - *ns* is the predefined namespace string "http://onto-ns.com/data"
  - *uri* is a valid URI with no query or fragment parts.
    Ex: "http://onto-ns.com/meta/0.1/MyDatamodel"
  - *name* is a string that is neither a UUID or a URL. Ex: "aa6060"

  A version 4 UUID is used for the random UUID and a version 5 UUID
  (with the DNS namespace) is used fhr the hash.

  Returns the DLite ID type or a negative error code on error.
 */
DLiteIdType dlite_get_uuid(char *buff, const char *id);

/**
  Like dlite_get_uuid(), but takes the the length of `id` as an
  additional parameter.
 */
DLiteIdType dlite_get_uuidn(char *buff, const char *id, size_t len);

/*
  Internal help-function for dlite_get_uuidn(), which takes the
  namespacedID behavior as argument.
 */
DLiteIdType _dlite_get_uuidn(char *buff, const char *id, size_t len,
                             int namespacedID);

/**
  Returns an unique uri for metadata defined by `name`, `version`
  and `namespace` as a newly malloc()'ed string or NULL on error.

  The returned uri is constructed as follows:

      namespace/version/name
 */
char *dlite_join_meta_uri(const char *name, const char *version,
                          const char *namespace);

/**
  Splits metadata `uri` into its components.  If `name`, `version` and/or
  `namespace` are not NULL, the memory they points to will be set to a
  pointer to a newly malloc()'ed string with the corresponding value.

  Returns non-zero on error.
*/
int dlite_split_meta_uri(const char *uri, char **name, char **version,
                         char **namespace);

/** @} */


/**
  @name Parsing options
  @{
 */

/** Struct used by dlite_getopt() */
typedef struct _DLiteOpt {
  int c;              /*!< Integer identifier for this option */
  const char *key;    /*!< Option key */
  const char *value;  /*!< Option value, initialised with default value */
  const char *descr;  /*!< Description of this option */
} DLiteOpt;

typedef enum {
  dliteOptDefault=0,  /*!< Default option flags */
  dliteOptStrict=1    /*!< Stict mode. Its an error if option is unknown */
} DLiteOptFlag;

/**
  Parses the options string `options` and assign corresponding values
  of the array `opts`.  The options string should be a valid url query
  string of the form:

      key1=value1;key2=value2...

  where the values should be encoded with `uri_encode()` and
  terminated by NUL or any of the characters in ";&#".  A hash (#)
  terminates the options.

  At return, `options` is modified. All values in the `options` string
  will be URI decoded and NUL-terminated.

  `opts` should be a NULL-terminated DLiteOpt array initialised with
  default values.  At return, the values of the provided options are
  updated.

  `flags` should be zero or `dliteOptStrict`.

  Returns non-zero on error.

  Example
  -------
  If the `opts` array only contains a few elements, accessing it by index
  is probably the most convinient.  However, if it contains many elements,
  switch might be a better option using the following pattern:

  ```c
  int i;
  DLiteOpt opts[] = {
    {'1', "key1", "default1", "description of key1..."},
    {'b', "key2", "default2", "description of key2..."},
    {NULL, NULL}
  };
  dlite_getopt(options, opts);
  for (i=0; opts[i].key; i++) {
    switch (opts[i].c) {
    case '1':
      // process option key1
      break;
    case 'b':
      // process option key2
      break;
    }
  }
  ```
 */
int dlite_option_parse(char *options, DLiteOpt *opts, DLiteOptFlag flags);

/** @} */


/**
  @name Path handling
  @{
 */

/**
  Returns a newly allocated url constructed from the arguments of the form

      driver://location?options#fragment

  The `driver`, `options` and `fragment` arguments may be NULL.
  Returns NULL on error.
 */
char *dlite_join_url(const char *driver, const char *location,
                     const char *options, const char *fragment);

/**
  Splits an `url` of the form

      driver://location?options#fragment

  into four parts: `driver`, `location`, `options` and `fragment`.
  For the arguments that are not NULL, the pointers they points to
  will be assigned to point to the corresponding section within `url`.

  This function modifies `url`.  Make a copy if you don't want that.

  Returns non-zero on error.

  @note:
  URLs are assumed to have the following syntax (ref. [wikipedia]):

      URL = scheme:[//authority]path[?query][#fragment]

  where the authority component divides into three subcomponents:

      authority = [userinfo@]host[:port]

  This function maps `scheme` to `driver`, `[authority]path` to `location`
  `query` to `options` and `fragment` to `fragment`.

  [wikipedia]: https://en.wikipedia.org/wiki/URL
 */
int dlite_split_url(char *url, char **driver, char **location, char **options,
                    char **fragment);

/**
  Like dlite_split_url(), but with one additional argument.

  If `winpath` is non-zero and `url` starts with "C:\" or "C:/", then
  the initial "C" is not treated as a driver, but rather as a part of
  the location.
 */
int dlite_split_url_winpath(char *url, char **driver, char **location,
                            char **options, char **fragment, int winpath);


/**
  Returns non-zero if paths should refer to build root instead of
  installation root.
 */
int dlite_use_build_root(void);

/**
  Sets whether paths should refer to build root.  Default is the
  installation root, unless the environment variable
  DLITE_USE_BUILD_ROOT is set and is not false.
*/
void dlite_set_use_build_root(int v);

/**
  Returns pointer to installation root.  It may be altered with environment
  variable DLITE_ROOT.
*/
const char *dlite_root_get(void);

/**
  Returns pointer to the DLite package installation root.

  It may be altered with environment variable DLITE_PKG_ROOT.
  With Python, it defaults to the DLite Python root directory, otherwise
  it defaults to DLITE_ROOT.

*/
const char *dlite_pkg_root_get(void);


/**
  On Windows, this function adds default directories to the DLL search
  path.  Based on whether the `DLITE_USE_BUILDROOT` environment
  variable is defined, the library directories under either the build
  directory or the installation root (environment variable DLITE_ROOT)
  are added to the DLL search path using AddDllDirectory().

  On Linux this function does nothing.

  Returns non-zero on error.
 */
int dlite_add_dll_path(void);

/** @} */


/**
  @name Managing global state
  @{
*/

/*
  Initialises dlite. This function may be called several times.
 */
void dlite_init(void);

/*
  Finalises DLite. Will be called by atexit().

  This function may be called several times.
 */
void dlite_finalize(void);

/**
  Globals handle.
 */
typedef struct _Session DLiteGlobals;

/**
  Returns reference to globals handle.
*/
DLiteGlobals *dlite_globals_get(void);

/**
  Set globals handle.
*/
void dlite_globals_set(DLiteGlobals *globals_handler);

/**
  Add global state with given name.

  `ptr` is a pointer to the state and `free_fun` is a function that frees it.
  Returns non-zero on error.
 */
int dlite_globals_add_state(const char *name, void *ptr,
                            void (*free_fun)(void *ptr));

/**
  Remove global state with the given name.
  Returns non-zero on error.
 */
int dlite_globals_remove_state(const char *name);

/**
  Returns global state with given name or NULL on error.
 */
void *dlite_globals_get_state(const char *name);

/**
  Returns non-zero if we are in an atexit handler.
 */
int dlite_globals_in_atexit(void);

/**
  Mark that we are in an atexit handler.
 */
void dlite_globals_set_atexit(void);

/** @} */


/**
  @name Functions controlling whether to hide warnings

  Warning parameters:
    - `warnings_hide`: whether to hide warnings (see below).
    - `warnings_pattern`: glob pattern matching the warning message.

  If `warnings_pattern` is NULL, warnings are hidden if `warnings_hide`
  is non-zero.

  If `warnings_pattern` is not NULL, then warnings are hidden if:
    - `warnings_pattern` match the warning message and `warnings_hide` is
      non-zero.
    - `warnings_pattern` don't match the warning message and `warnings_hide`
      is zero.
  @{
*/

/**
  Return parameters controlling whether warnings should be hidden.

  If `*pattern` is not NULL, it is assigned to a static pointer to
  `warnings_pattern` (owned by DLite).

  Returns `warnings_hide`.
 */

int dlite_get_warnings_hide(const char **pattern);

/**
  Set parameters controlling whether warnings should be hidden.
 */
void dlite_set_warnings_hide(int hide, char *pattern);

/** @} */


/**
  @name Wrappers around error functions
  The `DLITE_NOERR()` and `DLITE_NOERR_END` macros are intended to
  mark a code block in which the specified errors will not be printed.
  Use them as follows:

  ```c
  DLITE_NOERR(DLITE_ERRBIT(DLiteIOError) | DLITE_ERRBIT(DLiteRuntimeError));
    // code block where IO and runtime errors are ignored
    ...
  DLITE_NOERR_END;
  ```
  @{
*/

/* Bit mask of error codes to not print*/
typedef int64_t DLiteErrMask;

/* Get ans set global mask for error codes to not print. */
DLiteErrMask *_dlite_err_mask_get(void);
void _dlite_err_mask_set(DLiteErrMask mask);

#define DLITE_ERRBIT(code)                                              \
  ((int)1<<(int)((code >= 0) ? 0 : (code <= dliteLastError) ? -dliteLastError : -code))

#define DLITE_NOERR(mask)                                       \
  do {                                                          \
    DLiteErrMask *_errmaskptr = _dlite_err_mask_get();          \
    DLiteErrMask _errmask = (_errmaskptr) ? *_errmaskptr : 0;   \
    dlite_err_mask_set(mask)

#define DLITE_NOERR_END                                         \
    dlite_err_mask_set(_errmask);                               \
  } while (0)


/** Set whether to ignore printing given error code. */
void dlite_err_ignored_set(DLiteErrCode code, int value);

/** Return whether printing is ignored for given error code. */
int dlite_err_ignored_get(DLiteErrCode code);

/** Return current error message. */
const char *dlite_errmsg(void);

#ifndef __GNUC__
#define __attribute__(x)
#endif
#include <stdarg.h>
void dlite_fatal(int eval, const char *msg, ...)
  __attribute__ ((__noreturn__, __format__ (__printf__, 2, 3)));
void dlite_fatalx(int eval, const char *msg, ...)
  __attribute__ ((__noreturn__, __format__ (__printf__, 2, 3)));
int dlite_err(int eval, const char *msg, ...)
  __attribute__ ((__format__ (__printf__, 2, 3)));
int dlite_errx(int eval, const char *msg, ...)
  __attribute__ ((__format__ (__printf__, 2, 3)));
int dlite_warn(const char *msg, ...)
  __attribute__ ((__format__ (__printf__, 1, 2)));
int dlite_warnx(const char *msg, ...)
  __attribute__ ((__format__ (__printf__, 1, 2)));
int dlite_info(const char *msg, ...)
  __attribute__ ((__format__ (__printf__, 1, 2)));
int dlite_debug(const char *msg, ...)
  __attribute__ ((__format__ (__printf__, 1, 2)));

void dlite_vfatal(int eval, const char *msg, va_list ap)
  __attribute__ ((__noreturn__, __format__ (__printf__, 2, 0)));
void dlite_vfatalx(int eval, const char *msg, va_list ap)
  __attribute__ ((__noreturn__, __format__ (__printf__, 2, 0)));
int dlite_verr(int eval, const char *msg, va_list ap)
  __attribute__ ((__format__ (__printf__, 2, 0)));
int dlite_verrx(int eval, const char *msg, va_list ap)
  __attribute__ ((__format__ (__printf__, 2, 0)));
int dlite_vwarn(const char *msg, va_list ap)
  __attribute__ ((__format__ (__printf__, 1, 0)));
int dlite_vwarnx(const char *msg, va_list ap)
  __attribute__ ((__format__ (__printf__, 1, 0)));
int dlite_vinfo(const char *msg, va_list ap)
  __attribute__ ((__format__ (__printf__, 1, 0)));
int dlite_vdebug(const char *msg, va_list ap)
  __attribute__ ((__format__ (__printf__, 1, 0)));

int dlite_errval(void);
void dlite_errclr(void);
FILE *dlite_err_get_stream(void);
FILE *dlite_err_set_stream(FILE *stream);
void dlite_err_set_file(const char *filename);

int dlite_err_set_level(int level);
int dlite_err_get_level(void);
const char *dlite_err_set_levelname(const char *name);
const char *dlite_err_get_levelname(void);

int dlite_err_set_warn_mode(int mode);
int dlite_err_get_warn_mode(void);
int dlite_err_set_debug_mode(int mode);
int dlite_err_get_debug_mode(void);
int dlite_err_set_override_mode(int mode);
int dlite_err_get_override_mode(void);


/*
  Issues a deprecation warning.

  `version_removed` is the version the deprecated feature is expected
  to be finally removed.
  expected to be finally removed.
  `descr` is a description of the deprecated feature.

  Returns non-zero if `version_removed` has passed.
 */
#define dlite_deprecation_warning(version_removed, descr) \
  _dlite_deprecation_warning(version_removed, ERR_FILEPOS, _err_func, descr)

int _dlite_deprecation_warning(const char *version_removed,
                               const char *filepos, const char *func,
                               const char *descr);


/** @} */


#endif /* _DLITE_MISC_H */
