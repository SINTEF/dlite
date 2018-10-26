/*
 * Copyright (c) 2018, SINTEF Industry
 * By Jesper Friis
 * All rights reserved.
 *
 * Licensed under BSD v2.0.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the <organization> nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 * <COPYRIGHT HOLDER> BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#ifndef _STRTOB_H
#define _STRTOB_H


/**
  Converts the initial part of the string `ptr` to a boolean.

  The following values (case insensitive):

      "1", "true", ".true.", "yes" and "on"

  are considered true and will return 1, while the following values:

      NULL, "", "0", "false", ".false.", "no" and "off"

  are considered false and will return zero.  Initial blanks (except
  in front of the empty string "") will be stripped off.  Any other
  string is also considered true, but will return -1 to allow the user
  to distinguish between the proper true values above and other
  strings.

  If `endptr` is not NULL, strtob() stores the address of the
  first invalid character in `*endptr`.  In the case of a string not
  matching any of the proper true or false values above, initial blanks
  plus one non-blank character will be consumed.

  Returns non-zero for true, zero otherwise.
 */
int strtob(const char *ptr, char **endptr);


/**
  Converts a string to true (1) or false (0).
*/
int atob(const char *ptr);


#endif /* _STRTOB_H */
