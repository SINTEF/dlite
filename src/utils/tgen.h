/* tgen.h -- simple templated text generator */
#ifndef _TGEN_H
#define _TGEN_H
/**
  @file
  @brief Simple templated text generator

  The main function in this library is tgen(). It takes a template and
  a list of substitutions and produces a new document.

  An example template may look like

      We have:
        pi={pi}
        n={n}
      and the loop:
      {loop:  i={i} - data={data}
      }

  A pair of braces, "{" and "}" that encloses a string is called a
  *tags*.  When the template is processed, the tags are replaced with
  new content according to the substitutions.  Tags may, in the
  template, be written either as ``{VAR}`` or ``{VAR:TEMPL}``.  In the
  latter form, the `TEMPL` string may contain embedded braces, as long
  as the embedded opening and closing braces exactly match.  In the
  example above, corresponds `VAR` in the last tag to "loop" and
  `TEMPL` to "i={i} - data={data}".

  Literal braces may be included in the template and the `TEMPL`
  section, if they are escaped according the following table:

  escape sequence | result | comment
  --------------- | ------ | -------
  `{{`            | `{`    | literal start brace
  `}}`            | `}`    | literal end brace
  `{}`            | `}`    | only use this if `TEMPL` ends with a `}`


  There are also two types of substitutions, variable substitutions
  and function substitutions:

  A **variable substitution** relates `VAR` to a string replacing the
  tag.  If the tag contains a `TEMPL`-part, it will be ignored.

  A **function substitution** relates `VAR` to a function.  When the
  template is processed, the function is called replacing the tag with
  its output.  The function uses `TEMPL` as a (sub)template.

  So, if the above template is combined this with the substitutions


  `VAR` | value
  ----- | -----
  pi    | 3.14
  n     | 42
  loop  | a function that loops over `i` and `data`

  we may produce the following output:

      We have:
        pi=3.14
        n=42
      and the loop:
        i=0 - data=1
        i=1 - data=3
        i=2 - data=5

  In the example below is there a small program that implements exactly this.
  The strength of this approach is that you can produce the same information
  in a completely different format just by changing the template, without
  changing the substitutions or loop() function.

  ### Example code

      #include <stdio.h>
      #include "tgen.h"

      #define UNUSEDx) (void)(x)
      #define countof(arr) (sizeof(arr) / sizeof(arr[0]))

      static int loop(TGenBuf *s, const char *template, int len,
                      const TGenSubstitution *subs, size_t nsubs, void *context)
      {
        int i, data[] = {1, 3, 5};
        char var[64], value[64];
        TGenSubstitution subs2[] = {
          {"i", var, NULL},
          {"data", value, NULL},
          {"loop", NULL, loop}
        };
        UNUSED(subs);
        UNUSED(nsubs);

        for (i=0; i < countof(data); i++) {
          snprintf(var, sizeof(var), "%d", i);
          snprintf(value, sizeof(value), "%d", data[i]);
          tgen_append(s, template, len, subs2, countof(subs2), context);
        }
        return 0;
      }

      int main()
      {
        char template[] =
          "We have:\n"
          "  pi={pi}\n"
          "  n={n}\n"
          "and the loop:\n"
          "{loop:  i={i} - data={data}\n}";
        TGenSubstitution subs[] = {
          {"n",    "42",   NULL},
          {"pi",   "3.14", NULL},
          {"loop", NULL,   loop},
        };
        char *str = tgen(template, subs, countof(subs), NULL);

        printf("%s\n", str);

        free(str);
        return 0;
      }
 */

#include <stdlib.h>

/**
   Error codes used by this library
*/
enum {
  TGenOk,
  TGenAllocationError,
  TGenSyntaxError,
  TGenVariableError,
  TGenSubtemplateError,
};

/**
  Buffer for generated output.
*/
typedef struct _TGenBuf {
  char *buf;    /*!< buffer */
  size_t size;  /*!< allocated size of buffer */
  size_t pos;   /*!< current position */
} TGenBuf;

typedef struct _TGenSubstitution TGenSubstitution;

/**
  Prototype for substitution function that appends to output buffer.
  See tgen_append() for details.
*/
typedef int (*TGenSub)(TGenBuf *s, const char *template, int len,
                       const TGenSubstitution *subs, size_t nsubs,
                       void *context);


/**
  Struct defining a substitution.
*/
struct _TGenSubstitution {
  char *var;      /*!< Variable that should be substituted */
  char *repl;     /*!< String that the variable should be replaced with.
                       May also be used as subtemplate if `sub` is given
                       and the main template does not provide a subtemplate
                       for this substitution. */
  TGenSub sub;    /*!< substitution function, may be NULL */
};


/**
  Appends `n` bytes from string `src` to end of `s`.  If `n` is
  negative, all of `str` (NUL-terminated) is appended.

  Returns non-zero on error.
 */
int tgen_buf_append(TGenBuf *s, const char *src, int n);

/**
  Returns the line number of position `t` in `template`.
*/
int tgen_lineno(const char *template, const char *t);


/**
  Returns substitution corresponding to `var` or NULL if there are no
  substitution for `var`.  `n` is the lengths of the array of substitutions
  and `len` is the length of `var`.
*/
const TGenSubstitution *
tgen_get_substitution(const TGenSubstitution *subs, int nsubs,
                      const char *var, int len);

/**
  Returns a newly malloc'ed string based on `template`, where all
  occurences of ``{VAR}`` are replaced according to substitution `VAR`
  in the array `substitutions`, which has length `n`.

  The template may also refer to a substitution as ``{VAR:TEMPL}``.
  If the substitution corresponding to `VAR` provide a substitution
  function (via its `subs` member), `TEMPL` will be passed as
  subtemplate to the substitution function.  If `TEMPL` is not given,
  then the subtemplate will be taken from the `repl` member of the
  corresponding substitution.

  `context` is a pointer to user data passed on to the substitution
  function.

  The following escape sequences are interpreated:

  escape sequence | result | comment
  --------------- | ------ | -------
  `{{`            | `{`    | literal start brace
  `}}`            | `}`    | literal end brace
  `{}`            | `}`    | needed when `TEMPL` ends with a `}`

  Returns NULL, on error.
 */
char *tgen(const char *template, const TGenSubstitution *subs, size_t nsubs,
           void *context);

/**
  Like tgen(), but appends to `s` instead of returning the substituted
  template.  `len` is the length of `template`.

  Returns non-zero on error.
 */
int tgen_append(TGenBuf *s, const char *template, int len,
                const TGenSubstitution *subs, size_t nsubs, void *context);




#endif /* _TGEN_H */
