/* This is a very naiv implementation of a triplestore with O(n)
   lookup time and a full sorting of the entire table for each insert
   to avoid dublicates.

   Optimasations are definitely possible.  Possible routes:
     - use a third-party library
     - smart use of hashtables, e.g. with map.h (?)
     - use another datastructure (?)
*/

/* TODO
   - add locks to ensure that a triplestore is not reallocated or
     sorted while one works with pointers to the stored triplets.
     Example:

         TRIPLET_ACQUIRE_LOCK(triplestore)
         triplet = triplestore_find_first(triplestore, s, p, o)
         TRIPLET_RELEASE_LOCK(triplestore)
*/

#include <assert.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include "compat.h"
#include "err.h"
#include "sha1.h"
#include "triplestore.h"


/* Allocate triplestore memory in chunks of TRIPLESTORE_BUFFSIZE */
#define TRIPLESTORE_BUFFSIZE 1024



/* Default namespace */
char *default_namespace = NULL;

/* Free's default_namespace. Called by atexit(). */
static void free_default_namespace()
{
  if (default_namespace) free(default_namespace);
  default_namespace = NULL;
}

/* Sets default namespace to be prepended to triplet uri's. */
void triplet_set_default_namespace(const char *namespace)
{
  if (default_namespace) {
    free(default_namespace);
    atexit(free_default_namespace);
  }
  if (namespace)
    default_namespace = strdup(namespace);
  else
    default_namespace = NULL;
}


/*
  Frees up memory used by the s-p-o strings, but not the triplet itself.
*/
void triplet_clean(Triplet *t)
{
  free(t->s);
  free(t->p);
  free(t->o);
  free(t->uri);
}

/*
  Convinient function to assign a triplet. This allocates new memory
  for the internal s, p, o and uri pointers.  If `uri` is
  NULL, a new uri will be generated bases on `s`, `p` and `o`.
 */
int triplet_set(Triplet *t, const char *s, const char *p, const char *o,
                const char *uri)
{
  t->s = strdup(s);
  t->p = strdup(p);
  t->o = strdup(o);
  if (uri) {
    if (t->uri) free(t->uri);
    t->uri = strdup(uri);
  } else {
    t->uri = triplet_get_uri(NULL, s, p, o);
  }
  return 0;
}

/*
  Returns an newly malloc'ed hash string calculated from triplet.
*/
char *triplet_get_uri(const char *namespace, const char *s, const char *p,
                      const char *o)
{
  SHA1_CTX context;
  unsigned char digest[20];
  char *uri;
  int i, n=0, size=41;
  SHA1Init(&context);
  SHA1Update(&context, (unsigned char *)s, strlen(s));
  SHA1Update(&context, (unsigned char *)p, strlen(p));
  SHA1Update(&context, (unsigned char *)o, strlen(o));
  SHA1Final(digest, &context);
  if (!namespace) namespace = default_namespace;
  if (namespace) size += strlen(namespace);
  if (!(uri = malloc(size))) return NULL;
  if (namespace) n += snprintf(uri+n, size-n, "%s", namespace);
  for (i=0; i<20; i++)
    n += snprintf(uri+n, size-n, "%02x", digest[i]);
  return uri;
}

/*
  Returns a new empty triplestore or NULL on error.
 */
TripleStore *triplestore_create()
{
  TripleStore *store = calloc(1, sizeof(TripleStore));
  return store;
}


/*
  Frees triplestore `ts`.
 */
void triplestore_free(TripleStore *store)
{
  size_t i;
  for (i=0; i<store->length; i++)
    triplet_clean(store->triplets + i);
  if (store->triplets) free(store->triplets);
  free(store);
}


/*
  Returns the number of triplets in the store.
*/
size_t triplestore_length(TripleStore *store)
{
  return store->length;
}


/* Compare triplets (in s-o-p order) */
static int compar(const void *p1, const void *p2)
{
  int v;
  Triplet *t1 = (Triplet *)p1;
  Triplet *t2 = (Triplet *)p2;
  if ((v = strcmp(t1->s, t2->s))) return v;
  if ((v = strcmp(t1->o, t2->o))) return v;
  return strcmp(t1->p, t2->p);
}


/*
  Adds a single triplet to store.  Returns non-zero on error.
 */
int triplestore_add(TripleStore *store, const char *s, const char *p,
                    const char *o)
{
  Triplet t;
  t.s = (char *)s;
  t.p = (char *)p;
  t.o = (char *)o;
  return triplestore_add_triplets(store, &t, 1);
}


/*
  Adds `n` triplets to store.  Returns non-zero on error.
 */
int triplestore_add_triplets(TripleStore *store, const Triplet *triplets,
                             size_t n)
{
  size_t i;
  Triplet *t;

  if (store->size < store->length + n) {
    size_t m = (store->length + n - store->size) / TRIPLESTORE_BUFFSIZE;
    store->size += (m + 1) * TRIPLESTORE_BUFFSIZE;
    assert(store->size >= store->length + n);
    store->triplets = realloc(store->triplets, store->size * sizeof(Triplet));
    memset(store->triplets + store->length, 0,
           (store->size - store->length)*sizeof(Triplet));
  }

  /* append triplets */
  t = store->triplets + store->length;
  for (i=0; i<n; i++, t++)
    triplet_set(t, triplets[i].s, triplets[i].p, triplets[i].o, NULL);
  store->length += n;

  /* sort */
  t = store->triplets;
  qsort(t, store->length, sizeof(Triplet), compar);

  /* remove dublicates */
  for (i=1; i<store->length; i++) {
    if (compar(&t[i], &t[i-1]) == 0) {
      while (i < store->length && compar(&t[store->length - 1], &t[i-1]) == 0)
        triplet_clean(t + --store->length);
      if (i < store->length) {
        triplet_clean(t + i);
        t[i] = t[--store->length];
      }
    }
  }

  return 0;
}


/* Removes triplet number n. */
static int _remove(TripleStore *store, size_t n)
{
  Triplet *t = store->triplets;
  if (n >= store->length)
    return err(1, "invalid triplet index: %lu", n);
  triplet_clean(t + n);
  memcpy(t + n, t + --store->length, sizeof(Triplet));
  return 0;
}

/*
  Removes a triplet identified by it's `uri`.  Returns non-zero if no such
  triplet can be found.
*/
int triplestore_remove_by_uri(TripleStore *store, const char *uri)
{
  size_t i;
  for (i=0; i<store->length; i++)
    if (strcmp(store->triplets[i].uri, uri) == 0)
      return _remove(store, i);
  return 1;
}

/*
  Removes a triplet identified by `s`, `p` and `o`.  Any of these may
  be NULL, allowing for multiple matches.  Returns the number of
  triplets removed.
*/
int triplestore_remove(TripleStore *store, const char *s,
                       const char *p, const char *o)
{
  size_t i=0, n=0;
  while (i < store->length) {
    const Triplet *t = store->triplets + i;
    if ((!s || strcmp(s, t->s) == 0) &&
        (!p || strcmp(p, t->p) == 0) &&
        (!o || strcmp(o, t->o) == 0)) {
      if (_remove(store, i) == 0) n++;
    }
    i++;
  }
  return n;
}


/*
  Returns a pointer to triplet with given uri or NULL if no match can be found.
*/
const Triplet *triplestore_get_uri(const TripleStore *store, const char *uri)
{
  size_t i;
  for (i=0; i<store->length; i++)
    if (strcmp(store->triplets[i].uri, uri) == 0)
      return (Triplet *)&store->triplets[i];
  return NULL;
}


/*
  Returns a pointer to first triplet matching `s`, `p` and `o` or NULL
  if no match can be found.  Any of `s`, `p` or `o` may be NULL.
 */
const Triplet *triplestore_find_first(const TripleStore *store, const char *s,
                                      const char *p, const char *o)
{
  size_t i;
  for (i=0; i<store->length; i++) {
    const Triplet *t = store->triplets + i;
    if ((!s || strcmp(s, t->s) == 0) &&
        (!p || strcmp(p, t->p) == 0) &&
        (!o || strcmp(o, t->o) == 0))
      return t;
  }
  return NULL;
}


/*
  Initiates a TripleState for triplestore_find().
*/
void triplestore_init_state(const TripleStore *store, TripleState *state)
{
  (void)store;  /* unused */
  state->pos = 0;
}


/*
  This function should be called iteratively.  Before the first call
  it should be provided a `state` initialised with triplestore_init_state().

  For each call it will return a pointer to triplet matching `s`, `p`
  and `o`.  Any of `s`, `p` or `o` may be NULL.  When no more matches
  can be found, NULL is returned.
 */
const Triplet *triplestore_find(const TripleStore *store, TripleState *state,
                       const char *s, const char *p, const char *o)
{
  while (state->pos < store->length) {
    const Triplet *t = store->triplets + state->pos++;
    if ((!s || strcmp(s, t->s) == 0) &&
        (!p || strcmp(p, t->p) == 0) &&
        (!o || strcmp(o, t->o) == 0))
      return t;
  }
  return NULL;
}
