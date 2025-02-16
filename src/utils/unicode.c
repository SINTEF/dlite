/**
@file unicode.c

@brief Provides various unicode helper functions.

Currently only supports UTF-8, but that might change in the future.

Copyright (C) 2013-2014 Thomas Glyn Dennis.

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following
conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.
*/

#include "unicode.h"

/*
[PUBLIC] Try to read a UTF-8 character from a C-style string.
*/

int utf8decode(const char *text, long *unicode_value)
{

  const unsigned char *utxt = (const unsigned char*)text;

  long val = 0;
  int len, i;

  /* Determine how many bytes long this UTF-8 character value should be. */

  if      (!utxt               ) { len = 0; val = 0;            }
  else if ((utxt[0] & 128) == 0) { len = 1; val = utxt[0];      } /* ASCII.   */
  else if ((utxt[0] & 64 ) == 0) { len = 0; val = 0;            } /* Invalid. */
  else if ((utxt[0] & 32 ) == 0) { len = 2; val = utxt[0] & 31; }
  else if ((utxt[0] & 16 ) == 0) { len = 3; val = utxt[0] & 15; }
  else if ((utxt[0] & 8  ) == 0) { len = 4; val = utxt[0] & 7;  }
  else if ((utxt[0] & 4  ) == 0) { len = 5; val = utxt[0] & 3;  }
  else if ((utxt[0] & 2  ) == 0) { len = 6; val = utxt[0] & 1;  }
  else                           { len = 0; val = 0;            } /* Invalid. */

  /* Read the remaining bytes to get the complete UTF-8 character value. */

  for (i = 1; i < len; i++)
  {
    if ((utxt[i] & 128) == 0 || (utxt[i] & 64) != 0) { return 0; }
    val = (val << 6) + (utxt[i] & 63);
  }

  /* Return the length of this character in bytes, and optionally the value. */

  if (unicode_value) { *unicode_value = val; }
  return len;

}

/*
[PUBLIC] Write a unicode character value to a string in UTF-8 format.
*/

int utf8encode(long value, char *output)
{

  /* NOTE: The maximum UTF-8 character value allowed by RFC 3629 is 10FFFF. */

  if (value > 0x7FFFFFFF) { return 0; }

  /* Six-byte sequences have the highest 6 bits of the first byte set. */

  if (value >= 0x4000000)
  {
    if (output)
    {
      output[0] = 252 + ((value >> 30) &  1);
      output[1] = 128 + ((value >> 24) & 63);
      output[2] = 128 + ((value >> 18) & 63);
      output[3] = 128 + ((value >> 12) & 63);
      output[4] = 128 + ((value >>  6) & 63);
      output[5] = 128 + ((value >>  0) & 63);
      output[6] = 0;  /* NUL terminator. */
    }
    return 6;
  }

  /* Five-byte sequences have the highest 5 bits of the first byte set. */

  if (value >= 0x200000)
  {
    if (output)
    {
      output[0] = 248 + ((value >> 24) &  3);
      output[1] = 128 + ((value >> 18) & 63);
      output[2] = 128 + ((value >> 12) & 63);
      output[3] = 128 + ((value >>  6) & 63);
      output[4] = 128 + ((value >>  0) & 63);
      output[5] = 0;  /* NUL terminator. */
    }
    return 5;
  }

  /* Four-byte sequences have the highest 4 bits of the first byte set. */

  if (value >= 0x10000)
  {
    if (output)
    {
      output[0] = 240 + ((value >> 18) &  7);
      output[1] = 128 + ((value >> 12) & 63);
      output[2] = 128 + ((value >>  6) & 63);
      output[3] = 128 + ((value >>  0) & 63);
      output[4] = 0;  /* NUL terminator. */
    }
    return 4;
  }

  /* Three-byte sequences have the highest 3 bits of the first byte set. */

  if (value >= 0x0800)
  {
    if (output)
    {
      output[0] = 224 + ((value >> 12) & 15);
      output[1] = 128 + ((value >>  6) & 63);
      output[2] = 128 + ((value >>  0) & 63);
      output[3] = 0;  /* NUL terminator. */
    }
    return 3;
  }

  /* Two-byte sequences have the highest 2 bits of the first byte set. */

  if (value >= 0x0080)
  {
    if (output)
    {
      output[0] = 192 + ((value >>  6) & 31);
      output[1] = 128 + ((value >>  0) & 63);
      output[2] = 0;  /* NUL terminator. */
    }
    return 2;
  }

  /* Single-byte characters are ASCII, and thus do not set the highest bit. */

  if (value >= 0)
  {
    if (output)
    {
      output[0] = (char)value;
      output[1] = 0;  /* NUL terminator. */
    }
    return 1;
  }

  /* Negative values are invalid. */

  return 0;

}
