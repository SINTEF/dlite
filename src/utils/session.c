#include <assert.h>
#include "map.h"
#include "err.h"
#include "session.h"


/* Convenient macros for failing */
#define FAIL(msg) do { \
    errx(1, msg); goto fail; } while (0)
#define FAIL1(msg, a1) do { \
    errx(1, msg, a1); goto fail; } while (0)


#define DEFAULT_SESSION_ID ":default-session:"


typedef struct _State {
  void *ptr;                    // pointer to state
  void (*free_fun)(void *ptr);  // function for free'ing the state
} State;

typedef map_t(State) map_state_t;


struct _Session {
  const char *session_id;  // unique id identifying a session
  int freeing;             // whether we are freeing this session
  map_state_t states;      // map state names to states
};

typedef map_t(Session) map_session_t;


/* Global session store */
static map_session_t _sessions;

/* Number of sessions */
static int _sessions_count=0;

/* Returns pointer to `_sessions`.  Initialise it if needed. */
static map_session_t *get_sessions(void)
{
  if (!_sessions_count)
    map_init(&_sessions);
  return &_sessions;
}


/*
  Create a new session with given `session_id`.

  Return pointer to the session or NULL on error.
*/
Session *session_create(const char *session_id)
{
  map_session_t *sessions = get_sessions();
  Session s, *sp;
  memset(&s, 0, sizeof(s));
  if (map_get(sessions, session_id))
    return errx(-15, "cannot create new session with existing session id: %s",
                session_id), NULL;
  if (!(s.session_id = strdup(session_id)))
    return err(-12, "allocation failure"), NULL;
  if (map_set(sessions, session_id, s))
    return errx(-15, "failed to create new session with id: %s", session_id), NULL;
  map_init(&s.states);

  sp = map_get(sessions, session_id);
  assert(sp);
  map_init(&sp->states);

  _sessions_count++;
  return sp;
}

/*
  Free all memory associated with session `s`.
*/
void session_free(Session *s)
{
  map_session_t *sessions = get_sessions();
  map_iter_t iter = map_iter(&s->states);
  const char *name, *id=s->session_id;

  /* Indicate that we are freeing this session. This will avoid crashes in
     case that free_fun() calls session_add_state().  */
  s->freeing = 1;

  while ((name = map_next(&s->states, &iter))) {
    State *st = map_get(&s->states, name);
    if (st->free_fun) st->free_fun(st->ptr);
  }
  s->freeing = 0;
  map_deinit(&s->states);

  if (id) {
    map_remove(sessions, id);
    free((char *)id);
  }

  _sessions_count--;
  if (!_sessions_count)
    map_deinit(sessions);
}


/*
  Retrieve session from `session_id`.

  Return pointer to the session or NULL if no session exists with this id.
*/
Session *session_get(const char *session_id)
{
  map_session_t *sessions = get_sessions();
  Session *s = map_get(sessions, session_id);
  if (!s) return errx(-15, "no session with id: %s", session_id), NULL;
  return s;
}


/*
  Retrive session id from session pointer.

  Return session id or NULL on error.
 */
const char *session_get_id(Session *s)
{
  return s->session_id;
}



/*
  Retrieve default session

  A new default session will transparently be created if it does not already
  exists.

  Return pointer to default session or NULL on error.
*/
Session *session_get_default(void)
{
  map_session_t *sessions = get_sessions();
  Session *s = map_get(sessions, DEFAULT_SESSION_ID);
  if (!s) s = session_create(DEFAULT_SESSION_ID);
  return s;
}

/*
  Set default session

  It is an error if a default session already exists which differs
  from `s`.

  Return non-zero on error.
*/
int session_set_default(Session *s)
{
  map_session_t *sessions = get_sessions();
  Session *s2 = map_get(sessions, DEFAULT_SESSION_ID);
  if (s2 && s2 != s)
    return errx(-3, "a default session has already been set");
  map_set(sessions, DEFAULT_SESSION_ID, *s);
  return 0;
}


/*
  Add new global state

  `s` - Pointer to session
  `name` -  A new unique name associated with the state
  `ptr` -  Pointer to state data
  `free_fun` - Pointer to function that free state data

  Return non-zero on error.
 */
int session_add_state(Session *s, const char *name, void *ptr,
                           void (*free_fun)(void *ptr))
{
  State st, *sp;

  if (s->freeing)
    return errx(-3, "cannot add state while freeing session");

  memset(&st, 0, sizeof(st));
  st.ptr = ptr;
  st.free_fun = free_fun;
  if (map_get(&s->states, name))
    return errx(1, "cannot create existing state: %s", name);
  map_set(&s->states, name, st);
  sp = map_get(&s->states, name);
  assert(sp);
  assert(memcmp(sp, &st, sizeof(st)) == 0);
  return 0;
}

/*
  Remove global state with given name

  `name` must refer to an existing state.

  Return non-zero on error.
 */
int session_remove_state(Session *s, const char *name)
{
  State *st = map_get(&s->states, name);
  if (!st) return errx(-15, "no such global state: %s", name);
  if (st->free_fun) st->free_fun(st->ptr);
  map_remove(&s->states, name);
  return 0;
}

/*
  Retrieve global state corresponding to `name`

  Return pointer to global state or NULL if no state with this name exists.
 */
void *session_get_state(Session *s, const char *name)
{
  State *st = map_get(&s->states, name);
  return (st) ? st->ptr : NULL;
}

/*
  Dump a listing of all sessions to stdout.  For debugging
 */
void session_dump(void)
{
  map_iter_t iter = map_iter(&_sessions);
  const char *session_id, *key;
  while ((session_id = map_next(&_sessions, &iter))) {
    Session *s = map_get(&_sessions, session_id);
    map_iter_t stiter = map_iter(&s->states);
    printf("SESSION %s: (%p)\n", session_id, (void *)s);
    if (strcmp(s->session_id, session_id))
      printf("  WARNING session id mismatch: %s\n", s->session_id);
    while ((key = map_next(&s->states, &stiter))) {
      State *st = map_get(&s->states, key);
      printf("  - %s: %p\n", key, st->ptr);
    }
  }
}
