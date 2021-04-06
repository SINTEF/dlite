/* triplestore-builtin.c -- simple triplestore implementation */

/* This file will be included by triplestore.c */

/*
  TODO

  - Consider use a more advanced library like the [Redland RDF
    library](http://librdf.org/) (which depends on the sublibraries
    [rasqal](http://librdf.org/rasqal/) and
    [libraptor2](http://librdf.org/raptor/libraptor2.html)).

  - add locks to ensure that a triplestore is not reallocated or
    sorted while one works with pointers to the stored triples.
    Example:

        TRIPLE_ACQUIRE_LOCK(triplestore)
        triple = triplestore_find_first(triplestore, s, p, o)
        TRIPLE_RELEASE_LOCK(triplestore)
*/

#include <assert.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include "utils/err.h"
#include "utils/sha1.h"
#include "utils/map.h"
#include "triplestore.h"

#define UNUSED(x) (void)(x)



/* Allocate triplestore memory in chunks of TRIPLESTORE_BUFFSIZE */
#define TRIPLESTORE_BUFFSIZE 1024


/* Prototype for cleanup-function */
typedef void (*Freer)(void *ptr);

/* Triple store. */
struct _TripleStore {
  Triple *triples;  /*!< array of triples */
  size_t length;      /*!< logically number of triples (excluding pending
                           removes */
  size_t true_length; /*!< number of triples (including pending removes) */
  size_t size;        /*!< allocated number of triples */

  map_int_t map;      /*!< a mapping from triple id to its corresponding
                           index in `triples` */
  size_t niter;       /*!< counter for number of running iterators */
  int freed;          /*!< set to non-zero when this store is supposed to
                           be freed, but kept alive due to existing iterators */
};


/*
  Returns a new empty triplestore or NULL on error.
 */
TripleStore *triplestore_create()
{
  TripleStore *ts = calloc(1, sizeof(TripleStore));
  return ts;
}

/*
  Returns a new empty triplestore or NULL on error.
 */
TripleStore *triplestore_create_with_storage(const char *storage_name,
                                             const char *name,
                                             const char *options)
{
  UNUSED(storage_name);
  UNUSED(name);
  UNUSED(options);
  warn("builtin triplestore does not support a storage");
  return triplestore_create();
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
  Returns the number of triples in the store.
*/
size_t triplestore_length(TripleStore *ts)
{
  return ts->length;
}


///* Compare triples (in s-o-p order) */
//static int compar(const void *p1, const void *p2)
//{
//  int v;
//  Triple *t1 = (Triple *)p1;
//  Triple *t2 = (Triple *)p2;
//  if ((v = strcmp(t1->s, t2->s))) return v;
//  if ((v = strcmp(t1->o, t2->o))) return v;
//  return strcmp(t1->p, t2->p);
//}


/*
  Adds a single triple to store.  Returns non-zero on error.
 */
int triplestore_add(TripleStore *ts, const char *s, const char *p,
                    const char *o)
{
  Triple t;
  t.s = (char *)s;
  t.p = (char *)p;
  t.o = (char *)o;
  t.id = NULL;
  return triplestore_add_triples(ts, &t, 1);
}


/*
  Adds `n` triples to store.  Returns non-zero on error.
 */
int triplestore_add_triples(TripleStore *ts, const Triple *triples,
                             size_t n)
{
  size_t i;

  /* make space for new triples */
  if (ts->size < ts->true_length + n) {
    size_t m = (ts->true_length + n - ts->size) / TRIPLESTORE_BUFFSIZE;
    size_t size = ts->size + (m + 1) * TRIPLESTORE_BUFFSIZE;
    void *ptr;
    assert(size >= ts->true_length + n);
    if (!(ptr = realloc(ts->triples, size * sizeof(Triple))))
      return err(1, "allocation failure");
    ts->triples = ptr;
    ts->size = size;
    memset(ts->triples + ts->true_length, 0,
           (ts->size - ts->true_length)*sizeof(Triple));
  }

  /* append triples (avoid duplicates) */
  for (i=0; i<n; i++) {
    Triple *t = ts->triples + ts->true_length;
    char *id;
    if (triples[i].id) {
      if (!(id = strdup(triples[i].id))) return err(1, "allocation error");
    } else {
      if (!(id = triple_get_id(NULL, triples[i].s,
                                triples[i].p, triples[i].o)))
        return 1;
    }
    if (!map_get(&ts->map, id)) {
      if (!(t->s = strdup(triples[i].s))) return err(1, "allocation error");
      if (!(t->p = strdup(triples[i].p))) return err(1, "allocation error");
      if (!(t->o = strdup(triples[i].o))) return err(1, "allocation error");
      t->id = id;
      ts->length++;
      ts->true_length++;
      map_set(&ts->map, id, i);
    } else {
      free(id);
    }
  }

  return 0;
}


/* Removes triple number n.  Returns non-zero on error. */
static int _remove_by_index(TripleStore *ts, size_t n)
{
  Triple *t = ts->triples + n;
  if (n >= ts->true_length)
    return err(1, "triple index out of range: %zu", n);
  if (!t->id)
    return err(1, "triple %zu is already removed", n);
  map_remove(&ts->map, t->id);

  if (ts->niter) {
    /* running iterator, mark triple for deletion by setting id to NULL */
    free(t->id);
    t->id = NULL;
    ts->length--;
  } else {
    /* no running iterators, remove triple */
    assert(ts->length == ts->true_length);
    triple_clean(t);
    ts->length--;
    if (t < ts->triples + ts->length)
      memcpy(t, &ts->triples[ts->length], sizeof(Triple));
    ts->true_length = ts->length;
  }
  return 0;
}

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
  Removes a triple identified by `s`, `p` and `o`.  Any of these may
  be NULL, allowing for multiple matches.  Returns the number of
  triples removed.
*/
int triplestore_remove(TripleStore *ts, const char *s,
                       const char *p, const char *o)
{
  int i=ts->true_length, n=0;
  while (--i >= 0) {
    const Triple *t = ts->triples + i;
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
    if (ts->triples[n].id) _remove_by_index(ts, n);
  if (ts->triples) free(ts->triples);
  map_deinit(&ts->map);
  memset(ts, 0, sizeof(TripleStore));
  ts->niter = niter;
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


/*
  Returns a pointer to first triple matching `s`, `p` and `o` or NULL
  if no match can be found.  Any of `s`, `p` or `o` may be NULL.
 */
const Triple *triplestore_find_first(const TripleStore *ts, const char *s,
                                      const char *p, const char *o)
{
  size_t i;
  for (i=0; i<ts->true_length; i++) {
    const Triple *t = ts->triples + i;
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
    for (i=ts->true_length-1; i>=0 && !ts->triples[i].id; i--)
      ts->true_length--;
    for (i=ts->true_length-1; i>=0; i--) {
      Triple *t = ts->triples + i;
      if (!t->id) {
        Triple *tt = ts->triples + (--ts->true_length);
        assert(t < tt);
        triple_clean(t);
        memcpy(t, tt, sizeof(Triple));
      }
    }
    assert(ts->true_length == ts->length);
    if (ts->size > ts->length + TRIPLESTORE_BUFFSIZE) {
      ts->size = ts->length + ts->length % TRIPLESTORE_BUFFSIZE;
      ts->triples = realloc(ts->triples, ts->size*sizeof(Triple));
    }
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
  Increments state and returns a pointer to the current triple in the
  store or NULL if all triples have been visited.
 */
const Triple *triplestore_next(TripleState *state)
{
  TripleStore *ts = state->ts;
  while (state->pos < ts->true_length) {
    const Triple *t = ts->triples + state->pos++;
    if (t->id) return t;
  }
  return NULL;
}

/*
  Returns a pointer to the current triple in the store or NULL if all
  triples have been visited.
 */
const Triple *triplestore_poll(TripleState *state)
{
  TripleStore *ts = state->ts;
  while (state->pos < ts->true_length) {
    const Triple *t = ts->triples + state->pos;
    if (t->id) return t;
    state->pos++;
  }
  return NULL;
}


/*
  This function should be called iteratively.  Before the first call
  it should be provided a `state` initialised with triplestore_init_state().

  For each call it will return a pointer to triple matching `s`, `p`
  and `o`.  Any of `s`, `p` or `o` may be NULL.  When no more matches
  can be found, NULL is returned.
 */
const Triple *triplestore_find(TripleState *state,
                                const char *s, const char *p, const char *o)
{
  TripleStore *ts = state->ts;
  while (state->pos < ts->true_length) {
    const Triple *t = ts->triples + state->pos++;
    if (t->id &&
        (!s || strcmp(s, t->s) == 0) &&
        (!p || strcmp(p, t->p) == 0) &&
        (!o || strcmp(o, t->o) == 0))
      return t;
  }
  return NULL;
}
