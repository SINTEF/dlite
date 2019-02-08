/* -*- C -*-  (not really, but good for syntax highlighting) */
/*
   This file implements a layer between CPython and dlite typemaps.
*/

%{
#include <Python.h>
#include "boolean.h"
#ifndef HAVE_STDINT_H
#include "integers.h"
#endif
#include "floats.h"
#include "dlite.h"

#define SWIG_FILE_WITH_INIT  /* tell numpy that we initialize it in %init */


/* Returns a pointer to a new target language object for property `i`. */
void *dlite_swig_get_property_by_index(const DLiteInstance *inst, size_t i)
{
  void *ptr = DLITE_PROP(inst, i);
  DLiteProperty *p = inst->meta->properties + i;
  PyObject *pyobj = NULL;
  if (p->ndims == 0) {
    /* -- scalar property */
    switch (p->type) {

    case dliteBlob: {
      pyobj = PyByteArray_FromStringAndSize((const char *)ptr, p->size);
      break;
    }

    case dliteBool: {
      long value = *((bool *)ptr);
      pyobj = PyBool_FromLong(value);
      break;
    }

    case dliteInt: {
      long value;
      switch (p->size) {
      case 1: value = *((int8_t *)ptr);  break;
      case 2: value = *((int16_t *)ptr); break;
      case 4: value = *((int32_t *)ptr); break;
      case 8: value = *((int64_t *)ptr); break;
      default: FAIL1("invalid integer size: %zu", p->size);
      }
      pyobj = PyLong_FromLong(value);
      break;
    }

    case dliteUInt: {
      unsigned long value;
      switch (p->size) {
      case 1: value = *((uint8_t *)ptr);  break;
      case 2: value = *((uint16_t *)ptr); break;
      case 4: value = *((uint32_t *)ptr); break;
      case 8: value = *((uint64_t *)ptr); break;
      default: FAIL1("invalid unsigned integer size: %zu", p->size);
      }
      pyobj = PyLong_FromUnsignedLong(value);
      break;
    }

    case dliteFloat: {
      double value;
      switch (p->size) {
      case 4: value = *((float32_t *)ptr); break;
      case 8: value = *((float64_t *)ptr); break;
      //case 10: value = *((float80_t *)ptr); break;
      //case 16: value = *((float128_t *)ptr); break;
      default: FAIL1("invalid float size: %zu", p->size);
      }
      pyobj = PyFloat_FromDouble(value);
      break;
    }

    case dliteFixString: {
      pyobj = PyUnicode_FromStringAndSize(ptr, p->size);
      break;
    }

    case dliteStringPtr: {
      assert(ptr);
      pyobj = PyUnicode_FromString(*((char **)ptr));
      break;
    }

    case dliteDimension: {
      break;
    }

    case dliteProperty: {
      break;
    }

    case dliteRelation: {
      break;
    }

    }
  } else {
    /* -- array property */

  }
  if (!pyobj) goto fail;
  return (void *)pyobj;
 fail:
  if (pyobj) Py_DECREF(pyobj);
  if (!dlite_errval()) dlite_err(1, "error setting property \"%s\"", p->name);
  return NULL;
}


/* Sets property `i` to the value pointed to by the target language
   object `obj`.  Returns non-zero on error. */
int dlite_swig_set_property_by_index(const DLiteInstance *inst, size_t i,
                                     const void *obj)
{
  int status = 1;
  void *ptr = DLITE_PROP(inst, i);
  PyObject *pyobj = (PyObject *)obj;
  DLiteProperty *p = inst->meta->properties + i;

  PyErr_Clear();

  if (p->ndims == 0) {
    /* -- scalar property */
    switch (p->type) {

    case dliteBlob: {
      size_t size;
      PyObject *bytes = PyObject_Bytes(pyobj);
      if (!bytes) FAIL1("Setting property \"%s\": cannot convert to bytes",
                        p->name);
      size = PyByteArray_Size(bytes);
      if (size > p->size) {
        Py_DECREF(bytes);
        FAIL3("Length of bytearray is %zu. Exceeds size of blob property "
              "\"%s\": %zu", size, p->name, p->size);
      }
      memset(ptr, 0, p->size);
      memcpy(ptr, PyByteArray_AsString(pyobj), size);
      Py_DECREF(bytes);
      break;
    }

    case dliteBool: {
      int value = PyObject_IsTrue(pyobj);
      if (value < 0) FAIL1("Setting property \"%s\": cannot convert to boolean",
                           p->name);
      *((bool *)ptr) = value;
      break;
    }

    case dliteInt: {
      int overflow;
#ifdef HAVE_LONG_LONG
      long long value = PyLong_AsLongLongAndOverflow(pyobj, &overflow);
#else
      long value = PyLong_AsLongAndOverflow(pyobj, &overflow);
#endif
      if (overflow) FAIL1("Setting property \"%s\": overflow", p->name);
      if (PyErr_Occurred()) FAIL1("Failed setting property \"%s\"", p->name);
      switch (p->size) {
      case 1: *((int8_t *)ptr) = value;  break;
      case 2: *((int16_t *)ptr) = value; break;
      case 4: *((int32_t *)ptr) = value; break;
      case 8: *((int64_t *)ptr) = value; break;
      default: FAIL1("invalid integer size: %zu", p->size);
      }
      break;
    }

    case dliteUInt: {
#ifdef HAVE_LONG_LONG
      unsigned long long value = PyLong_AsUnsignedLongLong(pyobj);
#else
      unsigned long value = PyLong_AsUnsignedLong(pyobj);
#endif
      if (PyErr_Occurred()) FAIL1("Failed setting property \"%s\"", p->name);
      switch (p->size) {
      case 1: *((uint8_t *)ptr) = value;  break;
      case 2: *((uint16_t *)ptr) = value; break;
      case 4: *((uint32_t *)ptr) = value; break;
      case 8: *((uint64_t *)ptr) = value; break;
      default: FAIL1("invalid unsigned integer size: %zu", p->size);
      }
      break;
    }

    case dliteFloat: {
      double value = PyFloat_AsDouble(pyobj);
      if (PyErr_Occurred()) FAIL1("Failed setting property \"%s\"", p->name);
      switch (p->size) {
      case 4:  *((float32_t *)ptr) = value; break;
      case 8:  *((float64_t *)ptr) = value; break;
      //case 10: *((float80_t *)ptr) = value; break;
      //case 16: *((float128_t *)ptr) = value; break;
      default: FAIL1("invalid float size: %zu", p->size);
      }
      break;
    }

    case dliteFixString: {
      size_t size;
      Py_UCS1 *s;
      PyObject *str = PyObject_Str(pyobj);
      if (!str) FAIL1("Setting property \"%s\": cannot convert to string",
                      p->name);
      if (PyUnicode_READY(str)) {
        Py_DECREF(str);
        FAIL1("Setting property \"%s\": failed preparing string", p->name);
      }
      size = PyUnicode_GET_LENGTH(str);
      if (size > p->size) {
        Py_DECREF(str);
        FAIL3("Length of string is %zu. Exceeds size of property \"%s\": %zu",
              size, p->name, p->size);
      }
      s = PyUnicode_1BYTE_DATA(str);
      memset(ptr, 0, p->size);
      memcpy(ptr, s, size);
      Py_DECREF(str);
      break;
    }

    case dliteStringPtr: {
      size_t size;
      Py_UCS1 *s;
      PyObject *str = PyObject_Str(pyobj);
      if (!str) FAIL1("Setting property \"%s\": cannot convert to string",
                      p->name);
      if (PyUnicode_READY(str)) {
        Py_DECREF(str);
        FAIL1("Setting property \"%s\": failed preparing string", p->name);
      }
      size = PyUnicode_GET_LENGTH(str);
      if (size > p->size) {
        Py_DECREF(str);
        FAIL3("Length of string is %zu. Exceeds size of property \"%s\": %zu",
              size, p->name, p->size);
      }
      s = PyUnicode_1BYTE_DATA(str);
      *((char **)ptr) = realloc(*((char **)ptr), size+1);
      memcpy(*((char **)ptr), s, size);
      *((char **)ptr)[size] = '\0';
      Py_DECREF(str);
      break;
    }

    case dliteDimension: {
      break;
    }

    case dliteProperty: {
      break;
    }

    case dliteRelation: {
      break;
    }

    }
  } else {
    /* -- array property */


  }

  status = 0;
 fail:
  return status;
}




%}

/**********************************************
 ** Module initialisation
 **********************************************/

%include "numpy.i"  // slightly changed to fit out needs, search for "XXX"

%init %{
  /* Initialize numpy */
  import_array();
%}
