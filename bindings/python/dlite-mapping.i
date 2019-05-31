/* -*- C -*-  (not really, but good for syntax highlighting) */

/*
  TODO:
    - add typemap for (const DLiteInstance **instances, int n)
    - add tests
    - check dlite_python_mapping_paths_insert()
*/

%{
#include "dlite-mapping.h"
#include "pyembed/dlite-python-mapping.h"
%}

%rename("%(strip:[dlite_])s") "";

%feature("docstring", "\
  Returns a new instance of metadata `output_uri` by mapping the `n` input
  instances in the array `instances`.

  This is the main function in the mapping api.
") dlite_mapping;
DLiteInstance *dlite_mapping(const char *output_uri,
                             const DLiteInstance **instances, int n);


/* Python mapping paths */

%feature("docstring", "\
  Clears Python mapping search path.
") dlite_python_mapping_paths_clear;
void dlite_python_mapping_paths_clear(void);

%feature("docstring", "\
  Inserts `path` into Python mapping paths before position `n`. If
  `n` is negative, it counts from the end (like Python).

  Returns the index of the newly inserted element or -1 on error.
") dlite_python_mapping_paths_insert;
int dlite_python_mapping_paths_insert(const char *path, int n);

%feature("docstring", "\
  Appends `path` to Python mapping paths.
  Returns the index of the newly appended element or -1 on error.
") dlite_python_mapping_paths_append;
int dlite_python_mapping_paths_append(const char *path);

%feature("docstring", "\
  Removes path index `n` to Python mapping paths.
  Returns non-zero on error.
") dlite_python_mapping_paths_remove;
int dlite_python_mapping_paths_remove(int n);

%feature("docstring", "\
  Returns a pointer to the current Python mapping plugin search path
  or NULL on error.
") dlite_python_mapping_paths_get;
const char **dlite_python_mapping_paths_get(void);



%feature("docstring", "\
  Loads all Python mappings (if needed).

  Returns a borrowed reference to a list of mapping plugins (casted to
  void *) or NULL on error.
") dlite_python_mapping_load;
void *dlite_python_mapping_load(void);

%feature("docstring", "\
  Unloads all currently loaded mappings.
") dlite_python_mapping_unload;
void dlite_python_mapping_unload(void);
