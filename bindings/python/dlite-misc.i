/* -*- C -*-  (not really, but good for syntax highlighting) */

%{
  #include "utils/strtob.h"
  #include "utils/globmatch.h"
  //#include "dlite-misc.h"

  posstatus_t get_uuid_version(const char *id) {
    char buff[DLITE_UUID_LENGTH+1];
    return dlite_get_uuid(buff, id);
  }

  status_t split_url(char *url, char **driver, char **location,
                           char **options, char **fragment) {
    status_t retval=0;
    char *url2, *drv=NULL, *loc=NULL, *opt=NULL, *frg=NULL;
    if (!(url2 = strdup(url))) return dlite_err(1, "allocation failure");
    retval = dlite_split_url(url, &drv, &loc, &opt, &frg);
    if (driver)   *driver   = strdup((drv) ? drv : "");
    if (location) *location = strdup((loc) ? loc : "");
    if (options)  *options  = strdup((opt) ? opt : "");
    if (fragment) *fragment = strdup((frg) ? frg : "");
    free(url2);
    return retval;
  }

  bool asbool(const char *str) {
    return atob(str);
  }

  int64_t _err_mask_get(void) {
    return *_dlite_err_mask_get();
  }

  /* Just check for errors, do nothing else. */
  void errcheck(void) {}
%}


%include <stdint.i>

%feature("docstring", "\
Returns the current version of DLite.
") dlite_get_version;
const char *dlite_get_version(void);

%feature("docstring", "\
Returns DLite licenses information.
") dlite_get_license;
const char *dlite_get_license(void);

%feature("docstring", "\
Returns an UUID, depending on:

- If `id` is NULL or empty, generates a new random version 4 UUID.
- If `id` is not a valid UUID string, generates a new version 5 sha1-based
  UUID from `id` using the DNS namespace.

Otherwise return `id` (which already must be a valid UUID).
") dlite_get_uuid;
%cstring_bounded_output(char *buff36, DLITE_UUID_LENGTH+1);
void dlite_get_uuid(char *buff36, const char *id=NULL);

%feature("docstring", "\
Returns the generated UUID version number if `id` had been passed to
get_uuid() or zero if `id` is already a valid UUID.
") get_uuid_version;
posstatus_t get_uuid_version(const char *id=NULL);

%feature("docstring", "\
Returns a (metadata) uri by combining `name`, `version` and `namespace` as:

    namespace/version/name
") dlite_join_meta_uri;
%newobject dlite_join_meta_uri;
char *dlite_join_meta_uri(const char *name, const char *version,
                          const char *namespace);


%feature("docstring", "\
Returns (name, version, namespace)-tuplet from valid metadata `uri`.
") dlite_split_meta_uri;
%cstring_output_allocate(char **name,      if (*$1) free(*$1));
%cstring_output_allocate(char **version,   if (*$1) free(*$1));
%cstring_output_allocate(char **namespace, if (*$1) free(*$1));
status_t dlite_split_meta_uri(const char *uri, char **name, char **version,
			      char **namespace);

//int dlite_option_parse(char *options, DLiteOpt *opts, int modify);

%feature("docstring", "\
Returns an url constructed from the arguments of the form:

    driver://location?options#fragment

The `driver`, `options` and `fragment` arguments may be None.
") dlite_join_url;
%newobject dlite_join_url;
char *dlite_join_url(const char *driver, const char *location,
                     const char *options=NULL, const char *fragment=NULL);

%feature("docstring", "\
Returns a (driver, location, options, fragment)-tuplet by splitting
`url` of the form

    driver://location?options#fragment

into four parts.
") split_url;
%cstring_output_allocate(char **driver,   if (*$1) free(*$1));
%cstring_output_allocate(char **location, if (*$1) free(*$1));
%cstring_output_allocate(char **options,  if (*$1) free(*$1));
%cstring_output_allocate(char **fragment, if (*$1) free(*$1));
status_t split_url(char *url, char **driver, char **location,
                   char **options, char **fragment);


%feature("docstring", "\
Match string 's' against glob pattern 'pattern' and return zero on
match.

Understands the following patterns:

- `*`: Any number of characters.
- `?`: Any single character.
- `[a-z]`: Any single character in the range a-z.
- `[^a-z]`: Any single character not in the range a-z.
- `\\x`: Match x.
") globmatch;
int globmatch(const char *pattern, const char *s);

%feature("docstring", "Tell DLite that we are in a Python atexit handler.")
         dlite_globals_mark_python_atexit;
void dlite_globals_mark_python_atexit(void);

%feature("docstring", "\
Clear the last error (setting its error code to zero).
") dlite_errclr;
void dlite_errclr(void);

%feature("docstring", "\
Returns the error code (error value) or the last error or zero if no errors
have occured since the last call to dlite.errclr().
") dlite_errval;
int dlite_errval(void);

%feature("docstring", "\
Returns the error message of the last error.  An empty string is returned if
no errors have occured since the last call to dlite.errclr().
") dlite_errmsg;
const char *dlite_errmsg(void);

%feature("docstring", "\
Get current error stream.
") dlite_err_set_file;
FILE *dlite_err_get_stream(void);

%feature("docstring", "\
Set error stream.
") dlite_err_set_file;
void dlite_err_set_stream(FILE *);

%feature("docstring", "\
Set error log file. Special values includes:

- `None` or empty: Turn off error output.
- `<stderr>`: Standard error.
- `<stdout>`: Standard output.

All other values are treated as a filename that will be opened in append mode.
") dlite_err_set_file;
void dlite_err_set_file(const char *filename);

%feature("docstring", "\
Return DLite error code corresponding to `name`.  Unknown names will return
`dliteUnknownError`.
") dlite_errcode;
%rename(_err_getcode) dlite_errcode;
int dlite_errcode(const char *name);

/* Private help functions */
%feature("docstring", "\
Set whether to ignore printing given error code.
") dlite_err_ignored_set;
%rename(_err_ignore_set) dlite_err_ignored_set;
void dlite_err_ignored_set(int code, int value);

%feature("docstring", "\
Return whether printing is ignored for given error code.
") dlite_err_ignored_get;
%rename(_err_ignore_get) dlite_err_ignored_get;
int dlite_err_ignored_get(int code);

int64_t _err_mask_get(void);
%rename(_err_mask_set) _dlite_err_mask_set;
void _dlite_err_mask_set(int64_t mask);

%feature("docstring", "Just check for errors, do nothing else.") errcheck;
void errcheck(void);


/* ------------------------------
 * Expose other utility functions
 * ------------------------------ */
%feature("docstring", "\
Set error stream.
") asbool;
bool asbool(const char *str);



/* -----------------------------------
 * Target language-spesific extensions
 * ----------------------------------- */
#ifdef SWIGPYTHON
%include "dlite-misc-python.i"
#endif
