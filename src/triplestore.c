/* triplestore.c -- wrapper around different triplestore implementations */

#include "config.h"

#ifdef HAVE_REDLAND
#include "triplestore-redland.c"
#else
#include "triplestore-builtin.c"
#endif
