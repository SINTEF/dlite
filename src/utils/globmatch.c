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

/*
 * globmatch.c - match a string against a simple pattern
 *
 * Understands the following patterns:
 *
 *      *       any number of characters
 *      ?       any single character
 *      [a-z]   any single character in the range a-z
 *      [^a-z]  any single character not in the range a-z
 *      \x      match x
 *
 * External functions:
 *
 *      globmatch() - match a string against a simple pattern
 *
 * Internal functions:
 *
 *      globchars() - build a bitlist to check for character group match
 */

#include <string.h>

#include "globmatch.h"

# define CHECK_BIT( tab, bit ) ( tab[ (bit)/8 ] & (1<<( (bit)%8 )) )
# define BITLISTSIZE 16 /* bytes used for [chars] in compiled expr */

static void globchars();

/*
 * globmatch() - match a string against a simple pattern
 */

int globmatch(const char *pattern, const char *string)
{
        register char *c = (char *)pattern;
	register char *s = (char *)string;
        char bitlist[ BITLISTSIZE ];
        char *here;

        memset(bitlist, 0, sizeof(bitlist));

        for( ;; )
            switch( *c++ )
        {
        case '\0':
                return *s ? -1 : 0;

        case '?':
                if( !*s++ )
                    return 1;
                break;

        case '[':
                /* scan for matching ] */

                here = c;
                do if( !*c++ )
                        return 1;
                while( here == c || *c != ']' );
                c++;

                /* build character class bitlist */

                globchars( here, c, bitlist );

                if( !CHECK_BIT( bitlist, *(unsigned char *)s ) )
                        return 1;
                s++;
                break;

        case '*':
                here = s;

                while( *s )
                        s++;

                /* Try to match the rest of the pattern in a recursive */
                /* call.  If the match fails we'll back up chars, retrying. */

                while( s != here )
                {
                        int r;

                        /* A fast path for the last token in a pattern */

                        r = *c ? globmatch( c, s ) : *s ? -1 : 0;

                        if( !r )
                                return 0;
                        else if( r < 0 )
                                return 1;

                        --s;
                }
                break;

        case '\\':
                /* Force literal match of next char. */

                if( !*c || *s++ != *c++ )
                    return 1;
                break;

        default:
                if( *s++ != c[-1] )
                    return 1;
                break;
        }
}

/*
 * globchars() - build a bitlist to check for character group match
 */

static void
globchars( s, e, b )
char *s, *e, *b;
{
        int neg = 0;

        memset( b, '\0', BITLISTSIZE  );

        if( *s == '^')
                neg++, s++;

        while( s < e )
        {
                int c;

                if( s+2 < e && s[1] == '-' )
                {
                        for( c = s[0]; c <= s[2]; c++ )
                                b[ c/8 ] |= (1<<(c%8));
                        s += 3;
                } else {
                        c = *s++;
                        b[ c/8 ] |= (1<<(c%8));
                }
        }

        if( neg )
        {
                int i;
                for( i = 0; i < BITLISTSIZE; i++ )
                        b[ i ] ^= 0377;
        }

        /* Don't include \0 in either $[chars] or $[^chars] */

        b[0] &= 0376;
}
