/**
  @file
  @brief A subject-predicate-object triple type.

  This library defines triples as subject-predicate-object tuplets
  with an id.  This allow a allows the subject or object to refer to
  another triple via its id, as one would expect for RDF triples
  (see https://en.wikipedia.org/wiki/Semantic_triple).
 */
#ifndef _TRIPLE_H
#define _TRIPLE_H

#include "triple.h"


/**
  A subject-predicate-object triple used to represent a relation.
  The s-p-o-id strings should be allocated with malloc.
*/
typedef struct _Triple {
  char *s;     /*!< subject */
  char *p;     /*!< predicate */
  char *o;     /*!< object */
  char *id;    /*!< unique ID identifying this triple */
} Triple;


/**
  Sets default namespace to be prepended to triple id's.

  Use this function to convert the id's to proper URI's.
*/
void triple_set_default_namespace(const char *namespace);

/**
  Returns default namespace.
*/
const char *triple_get_default_namespace(void);

/**
  Frees up memory used by the s-p-o strings, but not the triple itself.
*/
void triple_clean(Triple *t);

/**
  Convinient function to assign a triple. This allocates new memory
  for the internal s, p, o and id pointers.  If `id` is
  NULL, a new id will be generated bases on `s`, `p` and `o`.
 */
int triple_set(Triple *t, const char *s, const char *p, const char *o,
                const char *id);

/**
  Like triple_set(), but free's allocated memory in `t` before re-assigning
  it.  Don't use this function if `t` has not been initiated.
 */
int triple_reset(Triple *t, const char *s, const char *p, const char *o,
                  const char *id);

/**
  Returns an newly malloc'ed unique id calculated from triple.

  If `namespace` is NULL, the default namespace set with
  triple_set_default_namespace() will be used.

  Returns NULL on error.
*/
char *triple_get_id(const char *namespace, const char *s, const char *p,
                      const char *o);

/**
  Copies triple `src` to `dest` and returns a pointer to `dest`.
 */
Triple *triple_copy(Triple *dest, const Triple *src);


#endif  /* _TRIPLE_H */
