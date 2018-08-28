/**
  A very simple triplestore for strings
  =====================================
  Triplets can be referred to as objects, using their id.
  This is supported by the triplet_get_id() and ts_get_id() functions.

  Should probably be replaced with more advanced libraries like
  [libraptor2](http://librdf.org/raptor/libraptor2.html).
 */
#ifndef _TRIPLESTORE_H
#define _TRIPLESTORE_H

#include "triplestore-private.h"

/**
  A subject-predicate-object triplet used to represent a relation.
  The s-p-o strings are assumed to be allocated with malloc.
*/
typedef struct _Triplet {
  char *s;     /*!< subject */
  char *p;     /*!< predicate */
  char *o;     /*!< object */
} Triplet;

/** Triplet store. */
typedef struct _Triplestore Triplestore;

/** State used by triplestore_find. */
typedef struct _TripleState TripleState;




/**
  Frees up memory used by the s-p-o strings, but not the triplet itself.
*/
void triplet_clean(Triplet *t);

/**
  Convinient function to assign a triplet. This allocates new memory
  for the internal s-p-o pointers.
 */
int triplet_set(Triplet *t, const char *s, const char *p, const char *o);

/**
  Returns an newly malloc'ed hash string calculated from triplet.
*/
char *triplet_get_id(const Triplet *t);



/**
  Returns a new empty triplestore or NULL on error.
 */
Triplestore *triplestore_create();


/**
  Frees triplestore `ts`.
 */
void triplestore_free(Triplestore *store);


/**
  Returns the number of triplets in the store.
*/
size_t triplestore_length(Triplestore *store);


/**
  Adds a single triplet to store.  Returns non-zero on error.
 */
int triplestore_add(Triplestore *store, const char *s, const char *p,
                    const char *o);


/**
  Adds `n` triplets to store.  Returns non-zero on error.
 */
int triplestore_add_triplets(Triplestore *store, const Triplet *triplets,
                             size_t n);


/**
  Removes a triplet identified by it's id.  Returns non-zero if no such
  triplet can be found.
*/
int triplestore_remove_by_id(Triplestore *store, const char *id);


/**
  Removes a triplet identified by `s`, `p` and `o`.  Any of these may
  be NULL, allowing for multiple matches.  Returns the number of
  triplets removed.
*/
int triplestore_remove(Triplestore *store, const char *s,
                       const char *p, const char *o);


/**
  Returns a pointer to triplet with given id or NULL if no match can be found.
*/
const Triplet *triplestore_get_id(const Triplestore *store, const char *id);


/**
  Returns a pointer to first triplet matching `s`, `p` and `o` or NULL
  if no match can be found.  Any of `s`, `p` or `o` may be NULL.
 */
const Triplet *triplestore_find_first(const Triplestore *store, const char *s,
                             const char *p, const char *o);


/**
  Initiates a TripleState for triplestore_find().
*/
void triplestore_init_state(const Triplestore *store, TripleState *state);


/**
  This function should be called iteratively.  Before the first call
  it should be provided a `state` initialised with triplestore_init_state().

  For each call it will return a pointer to triplet matching `s`, `p`
  and `o`.  Any of `s`, `p` or `o` may be NULL.  When no more matches
  can be found, NULL is returned.

  No other calls to triplestore_add() or triplestore_find() should be
  done while searching.
 */
const Triplet *triplestore_find(const Triplestore *store, TripleState *state,
                       const char *s, const char *p, const char *o);


#endif /* _TRIPLESTORE_H */
