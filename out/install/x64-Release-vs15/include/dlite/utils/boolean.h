/* boolean.h - cross-platform header for a bool type
 */
#ifndef _BOOLEAN_H
#define _BOOLEAN_H

#define HAVE_STDBOOL_H
/* #undef HAVE__BOOL */

#ifdef HAVE_STDBOOL_H
# include <stdbool.h>
#else
# ifndef HAVE_SIZEOF_BOOL
#  typedef _Bool unsigned char
# endif
# define bool _Bool
# define true    1
# define false   0
# define __bool_true_false_are_defined 1
#endif

#define SIZEOF_BOOL 1

#endif /* _BOOLEAN_H */
