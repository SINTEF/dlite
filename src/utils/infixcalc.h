/* infix-calc.h -- simple infix calculator for integer arithmetic
 *
 * Copyright (C) 2017 SINTEF
 *
 * Distributed under terms of the MIT license.
 */
#ifndef _INFIX_CALC_H
#define _INFIX_CALC_H
/**
  @file
  @brief Simple infix calculator for integer arithmetic
  Only the following binary operators are implemented:

      |  logical or
      &  logical and
      =  logical equal
      !  logical not equal
      >  logical greather than
      <  logical smaller than
      +  plus
      -  minus
      *  times
      /  division
      %  modulus
      ^  power

  in addition to parenthesis.
*/


#ifndef INT_MIN
#define INT_MIN ((-(1 << (8*sizeof(int) - 2)))*2) /* avoid overflow */
#endif

/** Variable with associated value */
typedef struct {
  const char *name;  /*!< variable name */
  int value;         /*!< value */
} InfixCalcVariable;


/**
  Parses the infix expression `expr` and returns the evaluated result.

  The array `vars` lists available variables and should have length `nvars`.
  If there are no variables, `vars` may be set to NULL.

  On error INT_MIN is returned and a message will be written to `err`.
  No more than `errlen` characters will be written.  On success
  `err[0]` will be set to NUL.  If one is not interested to check for
  errors, one can set `err` to NULL.
*/
int infixcalc(const char *expr, const InfixCalcVariable *vars, size_t nvars,
              char *err, size_t errlen);


/**
  Returns non-zero if variable `varname` is in expression `expr`.
 */
int infixcalc_depend(const char *expr, const char *varname);

#endif /* _INFIX_CALC_H */
