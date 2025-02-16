/**
@file unicode.h

@brief Provides various unicode helper functions.

Currently only supports UTF-8, but that might change in the future.
*/

/*
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

#ifndef _UNICODE_H
#define _UNICODE_H

#ifdef __cplusplus
extern "C" {
#endif

/**
@brief Try to read a UTF-8 character from a C-style string.

Invalid UTF-8 sequences are reported as being zero bytes long.

@param text The text string to read a Unicode character from.
@param unicode_value If provided, the actual Unicode value will be stored here.
@return The number of bytes used to represent the UTF-8 code, or zero on error.
*/

int utf8decode(const char *text, long *unicode_value);

/**
@brief Write a unicode character value to a string in UTF-8 format.

Invalid UTF-8 sequences are reported as being zero bytes long.

NOTE: A NUL terminator byte is automatically added to the end of the output.

@param value The unicode character to be encoded as a UTF-8 sequence.
@param output If provided, the character will be written in UTF-8 format here.
@return The number of bytes used to represent the UTF-8 code, or zero on error.
*/

int utf8encode(long value, char *output);

#ifdef __cplusplus
}
#endif

#endif /* _UNICODE_H */
