#ifndef _DLITE_BEHAVIOR_H
#define _DLITE_BEHAVIOR_H

/**
  @file
  @brief Handling of behavior changes in the code

  See https://sintef.github.io/dlite/contributors_guide/code_changes.html
*/


/** Columns in the behavior_table. */
typedef struct _DLiteBehavior {
  char *name;           /*!< Name of behavior.  Should be a unique identifier. */
  char *version_added;  /*!< Version number when the behavior was added. */
  char *version_new;    /*!< Version number when the new behavior is default. */
  char *version_remove; /*!< Expected version when the behavior is removed. */
  char *description;    /*!< Description of the behavior. */
  int value;            /*!< Behavior value: 1=on, 0=off, -1=unset */
} DLiteBehavior;


/**
  Return the number of registered behaviors.
*/
size_t dlite_behavior_nrecords(void);

/**
  Return a pointer to record with the given number or NULL if `n` is out of
  range.
*/
const DLiteBehavior *dlite_behavior_recordno(size_t n);

/**
  Return a pointer to the given behavior record, or NULL if `name` is
  not in the behavior table.

  Note: Please use dlite_behavior_get() to access the record value,
  since it not be fully initialised by this function.
 */
const DLiteBehavior *dlite_behavior_record(const char *name);

/**
  Return the value of given behavior or a netative error code on error.
 */
int dlite_behavior_get(const char *name);

/**
  Assign value of given behavior: 1=on, 0=off.

  Returns non-zero on error.
*/
int dlite_behavior_set(const char *name, int value);



#endif /* _DLITE_BEHAVIOR_H */
