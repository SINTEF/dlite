/**
  @file
  @brief A simple triplestore for strings

  This library defines triplets as subject-predicate-object tuplets
  with an id.  This allow a allows the subject or object to refer to
  another triplet via its id, as one would expect for RDF triplets
  (see https://en.wikipedia.org/wiki/Semantic_triple).


 */
#ifndef _TRIPLESTORE_H
#define _TRIPLESTORE_H

#include "triple.h"

/** Namespaces */
#define XML     "http://www.w3.org/XML/1998/namespace:"
#define XSD     "http://www.w3.org/2001/XMLSchema#"
#define RDF     "http://www.w3.org/1999/02/22-rdf-syntax-ns#"
#define RDFS    "http://www.w3.org/2000/01/rdf-schema#"
#define OWL     "http://www.w3.org/2002/07/owl#"
#define SKOS    "http://www.w3.org/2004/02/skos/core#"
#define DCTERMS "http://purl.org/dc/terms/"
#define EMMO    "http://emmo.info/emmo#"
#define SOFT    "http://emmo.info/soft#"


/** Triplet store. */
typedef struct _TripleStore TripleStore;


/** State used by triplestore_find.
    Don't rely on current definition, it may be optimised later. */
typedef struct _TripleState {
  TripleStore *ts;    /*!< reference to corresponding TripleStore */
  size_t pos;         /*!< current position */
  void *data;         /*!< internal data depending on the implementation */
} TripleState;


/* ================================== */
/* Functions specific to librdf       */
/* ================================== */
#ifdef HAVE_REDLAND
#include "redland.h"

/** Returns a pointer to the default world.  A new default world is
    created if it doesn't already exists. */
librdf_world *triplestore_get_default_world();

/** Returns the internal librdf world. */
librdf_world *triplestore_get_world(TripleStore *ts);

/** Returns the internal librdf model. */
librdf_model *triplestore_get_model(TripleStore *ts);

#endif



/* ================================== */
/* Generic functions                  */
/* ================================== */

/**
  Returns a new empty triplestore or NULL on error.
 */
TripleStore *triplestore_create();

/**
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
                                             const char *options);

/**
  Frees triplestore.
 */
void triplestore_free(TripleStore *ts);


/**
  Set default namespace
*/
void triplestore_set_namespace(TripleStore *ts, const char *ns);

/**
  Returns a pointer to default namespace.
  It may be NULL if it hasn't been set.
*/
const char *triplestore_get_namespace(TripleStore *ts);


/**
  Returns the number of triples in the store.
*/
size_t triplestore_length(TripleStore *ts);


/**
  Adds a single (s,p,o) triple to store.

  If `literal` is non-zero the object will be considered to be a
  literal, otherwise it is considered to be an URI.

  If `lang` is not NULL, it must be a valid XML language abbreviation,
  like "en". Only used if `literal` is non-zero.

  If `datatype_uri` is not NULL, it should be an uri for the literal
  datatype. Ex: "xsd:integer".

  Returns non-zero on error.
 */
int triplestore_add2(TripleStore *ts, const char *s, const char *p,
                     const char *o, int literal, const char *lang,
                     const char *datatype_uri);


/**
  Adds a single triple to store.  The object is considered to be a
  literal with no language.  Returns non-zero on error.
 */
int triplestore_add(TripleStore *ts, const char *s, const char *p,
                    const char *o);


/**
  Adds a single triple to store.  The object is considered to be an
  english literal.  Returns non-zero on error.
 */
int triplestore_add_en(TripleStore *ts, const char *s, const char *p,
                    const char *o);


/**
  Adds a single triple to store.  The object is considered to be an URI.
  Returns non-zero on error.
 */
int triplestore_add_uri(TripleStore *ts, const char *s, const char *p,
                        const char *o);


/**
  Adds `n` triples to store.  Returns non-zero on error.
 */
int triplestore_add_triples(TripleStore *ts, const Triple *triples, size_t n);


/**
  Removes a triple identified by it's `id`.  Returns non-zero if no such
  triple can be found.
*/
int triplestore_remove_by_id(TripleStore *ts, const char *id);


/**
  Removes a triple identified by `s`, `p` and `o`.  Any of these may
  be NULL, allowing for multiple matches.  Returns the number of
  triples removed.
*/
int triplestore_remove(TripleStore *ts, const char *s,
                       const char *p, const char *o);


/**
  Removes all relations in triplestore and releases all references to
  external memory.  Only references to running iterators is kept.
 */
void triplestore_clear(TripleStore *ts);

/**
  Returns a pointer to triple with given id or NULL if no match can be found.
*/
const Triple *triplestore_get(const TripleStore *ts, const char *id);


/**
  Returns a pointer to first triple matching `s`, `p` and `o` or NULL
  if no match can be found.  Any of `s`, `p` or `o` may be NULL.
 */
const Triple *triplestore_find_first(const TripleStore *ts, const char *s,
                             const char *p, const char *o);


/**
  Initiates a TripleState for triplestore_find().
*/
void triplestore_init_state(TripleStore *ts, TripleState *state);

/**
  Deinitiates a TripleState initialised with triplestore_init_state().
*/
void triplestore_deinit_state(TripleState *state);

/**
  Resets iterator.
*/
void triplestore_reset_state(TripleState *state);


/**
  Increments state and returns a pointer to the current triple in the
  store or NULL if all triples have been visited.
 */
const Triple *triplestore_next(TripleState *state);

/**
  Returns a pointer to the current triple in the store or NULL if all
  triples have been visited.
 */
const Triple *triplestore_poll(TripleState *state);

/**
  This function should be called iteratively.  Before the first call
  it should be provided a `state` initialised with triplestore_init_state().

  For each call it will return a pointer to triple matching `s`, `p`
  and `o`.  Any of `s`, `p` or `o` may be NULL.  When no more matches
  can be found, NULL is returned.

  No other calls to triplestore_add() or triplestore_find() should be
  done while searching.

  NULL is also returned on error.
 */
const Triple *triplestore_find(TripleState *state,
                                const char *s, const char *p, const char *o);


/**
  Like triplestore_find(), but has two additional arguments.

  If `literal` is non-zero the object will be considered to be a
  literal, otherwise it is considered to be an URI.

  If `lang` is not NULL, it must be a valid XML language abbreviation,
  like "en". Only used if `literal` is non-zero.

  If redland is not available, it is equivalent to triplestore_find().
 */
const Triple *triplestore_find2(TripleState *state,
                                const char *s, const char *p, const char *o,
                                int literal, const char *lang);


#endif /* _TRIPLESTORE_H */
