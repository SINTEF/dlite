#include <assert.h>

#include "utils/strtob.h"
#include "utils/strutils.h"
#include "dlite.h"
#include "dlite-behavior.h"


/* Table listing all defined behaviors.

   name: Name of behavior.  Should be a unique and valid Python identifier
       of at most 48 characters length.
   v_added: Version number when the behavior was added.
   v_new: Version number when the new behavior will be the default.
   v_removed: Expected version number when the behavior is removed.
   description: Full description of the new feature.
   value: Just set it to zero. It will be initialised the first time the table
       is accessed.
*/

//  name                           v_added    v_new      v_removed
//  description
static DLiteBehavior behavior_table[] = {
  { "singleInterpreter",           "0.5.17",  "0.7.0",   "0.9.0",
    "Evaluate Python plugins from calling interpreter when DLite is called "
    "from Python.  The old behavior is to call the plugins from an internal "
    "interpreter", -1 },
  { "storageQuery",                "0.5.23",  "0.6.0",   "0.8.0",
    "Fix typo and rename method queue() to query() in storage plugins.", -1},
  { NULL,                          NULL,      NULL,      NULL,     NULL, 0 }
};



/*
  Initialise the behavior table. Will be called automatically, so no need
  to call it explicitly.  It a noop after the first call.

  Default values can currently only be configured via environment
  variables.  If the environment variable `DLITE_BEHAVIOR_<name>` is
  defined, the behavior whos name is `<name>` is assigned.  An
  environment variable with no value is interpreted as true.

  If no default is given, the behavior will be disabled (value=0) if
  the current DLite version is less than `version_new` and enabled
  (value=1) otherwise.
*/
void dlite_behavior_table_init(void)
{
  static int initialised=0;

  if (!initialised) {
    DLiteBehavior *b = (DLiteBehavior *)behavior_table;
    while (b->name) {
      const char *env;
      b->value = -1;

      /* Check environment variable: DLITE_BEHAVIOR */
      if ((env = getenv("DLITE_BEHAVIOR"))) b->value = (*env) ? atob(env) : 1;

      /* Check environment variable: DLITE_BEHAVIOR_<name> */
      char buf[64];
      snprintf(buf, sizeof(buf), "DLITE_BEHAVIOR_%s", b->name);
      if ((env = getenv(buf))) b->value = (*env) ? atob(env) : 1;

      /* Warn if behavior is expected to be removed. */
      if (strcmp_semver(dlite_get_version(), b->version_remove) >= 0)
        dlite_warn("Behavior `%s` was scheduled for removal since version %s",
                   b->name, b->version_remove);

      b++;
    }
    initialised = 1;
  }
}


/*
  Return the number of registered behaviors.
*/
size_t dlite_behavior_nrecords(void)
{
  return sizeof(behavior_table) / sizeof(DLiteBehavior) - 1;
}


/*
  Return a pointer to record with the given number or NULL if `n` is out of
  range.
*/
const DLiteBehavior *dlite_behavior_recordno(size_t n)
{
  size_t N = dlite_behavior_nrecords();
  if (n > N) return NULL;
  return behavior_table + n;
}


/*
  Return a pointer to the given behavior record, or NULL if `name` is
  not in the behavior table.

  Note: Please use dlite_behavior_get() to access the record value,
  since it may not be fully initialised by this function.
 */
const DLiteBehavior *dlite_behavior_record(const char *name)
{
  const DLiteBehavior *b = behavior_table;
  dlite_behavior_table_init();
  while (b->name) {
    if (strcmp(b->name, name) == 0) return b;
    b++;
  }
  return NULL;
}


/*
  Return the value of given behavior or a netative error code on error.
 */
int dlite_behavior_get(const char *name)
{
  DLiteBehavior *b = (DLiteBehavior *)dlite_behavior_record(name);
  if (!b) return dlite_err(dliteNameError, "No behavior with name: %s", name);

  /* If value is unset, enable behavior if DLite version > version_new */
  if (b->value < 0) {
    const char *ver = dlite_get_version();  // current version
    b->value = (strcmp_semver(ver, b->version_new) >= 0) ? 1 : 0;

    dlite_warnx("Behavior change `%s` is not configured. "
                "It will be enabled by default from v%s. "
                "See https://sintef.github.io/dlite/user_guide/configure_behavior_changes.html for more info.",
                b->name, b->version_new);
  }

  assert(b->value >= 0);
  return b->value;
}


/*
  Assign value of given behavior: 1=on, 0=off.

  Returns non-zero on error.
*/
int dlite_behavior_set(const char *name, int value)
{
  DLiteBehavior *b = (DLiteBehavior *)dlite_behavior_record(name);
  if (!b) return dlite_err(dliteNameError, "No behavior with name: %s", name);
  b->value = value;
  return 0;
}
