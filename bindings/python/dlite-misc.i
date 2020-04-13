/* -*- C -*-  (not really, but good for syntax highlighting) */

%{
  #include "utils/globmatch.h"

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

%}

%feature("docstring", "\
Returns DLite version.
") dlite_get_version;
const char *dlite_get_version(void);


%feature("docstring", "\
Returns an UUID, depending on:
  - If `id` is NULL or empty, generates a new random version 4 UUID.
  - If `id` is not a valid UUID string, generates a new version 5 sha1-based
    UUID from `id` using the DNS namespace.
  - Otherwise return `id` (which already must be a valid UUID).
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
     *       any number of characters
     ?       any single character
     [a-z]   any single character in the range a-z
     [^a-z]  any single character not in the range a-z
     \x      match x
") globmatch;
int globmatch(const char *pattern, const char *s);
