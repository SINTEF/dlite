/* infix-calc.c -- simple infix calculator for integer arithmetic */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "infixcalc.h"

/*
  The algorithm:

  1. While there are still tokens to be read in,
     1.1 Get the next token.
     1.2 If the token is:
         1.2.1 A number: push it onto the value stack.
         1.2.2 A variable: get its value, and push onto the value stack.
         1.2.3 A left parenthesis: push it onto the operator stack.
         1.2.4 A right parenthesis:
           1 While the thing on top of the operator stack is not a
             left parenthesis,
               1 Pop the operator from the operator stack.
               2 Pop the value stack twice, getting two operands.
               3 Apply the operator to the operands, in the correct order.
               4 Push the result onto the value stack.
           2 Pop the left parenthesis from the operator stack, and discard it.
         1.2.5 An operator (call it thisOp):
           1 While the operator stack is not empty, and the top thing on the
             operator stack has the same or greater precedence as thisOp,
             1 Pop the operator from the operator stack.
             2 Pop the value stack twice, getting two operands.
             3 Apply the operator to the operands, in the correct order.
             4 Push the result onto the value stack.
           2 Push thisOp onto the operator stack.
  2. While the operator stack is not empty,
      1 Pop the operator from the operator stack.
      2 Pop the value stack twice, getting two operands.
      3 Apply the operator to the operands, in the correct order.
      4 Push the result onto the value stack.
  3. At this point the operator stack should be empty, and the value
     stack should have only one value in it, which is the final result.

  Source: https://www.geeksforgeeks.org/expression-evaluation/
 */


#define CHUNKSIZE 64  /* stack allocation chunk size */


typedef enum {
  opParentOpen  = '(',
  opParentClose = ')',
  opPlus        = '+',
  opMinus       = '-',
  opTimes       = '*',
  opDivide      = '/',
  opMod         = '%',
  opExponent    = '^',
} Operator;

/* Token types */
typedef enum {
  typeVal,  /* Value (number or variable) */
  typeOp    /* Operator */
} TokenType;

/* Token value */
typedef struct {
  TokenType type;  /* Token type */
  union {
    int val;       /* Value for numbers and variables */
    int op;        /* Operator */
  };
} TokenValue;

/* Stack */
typedef struct {
  size_t len;
  size_t size;
  int *items;
} Stack;

typedef struct {
  Operator op;     /* operator number */
  int precedence;  /* precedence */
  size_t nargs;    /* number of operands */
} OpInfo;

static const OpInfo _opinfo[] = {
  {'(', 0, 0},
  {')', 0, 0},
  {'+', 1, 2},
  {'-', 1, 2},
  {'*', 2, 2},
  {'/', 2, 2},
  {'%', 2, 2},
  {'^', 3, 2},
  {0, 0, 0}
};

/* Returns pointer to info about operator `op` or NULL if `op` is not a
   known operator. */
static const OpInfo *get_opinfo(Operator op)
{
  int i;
  for (i=0; _opinfo[i].op; i++)
    if (op == _opinfo[i].op) return _opinfo + i;
  return NULL;
}


/* Returns a pointer to the element in `vars` that corresponds to the
   initial part of `str` or NULL on error.

   Variables must be valid C identifiers. */
static const InfixCalcVariable *get_variable(const char *str,
                                             const InfixCalcVariable *vars,
                                             int nvars)
{
  int i;
  size_t n=0;

  if (!vars) return NULL;

  /* Determine length of variable */
  if (!isalpha(str[n]) && str[n] != '_') return NULL;
  n++;
  while (isalnum(str[n]) || str[n] == '_') n++;

  for (i=0; i<nvars; i++)
    if (strlen(vars[i].name) == n && strncmp(vars[i].name, str, n) == 0)
      return vars + i;

  return NULL;
}


/* Push `item` to `stack`. */
static void push(Stack *stack, int item)
{
  //printf("  + push %d (%c)\n", item, isgraph(item) ? item : ' ');
  if (stack->size <= stack->len) {
    stack->size += CHUNKSIZE;
    stack->items = realloc(stack->items, stack->size*sizeof(item));
    assert(stack->items);
  }
  stack->items[stack->len++] = item;
}

/* Pop an item from `stack`. */
static int pop(Stack *stack)
{
  int item;
  assert(stack->len > 0);
  item = stack->items[--stack->len];
  //printf("  - pop %d (%c)\n", item, isgraph(item) ? item : ' ');
  return item;
}

/* Returns top item from `stack` without removing it. */
static int poll(Stack *stack)
{
  assert(stack->len > 0);
  return stack->items[stack->len-1];
}


static int binary_eval(Operator op, int arg1, int arg2)
{
  switch (op) {
  case '+':  return arg1 + arg2;
  case '-':  return arg1 - arg2;
  case '*':  return arg1 * arg2;
  case '/':  return arg1 / arg2;
  case '%':  return arg1 % arg2;
  case '^':
    {
      int i, v=1;
      for (i=0; i<arg2; i++) v *= arg1;
      return v;
    }
  default:   return 0;
  }
}

/* Parses a token from `str` and returns the number of characters consumed.
   The value pointed to by `val` is updated for the parsed token.

   On error, -1 is returned.
*/
static int parse_token(const char *str, TokenValue *val,
                       const InfixCalcVariable *vars, int nvars)
{
  const OpInfo *opinfo;
  const InfixCalcVariable *var;
  if (!str || !str[0]) return -1;

  if (isdigit(str[0])) {
    char *endptr;
    val->type = typeVal;
    val->val = strtol(str, &endptr, 0);
    return endptr - str;

  } else if ((opinfo = get_opinfo(str[0]))) {
    val->type = typeOp;
    val->op = str[0];
    return 1;

  } else if ((var = get_variable(str, vars, nvars))) {
    val->type = typeVal;
    val->val = var->value;
    return strlen(var->name);
  }

  return -1;
}

/* Evaluates operator `op` taking operands from the value stack
  `vstack` and and push the result back onto `vstack`.

  Returns non-zero on error and write an message to `err`. */
static int eval(Operator op, Stack *vstack, char *err, int errlen)
{
  const OpInfo *opinfo = get_opinfo(op);
  int value;
  if (vstack->len < opinfo->nargs) {
    snprintf(err, errlen, "too few arguments for operator '%c'", op);
    return -1;
  }
  if (opinfo->nargs == 2) {
    int arg2 = pop(vstack);
    int arg1 = pop(vstack);
    value = binary_eval(op, arg1, arg2);
  } else {
    snprintf(err, errlen, "%zu-ary operators are not implemented",
             opinfo->nargs);
    return -1;
  }
  push(vstack, value);
  return 0;
}


/*
  Parses the infix expression `expr` and returns the evaluated result.

  The array `vars` lists available variables and should have length `nvars`.
  If there are no variables, `vars` may be set to NULL.

  On error, a message will be written to `err`.  No more than `errlen`
  characters will be written.  On success `err[0]` will be set to NUL.
  If one is not interested to check for errors, one can set `err` to NULL.
*/
int infixcalc(const char *expr, const InfixCalcVariable *vars, size_t nvars,
              char *err, size_t errlen)
{
  Stack vstack, ostack;
  const char *p=expr;
  const OpInfo *opinfo;
  TokenValue token;
  Operator op;
  int result=INT_MIN;

  if (err && errlen) err[0] = '\0';
  memset(&vstack, 0, sizeof(vstack));
  memset(&ostack, 0, sizeof(ostack));

  while (isspace(*p)) p++;

  while (*p) {
    int n;
    if ((n = parse_token(p, &token, vars, nvars)) < 0) {
      snprintf(err, errlen, "invalid token at position %d in expression \"%s\"",
               (int)(p - expr), expr);
      goto fail;
    }
    switch (token.type) {
    case typeVal:
      push(&vstack, token.val);
      break;

    case typeOp:
      switch (token.op) {
      case '(':
        push(&ostack, token.op);
        break;
      case ')':
        while ((op = pop(&ostack)) != '(')
          if (eval(op, &vstack, err, errlen)) goto fail;
        break;
      default:
        opinfo = get_opinfo(token.op);
        while (ostack.len) {
          const OpInfo *opinfo2 = get_opinfo(poll(&ostack));
          if (opinfo2->precedence < opinfo->precedence) break;
          op = pop(&ostack);
          if (eval(op, &vstack, err, errlen)) goto fail;
        }
        push(&ostack, token.op);
        break;
      }
    }
    p += n;
    while (isspace(*p)) p++;
  }

  while (ostack.len) {
    op = pop(&ostack);
    if (eval(op, &vstack, err, errlen)) goto fail;
  }

  if (vstack.len > 1) {
    snprintf(err, errlen, "missing operator in expression \"%s\"", expr);
    goto fail;
  } else  if (vstack.len < 1) {
    snprintf(err, errlen, "missing operands in expression \"%s\"", expr);
    goto fail;
  }
  assert(ostack.len == 0);
  assert(vstack.len == 1);
  result = pop(&vstack);

 fail:
  if (vstack.size) free(vstack.items);
  if (ostack.size) free(ostack.items);
  return result;
}
