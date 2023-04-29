/* clp2.h -- rounding up to next power of two
 *
 * Copyright (C) 2021 SINTEF
 *
 * Distributed under terms of the MIT license.
 */

/**
  Fast implementations of clp2() and flp2(), based on
  http://ptgmedia.pearsoncmg.com/images/0201914654/samplechapter/warrench03.pdf
 */
#ifndef _CLP2_H
#define _CLP2_H

#include <stdlib.h>


/** Returns x rounded up to next power of two. */
size_t clp2(size_t n);


/** Returns x rounded down to previous power of two. */
size_t flp2(size_t n);


#endif  /* _CLP2_H */
