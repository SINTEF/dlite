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

//#include "triplestore-private.h"

/**
  A subject-predicate-object triplet used to represent a relation.
  The s-p-o strings are assumed to be allocated with malloc.

  The uri uniquely identifies a triplet and allows the subject or
  object to refer to another triplet.
  See https://en.wikipedia.org/wiki/Semantic_triple.
*/
typedef struct _Triplet {
  char *s;     /*!< subject */
  char *p;     /*!< predicate */
  char *o;     /*!< object */
  char *uri;   /*!< unique URI identifying this triplet */
} Triplet;

/** Triplet store. */
typedef struct _TripleStore {
  Triplet *triplets;       /*!< array of triplets */
  size_t length;           /*!< number of triplets */
  size_t size;             /*!< allocated size */
} TripleStore;

/** State used by triplestore_find.
    Don't rely on current definition, it may be optimised later. */
typedef struct _TripleState {
  size_t pos;              /*!< current position */
} TripleState;




/**
    Sets default namespace to be prepended to triplet uri's.
*/
void triplet_set_default_namespace(const char *namespace);

/**
  Frees up memory used by the s-p-o strings, but not the triplet itself.
*/
void triplet_clean(Triplet *t);

/**
  Convinient function to assign a triplet. This allocates new memory
  for the internal s, p, o and uri pointers.  If `uri` is
  NULL, a new uri will be generated bases on `s`, `p` and `o`.
 */
int triplet_set(Triplet *t, const char *s, const char *p, const char *o,
                const char *uri);

/**
  Returns an newly malloc'ed unique uri calculated from triplet.
*/
char *triplet_get_uri(const char *namespace, const char *s, const char *p,
                      const char *o);



/**
  Returns a new empty triplestore or NULL on error.
 */
TripleStore *triplestore_create();


/**
  Frees triplestore `ts`.
 */
void triplestore_free(TripleStore *store);


/**
  Returns the number of triplets in the store.
*/
size_t triplestore_length(TripleStore *store);


/**
  Adds a single triplet to store.  Returns non-zero on error.
 */
int triplestore_add(TripleStore *store, const char *s, const char *p,
                    const char *o);


/**
  Adds `n` triplets to store.  Returns non-zero on error.
 */
int triplestore_add_triplets(TripleStore *store, const Triplet *triplets,
                             size_t n);


/**
  Removes a triplet identified by it's id.  Returns non-zero if no such
  triplet can be found.
*/
int triplestore_remove_by_id(TripleStore *store, const char *id);


/**
  Removes a triplet identified by `s`, `p` and `o`.  Any of these may
  be NULL, allowing for multiple matches.  Returns the number of
  triplets removed.
*/
int triplestore_remove(TripleStore *store, const char *s,
                       const char *p, const char *o);


/**
  Returns a pointer to triplet with given id or NULL if no match can be found.
*/
const Triplet *triplestore_get_id(const TripleStore *store, const char *id);


/**
  Returns a pointer to first triplet matching `s`, `p` and `o` or NULL
  if no match can be found.  Any of `s`, `p` or `o` may be NULL.
 */
const Triplet *triplestore_find_first(const TripleStore *store, const char *s,
                             const char *p, const char *o);


/**
  Initiates a TripleState for triplestore_find().
*/
void triplestore_init_state(const TripleStore *store, TripleState *state);


/**
  This function should be called iteratively.  Before the first call
  it should be provided a `state` initialised with triplestore_init_state().

  For each call it will return a pointer to triplet matching `s`, `p`
  and `o`.  Any of `s`, `p` or `o` may be NULL.  When no more matches
  can be found, NULL is returned.

  No other calls to triplestore_add() or triplestore_find() should be
  done while searching.
 */
const Triplet *triplestore_find(const TripleStore *store, TripleState *state,
                       const char *s, const char *p, const char *o);


#endif /* _TRIPLESTORE_H */
