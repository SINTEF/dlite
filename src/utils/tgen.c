#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "err.h"
#include "tgen.h"


#define CHUNKSIZE  512



/*
  Appends `n` bytes from string `src` to end of `s`.  If `n` is
  negative, all of `str` (NUL-terminated) is appended.

  Returns non-zero on error.
 */
int tgen_buf_append(TGenBuf *s, const char *src, int n)
{
  size_t len = (n < 0) ? strlen(src) : (size_t)n;
  size_t needed = s->pos + len;
  assert(!s->size || s->buf);

  if (needed >= s->size) {
    s->size = (needed / CHUNKSIZE + 1) * CHUNKSIZE;
    if (!(s->buf = realloc(s->buf, s->size)))
      return err(TGenAllocationError, "allocation failure");
  }
  memcpy(s->buf + s->pos, src, len);
  s->pos += len;
  s->buf[s->pos] = '\0';
  return 0;
}


/*
  Returns the line number of position `t` in `template`.
*/
int tgen_lineno(const char *template, const char *t)
{
  int lineno = 1;
  const char *tt = template;
  while (tt < t) {
    assert(*tt);  /* t must point within template */
    if (*tt == '\n') lineno++;
    tt++;
  }
  return lineno;
}


/*
  Returns substitution corresponding to `var` or NULL if there are no
  substitution in `subs` for `var`.

  `nsubs` is the lengths of the `subs` array.  If it is negative,
  `subs` is assumed to be NULL-terminated (that is, the `var` member
  of the last element is NULL).

  `len` is the length of the `var` string.  If `len` is negative, it is
  set to ``strlen(var)``.
*/
const TGenSubstitution *
tgen_get_substitution(const TGenSubstitution *subs, int nsubs,
                      const char *var, int len)
{
  int i;
  if (nsubs < 0)
    for (nsubs=0; subs[nsubs].var; ) nsubs++;
  if (len < 0) len = strlen(var);
  for (i=0; i<nsubs; i++)
    if (strncmp(subs[i].var, var, len) == 0 &&
        (int)strlen(subs[i].var) == len)
      return subs + i;
  return NULL;
}


/*
  Returns a newly malloc'ed string based on `template`, where all
  occurences of ``{VAR}`` are replaced according to substitution `VAR`
  in the array `subs`, which has length `nsubs`.

  The template may also refer to a substitution as ``{VAR:TEMPL}``.
  If the substitution corresponding to `VAR` provide a substitution
  function (via its `subs` member), `TEMPL` will be passed as
  subtemplate to the substitution function.  If `TEMPL` is not given,
  then the subtemplate will be taken from the `repl` member of the
  corresponding substitution.

  Returns NULL, on error.
 */
char *tgen(const char *template, const TGenSubstitution *subs, size_t nsubs,
           void *context)
{
  TGenBuf s;
  memset(&s, 0, sizeof(s));
  if (tgen_append(&s, template, -1, subs, nsubs, context)) {
    if (s.buf) free(s.buf);
    return NULL;
  }
  return s.buf;
}


/*
  Like tgen(), but appends to `s` instead of returning the substituted
  template.  `tlen` is the length of `template`.  If it is negative, the
  full content of `template` will be used.

  Returns non-zero on error.
 */
int tgen_append(TGenBuf *s, const char *template, int tlen,
                const TGenSubstitution *subs, size_t nsubs, void *context)
{
  const TGenSubstitution *sub;
  const char *templ, *t = template;
  int templ_len, stat;
  if (tlen < 0) tlen = strlen(template);

  while (*t && t < template + tlen) {
    int len = strcspn(t, "{}");
    tgen_buf_append(s, t, len);
    t += len;
    if (t - template == (long)tlen) return 0;
    assert(t < template + tlen);

    switch (*(t++)) {

    case '\0':
      return 0;

    case '{':
      switch (*t) {
      case '\0':  /* unmatched opening brace */
        return err(TGenSyntaxError, "line %d: template ends with "
                   "unmatched '{'", tgen_lineno(template, t));
      case '{':  /* escaped opening brace */
        tgen_buf_append(s, "{", 1);
        t++;
        break;
      case '}':
        break;
      default:  /* substitution */
        len = strcspn(t, ":{}");
        if (t[len] == '\0')
          return err(TGenSyntaxError, "line %d: template ends with "
                     "unmatched '{'", tgen_lineno(template, t));
        if (t[len] == '{')
          return err(TGenSyntaxError, "line %d: unexpected '{' within a "
                     "substitution", tgen_lineno(template, t));
        if (!(sub = tgen_get_substitution(subs, nsubs, t, len)))
          return err(TGenVariableError, "line %d: unknown var '%.*s'",
                     tgen_lineno(template, t), len, t);

        templ = sub->repl;
        templ_len = -1;
        if (t[len] == ':') {
          int depth = 0;
          const char *tt = t + len + 1;
          templ = tt;
          while (*tt && *tt != '}') {
            int m = strcspn(tt, "{}");
            tt += m;
            switch (*(tt++)) {
            case '\0':
              return err(TGenSyntaxError, "line %d: unterminated "
                         "subtemplate in substitution for '%s'",
                         tgen_lineno(template, t), sub->var);
            case '{':
              if (*tt == '{')
                tt++;
              else if (*tt != '}')
                depth++;
              break;

            case '}':
              if (*tt == '}')
                tt++;
              else if (depth-- <= 0)
                tt--;
              break;

            default:
              abort();
            }
          }
          t = tt;
          templ_len = tt - templ;
        }

        if (sub->sub) {
          if (!templ)
            return err(TGenSubtemplateError, "line %d: subtemplate must "
                       "be provided for var '%s'", tgen_lineno(template, t),
                       sub->var);
          if ((stat = sub->sub(s, templ, templ_len, subs, nsubs, context)))
            return stat;
        } else {
          if ((stat = tgen_buf_append(s, sub->repl, -1)))
            return stat;
        }
        len = strcspn(t, "}");
        assert(t[len]);
        t += len + 1;
      }
      break;

    case '}':
      if (*t != '}')
        return err(TGenSyntaxError, "line %d: unescaped terminating brace",
                   tgen_lineno(template, t));
      tgen_buf_append(s, "}", 1);
      t++;
      break;

    default:
      abort();  /* should never be reached*/
    }
  }

  return 0;
}
