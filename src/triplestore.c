/* triplestore.c -- wrapper around different triplestore implementations */

#include "config.h"

#ifdef HAVE_REDLAND
#include "triplestore-redland.c"
#else
#include "triplestore-builtin.c"
#endif


/*
  Adds a single triple to store.  The object is considered to be an
  english literal.  Returns non-zero on error.
 */
int triplestore_add_en(TripleStore *ts, const char *s, const char *p,
                    const char *o)
{
  return triplestore_add(ts, s, p, o, "@en");
}

/*
  Adds a single triple to store.  The object is considered to be an URI.
  Returns non-zero on error.
 */
int triplestore_add_uri(TripleStore *ts, const char *s, const char *p,
                        const char *o)
{
  return triplestore_add(ts, s, p, o, NULL);
}

/* Set default namespace */
void triplestore_set_namespace(TripleStore *ts, const char *ns)
{
  if (ts->ns) free((char *)ts->ns);
  ts->ns = (ns) ? strdup(ns) : NULL;
}

/* Returns a pointer to default namespace.
   It may be NULL if it hasn't been set */
const char *triplestore_get_namespace(TripleStore *ts)
{
  return ts->ns;
}
