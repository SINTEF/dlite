/* -*- C -*-  (not really, but good for syntax highlighting) */

/*
  TODO:
    - add tests
    - check dlite_python_mapping_paths_insert()
*/

%{
#include "dlite-mapping.h"
#include "pyembed/dlite-python-mapping.h"

struct _DLiteInstance *swig_mapping(const char *output_uri,
                                    struct _DLiteInstance **instances,
                                    int ninstances)
{
 DLiteInstance *inst=NULL;
 int i;

 /* Increate refcount to instances.
    They are decreased again by mapper() */
 for (i=0; i<ninstances; i++) dlite_instance_incref(instances[i]);

 if (!(inst = dlite_mapping(output_uri, (const DLiteInstance **)instances,
                            ninstances)))
      return dlite_err(1, "mapping failed '%s'", output_uri), NULL;
  return inst;
}


%}



%rename(mapping) swig_mapping;
%newobject swig_mapping;
%feature("docstring", "\
  Returns a new instance of metadata `output_uri` by mapping the
  input instances.
") swig_mapping;
struct _DLiteInstance *swig_mapping(const char *output_uri,
                                    struct _DLiteInstance **instances,
                                    int ninstances);

%rename("%(strip:[dlite_])s") "";

%feature("docstring", "\
  Unloads all currently loaded mappings.
") dlite_python_mapping_unload;
void dlite_python_mapping_unload(void);


%feature("docstring", "\
  Unloads mapping plugin with given name.  If `name` is None, all mapping
  plugins are unloaded.
") dlite_python_mapping_unload;
int dlite_mapping_plugin_unload(const char *name=NULL);
