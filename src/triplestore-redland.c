/* triplestore-redland.c -- wrapper around librdf */

/* This file will be included by triplestore.c */

#include <assert.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include <redland.h>

#include "utils/err.h"
#include "utils/sha1.h"
#include "dlite-macros.h"
#include "triplestore.h"


/* Prototype for cleanup-function */
typedef void (*Freer)(void *ptr);

struct _TripleStore {
  librdf_world *world;        /* Convenient Reference to librdf world */
  librdf_model *model;        /* The RDF graph - librdf model */
  const char *storage_name;   /* Name of storage module */
  const char *name;           /* Identifier for the storage. */
  const char *options;        /* Options provided when creating the storage. */
  Triple triple;              /* A triple with current result used by
                                 triplestore_find() and
                                 triplestore_find_first() */
};




/* Default world */
static librdf_world *default_world = NULL;

/* Name of default storage */
static const char *default_storage_name = "memory";

/* Number of created models */
static int nmodels = 0;

/* Flags indicating whether the triplestore has been initiated or will
   finalize when the last storage has been freed. */
static int initialized = 0;
static int finalize_pending = 0;

/* Name of available storage modules */
char *storage_module_names[] = {
  "memory",
  "hashes",
  "file",
  "mysql",
  "postgresql",
  "sqlite",
  "tstore",
  "uri",
  "Virtuoso",
  NULL
};


/* Logger for librdf for all log levels */
static int logger(void *user_data, librdf_log_message *message)
{
  int code = librdf_log_message_code(message);
  librdf_log_level level = librdf_log_message_level(message);
  const char *msg = librdf_log_message_message(message);
  /* facility indicates where the log message comes from */
  //librdf_log_facility level = librdf_log_message_facility(message);
  UNUSED(user_data);

  fprintf(stderr, "\n*** logger: %s\n", msg);  // XXX

  switch (level) {
  case LIBRDF_LOG_NONE: return 0;
  case LIBRDF_LOG_DEBUG: warnx("DEBUG: %s", msg); break;
  case LIBRDF_LOG_INFO:  warnx("INFO: %s", msg);  break;
  case LIBRDF_LOG_WARN:  warnx("%s", msg);        break;
  case LIBRDF_LOG_ERROR: errx(code, "%s", msg);   break;
  case LIBRDF_LOG_FATAL: fatalx(code, "%s", msg); break;
  }
  return 1;
}

/* Check whether the Finalizes the triplestore */
static void finalize_check()
{
  if (finalize_pending && nmodels == 0 && default_world) {
    librdf_free_world(default_world);
    default_world = NULL;
    finalize_pending = 0;
  }
}


/*
  Returns a new librdf stream with all triples matching `s`, `p` and
  `o`.  Any of these may be NULL, allowing for multiple matches.

  If `literal` is non-zero the object will be considered to be a
  literal, otherwise it is considered to be an URI.

  If `lang` is not NULL, it must be a valid XML language abbreviation,
  like "en". Only used if `literal` is non-zero.

  Returns NULL on error.
*/
static librdf_stream *find(TripleStore *ts, const char *s, const char *p,
                           const char *o, int literal, const char *lang)
{
  librdf_node *ns=NULL, *np=NULL, *no=NULL;
  librdf_statement *statement=NULL;
  librdf_stream *stream=NULL;
  int failed = 1;
  if (s &&!(ns = librdf_new_node_from_uri_string(ts->world,
                                                 (const unsigned char *)s)))
    FAIL1("error creating node for subject: '%s'", s);
  if (p && !(np = librdf_new_node_from_uri_string(ts->world,
                                                  (const unsigned char *)p)))
    FAIL1("error creating node for predicate: '%s'", p);
  if (literal) {
    if (o && !(no = librdf_new_node_from_literal(ts->world,
                                                 (const unsigned char *)o,
                                                 lang, 0)))
      FAIL1("error creating node for literal object: '%s'", o);
  } else {
    if (o && !(no = librdf_new_node_from_uri_string(ts->world,
                                                    (const unsigned char *)o)))
      FAIL1("error creating node for object: '%s'", o);
  }
  if (!(statement = librdf_new_statement_from_nodes(ts->world, ns, np, no))) {
    ns = np = no = NULL;
    FAIL3("error creating statement: (%s, %s, %s)", s, p, o);
  }
  if (!(stream = librdf_model_find_statements(ts->model, statement)))
    FAIL3("error finding statements matching (%s, %s, %s)", s, p, o);

  failed = 0;
 fail:
  if (statement) {
    librdf_free_statement(statement);
  } else if (failed) {
    if (ns) librdf_free_node(ns);
    if (np) librdf_free_node(np);
    if (no) librdf_free_node(no);
  }
  if (failed) {
    if (stream) librdf_free_stream(stream);
    stream = NULL;
  }
  return stream;
}


/* Assign triple `t` from RDF `statement`.  Returns non-zero on error. */
static int assign_triple_from_statement(Triple *t,
                                         librdf_statement *statement)
{
  librdf_node *node;
  t->s = (char *)
    librdf_uri_to_string(librdf_node_get_uri(librdf_statement_get_subject(statement)));

  t->p = (char *)
    librdf_uri_to_string(librdf_node_get_uri(librdf_statement_get_predicate(statement)));

  node = librdf_statement_get_object(statement);
  switch (librdf_node_get_type(node)) {
  case LIBRDF_NODE_TYPE_UNKNOWN:
    return err(1, "unknown node type");
  case LIBRDF_NODE_TYPE_RESOURCE:
    t->o = (char *)librdf_uri_to_string(librdf_node_get_uri(node));
    break;
  case LIBRDF_NODE_TYPE_LITERAL:
    t->o = (char *)librdf_node_get_literal_value(node);
    break;
  case LIBRDF_NODE_TYPE_BLANK:
    t->o = (char *)librdf_node_get_blank_identifier(node);
    break;
  }
  return 0;
}


/* ================================== */
/* Public functions                   */
/* ================================== */

/* Mark the triplestore to be finalized when the last model has
   been freed. */
void triplestore_finalize()
{
  finalize_pending = 1;
  finalize_check();
}

/* Initiates the triplestore */
void triplestore_init()
{
  if (!initialized) {
    initialized = 1;
    atexit(triplestore_finalize);
  }
  finalize_pending = 0;
}


/* Set the default world. */
void triplestore_set_default_world(librdf_world *world)
{
  default_world = world;
}

/* Returns a pointer to the default world. */
librdf_world *triplestore_get_default_world()
{
  return default_world;
}

/* Returns a pointer to the default world.  Unlike
   triplestore_get_default_world(), the default world is created if it
   doesn't already exists. */
librdf_world *triplestore_get_world()
{
  if (!default_world) {
    triplestore_init();
    if (!(default_world = librdf_new_world()))
      return err(1, "Failure to create new librdf world"), NULL;
    librdf_world_set_logger(default_world, NULL, logger);
    //librdf_world_set_digest();
    //librdf_world_set_feature();
    librdf_world_open(default_world);
  }
  return default_world;
}


/* Set the default storage name.  Returns non-zero on error. */
int triplestore_set_default_storage(const char *name)
{
  char **p = storage_module_names;
  while (p) {
    if (strcasecmp(name, *p) == 0) {
      default_storage_name = name;
      return 0;
    }
  }
  return err(1, "no such triplestore storage: %s", name);
}

/* Returns a pointer to the name of default storage or NULL on error. */
const char *triplestore_get_default_storage()
{
  return default_storage_name;
}


/*
  Like triplestore_create_with_storage(), but also takes a librdf world
  as argument.
 */
TripleStore *triplestore_create_with_world(librdf_world *world,
                                           const char *storage_name,
                                           const char *name,
                                           const char *options)
{
  TripleStore *ts=NULL;
  librdf_storage *storage=NULL;
  triplestore_init();
  if (!world) world = triplestore_get_world();
  if (!storage_name) storage_name = default_storage_name;

  if (!(storage = librdf_new_storage(world, storage_name, name, options)))
    goto fail;

  if (!(ts = calloc(1, sizeof(TripleStore)))) FAIL("Allocation failure");
  ts->world = world;
  if (!(ts->model = librdf_new_model(world, storage, NULL))) goto fail;
  if (storage_name) ts->storage_name = strdup(storage_name);
  if (name) ts->name = strdup(name);
  if (options) ts->options = strdup(options);

  nmodels++;
  return ts;
 fail:
  if (ts) triplestore_free(ts);
  if (storage) librdf_free_storage(storage);
  return NULL;
}


/*
  Returns a new empty triplestore.

  Arguments:
    storage_name: Name of storage module. If NULL, the default storage will
                  be used.
    name:         An identifier for the storage.
    options:      Options for `storage_name`. May be NULL if the storage
                  allows it.  See
                  http://librdf.org/docs/api/redland-storage-modules.html
                  for more info.

  Returns NULL on error.
 */
TripleStore *triplestore_create_with_storage(const char *storage_name,
                                             const char *name,
                                             const char *options)
{
  return triplestore_create_with_world(NULL, storage_name, name, options);
}


/*
  Returns a new empty triplestore or NULL on error.
 */
TripleStore *triplestore_create()
{
  return triplestore_create_with_world(NULL, NULL, NULL, NULL);
}


/*
  Frees triplestore `ts`.
 */
void triplestore_free(TripleStore *ts)
{
  librdf_storage *storage = librdf_model_get_storage(ts->model);
  assert(storage);
  assert(nmodels > 0);
  nmodels--;
  librdf_free_model(ts->model);
  librdf_free_storage(storage);
  if (ts->storage_name) free((char *)ts->storage_name);
  if (ts->name) free((char *)ts->name);
  if (ts->options) free((char *)ts->options);
  free(ts);
  finalize_check();
}


/*
  Returns the number of triples in the store.
*/
size_t triplestore_length(TripleStore *ts)
{
  int n = librdf_model_size(ts->model);
  if (n < 0) return err(-1, "cannot determine length of triplestore: %s",
                        ts->name);
  return n;
}


/*
  Adds a single (s,p,o) triple to store.

  If `literal` is non-zero the object will be considered to be a
  literal, otherwise it is considered to be an URI.

  If `lang` is not NULL, it must be a valid XML language abbreviation,
  like "en". Only used if `literal` is non-zero.

  Returns non-zero on error.
 */
int triplestore_add2(TripleStore *ts, const char *s, const char *p,
                     const char *o, int literal, const char *lang)
{
  librdf_node *ns=NULL, *np=NULL, *no=NULL;
  if (!(ns = librdf_new_node_from_uri_string(ts->world,
                                             (const unsigned char *)s)))
    FAIL1("error creating node for subject: '%s'", s);
  if (!(np = librdf_new_node_from_uri_string(ts->world,
                                             (const unsigned char *)p)))
    FAIL1("error creating node for predicate: '%s'", p);
  if (literal) {
    if (librdf_model_add_string_literal_statement(ts->model, ns, np,
                                                  (const unsigned char *)o,
                                                  lang, 0))
      FAIL("error adding literal triple");
  } else {
    if (!(no = librdf_new_node_from_uri_string(ts->world,
                                               (const unsigned char *)o)))
      FAIL1("error creating node for object: '%s'", o);
    if (librdf_model_add(ts->model, ns, np, no))
      FAIL("error adding triple");
  }
  return 0;
 fail:
  if (ns) librdf_free_node(ns);
  if (np) librdf_free_node(np);
  if (no) librdf_free_node(no);
  return 1;
}


/*
  Adds a single (s,p,o) triple (of URIs) to store.  The object is
  considered to be a literal with no language.

  Returns non-zero on error.
 */
int triplestore_add(TripleStore *ts, const char *s, const char *p,
                    const char *o)
{
  return triplestore_add2(ts, s, p, o, 1, NULL);
}


/*
  Adds `n` triples to store.  Returns non-zero on error.
 */
int triplestore_add_triples(TripleStore *ts, const Triple *triples, size_t n)
{
  size_t i;
  for (i=0; i<n; i++) {
    const Triple *t = triples + i;
    if (triplestore_add(ts, t->s, t->p, t->o)) return 1;
  }
  return 0;
}


/*
  Removes a triple identified by `s`, `p` and `o`.  Any of these may
  be NULL, allowing for multiple matches.

  If `literal` is non-zero the object will be considered to be a
  literal, otherwise it is considered to be an URI.

  If `lang` is not NULL, it must be a valid XML language abbreviation,
  like "en". Only used if `literal` is non-zero.

  Returns the number of triples removed or -1 on error.
*/
int triplestore_remove2(TripleStore *ts, const char *s, const char *p,
                        const char *o, int literal, const char *lang)
{
  librdf_stream *stream;
  int failed=0, removed=0;
  if (!(stream = find(ts, s, p, o, literal, lang))) return -1;
  do {
    librdf_statement *statement;
    if (!(statement = librdf_stream_get_object(stream))) break;
    if (librdf_model_remove_statement(ts->model, statement)) failed=1;
    removed++;
  } while (!failed && !librdf_stream_next(stream));
  librdf_free_stream(stream);
  return (failed) ? -1 : removed;
}


/*
  Removes a triple identified by `s`, `p` and `o`.  Any of these may
  be NULL, allowing for multiple matches.  The object is assumed to
  be a literal with no language.

  Returns the number of triples removed or -1 on error.
*/
int triplestore_remove(TripleStore *ts, const char *s,
                       const char *p, const char *o)
{
  return triplestore_remove2(ts, s, p, o, 1, NULL);
}


/*
  Removes all relations in triplestore and releases all references to
  external memory.
 */
void triplestore_clear(TripleStore *ts)
{
  triplestore_remove(ts, NULL, NULL, NULL);
}


/* Removes triple number n.  Returns non-zero on error. */
int triplestore_remove_by_index(TripleStore *ts, size_t n)
{
  librdf_stream *stream;
  librdf_statement *statement=NULL;
  int failed = 1;
  if (!(stream = librdf_model_as_stream(ts->model)))
    FAIL("error creating stream of (s,p,o) statements");
  while (n--) {
    if (librdf_stream_next(stream)) FAIL1("index out of range: %zu", n);
  }
  if (!(statement = librdf_stream_get_object(stream)))
    FAIL("cannot get statement from RDF stream");
  if (librdf_model_remove_statement(ts->model, statement))
    FAIL("error removing statement");
  failed = 0;
 fail:
  librdf_free_stream(stream);
  return failed;
}


/*
  Initiates a TripleState for triplestore_find().  The state must be
  deinitialised with triplestore_deinit_state().
*/
void triplestore_init_state(TripleStore *ts, TripleState *state)
{
  memset(state, 0, sizeof(TripleState));
  state->ts = ts;
}

/*
  Deinitiates a TripleState initialised with triplestore_init_state().
*/
void triplestore_deinit_state(TripleState *state)
{
  if (state->data) librdf_free_stream((librdf_stream *)state->data);
}

/*
  Resets iterator.
*/
void triplestore_reset_state(TripleState *state)
{
  TripleStore *ts = state->ts;
  triplestore_deinit_state(state);
  triplestore_init_state(ts, state);
}

/*
  Increments state and returns a pointer to the current triple in the
  store or NULL if all triples have been visited.
 */
const Triple *triplestore_next(TripleState *state)
{
  const Triple *t;
  if (!(t = triplestore_poll(state))) return NULL;
  librdf_stream_next((librdf_stream *)state->data);
  return t;
}

/*
  Returns a pointer to the current triple in the store or NULL if all
  triples have been visited.
 */
const Triple *triplestore_poll(TripleState *state)
{
  TripleStore *ts = state->ts;
  librdf_statement *statement;
  librdf_stream *stream;
  if (!state->data &&
      !(state->data = librdf_model_as_stream(ts->model))) return NULL;
  stream = (librdf_stream *)state->data;
  if (!(statement = librdf_stream_get_object(stream))) return NULL;
  if (assign_triple_from_statement(&ts->triple, statement)) return NULL;
  return &ts->triple;
}


/*
  This function should be called iteratively.  Before the first call
  it should be provided a `state` initialised with triplestore_init_state().

  If `literal` is non-zero the object will be considered to be a
  literal, otherwise it is considered to be an URI.

  If `lang` is not NULL, it must be a valid XML language abbreviation,
  like "en". Only used if `literal` is non-zero.

  For each call it will return a pointer to triple matching `s`, `p`
  and `o`.  Any of `s`, `p` or `o` may be NULL.  When no more matches
  can be found, NULL is returned.

  NULL is also returned on error.
 */
const Triple *triplestore_find2(TripleState *state,
                                 const char *s, const char *p, const char *o,
                                 int literal, const char *lang)
{
  librdf_stream *stream;
  librdf_statement *statement;
  Triple *t = &state->ts->triple;
  assert(t);
  if (!state->data && !(state->data = find(state->ts, s, p, o, literal, lang)))
    return err(1, "cannot create model stream"), NULL;
  stream = (librdf_stream *)state->data;
  if (!(statement = librdf_stream_get_object(stream))) return NULL;
  if (assign_triple_from_statement(t, statement)) return NULL;
  librdf_stream_next(stream);
  return t;
}


/*
   Equivalent to `triplestore_find2(state, s, p, o, 1, NULL)`.
*/
const Triple *triplestore_find(TripleState *state,
                                const char *s, const char *p, const char *o)
{
  return triplestore_find2(state, s, p, o, 1, NULL);
}


/*
  Returns a pointer to first triple matching `s`, `p` and `o` or NULL
  if no match can be found.  Any of `s`, `p` or `o` may be NULL.
 */
const Triple *triplestore_find_first2(const TripleStore *ts, const char *s,
                                       const char *p, const char *o,
                                       int literal, const char *lang)
{
  TripleState state;
  const Triple *t;
  triplestore_init_state((TripleStore *)ts, &state);
  t = triplestore_find2(&state, s, p, o, literal, lang);
  triplestore_deinit_state(&state);
  return t;
}


/*
  Returns a pointer to first triple matching `s`, `p` and `o` or NULL
  if no match can be found.  Any of `s`, `p` or `o` may be NULL.
 */
const Triple *triplestore_find_first(const TripleStore *ts, const char *s,
                                      const char *p, const char *o)
{
  return triplestore_find_first2(ts, s, p, o, 1, NULL);
}


#if 0


/*
  Removes a triple identified by it's `id`.  Returns non-zero on
  error or if no such triple can be found.
*/
int triplestore_remove_by_id(TripleStore *ts, const char *id)
{
  int *n;
  if (!(n = map_get(&ts->map, id)))
    return err(1, "no such triple id: \"%s\"", id);
  return _remove_by_index(ts, *n);
}



/*
  Returns a pointer to triple with given id or NULL if no match can be found.
*/
const Triple *triplestore_get(const TripleStore *ts, const char *id)
{
  int *n = map_get(&((TripleStore *)ts)->map, id);
  if (!n)
    return errx(1, "no triple with id \"%s\"", id), NULL;
  if (!ts->triples[*n].id)
    return errx(1, "triple \"%s\" has been removed", id), NULL;
  return ts->triples + *n;
}




#endif
