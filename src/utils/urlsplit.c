/* urlsplit.c -- splits an URL into its components according to RFC 3986
 *
 * Copyright (C) 2023 SINTEF
 *
 * Distributed under terms of the MIT license.
 */
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "strutils.h"
#include "urlsplit.h"


/*
  Returns length of percent-encoded segment from the start of string `s`.
  Will either return zero or 3.
*/
static int percent_encoded(const char *s)
{
  if (s[0] == '%' && isxdigit(s[1]) && isxdigit(s[2])) return 3;
  return 0;
}

/*
  Convenience function that returns the length of the initial segment of
  characters in `s` that either:
    - belong to categories less or equal to `cat`
    - are percent-encoding if `pct` is non-zero
    - are found `accepted`
*/
static int jspn(const char *s, StrCategory cat, int pct, const char *accept)
{
  int n=0, prev;
  do {
    prev = n;
    n += strcatjspn(s+n, cat);
    if (pct) n += percent_encoded(s+n);
    n += (int)strspn(s+n, accept);
  } while (n > prev);
  return n;
}

/*
  Returns non-zero if `url` is a valid URL.

  Note: If `url` starts with an upper case letter followed by colon
  (e.g. "C:"), then it is interpreted as a Windows drive and not an
  URL.
 */
int isurl(const char *url)
{
  return isurln(url, -1);
}


/*
  Like isurl(), but only considers the first `len` bytes of `url`.

  If `len` is negative, all of `url` is checked.
 */
int isurln(const char *url, int len)
{
  int n = (len < 0) ? (int)strlen(url) : len;
  if (n >= 2 && isupper(url[0]) && url[1] == ':') return 0;
  return (n > 0 && urlsplitn(url, n, NULL) == n) ? 1 : 0;
}


/*
  Returns the length of the initial segment of `url` that correspond
  to a valid URL.  Hence, zero is returned if `url` is not a valid URL.

  If `components` is not NULL, the memory it points to is filled in with
  pointer and length information of the different components of `url`.
 */
int urlsplit(const char *url, UrlComponents *components)
{
  return urlsplitn(url, -1, components);
}


/*
  Like urlsplit(), but takes an extra argument `len`.

  If `len` is positive, the `len` first characters of `url` must be a
  valid URL (in which case `len` is returned), otherwise zero is returned.

  A negative `len` means that the whole length of `url` is parsed
  (equivalent to ``len=strlen(url)``).
 */
int urlsplitn(const char *url, int len, UrlComponents *components)
{
  int n=0;
  if (len == 0) return 0;
  if (len < 0) len = (int)strlen(url);
  if (components) memset(components, 0, sizeof(UrlComponents));

  /* check scheme */
  if (components) components->scheme = url;
  n += strcatjspn(url+n, strcatLower);
  if (!n) return 0;
  n += jspn(url+n, strcatDigit, 0, "+.-");
  if (url[n++] != ':') return 0;
  if (len && n > len) return 0;
  if (components) components->scheme_len = n-1;

  if (components) components->path = url+n;  // path must always be defined:
                                             // set now - update later
  if (n == len) return n;

  /* check authority */
  if (url[n] == '/' && url[n+1] == '/') {
    int k, authlen=(int)strcspn(url+n+2, "/?#");
    n += 2;
    if (components) components->authority = url+n;
    if ((k = (int)strcspn(url+n, "@")) > 0 && k > 0 && k < authlen) {
      /* userinfo */
      assert(url[n+k] == '@');
      if (components) components->userinfo = url+n;
      n += jspn(url+n, strcatSubDelims, 1, ":");
      if (components) components->userinfo_len =
                        (int)(url+n - components->userinfo);
      if (url[n++] != '@') return 0;
    }
    /* host */
    if (components) components->host = url+n;
    if (url[n] == '[') {
      /* IP-literal */
      n++;
      if (url[n] == 'v' || url[n] == 'V') {
        /* IPvFuture */
        if (!isxdigit(url[n+1])) return 0;
        if (url[n+2] != '.') return 0;
        n += 3;
        n += jspn(url+n, strcatSubDelims, 1, ":");
      } else {
        /* IPv6 host */
        while (isxdigit(url[n]) || url[n] == ':') n++;
      }
      if (url[n++] != ']') return 0;
    } else {
      /* host (IPv4 or name) */
      n += jspn(url+n, strcatSubDelims, 1, "");
    }
    if (components) components->host_len = (int)(url+n - components->host);
    if (url[n] == ':') {
      /* port */
      n++;
      if (components) components->port = url+n;
      n += strcatspn(url+n, strcatDigit);
      if (components) components->port_len = (int)(url+n - components->port);
    }
    if (components) components->authority_len =
                      (int)(url+n - components->authority);
    /* RFC 3986: If a URI contains an authority component, then the
       path component must either be empty or begin with a slash ("/")
       character. */
    if (url[n] && !strchr("/?#", url[n])) return 0;
  }
  if (len && len < n) return 0;

  /* check path */
  if (url[n] == '/' && url[n+1] == '/') return 0;  // must not start with "//"
  if (components) components->path = url+n;
  n += jspn(url+n, strcatSubDelims, 1, "/:@");
  if (len > 0 && n >= len) n = len;
  if (components) components->path_len = (int)(url+n - components->path);
  if (len > 0 && n >= len) return len;

  /* check query */
  if (url[n] == '?') {
    n++;
    if (components) components->query = url+n;
    n += jspn(url+n, strcatSubDelims, 1, "/?:@");
    if (len > 0 && n >= len) n = len;
    if (components) components->query_len = (int)(url+n - components->query);
    if (len > 0 && n >= len) return len;
  }

  /* check fragment */
  if (url[n] == '#') {
    n++;
    if (components) components->fragment = url+n;
    n += jspn(url+n, strcatSubDelims, 1, "/?:@");
    if (len > 0 && n >= len) n = len;
    if (components) components->fragment_len =
                      (int)(url+n - components->fragment);
    if (len > 0 && n >= len) return len;
  }

  if (len <= 0 && url[n]) return 0;
  return n;
}


/*
  Join URL components `components` and write them to buffer `buf`.

  At most `size` bytes are written to `buf` (incl. terminating NUL.

  If the 'host' field of `components` is not NULL, the authority will
  be derived from the 'username', 'host' and 'port' fields, otherwise
  the `authority` field will be used.

  Return number of bytes written to `buf`, or the number of bytes that would
  have been written to `buf` if `size` would be big enough.
  On error, a negative value is returned.
 */
int urljoin(char *buf, long size, UrlComponents *components)
{
  int m, n=0;
  UrlComponents *c = components;
  if (!c->scheme) return -1;
  if (!c->path) return -1;

  /* scheme */
  n += strsetn(buf+n, size-n, c->scheme, c->scheme_len);
  n += strsetc(buf+n, size-n, ':');

  /* authority */
  if (c->host && c->host_len > 0) {
    n += strsets(buf+n, size-n, "//");
    if (c->userinfo && c->userinfo_len > 0) {
      if ((m = pct_xencode(buf+n, size-n, c->userinfo, c->userinfo_len,
                           strcatSubDelims, ":")) < 0)
        return -1;
      n += m;
      n += strsetc(buf+n, size-n, '@');
    }
    if ((m = pct_xencode(buf+n, size-n, c->host, c->host_len,
                         strcatSubDelims, ":")) < 0) return -1;
    n += m;
    if (c->port && c->port_len) {
      n += strsetc(buf+n, size-n, ':');
      n += strsetn(buf+n, size-n, c->port, c->port_len);
    }
  } else if (c->authority && c->authority_len) {
    n += strsets(buf+n, size-n, "//");
    if ((m = pct_xencode(buf+n, size-n, c->authority, c->authority_len,
                         strcatSubDelims, ":@")) < 0)
      return -1;
    n += m;
  }

  /* path */
  if ((m = pct_xencode(buf+n, size-n, c->path, c->path_len,
                       strcatSubDelims, "/:@")) < 0) return -1;
  n += m;

  /* query */
  if (c->query && c->query_len) {
    n += strsetc(buf+n, size-n, '?');
    if ((m = pct_xencode(buf+n, size-n, c->query, c->query_len,
                         strcatSubDelims, "/:@?")) < 0)
      return -1;
    n += m;
  }

  /* fragment */
  if (c->fragment && c->fragment_len) {
    n += strsetc(buf+n, size-n, '#');
    if ((m = pct_xencode(buf+n, size-n, c->fragment, c->fragment_len,
                         strcatSubDelims, "/:@?")) < 0)
      return -1;
    n += m;
  }

  return n;
}

// /*
//   Returns a newly allocated string from the given components.
//
//   The returned URL will be properly %-escaped.
//   All components, except `scheme` may be NULL.
//
//   Returns NULL on error.
// */
// char *urljoin(const char *scheme, const char *userinfo, const char *host,
//               const char *port, const char *path,
//               const char *query, const char *fragment)
// {
//   size_t n=0;
//   char *s=NULL;
//
//
// }


/*
  Write percent-encoded copy of `src` to `buf`.

  At most `size` bytes are written (including NUL-termination).

  Return the number of bytes that would have been written to `buf`
  (excluding NUL-terminator), or would have been written to `buf` if
  it would have been large enough.
  On error a negative number is returned.

  Equivalent to pct_xencode(buf, size, src, -1, strcatSubDelims, NULL);
 */
int pct_encode(char *buf, long size, const char *src)
{
  return pct_nencode(buf, size, src, -1);
}

/*
  Like pct_encode(), but at most `len` bytes are read from `src`.
  If `len` is negative, all of `src` is read.
 */
int pct_nencode(char *buf, long size, const char *src, long len)
{
  return pct_xencode(buf, size, src, len, strcatSubDelims, NULL);
}

/*
  Write percent-encoded copy of the first `len` bytes of `src` to `buf`.

  At most `size` bytes are written (including NUL-termination).
  Character in categories larger than `maxcat` and not in `accepted`,
  will be percent-encoded.

  Return the number of bytes that would have been written to `buf`
  (excluding NUL-terminator), or would have been written to `buf` if
  it would have been large enough.
  On error a negative number is returned.
 */
int pct_xencode(char *buf, long size, const char *src, long len,
                StrCategory maxcat, const char *accepted)
{
  int n=0;
  long i;
  if (len < 0) len = (long)strlen(src);
  for (i=0; i<len; i++) {
    if (strcategory(src[i]) <= maxcat ||
        (accepted && strchr(accepted, src[i]))) {
      n += strsetc(buf+n, size-n, src[i]);
    } else if (size-n > 0) {
      int m = snprintf(buf+n, size-n, "%%%02X", (unsigned char)(src[i]));
      if (m < 0) return -1;
      assert(m == 3);
      if (size-n <= 3) buf[n] = '\0';  // do not write partial encodings
      n += m;
    } else {
      n += 3;
    }
  }
  return n;
}

/*
  Write percent-decoded copy of `encoded` to `buf`.

  At most `size` bytes are written (including NUL-termination).

  Return the number of bytes that would have been written to `buf`
  (excluding NUL-terminator), or would have been written to `buf` if
  it would have been large enough.
  On error a negative number is returned.

 */
int pct_decode(char *buf, long size, const char *encoded)
{
  return pct_ndecode(buf, size, encoded, -1);
}

/*
  Like pct_decode(), but at most `len` bytes are read from `encoded`.
  If `len` is negative, all of `encoded` is read.
 */
int pct_ndecode(char *buf, long size, const char *encoded, long len)
{
  int n=0;
  long i=0;
  if (len < 0) len = (long)strlen(encoded);
  while (encoded[i] && i < len) {
    if (encoded[i] == '%') {
      if (i+2 >= len || !isxdigit(encoded[i+1]) || !isxdigit(encoded[i+2]))
        return -1;
      unsigned int c;
      int m = sscanf(encoded+i+1, "%2x", &c);
      if (m < 0) return -1;
      assert(m == 1);
      n += strsetc(buf+n, size-n, c);
      i += 3;
    } else
      n += strsetc(buf+n, size-n, encoded[i++]);
  }
  return n;
}
