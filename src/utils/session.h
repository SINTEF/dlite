/* session.h -- simple session manager
 *
 * Copyright (c) 2021, SINTEF
 *
 * Distributed under terms of the MIT license.
 */
#ifndef _SESSION_H
#define _SESSION_H

/**
  @file
  @brief Simple session manager.

  The purpose of this session manager is to maintain global states.
  This library supports multiple sessions, but may also be used when you
  only want to maintain a single global state.  In this case, use
  session_get_default() instead of session_create().
 */


/**
  @brief Opaque session type.
*/
typedef struct _Session Session;


/**
  @name Creating, freeing and accessing sessions
  @{
*/

/**
  @brief Create a new session with given `session_id`.

  @return Pointer to the session or NULL on error.
*/
Session *session_create(const char *session_id);

/**
  @brief Free all memory associated with `session`.
*/
void session_free(Session *s);

/**
  @brief Retrieve session from `session_id`.

  @return Pointer to the session or NULL if no session exists with this id.
*/
Session *session_get(const char *session_id);

/**
  @brief Retrive session id from session pointer.

  @return Session id or NULL on error.
 */
const char *session_get_id(Session *s);


/**
 @}

 @name Accessing and setting default session

 Useful these functions to maintain a single global state.
 @{
*/

/**
  @brief Retrieve default session

  A new default session will transparently be created if it does not
  already exists.

  @return Pointer to default session or NULL on error.
*/
Session *session_get_default(void);

/**
  @brief Set default session

  It is an error if a default session already exists which differs
  from `s`.

  @return Non-zero on error.
*/
int session_set_default(Session *s);


/**
 @}

 @name Setting and accessing global states

 Useful these functions to maintain a single global state.
 @{
*/

/**
  @brief Add new global state

  @param s Pointer to session
  @param name A new unique name associated with the state
  @param ptr Pointer to state data
  @param free_fun Pointer to function that free state data
  @return Non-zero on error.
 */
int session_add_state(Session *s, const char *name, void *ptr,
                           void (*free_fun)(void *ptr));

/**
  @brief Remove global state with given name

  `name` must refer to an existing state.

  @return Non-zero on error.
 */
int session_remove_state(Session *s, const char *name);

/**
  @brief Retrieve global state corresponding to `name`

  @return Pointer to global state or NULL if no state with this name exists.
 */
void *session_get_state(Session *s, const char *name);

/**
  Dump a listing of all sessions to stdout.  For debugging
 */
void session_dump(void);


#endif /*  _SESSION_H */
