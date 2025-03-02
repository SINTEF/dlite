#include <assert.h>
#include <ctype.h>
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
#include "utils/strutils.h"
#include "utils/session.h"
#include "utils/rng.h"
#include "utils/uri_encode.h"
#include "utils/urlsplit.h"
#include "utils/uuid.h"
#include "utils/uuid4.h"
#include "utils/globmatch.h"
#include "dlite.h"
#include "dlite-macros.h"
#include "dlite-errors.h"
#include "dlite-behavior.h"

#ifdef WITH_PYTHON
#include "pyembed/dlite-python-path.h"
#endif

/* Global variables */
static int use_build_root = -1;   /* whether to use build root */
static int dlite_platform = 0;    /* dlite platform */


/********************************************************************
 * General utility functions
 ********************************************************************/

/*
  Returns a pointer to the version string of DLite.
*/
const char *dlite_get_version(void)
{
  return dlite_VERSION;
}

/*
  Returns current platform based on the DLITE_PLATFORM environment
  variable.  Used when initiating paths.
 */
FUPlatform dlite_get_platform(void)
{
  if (!dlite_platform) {
    FUPlatform platform;
    char *s = getenv("DLITE_PLATFORM");
    if (s && (platform = fu_platform(s)) >= 0) {
      if (platform == fuNative) platform = fu_native_platform();
      dlite_platform = platform;
    }
  }
  return dlite_platform;
}


/*
  Returns non-zero if `id` is a valid UUID.
*/
int dlite_isuuid(const char *id)
{
  return isuuid(id);
}

/*
  Returns the ID type of `id`.
 */
DLiteIdType dlite_idtype(const char *id)
{
  return dlite_idtypen(id, (id) ? strlen(id) : 0);
}

/*
  Like dlite_idtype(), but takes the the length of `id` as an
  additional argument.
*/
DLiteIdType dlite_idtypen(const char *id, int len)
{
  if (!len || !id || !*id) return dliteIdRandom;

  if (id[len-1] && strchr("/#", id[len-1])) len--;

  if (len == DLITE_UUID_LENGTH && isuuid(id)) return dliteIdCopy;

  if (isurln(id, len))
    return (len > DLITE_UUID_LENGTH+9 && isuuid(id + len-DLITE_UUID_LENGTH)) ?
      dliteIdCopy : dliteIdHash;

  return dliteIdHash;
}

/*
  Write normalised `id` to `buff`, which is a buffer of size `n`.

  The normalisation is done according to the following table:

  | ID             | Normalised ID  |
  |----------------|----------------|
  | NULL           | NULL           |
  | *uuid*         | *ns* / *uuid*  |
  | *uri* / *uuid* | *uri* / *uuid* |
  | *uri*          | *uri*          |
  | *id*           | *ns* / *id*    |

  where;

  - *uuid* is a valid UUID. Ex: "0a46cacf-ce65-5b5b-a7d7-ad32deaed748"
  - *ns* is the predefined namespace string "http://onto-ns.com/data"
  - *uri* is a valid URI with no query or fragment parts.
    Ex: "http://onto-ns.com/meta/0.1/MyDatamodel"
  - *id* is a string that is neither a UUID or a URL. Ex: "aa6060"

  A final hash or slash in `id` is stripped off.

  Return the number of bytes written `buff` (excluding the terminating
  NUL) or would have been written to `buff` if it is not large enough.
  A negative number is returned on error.
 */
int dlite_normalise_id(char *buff, size_t n, const char *id, const char *uri)
{
  return dlite_normalise_idn(buff, n, id, (id) ? strlen(id) : 0, uri);
}

/*
  Like dlite_normalise_id(), but takes `len`, the the length of `id` as an
  additional argument.
*/
int dlite_normalise_idn(char *buff, size_t n,
                        const char *id, size_t len,
                        const char *uri)
{
  // id: NULL
  if (!len || !id || !*id) {
    if (n > 0) buff[0] = '\0';
    return 0;
  }

  // id: uri/uuid, uri
  if (isurln(id, len)) {
    int m = len - DLITE_UUID_LENGTH;
    if (len > DLITE_UUID_LENGTH+9 && isuuid(id+m))
      return snprintf(buff, n, "%.*s", DLITE_UUID_LENGTH, id+m);
    return snprintf(buff, n, "%.*s", (int)len, id);
  }

  if (!uri) uri = DLITE_NS_DATA;
  char *sep = (strchr("/#", uri[strlen(uri)-1])) ? "" : "/";

  // id: uuid, id
  return snprintf(buff, n, "%s%s%.*s", uri, sep, (int)len, id);
}


/*
  Write a version 5 UUID hash (using the DNS namespace) of the `len`
  first characters of `id` to `buff`.
*/
static void uuid5n(char *buff, const char *id, size_t len)
{
  uuid_s uuid;
  uuid_create_sha1_from_name(&uuid, NameSpace_DNS, id, len);
  uuid_as_string(&uuid, buff);
}

/*
  Writes instance UUID to `buff` based on `id`.

  Whether and what kind of UUID that is generated depends on `id`:

    - If `id` is NULL or empty, a new random version 4 UUID is generated.
      Returns dliteIdRandom
    - If `id` is a valid UUID, it is copied to `buff`.
      Returns dliteIdCopy
    - If `id` is a valid URL that ends with an UUID, the UUID part is copied
      to `buff`.
      Returns dliteIdCopy
    - If `id` is a valid URL (that doesn't ends with an UUID), a hash
      of `id` is written to `buff`.
      Returns dliteIdHash
    - Otherwise, the generated uuid depends on the `namespacedID` behaviour.

        - If `namespacedID` is on, the UUID is generated from
          `ns`/`id`, where `ns` is DLITE_NS_DATA.
        - If `namespacedID` is off, the UUID is generated from `id`.

      Returns dliteIdHash

  Length of `buff` must at least (DLITE_UUID_LENGTH + 1) bytes (36 bytes
  for UUID + NUL termination).

  Returns the DLite ID type or a negative error code on error.
 */
DLiteIdType dlite_get_uuid(char *buff, const char *id)
{
  return dlite_get_uuidn(buff, id, (id) ? strlen(id) : 0);
}

/*
  Like dlite_get_uuid(), but takes the the length of `id` as an
  additional parameter.
 */
DLiteIdType dlite_get_uuidn(char *buff, const char *id, size_t len)
{
  DLiteIdType type;

  if (!len || !id || !*id) {  // id: NULL
    if (uuid4_generate(buff))
      return err(dliteRuntimeError, "cannot generate version 4 UUID");
    type = dliteIdRandom;
  } else {
    if (len == DLITE_UUID_LENGTH && isuuid(id)) {  // id: uuid
      strncpy(buff, id, DLITE_UUID_LENGTH);
      buff[DLITE_UUID_LENGTH] = '\0';
      type = dliteIdCopy;
    } else if (isurln(id, len)) {  // id: uri/uuid, uri
      int m = len - DLITE_UUID_LENGTH;
      if (m > 9 && isuuid(id+m)) {
        strncpy(buff, id+m, DLITE_UUID_LENGTH);
        buff[DLITE_UUID_LENGTH] = '\0';
        type = dliteIdCopy;
      } else {
        uuid5n(buff, id, len);
        type = dliteIdHash;
      }
    } else if (dlite_behavior_get("namespacedID")) {  // id: id
      char tmp[256], *s;
      size_t needed_len = len + sizeof(DLITE_NS_DATA) + 2;
      if (needed_len <= sizeof(tmp)) {
        snprintf(tmp, sizeof(tmp), "%s/%*.s", DLITE_NS_DATA, (int)len, id);
        uuid5n(buff, tmp, sizeof(tmp));
      } else {
        if (!(s = malloc(needed_len)))
          return err(dliteMemoryError, "allocation failure");
        uuid5n(buff, s, needed_len-1);
        free(s);
      }
      type = dliteIdHash;
    } else {  // id: id
      uuid5n(buff, id, len);
      type = dliteIdHash;
    }
  }

  /* For reprodusability, always convert to lower case */
  int i;
  for (i=0; i < DLITE_UUID_LENGTH; i++)
    buff[i] = tolower(buff[i]);

  return type;
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
    if (!(uri = malloc(size)))
     return err(dliteMemoryError, "allocation failure"), NULL;
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
    if (!(namep = strdup(p + 1)))
     FAILCODE(dliteMemoryError, "allocation failure");
  }
  if (version) {
    int size = p - q;
    assert(size > 0);
    if (!(versionp = malloc(size)))
     FAILCODE(dliteMemoryError, "allocation failure");
    memcpy(versionp, q + 1, size - 1);
    versionp[size - 1] = '\0';
  }
  if (namespace) {
    int size = q - uri + 1;
    assert(size > 0);
    if (!(namespacep = malloc(size)))
     FAILCODE(dliteMemoryError, "allocation failure");
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


/********************************************************************
 * Parsing options
 ********************************************************************/

/*
  Parses the options string `options` and assign corresponding values
  of the array `opts`.  The options string should be a valid url query
  string of the form

      key1=value1;key2=value2...

  where the values should be encoded with `uri_encode()` and
  terminated by NUL or any of the characters in ";&#".  A hash (#)
  terminates the options.

  At return, `options` is modified. All values in the `options` string
  will be URI decoded and NUL-terminated.

  `opts` should be a NULL-terminated DLiteOpt array initialised with
  default values.  At return, the values of the provided options are
  updated.

  Returns non-zero on error.
*/
int dlite_option_parse(char *options, DLiteOpt *opts, DLiteOptFlag flags)
{
  char *p = options;
  char *buf = NULL;
  int status = 0;
  if (!options || !*options) return 0;
  if (!(buf = malloc(strlen(options)+1))) {
    status = err(dliteMemoryError, "allocation failure");
      goto fail;
  }
  while (*p && *p != '#') {
    size_t i, len = strcspn(p, "=;&#");
    if (p[len] != '=') {
      status = errx(dliteValueError,
                    "no value for key '%.*s' in option string '%s'",
                    (int)len, p, options);
      goto fail;
    }
    for (i=0; opts[i].key; i++) {
      if (strncmp(opts[i].key, p, len) == 0 && strlen(opts[i].key) == len) {
        p += len;
        if (*p == '=') p++;
        opts[i].value = p;
        size_t n = strcspn(p, ";&#");
        size_t m = uri_decode(p, n, buf);
        assert(m <= n);  // decoding should never increase the string length
        if (m < n) memcpy(p, buf, m);
        char *q = p + m;
        p += n;
        if (*p && strchr(";&", *p)) p++;
        *q = '\0';
        break;
      }
    }
    if (!opts[i].key) {
      if (flags & dliteOptStrict) {
        int len = strcspn(p, "=;&#");
        status = errx(dliteValueError, "unknown option key: '%.*s'", len, p);
        goto fail;
      } else {
        p += strcspn(p, ";&#");
        if (*p && strchr(";&", *p)) p++;
      }
    }
  }
 fail:
  if (buf) free(buf);
  return status;
}


/********************************************************************
 * Path handling
 ********************************************************************/

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
  if (winpath && strlen(url) > 3 &&
      ((url[0] >= 'A' && url[0] <= 'Z') || (url[0] >= 'a' && url[0] <= 'z')) &&
      url[1] == ':' &&
      (url[2] == '\\' || url[2] == '/')) {
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
  Returns non-zero if paths refer to build root rather than the
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
        if (v >= 0)
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

/*
  Returns pointer to the DLite package installation root.

  It may be altered with environment variable DLITE_PKG_ROOT.
  With Python, it defaults to the DLite Python root directory, otherwise
  it defaults to DLITE_ROOT.

*/
const char *dlite_pkg_root_get(void)
{
  static char *dlite_pkg_root = NULL;
  if (!dlite_pkg_root) {
    char *v = getenv("DLITE_PKG_ROOT");
    dlite_pkg_root = (v) ? v :
#ifdef WITH_PYTHON
      dlite_python_root();
#else
      DLITE_PKG_ROOT;
#endif
  }
  return dlite_pkg_root;
}


/* Help function for dlite_add_dll_path() */
#ifdef WINDOWS
static void _add_dll_dir(const char *path)
{
  size_t n;
  wchar_t wcstr[256];
  mbstowcs_s(&n, wcstr, 256, path, 255);
  AddDllDirectory(wcstr);
}

/* Help function for dlite_add_dll_path() */
static void _add_dll_paths(const char *paths)
{
  char buf[4096];
  if (fu_winpath(paths, buf, sizeof(buf), NULL)) {
    char *endptr=NULL;
    const char *p;
    while ((p = fu_nextpath(buf, &endptr, NULL))) {
      char *s = strndup(p, endptr - p);
      _add_dll_dir(s);
      free(s);
    }
  }
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
  char buf[4096];

  if (called) return 0;
  called = 1;

  if (dlite_use_build_root()) {
    _add_dll_paths(dlite_PATH);
    //_add_dll_paths(dlite_PATH_EXTRA);
  } else {
    char libdir[256];
    snprintf(libdir, sizeof(libdir), "%s/%s", dlite_root_get(),
             DLITE_LIBRARY_DIR);
    _add_dll_dir(fu_winpath(libdir, buf, sizeof(buf), NULL));

  }
#endif
  return 0;
}


/********************************************************************
 * Managing global state
 ********************************************************************/

#define ERR_STATE_ID "err-globals-id"
#define ERR_MASK_ID "err-ignored-id"


/* Local state for this module. */
typedef struct {
  int in_atexit;           // whether we are in atexit()
  int finalizing;          // whether dlite is finalising
  int warnings_hide;       // whether to hide warnings
  char *warnings_pattern;  // If given and `warnings_hide` is true, hide
                           // warnings matching the glob pattern.  Otherwise,
                           // if `warnings_hide` is false, show only
                           // warnings matching the glob pattern.
} Locals;


/* A cache pointing to the current session handler */
static DLiteGlobals *_globals_handler=NULL;

/* Pointer to local state */
Locals *_locals=NULL;

/* Free variables in local state. */
static void free_locals()
{
  if (_locals && _locals->warnings_pattern) free(_locals->warnings_pattern);
  _locals = NULL;
}

/* Return a pointer to local state. */
static Locals *get_locals(void)
{
  if (!_locals) {
    static Locals locals;
    _locals = &locals;
    memset(_locals, 0, sizeof(Locals));
  }
  return _locals;
}


/* Called by atexit(). */
static void _handle_atexit(void) {

  /* No extra finalisation is needed if we already are in an atexit handler */
  if (dlite_globals_in_atexit() && !getenv("DLITE_ATEXIT_FREE")) return;

  /* Mark that we are in an atexit handler */
  dlite_globals_set_atexit();

  dlite_finalize();
}

/*
  Returns reference to globals handle.
*/
DLiteGlobals *dlite_globals_get(void)
{
  if (!_globals_handler) {
    _globals_handler = session_get_default();

    dlite_init();

    /* Make valgrind and other memory leak detectors happy by freeing
       up all globals at exit. */
    if (!dlite_globals_in_atexit())
      atexit(_handle_atexit);
  }
  return _globals_handler;
}

/*
  Set globals handle.  Should be called as the first thing by dynamic
  loaded plugins.
*/
void dlite_globals_set(DLiteGlobals *globals_handler)
{
  void *g;
  session_set_default((Session *)globals_handler);
  _globals_handler = globals_handler;

  /* Set globals in utils/err.c */
  if ((g = dlite_globals_get_state(ERR_STATE_ID)))
    err_set_state(g);
}

/*
  Return parameters controlling whether warnings should be hidden.

  See dlite_set_warnings_hide() for a description of these parameters.

  If `*pattern` is not NULL, it is assigned to a static pointer to
  `warnings_pattern` (owned by DLite).

  Returns `warnings_hide`.
 */
int dlite_get_warnings_hide(const char **pattern)
{
  Locals *locals = get_locals();
  if (pattern) *pattern = locals->warnings_pattern;
  return locals->warnings_hide;
}

/*
  Set parameters controlling whether warnings should be hidden.

  Warning parameters:
    - `warnings_hide`: whether to hide warnings (see below).
    - `warnings_pattern`: glob pattern matching the warning message.

  If `warnings_pattern` is NULL, warnings are hidden if `warnings_hide`
  is non-zero.

  If `warnings_pattern` is given, then warnings are hidden if:
    - `warnings_pattern` match the warning message and `warnings_hide` is
      non-zero.
    - `warnings_pattern` don't match the warning message and `warnings_hide`
      is zero.
 */
void dlite_set_warnings_hide(int hide, char *pattern)
{
  Locals *locals = get_locals();
  locals->warnings_hide = hide;
  if (locals->warnings_pattern) free(locals->warnings_pattern);
  locals->warnings_pattern = (pattern) ? strdup(pattern) : NULL;
}


/* Error handler for DLite.

   Since errors
   Print warnings, but not errors unless DLITE_PYDEBUG is set.
 */
static void dlite_err_handler(const ErrRecord *record)
{
  Locals *locals = get_locals();

  if (getenv("DLITE_DEBUG")
#ifdef WITH_PYTHON
      || getenv("DLITE_PYDEBUG")
#endif
      ) {
    err_default_handler(record);
    return;
  }

  switch (record->level) {
  case errLevelSuccess:
    break;
  case errLevelWarn:
    if (locals->warnings_pattern) {
      int match = !globmatch(locals->warnings_pattern, record->msg);
      if ((match && !locals->warnings_hide) ||
          (!match && locals->warnings_hide))
        err_default_handler(record);
    } else if (!locals->warnings_hide) {
      err_default_handler(record);
    }
    break;
  case errLevelError:
    if (!dlite_err_ignored_get(record->eval))
      err_default_handler(record);
    break;
  case errLevelException:
  case errLevelFatal:
    err_default_handler(record);
    break;
  }
}

/* dlite_errname() with correct call signature */
static const char *_errname(int eval) { return dlite_errname(eval); }

/*
  Initialises dlite. This function may be called several times.
 */
void dlite_init(void)
{
  static int initialized = 0;

  if (!initialized) {
    initialized = 1;

    /* Call get_locals() to ensure that the local state is initialised. */
    get_locals();

    /* Seed random number generator */
    srand_msws32(0);
    srand_msws64(0);

    /* Set up global state for utils/err.c */
    if (!dlite_globals_get_state(ERR_STATE_ID))
      dlite_globals_add_state(ERR_STATE_ID, err_get_state(), NULL);

    /* Set up error handling */
    err_set_handler(dlite_err_handler);
    err_set_nameconv(_errname);
  }
}


/*
  Finalises DLite. Will be called by atexit().

  This function may be called several times.
 */
void dlite_finalize(void)
{
  Session *s = session_get_default();
  Locals *locals = get_locals();

  /* Ensure that we only finalize once. */
  if (locals->finalizing) return;
  locals->finalizing = 1;

  /* Don't free anything if we are in an atexit handler */
  if (dlite_globals_in_atexit() && !getenv("DLITE_ATEXIT_FREE")) return;

  /* Reset error handling */
  err_set_handler(NULL);
  err_set_nameconv(NULL);

  session_free(s);
  _globals_handler = NULL;

  free_locals();
}



/*
  Add global state with given name.

  `ptr` is a pointer to the state and `free_fun` is a function that frees it.
  Returns non-zero on error.
 */
int dlite_globals_add_state(const char *name, void *ptr,
                            void (*free_fun)(void *ptr))
{
  Session *s = (Session *)dlite_globals_get();
  return session_add_state(s, name, ptr, free_fun);
}


/*
  Remove global state with the given name.
  Returns non-zero on error.
 */
int dlite_globals_remove_state(const char *name)
{
  Session *s = (Session *)dlite_globals_get();
  return session_remove_state(s, name);
}

/*
  Returns global state with given name or NULL on error.
 */
void *dlite_globals_get_state(const char *name)
{
  Session *s = (Session *)dlite_globals_get();
  return session_get_state(s, name);
}

/*
  Returns non-zero if we are in an atexit handler.
 */
int dlite_globals_in_atexit(void)
{
  Locals *locals = get_locals();
  return locals->in_atexit;
}

/*
  Mark that we are in an atexit handler.
 */
void dlite_globals_set_atexit(void)
{
  Locals *locals = get_locals();
  locals->in_atexit = 1;
}


/********************************************************************
 * Wrappers around error functions
 ********************************************************************/


/* Returns pointer to bit flags for error codes to not print or NULL
   on error. */
DLiteErrMask *_dlite_err_mask_get(void)
{
  DLiteErrMask *mask = dlite_globals_get_state(ERR_MASK_ID);
  if (!mask) {
    // Check that if we have fewer error codes than bits in DLiteErrMask
    assert(8*sizeof(DLiteErrMask) > -dliteLastError);

    if (!(mask = calloc(1, sizeof(DLiteErrMask))))
      return fprintf(stderr, "** allocation failure"), NULL;
    dlite_globals_add_state(ERR_MASK_ID, mask, free);
  }
  return mask;
}

/* Set mask for error to not print. */
void _dlite_err_mask_set(DLiteErrMask mask)
{
  DLiteErrMask *errmask = _dlite_err_mask_get();
  if (errmask) *errmask = mask;
}


/* Set whether to ignore printing given error code. */
void dlite_err_ignored_set(DLiteErrCode code, int value)
{
  DLiteErrMask *mask = _dlite_err_mask_get();
  int bit = DLITE_ERRBIT(code);
  if (mask) {
    if (value)
      *mask |= bit;
    else
      *mask &= ~bit;
  }
}

/* Return whether printing is ignored for given error code. */
int dlite_err_ignored_get(DLiteErrCode code)
{
  DLiteErrMask *mask = _dlite_err_mask_get();
  if (!mask) return 0;
  if (code > 0 && (*mask & DLITE_ERRBIT(dliteUnknownError))) return 1;
  return (int)(*mask) & DLITE_ERRBIT(code);
}


/* ----------------------------------------------------------- */
/* TODO:
 * Add explanation for why we need these functions and do not
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
FILE *dlite_err_get_stream(void) { return err_get_stream(); }
void dlite_err_set_stream(FILE *stream) { err_set_stream(stream); }

/* Like dlite_err_set_stream(), but takes a filename instead of a stream. */
void dlite_err_set_file(const char *filename)
{
  FILE *fp;
  if (!filename || !*filename)
    err_set_stream(NULL);
  else if (strcmp(filename, "<stdout>"))
    err_set_stream(stdout);
  else if (strcmp(filename, "<stderr>"))
    err_set_stream(stderr);
  else if ((fp = fopen(filename, "a")))
    err_set_stream(fp);
  else
    err(1, "cannot open error file: %s", filename);
}

int dlite_err_set_warn_mode(int mode) { return err_set_warn_mode(mode); }
int dlite_err_get_warn_mode(void) { return err_get_warn_mode(); }
int dlite_err_set_debug_mode(int mode) { return err_set_debug_mode(mode); }
int dlite_err_get_debug_mode(void) { return err_get_debug_mode(); }
int dlite_err_set_override_mode(int mode) {return err_set_override_mode(mode);}
int dlite_err_get_override_mode(void) { return err_get_override_mode(); }



/*
  Issues a deprecation warning.

  `version_removed` is the version the deprecated feature is expected
  to be finally removed.
  `descr` is a description of the deprecated feature.

  Returns non-zero on error.
 */
int _dlite_deprecation_warning(const char *version_removed,
                               const char *filepos, const char *func,
                               const char *descr)
{
  static int initialised=0;
  static map_int_t reported;

  /* Return if this deprecation warning has already been issued. */
  if (!initialised) {
    map_init(&reported);
    initialised = 1;
  }
  if (map_get(&reported, filepos)) return 0;
  map_set(&reported, filepos, 1);

  dlite_warnx("deprecated: %s", descr);

  /* Check that version numbers are semantic.  */
  if (strchk_semver(version_removed) < 0)
    return dlite_errx(dliteSystemError,
                      "argument version_removed=\"%s\" of %s must be a valid "
                      "semantic version number. In %s",
                      version_removed, func, filepos);
  if (strchk_semver(dlite_VERSION) < 0)
    return dlite_errx(dliteSystemError,
                      "DLite version number is not semantic '%s'",
                      dlite_VERSION);

  /* Issue a system error if `version_removed` has passed.  */
  if (strcmp_semver(version_removed, dlite_VERSION) <= 0)
    return dlite_errx(dliteSystemError, "Deprecated feature was supposed to be "
                "removed in version %s: %s", version_removed, descr);

  return 0;
}
