/*
 * Copyright 1994 Christopher Seiwald.  All rights reserved.
 *
 * License is hereby granted to use this software and distribute it
 * freely, as long as this copyright notice is retained and modifications
 * are clearly marked.
 *
 * ALL WARRANTIES ARE HEREBY DISCLAIMED.
 *
 * ChangeLog:
 * - Changed function name from jam_glob() to globmatch() - Jesper Friis, 2011
 */
#ifndef _GLOBMATCH_H
#define _GLOBMATCH_H

/**
 * @file
 * @brief Match a string against a simple pattern
 *
 * Match string 's' against glob pattern 'pattern' and return zero on
 * match.
 *
 * Understands the following patterns:
 *
 *      *       any number of characters
 *      ?       any single character
 *      [a-z]   any single character in the range a-z
 *      [^a-z]  any single character not in the range a-z
 *      \x      match x
 */
int globmatch(const char *pattern, const char *s);

#endif /* _GLOBMATCH_H */
