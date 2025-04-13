/* -*- C -*-  (not really, but good for syntax highlighting) */

%{
  posstatus_t from_typename(const char *typename, int *size) {
    size_t v = 0;
    DLiteType type=-1, *dt=&type;
    if (dlite_type_set_dtype_and_size(typename, dt, &v))
      return -1;
    *size = (int)v;
    return type;
  }

  char *to_typename(int type, int size) {
    char *s;
    if (!(s = malloc(16))) return NULL;
    if (size < 0) {
      //s = strdup(dlite_type_get_enum_name(type));
      s = strdup(dlite_type_get_dtypename(type));
    } else if (dlite_type_set_typename(type, size, s, 16)) {
      free(s);
      return NULL;
    }
    return s;
  }
%}


/* -----
 * Types
 * ----- */
%rename(Type) DLiteType;
%rename("%(regex:/dlite(.*)/\\1Type/)s", %$isenumitem) "";
enum _DLiteType {
  dliteBlob,             /*!< Binary blob, sequence of bytes */
  dliteBool,             /*!< Boolean */
  dliteInt,              /*!< Signed integer */
  dliteUInt,             /*!< Unigned integer */
  dliteFloat,            /*!< Floating point */
  dliteFixString,        /*!< Fix-sized NUL-terminated string */
  dliteStringPtr,        /*!< Pointer to NUL-terminated string */
  dliteRef,              /*!< Pointer to another instance */

  dliteDimension,        /*!< Dimension, for entities */
  dliteProperty,         /*!< Property, for entities */
  dliteRelation,         /*!< Subject-predicate-object relation */
};


%apply int *OUTPUT { int *TYPESIZE };
%feature(
  "docstring",
  "Returns type number and size from given type name."
) from_typename;
posstatus_t from_typename(const char *typename, int *TYPESIZE);

%newobject to_typename;
%feature(
  "docstring",
  "Returns type name for given type number and size. "
  "If `size` is negative, only the name of `type` is returned."
) to_typename;
char *to_typename(int type, int size=-1);

%feature(
  "docstring",
  "Returns DLite type number corresponding to `dtypename`."
) dlite_type_get_dtype;
%rename(to_typenumber) dlite_type_get_dtype;
int dlite_type_get_dtype(const char *dtypename);


%rename(get_alignment) dlite_type_get_alignment;
size_t dlite_type_get_alignment(int type, size_t size);
