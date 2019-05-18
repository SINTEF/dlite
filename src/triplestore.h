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

#include "utils/map.h"

/**
  A subject-predicate-object triplet used to represent a relation.
  The s-p-o-id strings should be allocated with malloc.
*/
typedef struct _Triplet {
  char *s;     /*!< subject */
  char *p;     /*!< predicate */
  char *o;     /*!< object */
  char *id;    /*!< unique ID identifying this triplet */
} Triplet;


/** Triplet store. */
typedef struct _TripleStore TripleStore;


/** State used by triplestore_find.
    Don't rely on current definition, it may be optimised later. */
typedef struct _TripleState {
  TripleStore *ts;    /*!< reference to corresponding TripleStore */
  size_t pos;         /*!< current position */
} TripleState;


/**
  Sets default namespace to be prepended to triplet id's.

  Use this function to convert the id's to proper URI's.
*/
void triplet_set_default_namespace(const char *namespace);

/**
  Returns default namespace.
*/
const char *triplet_get_default_namespace(void);

/**
  Frees up memory used by the s-p-o strings, but not the triplet itself.
*/
void triplet_clean(Triplet *t);

/**
  Convinient function to assign a triplet. This allocates new memory
  for the internal s, p, o and id pointers.  If `id` is
  NULL, a new id will be generated bases on `s`, `p` and `o`.
 */
int triplet_set(Triplet *t, const char *s, const char *p, const char *o,
                const char *id);

/**
  Like triplet_set(), but free's allocated memory in `t` before re-assigning
  it.  Don't use this function if `t` has not been initiated.
 */
int triplet_reset(Triplet *t, const char *s, const char *p, const char *o,
                  const char *id);

/**
  Returns an newly malloc'ed unique id calculated from triplet.

  If `namespace` is NULL, the default namespace set with
  triplet_set_default_namespace() will be used.

  Returns NULL on error.
*/
char *triplet_get_id(const char *namespace, const char *s, const char *p,
                      const char *o);


/**
  Returns a new empty triplestore that stores its triplets and the number of
  triplets in the external memory pointed to by `*p` and `*lenp`, respectively.

  `freer` is a cleanup-function.  If not NULL, it is called by
  triplestore_free() with `freedata` as argument.

  Returns NULL on error.
 */
TripleStore *triplestore_create_external(Triplet **p, size_t *lenp,
                                         void (*freer)(void *), void *freedata);


/**
  Returns a new empty triplestore or NULL on error.
 */
TripleStore *triplestore_create();


/**
  Frees triplestore.
 */
void triplestore_free(TripleStore *ts);


/**
  Returns the number of triplets in the store.
*/
size_t triplestore_length(TripleStore *ts);


/**
  Adds a single triplet to store.  Returns non-zero on error.
 */
int triplestore_add(TripleStore *ts, const char *s, const char *p,
                    const char *o);


/**
  Adds `n` triplets to store.  Returns non-zero on error.
 */
int triplestore_add_triplets(TripleStore *store, const Triplet *triplets,
                             size_t n);


/**
  Removes a triplet identified by it's `id`.  Returns non-zero if no such
  triplet can be found.
*/
int triplestore_remove_by_id(TripleStore *ts, const char *id);


/**
  Removes a triplet identified by `s`, `p` and `o`.  Any of these may
  be NULL, allowing for multiple matches.  Returns the number of
  triplets removed.
*/
int triplestore_remove(TripleStore *ts, const char *s,
                       const char *p, const char *o);


/**
  Returns a pointer to triplet with given id or NULL if no match can be found.
*/
const Triplet *triplestore_get(const TripleStore *ts, const char *id);


/**
  Returns a pointer to first triplet matching `s`, `p` and `o` or NULL
  if no match can be found.  Any of `s`, `p` or `o` may be NULL.
 */
const Triplet *triplestore_find_first(const TripleStore *ts, const char *s,
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
  Returns a pointer to the next triplet in the store or NULL if all
  triplets have been visited.
 */
const Triplet *triplestore_next(TripleState *state);

/**
  Returns a pointer to the current triplet in the store or NULL if all
  triplets have been visited.
 */
const Triplet *triplestore_poll(TripleState *state);

/**
  This function should be called iteratively.  Before the first call
  it should be provided a `state` initialised with triplestore_init_state().

  For each call it will return a pointer to triplet matching `s`, `p`
  and `o`.  Any of `s`, `p` or `o` may be NULL.  When no more matches
  can be found, NULL is returned.

  No other calls to triplestore_add() or triplestore_find() should be
  done while searching.
 */
const Triplet *triplestore_find(TripleState *state,
                                const char *s, const char *p, const char *o);


#endif /* _TRIPLESTORE_H */
