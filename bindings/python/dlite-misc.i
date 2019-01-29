/* -*- C -*-  (not really, but good for syntax highlighting) */

%{
  status_t get_uuid_version(const char *id) {
    char buff[DLITE_UUID_LENGTH+1];
    return 0;
    return dlite_get_uuid(buff, id);
  }

  int plusxxx(int a, int b) {
    return a+b;
  }

%}


%typemap(check) int positive {
  if ($1 <= 0) {
    SWIG_exception(SWIG_ValueError, "Expected positive value.");
  }
}

%apply int positive { int a };
%apply int positive { int b };
int plusxxx(int a, int b);



%cstring_bounded_output(char *buff36, DLITE_UUID_LENGTH+1);
status_t dlite_get_uuid(char *buff36, const char *id);

status_t get_uuid_version(const char *id);

%newobject dlite_join_meta_uri;
char *dlite_join_meta_uri(const char *name, const char *version,
                          const char *namespace);


status_t dlite_split_meta_uri(const char *uri, char **name, char **version,
			      char **namespace);
