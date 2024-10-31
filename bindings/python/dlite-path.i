/* -*- C -*-  (not really, but good for syntax highlighting) */

/* --------
 * Typemaps
 * -------- */

/* A typemap that allows using pathlib as input to paths */
%typemap(in) (const char *path) {
  if (PyUnicode_Check($input)) {
    $1 = (char *)PyUnicode_AsUTF8($input);
  } else {
    PyObject *s = PyObject_Str($input);
    $1 = (char *)PyUnicode_AsUTF8(s);
    Py_DECREF(s);
  }
}

/* --------
 * Wrappers
 * -------- */
%{
#include <string.h>

  //#include "utils/compat.h"
#include "utils/fileutils.h"
  //#include "dlite-macros.h"
#include "dlite-codegen.h"
#include "pyembed/dlite-python-storage.h"
#include "pyembed/dlite-python-mapping.h"
#include "pyembed/dlite-python-protocol.h"

const char *platforms[] = {"native", "unix", "windows", "apple"};
%}


/* -----
 * _Path
 * ----- */
%rename(FUPath) _FUPaths;
struct _FUPaths {
  %immutable;
  int n;
};


%feature("docstring", "\
Creates a _Path instance of type `pathtype`.
") _FUPaths;
%extend struct _FUPaths {
  _FUPaths(const char *pathtype) {
    if (strcmp(pathtype, "storages") == 0) {
      return dlite_storage_paths();
    } else if (strcmp(pathtype, "templates") == 0) {
      return dlite_codegen_path_get();
    } else if (strcmp(pathtype, "storage-plugins") == 0) {
      return dlite_storage_plugin_paths_get();
    } else if (strcmp(pathtype, "mapping-plugins") == 0) {
      return dlite_mapping_plugin_paths_get();
    } else if (strcmp(pathtype, "python-storage-plugins") == 0) {
      return dlite_python_storage_paths();
    } else if (strcmp(pathtype, "python-mapping-plugins") == 0) {
      return dlite_python_mapping_paths();
    } else if (strcmp(pathtype, "python-protocol-plugins") == 0) {
      return dlite_python_protocol_paths();
    } else {
      return dlite_err(1, "invalid pathtype: %s", pathtype), NULL;
    }
  }

  ~_FUPaths(void) {
    (void)$self;
    //dlite_warnx("Paths objects are not destroyed...");
  }

  char *__repr__(void) {
    return fu_paths_string($self);
  }
  int __len__(void) {
    return (int)$self->n;
  }
  const char *getitem(int index) {
    if (index < 0) index += (int)$self->n;
    if (index < 0 || index >= (int)$self->n)
      return dlite_err(1, "index out of range: %d", index), NULL;
    return $self->paths[index];
  }
  void __setitem__(int index, const char *path) {
    if (index < 0) index += (int)$self->n;
    if (index < 0 || index >= (int)$self->n) {
      dlite_err(1, "index out of range: %d", index);
    } else {
      fu_paths_remove_index($self, index);
      fu_paths_insert($self, path, index);
    }
  }
  void __delitem__(int index) {
    fu_paths_remove_index($self, index);
  }

  void insert(int index, const char *path) {
    fu_paths_insert($self, path, index);
  }
  void append(const char *path) {
    fu_paths_append($self, path);
  }
  void extend(const char *paths, const char *pathsep=NULL) {
    fu_paths_extend($self, paths, pathsep);
  }

  const char *get_platform(void) {
    if ($self->platform < 0 || $self->platform >= (int)countof(platforms))
      return "unknown";
    return platforms[$self->platform];
  }
  void set_platform(const char *platform) {
    size_t i;
    const char **q=platforms;
    for (i=0; i < countof(platforms); i++, q++)
      if (strcmp(*q, platform) == 0) {
        $self->platform = i;
        break;
      }
  }

}


/* -----------------------------------
 * Target language-spesific extensions
 * ----------------------------------- */
#ifdef SWIGPYTHON
%include "dlite-path-python.i"
#endif
