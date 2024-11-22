/* triplestore-redland.c -- wrapper around librdf */

/* This file will be included by triplestore.c */

#include <assert.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include <redland.h>

#include "utils/err.h"
#include "utils/session.h"
#include "utils/sha1.h"
#include "dlite-macros.h"
#include "dlite-errors.h"
#include "triplestore.h"

#define TRIPLESTORE_REDLAND_GLOBALS_ID "triplestore-redland-globals-id"

/* Prototype for cleanup-function */
typedef void (*Freer)(void *ptr);

struct _TripleStore {
  librdf_world *world;        /* Reference to librdf world */
  librdf_storage *storage;    /* Reference to librdf storage */
  librdf_model *model;        /* The RDF graph - librdf model */
  const char *storage_name;   /* Name of storage module */
  const char *name;           /* Identifier for the storage. */
  const char *options;        /* Options provided when creating the storage. */
  const char *ns;             /* Namespace. */
  Triple triple;              /* A triple with current result used by
                                 triplestore_find() and
                                 triplestore_find_first() */
};

/* Global variables for this module */
typedef struct {
  librdf_world *default_world;
  const char *default_storage_name;
  int nmodels;           /* Number of created models */
  int initialized;       /* whether triplestore is initialised */
  int finalize_pending;  /* whether the last storage has been freed */
} Globals;


/* Name of available storage modules */
static char *storage_module_names[] = {
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

/* Forward declarations */
void triplestore_finalize();


/* Free's global variables. */
static void free_globals(void *globals)
{
  triplestore_finalize();
  free(globals);
}

/* Return pointer to global variables */
static Globals *get_globals(void)
{
  Session *s = session_get_default();
  Globals *g = session_get_state(s, TRIPLESTORE_REDLAND_GLOBALS_ID);
  if (!g) {
    if (!(g = calloc(1, sizeof(Globals))))
      return err(dliteMemoryError, "allocation failure"), NULL;
    g->default_world = NULL;
    g->default_storage_name = "memory";
    g->nmodels = 0;
    g->initialized = 0;
    g->finalize_pending = 0;
    session_add_state(s, TRIPLESTORE_REDLAND_GLOBALS_ID, g, free_globals);
  }
  return g;
}


/* Logger for librdf for all log levels */
static int logger(void *user_data, librdf_log_message *message)
{
  int code = librdf_log_message_code(message);
  librdf_log_level level = librdf_log_message_level(message);
  const char *msg = librdf_log_message_message(message);
  /* facility indicates where the log message comes from */
  //librdf_log_facility level = librdf_log_message_facility(message);
  UNUSED(user_data);

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

/* Free triplestore on pending finalise. */
static void finalize_check()
{
  Globals *g = get_globals();
  if (g->finalize_pending && g->nmodels == 0 && g->default_world) {
    if (!dlite_globals_in_atexit() || getenv("DLITE_ATEXIT_FREE"))
      librdf_free_world(g->default_world);
    g->default_world = NULL;
    g->finalize_pending = 0;
  }
}


/* Create context for stream filter. */
typedef struct { const char *o, *d; } StreamContext;

static void *stream_get_context(const char *o, const char *d)
{
  StreamContext *c;
  if (!(c = calloc(1, sizeof(StreamContext))))
    return err(dliteMemoryError, "allocation failure"), NULL;
  c->o = o;
  c->d = d;
  return (void *)c;
}

/* Free context for stream filter. */
static void stream_free(void *context)
{
  free(context);
}

/* Filter for the stream returned by find. Used to handle object and
   datatype. */
static librdf_statement *stream_map(librdf_stream *stream, void *context,
                                    librdf_statement *item)
{
  UNUSED(stream);
  librdf_node *no = librdf_statement_get_object(item);
  StreamContext *c = (StreamContext *)context;

  if (librdf_node_is_literal(no)) {
    if (c->o) {
      char *value = (char *)librdf_node_get_literal_value(no);
      if (strcmp(value, c->o) != 0) return NULL;
    }
    if (c->d) {
      char *lang = librdf_node_get_literal_value_language(no);
      librdf_uri *uri = librdf_node_get_literal_value_datatype_uri(no);
      if (lang) {
        if ((c->d[0] != '@') || (strcmp(c->d+1, lang) != 0)) return NULL;
      }
      if (uri) {
        if (strcmp(c->d, (char *)librdf_uri_as_string(uri)) != 0) return NULL;
      }
      if (c->d[0] == '\0' && (lang || uri)) return NULL;
    }
  } else {
    librdf_uri *iri = librdf_node_get_uri(no);
    if (c->d && c->d[0]) return NULL;
    if (c->o && iri && strcmp(c->o, (char *)librdf_uri_as_string(iri)) != 0)
      return NULL;
  }
  return item;
}



/*
  Returns a new librdf stream with all triples matching `s`, `p` and
  `o`.  Any of these may be NULL, allowing for multiple matches.

  If `o` is non-NULL and `d` is NULL, the object is first assumed to
  be an IRI. If there are no matches, the object is assumed to be a
  literal with no specified language or type.

  Returns NULL on error.
*/
static librdf_stream *find(TripleStore *ts, const char *s, const char *p,
                           const char *o, const char *d)
{
  librdf_node *ns=NULL, *np=NULL, *no=NULL;
  librdf_uri *dt=NULL;
  librdf_statement *statement=NULL;
  librdf_stream *stream=NULL;
  int failed = 1;

  if (s &&!(ns = librdf_new_node_from_uri_string(ts->world,
                                                 (const unsigned char *)s)))
    FAIL1("error creating node for subject: '%s'", s);
  if (p && !(np = librdf_new_node_from_uri_string(ts->world,
                                                  (const unsigned char *)p)))
    FAIL1("error creating node for predicate: '%s'", p);

  if (o && d && *d) {
    const char *lang=NULL;
    if (d[0] == '@')
      lang = d+1;
    else if (!(dt = librdf_new_uri(ts->world, (const unsigned char *)d)))
      FAIL1("error datatype '%s'", d);
    if (!(no = librdf_new_node_from_typed_literal(ts->world,
                                                  (const unsigned char *)o,
                                                  lang, dt)))
      FAIL2("error creating node for literal object: '%s' of type '%s'", o, d);
  }

  if (!(statement = librdf_new_statement_from_nodes(ts->world, ns, np, no))) {
    ns = np = no = NULL;  // now owned by statement
    FAIL4("error creating statement: (%s, %s, %s) (d=%s)", s, p, o, d);
  }
  if (!(stream = librdf_model_find_statements(ts->model, statement)))
    FAIL4("error finding statements matching (%s, %s, %s) (d=%s)", s, p, o, d);

  if ((o || d) && !(o && d && *d)) {
    void *context = stream_get_context(o, d);
    if (librdf_stream_add_map(stream, stream_map, stream_free, context))
      FAIL("error adding mapping function to stream");
  }

  failed = 0;
 fail:
  if (failed) {
    if (stream) librdf_free_stream(stream);
    stream = NULL;
  }
  if (statement) {
    librdf_free_statement(statement);
  } else if (failed) {
    if (ns) librdf_free_node(ns);
    if (np) librdf_free_node(np);
    if (no) librdf_free_node(no);
  }
  if (dt) librdf_free_uri(dt);
  return stream;
}


/*
  Assign triple `t` from RDF `statement`.

  The s-p-o-id pointers in `t` must either be NULL or points to
  allocated strings.  In the latter case they will be freed and
  new memory reallocated.

  Returns non-zero on error.
*/
static int assign_triple_from_statement(Triple *t,
                                         librdf_statement *statement)
{
  librdf_node *node;
  unsigned char *s=NULL, *p=NULL, *o=NULL, *d=NULL;
  char *lang;
  librdf_uri *datatype;
  errno = 0;
  s = librdf_uri_to_string(librdf_node_get_uri(librdf_statement_get_subject(statement)));
  p = librdf_uri_to_string(librdf_node_get_uri(librdf_statement_get_predicate(statement)));
  node = librdf_statement_get_object(statement);
  switch (librdf_node_get_type(node)) {
  case LIBRDF_NODE_TYPE_UNKNOWN:
    FAIL("unknown node type");
  case LIBRDF_NODE_TYPE_RESOURCE:
    o = librdf_uri_to_string(librdf_node_get_uri(node));
    break;
  case LIBRDF_NODE_TYPE_LITERAL:
    o = librdf_node_get_literal_value(node);
    if (o) o = (unsigned char *)strdup((char *)o);

    if ((datatype = librdf_node_get_literal_value_datatype_uri(node))) {
      if (!(d = librdf_uri_to_string(datatype)))
        FAIL("cannot convert datatype URI to string");
    } else if ((lang = librdf_node_get_literal_value_language(node))) {
      size_t n = strlen(lang);
      if (!(d = calloc(1, n+2)))
        FAILCODE(dliteMemoryError, "allocation failure");
      d[0] = '@';
      strncpy((char *)d+1, lang, n+1);
    }
    break;
  case LIBRDF_NODE_TYPE_BLANK:
    o = librdf_node_get_blank_identifier(node);
    if (o) o = (unsigned char *)strdup((char *)o);
    break;
  }
  if (s && p && o) {
    triple_clean(t);
    t->s = (char *)s;
    t->p = (char *)p;
    t->o = (char *)o;
    t->d = (char *)d;
    t->id = NULL;
    return 0;
  }
  FAIL("error in assign_triple_from_statement");
 fail:
  if (s) free(s);
  if (p) free(p);
  if (o) free(o);
  if (d) free(d);
  return 1;
}


/* ================================== */
/* Public functions                   */
/* ================================== */


/* Mark the triplestore to be finalized when the last model has
   been freed. */
void triplestore_finalize()
{
  Globals *g = get_globals();
  g->finalize_pending = 1;
  finalize_check();
}

/* Initiates the triplestore */
void triplestore_init()
{
  Globals *g = get_globals();
  if (!g->initialized) g->initialized = 1;
  g->finalize_pending = 0;
}


/* Set the default world. */
void triplestore_set_default_world(librdf_world *world)
{
  Globals *g = get_globals();
  g->default_world = world;
}

/* Returns a pointer to the default world.  A new default world is
   created if it doesn't already exists. */
librdf_world *triplestore_get_default_world()
{
  Globals *g = get_globals();
  if (!g->default_world) {
    triplestore_init();
    if (!(g->default_world = librdf_new_world()))
      return err(1, "Failure to create new librdf world"), NULL;
    librdf_world_set_logger(g->default_world, NULL, logger);
    //librdf_world_set_digest();
    //librdf_world_set_feature();
    librdf_world_open(g->default_world);
  }
  return g->default_world;
}


/* Returns the internal librdf world. */
librdf_world *triplestore_get_world(TripleStore *ts)
{
  return ts->world;
}

/* Returns the internal librdf model. */
librdf_model *triplestore_get_model(TripleStore *ts)
{
  return ts->model;
}

/* Set the default storage name.  Returns non-zero on error. */
int triplestore_set_default_storage(const char *name)
{
  char **p = storage_module_names;
  while (p) {
    if (strcasecmp(name, *p) == 0) {
      Globals *g = get_globals();
      g->default_storage_name = name;
      return 0;
    }
  }
  return err(1, "no such triplestore storage: %s", name);
}

/* Returns a pointer to the name of default storage or NULL on error. */
const char *triplestore_get_default_storage()
{
  Globals *g = get_globals();
  return g->default_storage_name;
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
  Globals *g = get_globals();
 TripleStore *ts=NULL;
  librdf_storage *storage=NULL;
  triplestore_init();
  if (!world) world = triplestore_get_default_world();
  if (!storage_name) storage_name = g->default_storage_name;

  if (!(storage = librdf_new_storage(world, storage_name, name, options)))
    goto fail;

  if (!(ts = calloc(1, sizeof(TripleStore)))) FAIL("Allocation failure");
  ts->world = world;
  ts->storage = storage;
  if (!(ts->model = librdf_new_model(world, storage, NULL))) goto fail;
  if (storage_name) ts->storage_name = strdup(storage_name);
  if (name) ts->name = strdup(name);
  if (options) ts->options = strdup(options);

  g->nmodels++;
  return ts;
 fail:
  if (ts) triplestore_free(ts);
  if (storage && !ts) librdf_free_storage(storage);
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
  Globals *g = get_globals();
  assert(g->nmodels > 0);
  g->nmodels--;
  librdf_free_storage(ts->storage);
  librdf_free_model(ts->model);
  if (ts->storage_name)  free((char *)ts->storage_name);
  if (ts->name)          free((char *)ts->name);
  if (ts->options)       free((char *)ts->options);
  if (ts->ns)            free((char *)ts->ns);
  triple_clean(&ts->triple);
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


/* Returns a newly created uri node from `uri`. If `uri` has no namespace,
   the default namespace is prepended. */
static librdf_node *new_uri_node(TripleStore *ts, const char *uri)
{
  librdf_node *node;
  if (ts->ns && !strchr(uri, ':')) {
    size_t len_ns = strlen(ts->ns);
    size_t len_uri = strlen(uri);
    unsigned char *buf = malloc(len_ns + len_uri + 2);
    memcpy(buf, ts->ns, len_ns);
    buf[len_ns] = ':';
    memcpy(buf + len_ns + 1, uri, len_uri + 1);
    node = librdf_new_node_from_uri_string(ts->world, buf);
    free(buf);
  } else {
    node = librdf_new_node_from_uri_string(ts->world,
                                           (const unsigned char *)uri);
  }
  return node;
}


/*
  Adds a single (s,p,o,d) triple (of URIs) to store.  If datatype `d` is NULL,
  is the object is considered to be an IRI. Otherwise it is a literal.

  Returns non-zero on error.
 */
int triplestore_add(TripleStore *ts, const char *s, const char *p,
                    const char *o, const char *d)
{
  librdf_node *ns=NULL, *np=NULL, *no=NULL;
  librdf_uri *uri=NULL;
  if (!(ns = new_uri_node(ts, s)))
    FAIL1("error creating node for subject: '%s'", s);
  if (!(np = new_uri_node(ts, p)))
    FAIL1("error creating node for predicate: '%s'", p);

  if (d && d[0] == '@') {
    if (!(no = librdf_new_node_from_typed_literal(ts->world, (unsigned char *)o,
                                                  d+1, NULL)))
      FAIL2("error creating language-tagged (%s) node for object: '%s'", d, o);
  } else if (d) {
    if (!(uri = librdf_new_uri(ts->world, (unsigned char *)d)))
      FAIL1("error creating datatype URI from: '%s'", d);
    if (!(no = librdf_new_node_from_typed_literal(ts->world, (unsigned char *)o,
                                                  NULL, uri)))
      FAIL2("error creating typed (%s) literal node for object: '%s'", d, o);
  } else {
    if (!(no = new_uri_node(ts, o)))
      FAIL1("error creating IRI node for object: '%s'", o);
  }
  if (librdf_model_add(ts->model, ns, np, no))
    FAIL4("error adding triple (%s, %s, %s, %s)", s, p, o, d);
  if (uri) librdf_free_uri(uri);
  return 0;
 fail:
  if (ns) librdf_free_node(ns);
  if (np) librdf_free_node(np);
  if (no) librdf_free_node(no);
  if (uri) librdf_free_uri(uri);
  return 1;
}


/*
  Adds `n` triples to store.  Returns non-zero on error.
 */
int triplestore_add_triples(TripleStore *ts, const Triple *triples, size_t n)
{
  size_t i;
  for (i=0; i<n; i++) {
    const Triple *t = triples + i;
    if (triplestore_add(ts, t->s, t->p, t->o, t->d)) return 1;
  }
  return 0;
}


/*
  Removes a triple identified by `s`, `p` and `o`.  Any of these may
  be NULL, allowing for multiple matches.  The object is assumed to
  be a literal with no language.

  Returns the number of triples removed or -1 on error.
*/
int triplestore_remove(TripleStore *ts, const char *s,
                       const char *p, const char *o, const char *d)
{
  librdf_stream *stream;
  int failed=0, removed=0;
  if (!(stream = find(ts, s, p, o, d))) return -1;
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
  Removes all relations in triplestore and releases all references to
  external memory.
 */
void triplestore_clear(TripleStore *ts)
{
  triplestore_remove(ts, NULL, NULL, NULL, NULL);
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

  It is not an errror to call this function multiple times.
*/
void triplestore_deinit_state(TripleState *state)
{
  if (state->data) librdf_free_stream((librdf_stream *)state->data);
  state->data = NULL;
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
  Return next triple matching s-p-o triple, where `d` is the datatype
  of the object. Any of `s`, `p`, `o` or `d` may be NULL, in which case
  they will match anything.

  If `d` starts with '@', it will match language-tagged plain text
  literal objects whos XML language abbreviation matches the string
  following the '@'-sign.

  if `d` is NUL (empty string), it will match non-literal objects.

  Any other non-NULL `d` will match literal objects whos datatype are `d`.

  When no more matches can be found, NULL is returned.  NULL is also
  returned on error.

  This function should be called iteratively.  Before the first call
  it should be provided a `state` initialised with triplestore_init_state()
  and deinitialised with triplestore_deinit_state().
*/
const Triple *triplestore_find(TripleState *state,
                               const char *s, const char *p, const char *o,
                               const char *d)
{
  librdf_stream *stream;
  librdf_statement *statement;
  Triple *t = &state->ts->triple;
  assert(t);
  if (!state->data && !(state->data = find(state->ts, s, p, o, d)))
    return err(1, "cannot create model stream"), NULL;
  stream = (librdf_stream *)state->data;
  if (!(statement = librdf_stream_get_object(stream))) return NULL;
  if (assign_triple_from_statement(t, statement)) return NULL;
  librdf_stream_next(stream);
  return t;
}


/*
  Returns a pointer to first triple matching `s`, `p` and `o` or NULL
  if no match can be found.  Any of `s`, `p` or `o` may be NULL.
 */
const Triple *triplestore_find_first(const TripleStore *ts, const char *s,
                                     const char *p, const char *o,
                                     const char *d)
{
  TripleState state;
  const Triple *t;
  triplestore_init_state((TripleStore *)ts, &state);
  t = triplestore_find(&state, s, p, o, d);
  triplestore_deinit_state(&state);
  return t;
}
