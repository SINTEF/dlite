#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "config.h"
#include "config-paths.h"

#include "utils/compat.h"
#include "utils/map.h"
#include "utils/err.h"
#include "utils/strtob.h"
#include "utils/tgen.h"
#include "utils/fileutils.h"
#include "getuuid.h"
#include "dlite.h"
#include "dlite-macros.h"
#include "dlite-storage-plugins.h"
#include "dlite-mapping-plugins.h"
#include "dlite-codegen.h"

#ifdef WITH_PYTHON
#include "pyembed/dlite-python-storage.h"
#include "pyembed/dlite-python-mapping.h"
#endif

/* New type mapping name to pointer to a FUPaths object. */
typedef map_t(FUPaths *) PathsMap;

/* Global variables */
static int use_build_root = -1;               /* whether to use build root */
static FUPlatform _path_standard = fuNative;  /* current path standard */
static PathsMap *_pathsmap = NULL;            /* registered path's */


/********************************************************************
 * Utility functions
 ********************************************************************/

/*
  Returns a pointer to the version string of DLite.
*/
const char *dlite_get_version(void)
{
  return dlite_VERSION;
}


/* Returns a pointer to map structure for paths */
static PathsMap *get_pathsmap(void)
{
  if (!_pathsmap) {
    static PathsMap pathsmap;
    map_init(&pathsmap);
    atexit(dlite_paths_unregister);

    _pathsmap = &pathsmap;

    /* For convenience, initialise the paths the first time we call this
       function so we have them available.

       We do that by asking them for their paths, which will create their
       paths and trigger registering them in pathsmap.

       Note that we for now do not call functions that reside in the
       dlite library.  Hence, dlite_python_storage_paths()
       and dlite_python_mapping_paths() are not called. */
    dlite_storage_plugin_paths_get();
    dlite_mapping_plugin_paths_get();
    //dlite_python_storage_paths();
    //dlite_python_mapping_paths();
    dlite_codegen_path_get();
    dlite_storage_paths();
  }
  return _pathsmap;
}

/*
  Returns current path formatting standard.
 */
FUPlatform dlite_paths_get_standard(void)
{
  if (_path_standard == fuNative) _path_standard = fu_native_platform();
  return _path_standard;
}

/*
  Sets path formatting standard according to `platform`. Returns the
  previous standard or -1 on error.
 */
FUPlatform dlite_paths_set_standard(FUPlatform platform)
{
  FUPlatform prev = _path_standard;
  const char *key;
  PathsMap *pathsmap = get_pathsmap();
  map_iter_t iter;
  if (platform == fuNative) platform = fu_native_platform();
  if (!fu_supported_platform(platform))
    return errx(-1, "unsupported platform: %d", platform);
  _path_standard = platform;

  iter = map_iter(pathsmap);
  while ((key = map_next(pathsmap, &iter))) {
    FUPaths **paths_ptr = map_get(pathsmap, key);
    fu_paths_set_platform(*paths_ptr, platform);
  }
  return prev;
}

/*
  Unregisters all paths.
 */
void dlite_paths_unregister(void)
{
  if (_pathsmap) map_deinit(_pathsmap);
  _pathsmap = NULL;
}

/*
  Registers `paths` with given name.
 */
int dlite_paths_register(const char *name, FUPaths *paths)
{
  PathsMap *pathsmap = get_pathsmap();
  map_set(pathsmap, name, paths);
  return 0;
}

/*
  Returns paths corresponding to `name` or NULL on error.
 */
FUPaths *dlite_paths_get(const char *name)
{

  PathsMap *pathsmap = get_pathsmap();
  FUPaths **p = map_get(pathsmap, name);
  if (!p) return errx(1, "invalid path name: %s", name), NULL;
  return *p;
}



/*
  Writes an UUID to `buff` based on `id`.

  Whether and what kind of UUID that is generated depends on `id`:
    - If `id` is NULL or empty, a new random version 4 UUID is generated.
    - If `id` is not a valid UUID string, a new version 5 sha1-based UUID
      is generated from `id` using the DNS namespace.
    - Otherwise is `id` already a valid UUID and it is simply copied to
      `buff`.

  Length of `buff` must at least (DLITE_UUID_LENGTH + 1) bytes (36 bytes
  for UUID + NUL termination).

  Returns the UUID version if a new UUID is generated or zero if `id`
  is already a valid UUID.  On error, -1 is returned.
 */
int dlite_get_uuid(char *buff, const char *id)
{
  return getuuid(buff, id);
}


/*
  Returns an unique uri for metadata defined by `name`, `version`
  and `namespace` as a newly malloc()'ed string or NULL on error.

  The returned url is constructed as follows:

      namespace/version/name
 */
char *dlite_join_meta_uri(const char *name, const char *version,
                          const char *namespace)
{
  char *uri = NULL;
  size_t size = 0;
  size_t n = 0;
  if (name) {
    size += strlen(name);
    n++;
  }
  if (version) {
    size += strlen(version);
    n++;
  }
  if (namespace) {
    size += strlen(namespace);
    n++;
  }
  if ((n == 3) && (size > 0)) {
    size += 3;
    if (!(uri = malloc(size))) return err(1, "allocation failure"), NULL;
    snprintf(uri, size, "%s/%s/%s", namespace, version, name);
  }
  return uri;
}

/*
  Splits `metadata` uri into its components.  If `name`, `version` and/or
  `namespace` are not NULL, the memory they points to will be set to a
  pointer to a newly malloc()'ed string with the corresponding value.

  Returns non-zero on error.
 */
int dlite_split_meta_uri(const char *uri, char **name, char **version,
                         char **namespace)
{
  char *p, *q, *namep=NULL, *versionp=NULL, *namespacep=NULL;

  if (!(p = strrchr(uri, '/')))
    FAIL1("invalid metadata uri: '%s'", uri);
  q = p-1;
  while (*q != '/' && q > uri) q--;
  if (q == uri)
    FAIL1("invalid metadata uri: '%s'", uri);

  if (name) {
    if (!(namep = strdup(p + 1))) FAIL("allocation failure");
  }
  if (version) {
    int size = p - q;
    if (!(versionp = malloc(size))) FAIL("allocation failure");
    memcpy(versionp, q + 1, size - 1);
    versionp[size - 1] = '\0';
  }
  if (namespace) {
    int size = q - uri + 1;
    if (!(namespacep = malloc(size))) FAIL("allocation failure");
    memcpy(namespacep, uri, size - 1);
    namespacep[size - 1] = '\0';
  }

  if (name) *name = namep;
  if (version) *version = versionp;
  if (namespace) *namespace = namespacep;
  return 0;
 fail:
  if (namep) free(namep);
  if (versionp) free(versionp);
  if (namespacep) free(namespacep);
  return 1;
}


/*
  Parses the options string `options` and assign corresponding values
  of the array `opts`.  The options string should be a valid url query
  string of the form

      key1=value1;key2=value2...

  where the values are terminated by NUL or any of the characters in ";&#".
  A hash (#) terminates the options.

  `opts` should be a NULL-terminated DLiteOpt array initialised with
  default values.  At return, the values of the provided options are
  updated.

  If `modify` is non-zero, `options` is modifies such that all values in
  `opts` are NUL-terminated.  Otherwise they may be terminated by any of
  the characters in ";&#".

  Returns non-zero on error.
*/
int dlite_option_parse(char *options, DLiteOpt *opts, int modify)
{
  char *q, *p = options;
  if (!options) return 0;
  while (*p && *p != '#') {
    size_t i, len = strcspn(p, "=;&#");
    if (p[len] != '=')
      return errx(1, "no value for key '%.*s' in option string '%s'",
                  (int)len, p, options);
    for (i=0; opts[i].key; i++) {
      if (strncmp(opts[i].key, p, len) == 0 && strlen(opts[i].key) == len) {
        p += len;
        if (*p == '=') p++;
        opts[i].value = p;
        p += strcspn(p, ";&#");
        q = p;
        if (*p && strchr(";&", *p)) p++;
        if (modify) q[0] = '\0';
        break;
      }
    }
    if (!opts[i].key) {
      int len = strcspn(p, "=;&#");
      return errx(1, "unknown option key: '%.*s'", len, p);
    }
  }
  return 0;
}


/*
  Returns a newly allocated url constructed from the arguments of the form

      driver://location?options#fragment

  The `driver`, `options` and `fragment` arguments may be NULL.
  Returns NULL on error.
 */
char *dlite_join_url(const char *driver, const char *location,
                     const char *options, const char *fragment)
{
  TGenBuf s;
  tgen_buf_init(&s);
  if (driver) tgen_buf_append_fmt(&s, "%s://", driver);
  tgen_buf_append(&s, location, -1);
  if (options) tgen_buf_append_fmt(&s, "?%s", options);
  if (fragment) tgen_buf_append_fmt(&s, "#%s", fragment);
  return tgen_buf_steal(&s);
}


/*
  Splits an `url` of the form

      driver://location?options#fragment

  into four parts: `driver`, `location`, `options` and `fragment`.
  For the arguments that are not NULL, the pointers they points to
  will be assigned to point to the corresponding section within `url`.

  This function modifies `url`.  Make a copy if you don't want that.

  Returns non-zero on error.

  Note:
  URLs are assumed to have the following syntax (ref. [wikipedia]):

      URL = scheme:[//authority]path[?query][#fragment]

  where the authority component divides into three subcomponents:

      authority = [userinfo@]host[:port]

  This function maps `scheme` to `driver`, `[authority]path` to `location`
  `query` to `options` and `fragment` to `fragment`.

  [wikipedia]: https://en.wikipedia.org/wiki/URL
 */
int dlite_split_url(char *url, char **driver, char **location, char **options,
                    char **fragment)
{
  return dlite_split_url_winpath(url, driver, location, options, fragment, 0);
}

/*
  Like dlite_split_url(), but with one additional argument.

  If `winpath` is non-zero and `url` starts with "C:\" or "C:/", then
  the initial "C" is not treated as a driver, but rather as a part of
  the location.
 */
int dlite_split_url_winpath(char *url, char **driver, char **location,
                            char **options, char **fragment, int winpath)
{
  size_t i;
  char *p;

  /* default to NUL */
  p = url + strlen(url);
  assert(*p == '\0');
  if (driver)   *driver   = p;
  if (location) *location = p;
  if (options)  *options  = p;
  if (fragment) *fragment = p;

  /* strip off and assign fragment */
  if ((p = strchr(url, '#'))) {
    if (fragment) *fragment = p+1;
    *p = '\0';
  }

  /* strip off query and assign options */
  if ((p = strchr(url, '?'))) {
    *p = '\0';
    if (options) *options = (p[1]) ? p+1 : NULL;
  } else {
    if (options) *options = NULL;
  }

  /* assign driver and location */
  i = strcspn(url, ":/");
  if (winpath && strlen(url) > 3 && (url[0] == 'C' || url[0] == 'c') &&
      url[1] == ':' && (url[2] == '\\' || url[2] == '/')) {
    /* special case: url is a windows path */
    if (driver) *driver = NULL;
    if (location) *location = url;
  } else if (url[i] == ':') {
    url[i] = '\0';
    if (driver) *driver = url;
    if (url[i+1] == '/' && url[i+2] == '/')
      p = url + i + 3;
    else
      p = url + i + 1;
    if (location) *location = (p[0]) ? p : NULL;
  } else {
    if (driver) *driver = NULL;
    if (location) *location = (url[0]) ? url : NULL;
  }

  return 0;
}


/*
  Returns non-zero if paths should refer to build root instead of
  installation root.
 */
int dlite_use_build_root(void)
{
  if (use_build_root == -1) {
    char *endptr, *p = getenv("DLITE_USE_BUILD_ROOT");
    use_build_root = 0;
    if (p) {
      if (!*p) {
        use_build_root = 1;
      } else {
        int v = strtob(p, &endptr);
        if (!endptr)
          use_build_root = (v) ? 1 : 0;
        else
          warn("environment variable DLITE_USE_BUILD_ROOT must have a "
               "valid boolean value: %s", p);
      }
    }
  }
  return use_build_root;
}

/*
  Sets whether paths should refer to build root.  Default is the
  installation root, unless the environment variable
  DLITE_USE_BUILD_ROOT is set and is not false.
*/
void dlite_set_use_build_root(int v)
{
  use_build_root = (v) ? 1 : 0;
}

/*
  Returns pointer to installation root.  It may be altered with environment
  variable DLITE_ROOT.
*/
const char *dlite_root_get(void)
{
  static char *dlite_root = NULL;
  if (!dlite_root) {
    char *v = getenv("DLITE_ROOT");
    dlite_root = (v) ? v : DLITE_ROOT;
  }
  return dlite_root;
}


/* Help function for dlite_add_dll_path() */
#ifdef WINDOWS
static void _add_dll_dir(const char *path)
{
  size_t n;
  wchar_t wcstr[256];
  printf("\n*** adding DLL dir: %s\n", path);
  mbstowcs_s(&n, wcstr, 256, path, 255);
  AddDllDirectory(wcstr);
}
#endif

/*
  On Windows, this function adds default directories to the DLL search
  path.  Based on whether the `DLITE_USE_BUILD_ROOT` environment
  variable is defined, the library directories under either the build
  directory or the installation root (environment variable DLITE_ROOT)
  are added to the DLL search path using AddDllDirectory().

  On Linux this function does nothing.

  Returns non-zero on error.
 */
int dlite_add_dll_path(void)
{
#ifdef WINDOWS
  static int called = 0;
  char buf[1024];

  if (called) return 0;
  called = 1;

  if (getenv("DLITE_USE_BUILD_ROOT")) {
    _add_dll_dir(fu_winpath((const char *)dlite_BUILD_ROOT "/src",
                            buf, sizeof(buf), NULL));
    _add_dll_dir(fu_winpath(dlite_BUILD_ROOT "/src/util",
                            buf, sizeof(buf), NULL));
#ifdef WITH_PYTHON
    _add_dll_dir(fu_winpath(dlite_BUILD_ROOT "/src/pyembed",
                            buf, sizeof(buf), NULL));
#endif
  } else {
    _add_dll_dir(fu_winpath(DLITE_ROOT "/" DLITE_LIBRARY_DIR,
                            buf, sizeof(buf), NULL));
  }
#endif
  return 0;
}




/* ----------------------------------------------------------- */
/* Add explanation for why we need these functions and do not
 * call the error functions in err.h directly.
 *
 * It had something to do with loading dlite as a shared library
 * in Python which itself can load shared library plugins.
 */

/* Wrappers around error functions */
#define BODY(fun, eval, msg)                            \
  va_list ap;                                           \
  va_start(ap, msg);                                    \
  dlite_v ## fun(eval, msg, ap);                        \
  va_end(ap)
#define BODY2(fun, msg)                                 \
  va_list ap;                                           \
  va_start(ap, msg);                                    \
  dlite_v ## fun(msg, ap);                              \
  va_end(ap)
void dlite_fatal(int eval, const char *msg, ...) {
  // cppcheck-suppress va_end_missing
  BODY(fatal, eval, msg); }
void dlite_fatalx(int eval, const char *msg, ...) {
  // cppcheck-suppress va_end_missing
  BODY(fatalx, eval, msg); }
int dlite_err(int eval, const char *msg, ...) {
  BODY(err, eval, msg); return eval; }
int dlite_errx(int eval, const char *msg, ...) {
  BODY(errx, eval, msg); return eval; }
int dlite_warn(const char *msg, ...) {
  BODY2(warn, msg); return 0; }
int dlite_warnx(const char *msg, ...) {
  BODY2(warnx, msg); return 0; }

/* Changed below to explicitely link against _err_vformat() to avoid
   to unintensionally link against the nonstandard BSD family of error
   functions, on systems implementing them.

   FIXME - double-check that this is the correct fix on Windows too.
*/
void dlite_vfatal(int eval, const char *msg, va_list ap) {
  exit(_err_vformat(errLevelFatal, eval, errno, NULL, NULL, msg, ap)); }
void dlite_vfatalx(int eval, const char *msg, va_list ap) {
  exit(_err_vformat(errLevelFatal, eval, 0, NULL, NULL, msg, ap)); }
int dlite_verr(int eval, const char *msg, va_list ap) {
  return _err_vformat(errLevelError, eval, errno, NULL, NULL, msg, ap); }
int dlite_verrx(int eval, const char *msg, va_list ap) {
  return _err_vformat(errLevelError, eval, 0, NULL, NULL, msg, ap); }
int dlite_vwarn(const char *msg, va_list ap) {
  return _err_vformat(errLevelWarn, 0, errno, NULL, NULL, msg, ap); }
int dlite_vwarnx(const char *msg, va_list ap) {
  return _err_vformat(errLevelWarn, 0, 0, NULL, NULL, msg, ap); }

int dlite_errval(void) { return err_geteval(); }
const char *dlite_errmsg(void) { return err_getmsg(); }
void dlite_errclr(void) { err_clear(); }
