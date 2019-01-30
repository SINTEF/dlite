/* -*- C -*-  (not really, but good for syntax highlighting) */


/* Typemap for argout blob */
%typemap("doc") (unsigned char **ARGOUT_BYTES, size_t *LEN) "Bytes."
%typemap(in,numinputs=0) (unsigned char **ARGOUT_BYTES, size_t *LEN)
  (unsigned char *tmp, size_t n)
{
  $1 = &tmp;
  $2 = &n;
}
%typemap(argout) (unsigned char **ARGOUT_BYTES, size_t *LEN)
{
  $result = PyByteArray_FromStringAndSize((char *)tmp$argnum, n$argnum);
}


/**********************************************
 ** Out typemaps
 **********************************************/
%{
  typedef int status_t;     // error if non-zero, no output
  typedef int posstatus_t;  // error if negative
%}


/* Convert non-zero return value to RuntimeError exception */
%typemap("doc") const_char ** "Non-zero on error."
%typemap(out) status_t {
  if ($1) SWIG_exception_fail(SWIG_RuntimeError,
			      "non-zero return value in $symname()");
  $result = Py_None;
  Py_INCREF(Py_None); // Py_None is a singleton so increment its refcount
}

/* Raise RuntimeError exception on negative return, otherwise return int */
%typemap("doc") const_char ** "Less than zero on error."
%typemap(out) posstatus_t {
  if ($1 < 0) SWIG_exception_fail(SWIG_RuntimeError,
				  "negative return value in $symname()");
  $result = PyLong_FromLong($1);
}
