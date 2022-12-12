/* strtob.h -- converts string to boolean
 *
 * Copyright (c) 2018-2020, SINTEF
 *
 * Distributed under terms of the MIT license.
 *
 */
#ifndef _STRTOB_H
#define _STRTOB_H

/**
  @file
  @brief Converts string to boolean.
*/

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
  Converts a string to true (non-zero) or false (zero).
*/
int atob(const char *ptr);


#endif /* _STRTOB_H */
