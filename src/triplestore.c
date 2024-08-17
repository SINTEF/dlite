/* triplestore.c -- wrapper around different triplestore implementations */

#include "config.h"

#ifdef HAVE_REDLAND
#include "triplestore-redland.c"
#else
#include "triplestore-builtin.c"
#include "dlite-macros.h"
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

/*
  Return pointer to the value for a pair of two criteria.

  Useful if one knows that there may only be one value.  The returned
  value is held by the triplestore and should be copied by the user
  since it may be overwritten by later calls to the triplestore.

  Parameters:
      s, p, o: Criteria to match. Two of these must be non-NULL.
      d: If not NULL, the required datatype of literal objects.
      fallback: Value to return if no matches are found.
      any: If non-zero, return first matching value.

  Returns a pointer to the value of the `s`, `p` or `o` that is NULL.
  On error NULL is returned.
 */
const char *triplestore_value(TripleStore *ts, const char *s, const char *p,
                              const char *o, const char *d,
                              const char *fallback, int any)
{
  int n=0, i=-1;
  const char *value=NULL;
  TripleState state;
  const Triple *t;
  triplestore_init_state(ts, &state);
  if (s) n++; else i=0;
  if (p) n++; else i=1;
  if (o) n++; else i=2;
  if (n != 2) FAILCODE3(dliteTypeError,
                        "triplestore_value() expects exact two of "
                        "s='%s', p='%s', o='%s' to be non-NULL", s, p, o);
  assert(i >= 0);
  if (!(t = triplestore_find(&state, s, p, o, d))) {
    if (!fallback)
      FAILCODE4(dliteLookupError, "no values matching the criteria: "
                "s='%s', p='%s', o='%s', d='%s'", s, p, o, d);
    value = fallback;
  } else {
    switch (i) {
    case 0: value = t->s; break;
    case 1: value = t->p; break;
    case 2: value = t->o; break;
    }
  }
  if (!any && triplestore_find(&state, s, p, o, d))
    FAILCODE4(dliteLookupError, "more than one value matching the criteria: "
              "s='%s', p='%s', o='%s', d='%s'.  Maybe you want to set `any` "
              "to true?", s, p, o, d);
  triplestore_deinit_state(&state);
  return value;
 fail:
  triplestore_deinit_state(&state);
  return NULL;
}
