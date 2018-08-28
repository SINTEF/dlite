/* Private declarations for triplestore. */
#ifndef _TRIPLESTORE_PRIVATE_H
#define _TRIPLESTORE_PRIVATE_H




/* Extended triplet, with id. Safe to cast to Triplet. */
typedef struct _XTriplet {
  char *s;    /* subject */
  char *p;    /* predicate */
  char *o;    /* object */
  char *id;   /* id, identifying this triplet. Can be referred to as
                 subject or object. */
} XTriplet;


struct _Triplestore {
  XTriplet *triplets; /* array of triplets */
  size_t length;      /* number of triplets */
  size_t size;        /* allocated size */
};


/** State used by triplestore_find.
    Don't rely on current definition, it may be optimised later. */
struct _TripleState {
  size_t pos;         /* current position */
};


#endif /* _TRIPLESTORE_PRIVATE_H */
