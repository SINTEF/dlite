/* -*- C -*-  (not really, but good for syntax highlighting) */

%{
  posstatus_t get_uuid_version(const char *id) {
    char buff[DLITE_UUID_LENGTH+1];
    return dlite_get_uuid(buff, id);
  }
%}



%cstring_bounded_output(char *buff36, DLITE_UUID_LENGTH+1);
void dlite_get_uuid(char *buff36, const char *id=NULL);

posstatus_t get_uuid_version(const char *id=NULL);

%newobject dlite_join_meta_uri;
char *dlite_join_meta_uri(const char *name, const char *version,
                          const char *namespace);


status_t dlite_split_meta_uri(const char *uri, char **name, char **version,
			      char **namespace);

//int dlite_option_parse(char *options, DLiteOpt *opts, int modify);

%newobject dlite_join_url;
char *dlite_join_url(const char *driver, const char *location,
                     const char *options=NULL, const char *fragment=NULL);

//%cstring_output_allocate(char **driver,   if (*$1) free(*$1));
//%cstring_output_allocate(char **location, if (*$1) free(*$1));
//%cstring_output_allocate(char **options,  if (*$1) free(*$1));
//%cstring_output_allocate(char **fragment, if (*$1) free(*$1));
//status_t dlite_split_url(char *url, char **driver, char **location,
//                         char **options, char **fragment);
