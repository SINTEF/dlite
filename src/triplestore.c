/* triplestore.c -- simple triplestore implementation */

/*
  TODO

  - Consider use a more advanced library like the [Redland RDF
    library](http://librdf.org/) (which depends on the sublibraries
    [rasqal](http://librdf.org/rasqal/) and
    [libraptor2](http://librdf.org/raptor/libraptor2.html)).

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

#include "utils/err.h"
#include "utils/sha1.h"
#include "triplestore.h"


/* Allocate triplestore memory in chunks of TRIPLESTORE_BUFFSIZE */
#define TRIPLESTORE_BUFFSIZE 1024


/* Prototype for cleanup-function */
typedef void (*Freer)(void *ptr);

/* Triplet store. */
struct _TripleStore {
  Triplet *triplets;  /*!< array of triplets */
  size_t length;      /*!< logically number of triplets (excluding pending
                           removes */
  size_t true_length; /*!< number of triplets (including pending removes) */
  size_t size;        /*!< allocated number of triplets */

  Triplet **p;        /*!< pointer to external memory pointing to triplets */
  size_t *lenp;       /*!< pointer to external memory pointing to length */
  Freer freer;        /*!< cleanup-function called by tripletstore_free() */
  void *freedata;     /*!< data passed to `freer` */

  map_int_t map;      /*!< a mapping from triplet id to its corresponding
                           index in `triplets` */
  size_t niter;       /*!< counter for number of running iterators */
  int freed;          /*!< set to non-zero when this store is supposed to
                           be freed, but kept alive due to existing iterators */
};


/* Default namespace */
static char *default_namespace = NULL;
static int atexit_registered = 0;

/* Free's default_namespace. Called by atexit(). */
static void free_default_namespace(void)
{
  if (default_namespace) free(default_namespace);
  default_namespace = NULL;
}

/* Sets default namespace to be prepended to triplet id's. */
void triplet_set_default_namespace(const char *namespace)
{
  if (!atexit_registered)
    atexit(free_default_namespace);
  if (default_namespace)
    free(default_namespace);
  if (namespace)
    default_namespace = strdup(namespace);
  else
    default_namespace = NULL;
}

/* Returns default namespace. */
const char *triplet_get_default_namespace(void)
{
  return (const char *)default_namespace;
}


/*
  Frees up memory used by the s-p-o strings, but not the triplet itself.
*/
void triplet_clean(Triplet *t)
{
  free(t->s);
  free(t->p);
  free(t->o);
  if (t->id) free(t->id);
  memset(t, 0, sizeof(Triplet));
}

/*
  Convinient function to assign a triplet. This allocates new memory
  for the internal s, p, o and id pointers.  If `id` is
  NULL, a new id will be generated bases on `s`, `p` and `o`.
 */
int triplet_set(Triplet *t, const char *s, const char *p, const char *o,
                const char *id)
{
  t->s = strdup((s) ? s : "");
  t->p = strdup((p) ? p : "");
  t->o = strdup((o) ? o : "");
  t->id = (id) ? strdup(id) : triplet_get_id(NULL, s, p, o);
  return 0;
}

/*
  Like triplet_set(), but free's allocated memory in `t` before re-assigning
  it.  Don't use this function if `t` has not been initiated.
 */
int triplet_reset(Triplet *t, const char *s, const char *p, const char *o,
                const char *id)
{
  if (t->s)  free(t->s);
  if (t->p)  free(t->p);
  if (t->o)  free(t->o);
  if (t->id) free(t->id);
  return triplet_set(t, s, p, o, id);
}


/*
  Returns an newly malloc'ed hash string calculated from triplet.
  Returns NULL on error, for instance if any of `s`, `p` or `o` are NULL.
*/
char *triplet_get_id(const char *namespace, const char *s, const char *p,
                      const char *o)
{
  SHA1_CTX context;
  unsigned char digest[20];
  char *id;
  int i, n=0;
  size_t size=41;
  if (!s || !p || !o) return NULL;
  SHA1Init(&context);
  SHA1Update(&context, (unsigned char *)s, strlen(s));
  SHA1Update(&context, (unsigned char *)p, strlen(p));
  SHA1Update(&context, (unsigned char *)o, strlen(o));
  SHA1Final(digest, &context);
  if (!namespace) namespace = default_namespace;
  if (namespace) size += strlen(namespace);
  if (!(id = malloc(size))) return NULL;
  if (namespace) n += snprintf(id+n, size-n, "%s", namespace);
  for (i=0; i<20; i++)
    n += snprintf(id+n, size-n, "%02x", digest[i]);
  return id;
}

/*
  Returns a new empty triplestore that stores its triplets and the number of
  triplets in the external memory pointed to by `*p` and `*lenp`, respectively.

  If `p` and `lenp` are NULL, internal memory is allocated.

  `freer` is a cleanup-function.  If not NULL, it is called by
  triplestore_free() with `freedata` as argument.

  Returns NULL on error.
 */
TripleStore *triplestore_create_external(Triplet **p, size_t *lenp,
                                         void (*freer)(void *), void *freedata)
{
  TripleStore *ts = calloc(1, sizeof(TripleStore));
  if (p) {
    ts->triplets = *p;
    if (*p) {
      if (!lenp) {
        free(ts);
        return errx(1, "in triplestore_create_external(): `lenp`, the number "
                    "of external triplets must be provided if `*p` is not "
                    "NULL"), NULL;
      }
      ts->length = ts->true_length = ts->size = *lenp;
    }
  }
  ts->p = p;
  ts->lenp = lenp;
  ts->freer = freer;
  ts->freedata = freedata;
  map_init(&ts->map);
  return ts;
}


/*
  Returns a new empty triplestore or NULL on error.
 */
TripleStore *triplestore_create()
{
  return triplestore_create_external(NULL, NULL, NULL, NULL);
}


/*
  Frees triplestore `ts`.
 */
void triplestore_free(TripleStore *ts)
{
  assert(ts->freed == 0 || ts->niter == 0);
  triplestore_clear(ts);
  if (ts->niter > 0)
    ts->freed = 1;
  else
    free(ts);
}


/*
  Returns the number of triplets in the store.
*/
size_t triplestore_length(TripleStore *ts)
{
  return ts->length;
}


///* Compare triplets (in s-o-p order) */
//static int compar(const void *p1, const void *p2)
//{
//  int v;
//  Triplet *t1 = (Triplet *)p1;
//  Triplet *t2 = (Triplet *)p2;
//  if ((v = strcmp(t1->s, t2->s))) return v;
//  if ((v = strcmp(t1->o, t2->o))) return v;
//  return strcmp(t1->p, t2->p);
//}


/*
  Adds a single triplet to store.  Returns non-zero on error.
 */
int triplestore_add(TripleStore *ts, const char *s, const char *p,
                    const char *o)
{
  Triplet t;
  t.s = (char *)s;
  t.p = (char *)p;
  t.o = (char *)o;
  t.id = NULL;
  return triplestore_add_triplets(ts, &t, 1);
}


/*
  Adds `n` triplets to store.  Returns non-zero on error.
 */
int triplestore_add_triplets(TripleStore *ts, const Triplet *triplets,
                             size_t n)
{
  size_t i;

  /* make space for new triplets */
  if (ts->size < ts->true_length + n) {
    size_t m = (ts->true_length + n - ts->size) / TRIPLESTORE_BUFFSIZE;
    size_t size = ts->size + (m + 1) * TRIPLESTORE_BUFFSIZE;
    void *ptr;
    assert(size >= ts->true_length + n);
    if (!(ptr = realloc(ts->triplets, size * sizeof(Triplet))))
      return err(1, "allocation failure");
    ts->triplets = ptr;
    ts->size = size;
    memset(ts->triplets + ts->true_length, 0,
           (ts->size - ts->true_length)*sizeof(Triplet));
    if (ts->p) *ts->p = ts->triplets;
  }

  /* append triplets (avoid duplicates) */
  for (i=0; i<n; i++) {
    Triplet *t = ts->triplets + ts->true_length;
    char *id;
    if (triplets[i].id) {
      if (!(id = strdup(triplets[i].id))) return err(1, "allocation error");
    } else {
      if (!(id = triplet_get_id(NULL, triplets[i].s,
                                triplets[i].p, triplets[i].o)))
        return 1;
    }
    if (!map_get(&ts->map, id)) {
      if (!(t->s = strdup(triplets[i].s))) return err(1, "allocation error");
      if (!(t->p = strdup(triplets[i].p))) return err(1, "allocation error");
      if (!(t->o = strdup(triplets[i].o))) return err(1, "allocation error");
      t->id = id;
      ts->length++;
      ts->true_length++;
      if (ts->lenp) *ts->lenp = ts->true_length;
      map_set(&ts->map, id, i);
    } else {
      free(id);
    }
  }

  return 0;
}


/* Removes triplet number n.  Returns non-zero on error. */
static int _remove_by_index(TripleStore *ts, size_t n)
{
  Triplet *t = ts->triplets + n;
  if (n >= ts->true_length)
    return err(1, "triplet index out of range: %zu", n);
  if (!t->id)
    return err(1, "triplet %zu is already removed", n);
  map_remove(&ts->map, t->id);

  if (ts->niter) {
    /* running iterator, mark triplet for deletion by setting id to NULL */
    free(t->id);
    t->id = NULL;
    ts->length--;
  } else {
    /* no running iterators, remove triplet */
    assert(ts->length == ts->true_length);
    triplet_clean(t);
    ts->length--;
    if (t < ts->triplets + ts->length)
      memcpy(t, &ts->triplets[ts->length], sizeof(Triplet));
    ts->true_length = ts->length;
    if (ts->lenp) *ts->lenp = ts->length;
  }
  return 0;
}

/*
  Removes a triplet identified by it's `id`.  Returns non-zero on
  error or if no such triplet can be found.
*/
int triplestore_remove_by_id(TripleStore *ts, const char *id)
{
  int *n;
  if (!(n = map_get(&ts->map, id)))
    return err(1, "no such triplet id: \"%s\"", id);
  return _remove_by_index(ts, *n);
}

/*
  Removes a triplet identified by `s`, `p` and `o`.  Any of these may
  be NULL, allowing for multiple matches.  Returns the number of
  triplets removed.
*/
int triplestore_remove(TripleStore *ts, const char *s,
                       const char *p, const char *o)
{
  int i=ts->true_length, n=0;
  while (--i >= 0) {
    const Triplet *t = ts->triplets + i;
    if (!t->id) continue;
    if ((!s || strcmp(s, t->s) == 0) &&
        (!p || strcmp(p, t->p) == 0) &&
        (!o || strcmp(o, t->o) == 0)) {
      if (_remove_by_index(ts, i) == 0) n++;
    }
  }
  return n;
}


/*
  Removes all relations in triplestore and releases all references to
  external memory.  Only references to running iterators is kept.
 */
void triplestore_clear(TripleStore *ts)
{
  int n=ts->true_length;
  int niter=ts->niter;
  while (--n >= 0)
    if (ts->triplets[n].id) _remove_by_index(ts, n);
  if (!ts->p && ts->triplets)
    free(ts->triplets);
  map_deinit(&ts->map);
  memset(ts, 0, sizeof(TripleStore));
  ts->niter = niter;
}


/*
  Returns a pointer to triplet with given id or NULL if no match can be found.
*/
const Triplet *triplestore_get(const TripleStore *ts, const char *id)
{
  int *n = map_get(&((TripleStore *)ts)->map, id);
  if (!n)
    return errx(1, "no triplet with id \"%s\"", id), NULL;
  if (!ts->triplets[*n].id)
    return errx(1, "triplet \"%s\" has been removed", id), NULL;
  return ts->triplets + *n;
}


/*
  Returns a pointer to first triplet matching `s`, `p` and `o` or NULL
  if no match can be found.  Any of `s`, `p` or `o` may be NULL.
 */
const Triplet *triplestore_find_first(const TripleStore *ts, const char *s,
                                      const char *p, const char *o)
{
  size_t i;
  for (i=0; i<ts->true_length; i++) {
    const Triplet *t = ts->triplets + i;
    if (t->id &&
        (!s || strcmp(s, t->s) == 0) &&
        (!p || strcmp(p, t->p) == 0) &&
        (!o || strcmp(o, t->o) == 0))
      return t;
  }
  return NULL;
}


/*
  Initiates a TripleState for triplestore_find().  The state must be
  deinitialised with triplestore_deinit_state().
*/
void triplestore_init_state(TripleStore *ts, TripleState *state)
{
  state->ts = ts;
  ts->niter++;
  state->pos = 0;
}


/*
  Deinitiates a TripleState initialised with triplestore_init_state().
*/
void triplestore_deinit_state(TripleState *state)
{
  TripleStore *ts = state->ts;
  int i;
  assert(ts->niter > 0 /* must match triplestore_init_state() */);
  ts->niter--;

  /* Number of pending iterators has reased zero - free the triplestore  */
  if (ts->freed && ts->niter <= 0) {
    triplestore_free(ts);
    return;
  }

  if (ts->niter == 0 && ts->true_length > ts->length) {
    for (i=ts->true_length-1; i>=0 && !ts->triplets[i].id; i--)
      ts->true_length--;
    for (i=ts->true_length-1; i>=0; i--) {
      Triplet *t = ts->triplets + i;
      if (!t->id) {
        Triplet *tt = ts->triplets + (--ts->true_length);
        assert(t < tt);
        triplet_clean(t);
        memcpy(t, tt, sizeof(Triplet));
      }
    }
    assert(ts->true_length == ts->length);
    if (ts->size > ts->length + TRIPLESTORE_BUFFSIZE) {
      ts->size = ts->length + ts->length % TRIPLESTORE_BUFFSIZE;
      ts->triplets = realloc(ts->triplets, ts->size*sizeof(Triplet));
      if (ts->p) *ts->p = ts->triplets;
    }
    if (ts->lenp) *ts->lenp = ts->length;
  }
}


/*
  Resets iterator.
*/
void triplestore_reset_state(TripleState *state)
{
  state->pos = 0;
}


/*
  Returns a pointer to the next triplet in the store or NULL if all
  triplets have been visited.
 */
const Triplet *triplestore_next(TripleState *state)
{
  TripleStore *ts = state->ts;
  while (state->pos < ts->true_length) {
    const Triplet *t = ts->triplets + state->pos++;
    if (t->id) return t;
  }
  return NULL;
}

/*
  Returns a pointer to the current triplet in the store or NULL if all
  triplets have been visited.
 */
const Triplet *triplestore_poll(TripleState *state)
{
  TripleStore *ts = state->ts;
  while (state->pos < ts->true_length) {
    const Triplet *t = ts->triplets + state->pos;
    if (t->id) return t;
    state->pos++;
  }
  return NULL;
}


/*
  This function should be called iteratively.  Before the first call
  it should be provided a `state` initialised with triplestore_init_state().

  For each call it will return a pointer to triplet matching `s`, `p`
  and `o`.  Any of `s`, `p` or `o` may be NULL.  When no more matches
  can be found, NULL is returned.
 */
const Triplet *triplestore_find(TripleState *state,
                                const char *s, const char *p, const char *o)
{
  TripleStore *ts = state->ts;
  while (state->pos < ts->true_length) {
    const Triplet *t = ts->triplets + state->pos++;
    if (t->id &&
        (!s || strcmp(s, t->s) == 0) &&
        (!p || strcmp(p, t->p) == 0) &&
        (!o || strcmp(o, t->o) == 0))
      return t;
  }
  return NULL;
}
