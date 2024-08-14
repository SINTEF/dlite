#include "utils/strtob.h"
#include "utils/strutils.h"
#include "dlite.h"
#include "dlite-behavior.h"


/* Table listing all defined behaviors. The name should be max 48 characters. */

//  name                           v_added    v_new      v_removed
//  description
static DLiteBehavior behavior_table[] = {
  { "singleInterpreter",           "0.5.17",  "0.7.0",   "0.9.0",
    "Plugins are executed by the calling interpreter when DLite is called "
    "from Python.", -1 },
  { NULL,                          NULL,      NULL,      NULL,     NULL, 0 }
};


//#define GLOBALS_ID "dlite-behavior-id"
//
///* Global variables for dlite-storage */
//typedef struct {
//  map_int_t behavior_values;  /* Maps behavior names to their values. */
//} Globals;
//
///* Frees global state for this module - called by atexit() */
//static void free_globals(void *globals)
//{
//  Globals *g = globals;
//  map_deinit(&g->behavior_values);
//  free(g);
//}
//
///* Return a pointer to global state for this module */
//static Globals *get_globals(void)
//{
//  Globals *g = dlite_globals_get_state(GLOBALS_ID);
//  if (!g) {
//    if (!(g = calloc(1, sizeof(Globals))))
//     FAILCODE(dliteMemoryError, "allocation failure");
//    map_init(&g->behavior_values);
//    dlite_globals_add_state(GLOBALS_ID, g, free_globals);
//  }
//  return g;
// fail:
//  if (g) free(g);
//  return NULL;
//}



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
 */
const DLiteBehavior *dlite_behavior_record(const char *name)
{
  const DLiteBehavior *b = behavior_table;
  while (b->name) {
    if (strcmp(b->name, name) == 0) {
      // Update from environment
      //dlite_behavior_get
      return b;
    }
    b++;
  }
  return NULL;
}


/*
  Get value of given behavior.

  If the behavior is unset, the environment variable `DLITE_BEHAVIOR_<name>`
  is checked.  If it is set with no value means on.

  Returns 1 if the behavior is on, 0 if it is off and a negative
  value on error.
 */
int dlite_behavior_get(const char *name)
{
  char buf[64];
  //Globals *g = get_globals();
  //int *value = map_get(&g->behavior_values, name);
  //return (value) ? *value : -1;
  const DLiteBehavior *b = dlite_behavior_record(name);
  if (!b) return dlite_err(dliteKeyError, "No behavior with name: %s", name);
  if (b->value >= 0) return b->value;

  snprintf(buf, sizeof(buf), "DLITE_BEHAVIOR_%s", name);
  const char *env = getenv(buf);
  if (env) return (*env) ? atob(env) : 1;

  if (strcmp_semver(dlite_get_version(), b->version_new) < 0) return 0;
  return 1;
}


/*
  Assign value of given behavior: 1=on, 0=off, -1=unset.

  Returns non-zero on error.
*/
int dlite_behavior_set(const char *name, int value)
{
  //Globals *g = get_globals();
  //if (value < 0)
  //  map_remove(&g->behavior_values, name);
  //else if (value == 0)
  //  map_set(
  //int *value = map_get(&g->behavior_values, name);
  DLiteBehavior *b = (DLiteBehavior *)dlite_behavior_record(name);
  if (!b) return dlite_err(dliteKeyError, "No behavior with name: %s", name);
  b->value = (value > 0) ? 1 : (value == 0) ? 0 : -1;
  return 0;
}
