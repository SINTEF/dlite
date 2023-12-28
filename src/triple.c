/* dlite-triple.c -- a subject-predicate-object triple */

#include <assert.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include "utils/err.h"
#include "utils/sha1.h"
#include "utils/session.h"
#include "triple.h"

#define TRIPLE_GLOBALS_ID "triple-globals-id"

/* Global variables for this modules */
typedef struct {
  char *default_namespace;
} TripleGlobals;


/* Free's global variables. */
static void free_globals(void *globals)
{
  TripleGlobals *g = globals;
  if (g->default_namespace) free(g->default_namespace);
  free(g);
}

/* Sets default namespace to be prepended to triple id's. */
void triple_set_default_namespace(const char *namespace)
{
  Session *s = session_get_default();
  TripleGlobals *g = session_get_state(s, TRIPLE_GLOBALS_ID);
  if (g->default_namespace)
    free(g->default_namespace);
  if (namespace)
    g->default_namespace = strdup(namespace);
  else
    g->default_namespace = NULL;
}

/* Returns default namespace. */
const char *triple_get_default_namespace(void)
{
  Session *s = session_get_default();
  TripleGlobals *g = session_get_state(s, TRIPLE_GLOBALS_ID);
  if (!g) {
    if (!(g = calloc(1, sizeof(TripleGlobals))))
      return err(1, "allocation failure"), NULL;
    session_add_state(s, TRIPLE_GLOBALS_ID, g, free_globals);
  }
  return (const char *)g->default_namespace;
}


/*
  Frees up memory used by the s-p-o strings, but not the triple itself.
*/
void triple_clean(Triple *t)
{
  if (t->s) free(t->s);
  if (t->p) free(t->p);
  if (t->o) free(t->o);
  if (t->d) free(t->d);
  if (t->id) free(t->id);
  memset(t, 0, sizeof(Triple));
}

/*
  Convinient function to assign a triple. This allocates new memory
  for the internal s, p, o and id pointers.  If `id` is
  NULL, a new id will be generated bases on `s`, `p` and `o`.
 */
int triple_set(Triple *t, const char *s, const char *p, const char *o,
               const char *d, const char *id)
{
  t->s = strdup((s) ? s : "");
  t->p = strdup((p) ? p : "");
  t->o = strdup((o) ? o : "");
  t->d = (d) ? strdup(d) : NULL;
  t->id = (id) ? strdup(id) : triple_get_id(NULL, s, p, o, d);
  return 0;
}

/*
  Like triple_set(), but free's allocated memory in `t` before re-assigning
  it.  Don't use this function if `t` has not been initiated.
 */
int triple_reset(Triple *t, const char *s, const char *p, const char *o,
                 const char *d, const char *id)
{
  triple_clean(t);
  return triple_set(t, s, p, o, d, id);
}

/*
  Returns an newly malloc'ed hash string calculated from triple.
  Returns NULL on error, for instance if any of `s`, `p` or `o` are NULL.
*/
char *triple_get_id(const char *namespace, const char *s, const char *p,
                    const char *o, const char *d)
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
  if (d) SHA1Update(&context, (unsigned char *)d, strlen(d));
  SHA1Final(digest, &context);
  if (!namespace) namespace = triple_get_default_namespace();
  if (namespace) size += strlen(namespace);
  if (!(id = malloc(size))) return NULL;
  if (namespace) n += snprintf(id+n, size-n, "%s", namespace);
  for (i=0; i<20; i++)
    n += snprintf(id+n, size-n, "%02x", digest[i]);
  return id;
}

/*
  Copies triple `src` to `dest` and returns a pointer to `dest`.

  Existing memory hold by `dest` is not free'ed.  So if `dest` may
  hold some memory, call `triple_clean()` before calling this
  function.
 */
Triple *triple_copy(Triple *dest, const Triple *src)
{
  assert(src);
  assert(dest);
  memset(dest, 0, sizeof(Triple));
  if (src->s  && !(dest->s = strdup(src->s)))   goto fail;
  if (src->p  && !(dest->p = strdup(src->p)))   goto fail;
  if (src->o  && !(dest->o = strdup(src->o)))   goto fail;
  if (src->d  && !(dest->d = strdup(src->d)))   goto fail;
  if (src->id && !(dest->id = strdup(src->id))) goto fail;
  return dest;
 fail:
  err(1, "allocation failure");
  return NULL;
}
