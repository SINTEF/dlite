/* -*- C -*-  (not really, but good for syntax highlighting) */

%{
#include "dlite.h"
#include "dlite-errors.h"
#include "dlite-json.h"

  /* If store `js` only has one instance, return its id, otherwise raise a
     DLiteLookupError. */
  static const char *_single_id(JStore *js)
  {
    const char *key = jstore_get_single_key(js);
    if (key) return key;
    return dlite_err(dliteLookupError,
                     "get_single() expect exactly 1 item in the storage. Got %d",
                     jstore_count(js)), NULL;
  }
%}


/* JStore iterator */
struct _DLiteJStoreIter {};
%feature("docstring", "\
Iterates over instances in JSON store `js`.  If `pattern` is given, only
instances whos metadata URI matches `pattern` are returned.
") _DLiteJStoreIter;
%extend _DLiteJStoreIter {
  _DLiteJStoreIter(struct _JStore *js, const char *pattern=NULL) {
    return dlite_jstore_iter_create(js, pattern);
  }
  ~_DLiteJStoreIter(void) {
    dlite_jstore_iter_free($self);
  }

  const char *next(void) {
    return dlite_jstore_iter_next($self);
  }
}


/* JStore */
struct _JStore {};
%feature("docstring", "Store for JSON data.") _JStore;
%rename(JStore) _JStore;
%extend _JStore {
  _JStore(void) {
    return jstore_open();
  }
  ~_JStore(void) {
    jstore_close($self);
  }

  %feature("docstring", "Remove instance with given `id`.") remove;
  void remove(const char *id) {
    dlite_jstore_remove($self, id);
  }

  %feature("docstring", "Load JSON content from file into the store.") loadf;
  void load_file(const char *INPUT, size_t LEN) {
    dlite_jstore_loadf($self, INPUT);
  }

  %feature("docstring",
           "Load JSON string into the store. "
           "Beware that this function has no validation of the input.") loads;
  void load_json(const char *INPUT, size_t LEN) {
    dlite_jstore_loads($self, INPUT, LEN);
  }

  %feature("docstring", "Add json representation of `inst` to store.") add;
  void add(const struct _DLiteInstance *inst) {
    dlite_jstore_add($self, inst, 0);
  }

  %feature("docstring",
           "Return instance with given `id` from store. "
           "If `id` is None and there is exactly one instance in the store, "
           "return the instance.  Otherwise raise an DLiteLookupError.") get;
  %newobject get;
  struct _DLiteInstance *_get(const char *id=NULL) {
    if (!id && !(id = _single_id($self))) return NULL;
    return dlite_jstore_get($self, id);
  }

  %feature("docstring", "Return JSON string for given id from store.") get_json;
  const char *get_json(const char *id) {
    const char *s;
    if (!id && !(id = _single_id($self))) return NULL;
    if (!(s = jstore_get($self, id))) {
      char uuid[DLITE_UUID_LENGTH+1];
      if (dlite_get_uuid(uuid, id) < 0) return NULL;
      s = jstore_get($self, uuid);
    }
    return s;
  }

  %feature("docstring",
           "If there is one instance in storage, return its id. "
           "Otherwise, raise an DLiteLookupError exception.") get_single_id;
  %newobject get_single_id;
  const char *get_single_id(void) {
    return _single_id($self);
  }

  %feature("docstring", "Iterate over all id's matching pattern.") get_ids;
  struct _DLiteJStoreIter *get_ids(const char *pattern=NULL) {
    return dlite_jstore_iter_create($self, pattern);
  }

  int __len__(void) {
    return jstore_count($self);
  }

  bool __bool__(void) {
    return jstore_count($self) > 0;
  }

  struct _JStore *__iadd__(struct _JStore *other) {
    if (jstore_update($self, other))
      return dlite_err(dliteTypeError, "Cannot update store"), NULL;
    return $self;
  }

}


/* -----------------------------------
 * Target language-spesific extensions
 * ----------------------------------- */
#ifdef SWIGPYTHON
%include "dlite-jstore-python.i"
#endif
