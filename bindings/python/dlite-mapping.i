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
