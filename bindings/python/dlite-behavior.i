/* -*- C -*-  (not really, but good for syntax highlighting) */
%{
#include "dlite-behavior.h"
%}

%rename(_BehaviorRecord) _DLiteBehavior;
struct _DLiteBehavior {
  %immutable;
  char *name;
  char *version_added;
  char *version_new;
  char *version_remove;
  char *description;
  %mutable;
  int value;
};

%rename("%(strip:[dlite])s") "";


%feature("docstring", "\
Return the number of registered behaviors.
") dlite_behavior_nrecords;
size_t dlite_behavior_nrecords(void);

%feature("docstring", "\
Return a pointer to record with the given number or NULL if `n` is out of
range.
") dlite_behavior_recordno;
const struct _DLiteBehavior *dlite_behavior_recordno(size_t n);

%feature("docstring", "\
Return a pointer to the given behavior record, or NULL if `name` is
not in the behavior table.
") dlite_behavior_record;
const struct _DLiteBehavior *dlite_behavior_record(const char *name);

%feature("docstring", "\
Get value of given behavior.

Returns 1 if the behavior is on, 0 if it is off and a negative
value on error.
") dlite_behavior_get;
int dlite_behavior_get(const char *name);

%feature("docstring", "\
Enable given behavior if `value` is non-zero.  Disable if `value` is zero.

Returns non-zero on error.
") dlite_behavior_set;
int dlite_behavior_set(const char *name, int value);


/* -----------------------------------
 * Target language-spesific extensions
 * ----------------------------------- */
#ifdef SWIGPYTHON
%include "dlite-behavior-python.i"
#endif
