/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPythonArgs.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-----------------------------------------------------------------------
The svtkPythonArgs class was created in Oct 2010 by David Gobbi.

This class provides methods for reading an argument tuple from Python
and converting it to types that can be used by SVTK.  It is meant to be
more efficient and flexible that the original PyArg_ParseTuple() code,
resulting in wrapper code that is faster and more compact.
-----------------------------------------------------------------------*/

// Keep svtkPythonArgs.h from declaring externs for templates we instantiate
#define svtkPythonArgs_cxx

#include "svtkPythonArgs.h"
#include "svtkPythonUtil.h"

#include "svtkObjectBase.h"

//--------------------------------------------------------------------
// Extract various C++ types from python objects.  The rules are
// identical to PyArg_ParseTuple except that range checking is done
// on unsigned values.

// Macro to mimic a check done in PyArg_ParseTuple
#define SVTK_PYTHON_FLOAT_CHECK()                                                                   \
  if (PyFloat_Check(o))                                                                            \
  {                                                                                                \
    PyErr_SetString(PyExc_TypeError, "integer argument expected, got float");                      \
    return false;                                                                                  \
  }

inline bool svtkPythonGetValue(PyObject* o, long& a)
{
  SVTK_PYTHON_FLOAT_CHECK();

  a = PyInt_AsLong(o);
  return (a != static_cast<long>(-1) || !PyErr_Occurred());
}

inline bool svtkPythonGetValue(PyObject* o, unsigned long& a)
{
  SVTK_PYTHON_FLOAT_CHECK();

  a = PyLong_AsUnsignedLong(o);
  return (a != static_cast<unsigned long>(-1) || !PyErr_Occurred());
}

template <class T>
inline bool svtkPythonGetLongLongValue(PyObject* o, T& a)
{
  SVTK_PYTHON_FLOAT_CHECK();

  PY_LONG_LONG i = PyLong_AsLongLong(o);
  a = static_cast<T>(i);
  return (i != static_cast<PY_LONG_LONG>(-1) || !PyErr_Occurred());
}

template <class T>
inline bool svtkPythonGetUnsignedLongLongValue(PyObject* o, T& a)
{
  SVTK_PYTHON_FLOAT_CHECK();

  // PyLong_AsUnsignedLongLong will fail if "o" is not a PyLong
  if (PyLong_Check(o))
  {
    unsigned PY_LONG_LONG i = PyLong_AsUnsignedLongLong(o);
    a = static_cast<T>(i);
    return (i != static_cast<unsigned PY_LONG_LONG>(-1) || !PyErr_Occurred());
  }

  unsigned long l = PyLong_AsUnsignedLong(o);
  a = static_cast<T>(l);
  return (l != static_cast<unsigned long>(-1) || !PyErr_Occurred());
}

Py_ssize_t svtkPythonGetStringSize(PyObject* o)
{
  if (PyBytes_Check(o))
  {
    return PyBytes_GET_SIZE(o);
  }
  else if (PyByteArray_Check(o))
  {
    return PyByteArray_GET_SIZE(o);
  }
#ifdef Py_USING_UNICODE
  else if (PyUnicode_Check(o))
  {
#if PY_VERSION_HEX >= 0x03030000
    Py_ssize_t size;
    PyUnicode_AsUTF8AndSize(o, &size);
    return size;
#else
    PyObject* s = _PyUnicode_AsDefaultEncodedString(o, nullptr);
    if (s)
    {
      return PyBytes_GET_SIZE(s);
    }
#endif
  }
#endif
  return 0;
}

bool svtkPythonGetStringValue(PyObject* o, const char*& a, const char* exctext)
{
  if (PyBytes_Check(o))
  {
    a = PyBytes_AS_STRING(o);
    return true;
  }
  else if (PyByteArray_Check(o))
  {
    a = PyByteArray_AS_STRING(o);
    return true;
  }
#ifdef Py_USING_UNICODE
  else if (PyUnicode_Check(o))
  {
#if PY_VERSION_HEX >= 0x03030000
    a = PyUnicode_AsUTF8(o);
    return true;
#else
    PyObject* s = _PyUnicode_AsDefaultEncodedString(o, nullptr);
    if (s)
    {
      a = PyBytes_AS_STRING(s);
      return true;
    }

    if (exctext)
    {
      // set a more specific error message
      exctext = "(unicode conversion error)";
    }
#endif
  }
#endif

  if (exctext)
  {
    PyErr_SetString(PyExc_TypeError, exctext);
  }
  return false;
}

inline bool svtkPythonGetStdStringValue(PyObject* o, std::string& a, const char* exctext)
{
  if (PyBytes_Check(o))
  {
    char* val;
    Py_ssize_t len;
    PyBytes_AsStringAndSize(o, &val, &len);
    a = std::string(val, len);
    return true;
  }
#ifdef Py_USING_UNICODE
  else if (PyUnicode_Check(o))
  {
#if PY_VERSION_HEX >= 0x03030000
    Py_ssize_t len;
    const char* val = PyUnicode_AsUTF8AndSize(o, &len);
    a = std::string(val, len);
    return true;
#else
    PyObject* s = _PyUnicode_AsDefaultEncodedString(o, nullptr);
    if (s)
    {
      char* val;
      Py_ssize_t len;
      PyBytes_AsStringAndSize(s, &val, &len);
      a = std::string(val, len);
      return true;
    }

    exctext = "(unicode conversion error)";
#endif
  }
#endif

  PyErr_SetString(PyExc_TypeError, exctext);
  return false;
}

//--------------------------------------------------------------------
// Overloaded methods, mostly based on the above templates

// Get a void pointer to the contents of a buffer of type "btype", where
// btype one of the type characters defined in the python "struct" module.
static bool svtkPythonGetValue(PyObject* o, const void*& a, Py_buffer* view, char btype)
{
  void* p = nullptr;
  Py_ssize_t sz = 0;
  const char* format = nullptr;
#ifndef SVTK_PY3K
  PyBufferProcs* b = Py_TYPE(o)->tp_as_buffer;
#endif

#if PY_VERSION_HEX < 0x02060000
  (void)view;
#else
#ifdef SVTK_PY3K
  PyObject* bytes = nullptr;
  if (PyUnicode_Check(o))
  {
    bytes = PyUnicode_AsUTF8String(o);
    PyBytes_AsStringAndSize(bytes, reinterpret_cast<char**>(&p), &sz);
  }
  else
#endif
    if (PyObject_CheckBuffer(o))
  {
    int flags = (PyBUF_ANY_CONTIGUOUS | PyBUF_FORMAT);
    if (btype == '\0')
    {
      // if btype indicates "void *", use simple buffer
      flags = PyBUF_SIMPLE;
    }
    // use the modern python buffer interface
    if (PyObject_GetBuffer(o, view, flags) == -1)
    {
      return false;
    }
    p = view->buf;
    sz = view->len;
    format = view->format;
    // check to see if the type is compatible
    if (btype != '\0')
    {
      // if "btype" is set, then check type compatibility
      char vtype = (format ? format[0] : 'B');
      if (vtype == '@')
      {
        vtype = format[1];
      }
      if (btype != vtype)
      {
        PyErr_Format(PyExc_TypeError, "incorrect buffer type, expected %c but received %s", btype,
          (format ? format : "B"));
        return false;
      }
    }
  }
#ifndef SVTK_PY3K
  else
#endif
#endif
#ifndef SVTK_PY3K
  // use the old buffer interface
  if (b && b->bf_getreadbuffer && b->bf_getsegcount)
  {
    if (b->bf_getsegcount(o, nullptr) == 1)
    {
      sz = b->bf_getreadbuffer(o, 0, &p);
    }
    else
    {
      PyErr_SetString(PyExc_TypeError, "buffer must be single-segment");
      return false;
    }
  }
#endif

#ifdef SVTK_PY3K
  if (bytes && btype == '\0')
#else
    if (p && sz >= 0 && sz <= SVTK_INT_MAX && btype == '\0' &&
      (format == nullptr || format[0] == 'c' || format[0] == 'B'))
#endif
  {
    // check for pointer mangled as string
    int s = static_cast<int>(sz);
    a = svtkPythonUtil::UnmanglePointer(reinterpret_cast<char*>(p), &s, "p_void");
#ifdef SVTK_PY3K
    Py_DECREF(bytes);
    if (s != 0)
    {
      PyErr_SetString(PyExc_TypeError, "requires a _addr_p_void string");
      return false;
    }
#else
    if (s == -1)
    {
      // matched _addr_ but not p_void, assume it isn't a swig ptr string:
      // use the buffer's pointer as the argument
      a = p;
    }
#endif
    return true;
  }
  else if (p && sz >= 0)
  {
    // directly use the pointer to the buffer contents
    a = p;
    return true;
  }

  PyErr_SetString(PyExc_TypeError, "object does not have a readable buffer");
  return false;
}

inline bool svtkPythonGetValue(PyObject* o, void*& a, Py_buffer* buf, char btype)
{
  // should have an alternate form for non-const "void *" that uses
  // writebuffer instead of readbuffer, but that would break existing code
  const void* b = nullptr;
  bool r = svtkPythonGetValue(o, b, buf, btype);
  a = const_cast<void*>(b);
  return r;
}

inline bool svtkPythonGetValue(PyObject* o, const char*& a)
{
  a = nullptr;

  return (o == Py_None || svtkPythonGetStringValue(o, a, "string or None required"));
}

inline bool svtkPythonGetValue(PyObject* o, std::string& a)
{
  if (svtkPythonGetStdStringValue(o, a, "string is required"))
  {
    return true;
  }
  return false;
}

inline bool svtkPythonGetValue(PyObject* o, svtkUnicodeString& a)
{
#ifdef Py_USING_UNICODE
  PyObject* s = PyUnicode_AsUTF8String(o);
  if (s)
  {
    a = svtkUnicodeString::from_utf8(PyBytes_AS_STRING(s));
    Py_DECREF(s);
    return true;
  }
  return false;
#else
  a.clear();
  PyErr_SetString(PyExc_TypeError, "python built without unicode support");
  return false;
#endif
}

inline bool svtkPythonGetValue(PyObject* o, char& a)
{
  static const char exctext[] = "a string of length 1 is required";
  const char* b;
  if (svtkPythonGetStringValue(o, b, exctext))
  {
    if (b[0] == '\0' || b[1] == '\0')
    {
      a = b[0];
      return true;
    }
    PyErr_SetString(PyExc_TypeError, exctext);
  }
  return false;
}

inline bool svtkPythonGetValue(PyObject* o, bool& a)
{
  int i = PyObject_IsTrue(o);
  a = (i != 0);
  return (i != -1);
}

inline bool svtkPythonGetValue(PyObject* o, float& a)
{
  a = static_cast<float>(PyFloat_AsDouble(o));
  return (a != -1.0f || !PyErr_Occurred());
}

inline bool svtkPythonGetValue(PyObject* o, double& a)
{
  a = PyFloat_AsDouble(o);
  return (a != -1.0f || !PyErr_Occurred());
}

inline bool svtkPythonGetValue(PyObject* o, signed char& a)
{
  long i = 0;
  if (svtkPythonGetValue(o, i))
  {
    a = static_cast<signed char>(i);
    if (i >= SVTK_SIGNED_CHAR_MIN && i <= SVTK_SIGNED_CHAR_MAX)
    {
      return true;
    }
    PyErr_SetString(PyExc_OverflowError, "value is out of range for signed char");
  }
  return false;
}

inline bool svtkPythonGetValue(PyObject* o, unsigned char& a)
{
  long i = 0;
  if (svtkPythonGetValue(o, i))
  {
    a = static_cast<unsigned char>(i);
    if (i >= SVTK_UNSIGNED_CHAR_MIN && i <= SVTK_UNSIGNED_CHAR_MAX)
    {
      return true;
    }
    PyErr_SetString(PyExc_OverflowError, "value is out of range for unsigned char");
  }
  return false;
}

inline bool svtkPythonGetValue(PyObject* o, short& a)
{
  long i = 0;
  if (svtkPythonGetValue(o, i))
  {
    a = static_cast<short>(i);
    if (i >= SVTK_SHORT_MIN && i <= SVTK_SHORT_MAX)
    {
      return true;
    }
    PyErr_SetString(PyExc_OverflowError, "value is out of range for short");
  }
  return false;
}

inline bool svtkPythonGetValue(PyObject* o, unsigned short& a)
{
  long i = 0;
  if (svtkPythonGetValue(o, i))
  {
    a = static_cast<unsigned short>(i);
    if (i >= SVTK_UNSIGNED_SHORT_MIN && i <= SVTK_UNSIGNED_SHORT_MAX)
    {
      return true;
    }
    PyErr_SetString(PyExc_OverflowError, "value is out of range for unsigned short");
  }
  return false;
}

inline bool svtkPythonGetValue(PyObject* o, int& a)
{
  long i = 0;
  if (svtkPythonGetValue(o, i))
  {
    a = static_cast<int>(i);
#if SVTK_SIZEOF_INT < SVTK_SIZEOF_LONG
    if (i >= SVTK_INT_MIN && i <= SVTK_INT_MAX)
    {
      return true;
    }
    PyErr_SetString(PyExc_OverflowError, "value is out of range for int");
#else
    return true;
#endif
  }
  return false;
}

inline bool svtkPythonGetValue(PyObject* o, unsigned int& a)
{
#if SVTK_SIZEOF_INT < SVTK_SIZEOF_LONG
  long i = 0;
  if (svtkPythonGetValue(o, i))
  {
    a = static_cast<unsigned int>(i);
    if (i >= SVTK_UNSIGNED_INT_MIN && i <= SVTK_UNSIGNED_INT_MAX)
    {
      return true;
    }
    PyErr_SetString(PyExc_OverflowError, "value is out of range for unsigned int");
  }
  return false;
#else
  unsigned long i = 0;
  if (svtkPythonGetValue(o, i))
  {
    a = static_cast<unsigned int>(i);
    return true;
  }
  return false;
#endif
}

inline bool svtkPythonGetValue(PyObject* o, long long& a)
{
  return svtkPythonGetLongLongValue(o, a);
}

inline bool svtkPythonGetValue(PyObject* o, unsigned long long& a)
{
  return svtkPythonGetUnsignedLongLongValue(o, a);
}

//--------------------------------------------------------------------
// Method for setting a C++ array from a Python sequence.

static bool svtkPythonSequenceError(PyObject* o, size_t n, size_t m);

template <class T>
inline bool svtkPythonGetArray(PyObject* o, T* a, size_t n)
{
  if (a)
  {
    Py_ssize_t m = static_cast<Py_ssize_t>(n);

    if (PyTuple_Check(o))
    {
      m = PyTuple_GET_SIZE(o);
      if (m == static_cast<Py_ssize_t>(n))
      {
        bool r = true;
        for (Py_ssize_t i = 0; i < m && r; i++)
        {
          PyObject* s = PyTuple_GET_ITEM(o, i);
          r = svtkPythonGetValue(s, a[i]);
        }
        return r;
      }
    }
    else if (PyList_Check(o))
    {
      m = PyList_GET_SIZE(o);
      if (m == static_cast<Py_ssize_t>(n))
      {
        bool r = true;
        for (Py_ssize_t i = 0; i < m && r; i++)
        {
          PyObject* s = PyList_GET_ITEM(o, i);
          r = svtkPythonGetValue(s, a[i]);
        }
        return r;
      }
    }
    else if (PySequence_Check(o))
    {
      m = PySequence_Size(o);
      if (m == static_cast<Py_ssize_t>(n))
      {
        bool r = true;
        for (Py_ssize_t i = 0; i < m && r; i++)
        {
          r = false;
          PyObject* s = PySequence_GetItem(o, i);
          if (s && svtkPythonGetValue(s, a[i]))
          {
            Py_DECREF(s);
            r = true;
          }
        }
        return r;
      }
    }

    return svtkPythonSequenceError(o, n, m);
  }

  return true;
}

inline bool svtkPythonGetArray(PyObject* o, char* a, size_t n)
{
  if (a)
  {
    Py_ssize_t m = static_cast<Py_ssize_t>(n);
    const char* b;

    if (svtkPythonGetStringValue(o, b, nullptr))
    {
      m = svtkPythonGetStringSize(o);
      if (m == static_cast<Py_ssize_t>(n))
      {
        for (size_t i = 0; i < n; i++)
        {
          a[i] = b[i];
        }
        // terminate so it can be used as a C string
        a[n] = '\0';
        return true;
      }
    }
    else if (PySequence_Check(o))
    {
      m = PySequence_Size(o);
      if (m == static_cast<Py_ssize_t>(n))
      {
        bool r = true;
        for (Py_ssize_t i = 0; i < m && r; i++)
        {
          r = false;
          PyObject* s = PySequence_GetItem(o, i);
          if (s && svtkPythonGetValue(s, a[i]))
          {
            Py_DECREF(s);
            r = true;
          }
        }
        return r;
      }
    }

    return svtkPythonSequenceError(o, n, m);
  }

  return true;
}

//--------------------------------------------------------------------
// Method for setting an n-dimensional C++ arrays from a Python sequence.

template <class T>
bool svtkPythonGetNArray(PyObject* o, T* a, int ndim, const size_t* dims)
{
  if (a)
  {
    size_t inc = 1;
    for (int j = 1; j < ndim; j++)
    {
      inc *= dims[j];
    }

    size_t n = dims[0];
    Py_ssize_t m = static_cast<Py_ssize_t>(n);

    if (PyList_Check(o))
    {
      m = PyList_GET_SIZE(o);
      if (m == static_cast<Py_ssize_t>(n))
      {
        bool r = true;
        if (ndim > 1)
        {
          for (Py_ssize_t i = 0; i < m && r; i++)
          {
            PyObject* s = PyList_GET_ITEM(o, i);
            r = svtkPythonGetNArray(s, a, ndim - 1, dims + 1);
            a += inc;
          }
        }
        else
        {
          for (Py_ssize_t i = 0; i < m && r; i++)
          {
            PyObject* s = PyList_GET_ITEM(o, i);
            r = svtkPythonGetValue(s, a[i]);
          }
        }
        return r;
      }
    }
    else if (PySequence_Check(o))
    {
      m = PySequence_Size(o);
      if (m == static_cast<Py_ssize_t>(n))
      {
        bool r = true;
        for (Py_ssize_t i = 0; i < m && r; i++)
        {
          r = false;
          PyObject* s = PySequence_GetItem(o, i);
          if (s)
          {
            if (ndim > 1)
            {
              r = svtkPythonGetNArray(s, a, ndim - 1, dims + 1);
              a += inc;
            }
            else
            {
              r = svtkPythonGetValue(s, a[i]);
            }
            Py_DECREF(s);
          }
        }
        return r;
      }
    }

    return svtkPythonSequenceError(o, n, m);
  }

  return true;
}

//--------------------------------------------------------------------
// Method for setting a python sequence from a C++ array

template <class T>
inline bool svtkPythonSetArray(PyObject* o, const T* a, size_t n)
{
  if (a)
  {
    Py_ssize_t m = static_cast<Py_ssize_t>(n);

    if (PyList_Check(o))
    {
      m = PyList_GET_SIZE(o);
      if (m == static_cast<Py_ssize_t>(n))
      {
        bool r = true;
        for (Py_ssize_t i = 0; i < m && r; i++)
        {
          r = false;
          PyObject* s = svtkPythonArgs::BuildValue(a[i]);
          if (s)
          {
            Py_DECREF(PyList_GET_ITEM(o, i));
            PyList_SET_ITEM(o, i, s);
            r = true;
          }
        }
        return r;
      }
    }
    else if (PySequence_Check(o))
    {
      m = PySequence_Size(o);
      if (m == static_cast<Py_ssize_t>(n))
      {
        bool r = true;
        for (Py_ssize_t i = 0; i < m && r; i++)
        {
          r = false;
          PyObject* s = svtkPythonArgs::BuildValue(a[i]);
          if (s)
          {
            r = (PySequence_SetItem(o, i, s) != -1);
            Py_DECREF(s);
          }
        }
        return r;
      }
    }

    return svtkPythonSequenceError(o, n, m);
  }

  return true;
}

inline bool svtkPythonSetArray(PyObject* o, const char* a, size_t n)
{
  if (a)
  {
    Py_ssize_t m = static_cast<Py_ssize_t>(n);

    if (PyByteArray_Check(o))
    {
      m = PyByteArray_GET_SIZE(o);
      if (m == static_cast<Py_ssize_t>(n))
      {
        char* b = PyByteArray_AS_STRING(o);
        for (Py_ssize_t i = 0; i < m; i++)
        {
          b[i] = a[i];
        }
        return true;
      }
    }
    else if (PySequence_Check(o))
    {
      m = PySequence_Size(o);
      if (m == static_cast<Py_ssize_t>(n))
      {
        bool r = true;
        for (Py_ssize_t i = 0; i < m && r; i++)
        {
          r = false;
          PyObject* s = svtkPythonArgs::BuildValue(a[i]);
          if (s)
          {
            r = (PySequence_SetItem(o, i, s) != -1);
            Py_DECREF(s);
          }
        }
        return r;
      }
    }

    return svtkPythonSequenceError(o, n, m);
  }

  return true;
}

//--------------------------------------------------------------------
// Method for setting a python array from an n-dimensional C++ array

template <class T>
bool svtkPythonSetNArray(PyObject* o, const T* a, int ndim, const size_t* dims)
{
  if (a)
  {
    size_t inc = 1;
    for (int j = 1; j < ndim; j++)
    {
      inc *= dims[j];
    }

    size_t n = dims[0];
    Py_ssize_t m = static_cast<Py_ssize_t>(n);

    if (PyList_Check(o))
    {
      m = PyList_GET_SIZE(o);
      if (m == static_cast<Py_ssize_t>(n))
      {
        bool r = true;
        if (ndim > 1)
        {
          for (Py_ssize_t i = 0; i < m && r; i++)
          {
            PyObject* s = PyList_GET_ITEM(o, i);
            r = svtkPythonSetNArray(s, a, ndim - 1, dims + 1);
            a += inc;
          }
        }
        else
        {
          for (Py_ssize_t i = 0; i < m && r; i++)
          {
            r = false;
            PyObject* s = svtkPythonArgs::BuildValue(a[i]);
            if (s)
            {
              Py_DECREF(PyList_GET_ITEM(o, i));
              PyList_SET_ITEM(o, i, s);
              r = true;
            }
          }
        }
        return r;
      }
    }
    else if (PySequence_Check(o))
    {
      m = PySequence_Size(o);
      if (m == static_cast<Py_ssize_t>(n))
      {
        bool r = true;
        if (ndim > 1)
        {
          for (Py_ssize_t i = 0; i < m && r; i++)
          {
            r = false;
            PyObject* s = PySequence_GetItem(o, i);
            if (s)
            {
              r = svtkPythonSetNArray(s, a, ndim - 1, dims + 1);
              a += inc;
              Py_DECREF(s);
            }
          }
        }
        else
        {
          for (Py_ssize_t i = 0; i < m && r; i++)
          {
            r = false;
            PyObject* s = svtkPythonArgs::BuildValue(a[i]);
            if (s)
            {
              r = (PySequence_SetItem(o, i, s) != -1);
              Py_DECREF(s);
            }
          }
        }
        return r;
      }
    }

    return svtkPythonSequenceError(o, n, m);
  }

  return true;
}

//--------------------------------------------------------------------
// Define all the "BuildValue" array methods defined in the class.

template <class T>
inline PyObject* svtkPythonBuildTuple(const T* a, size_t n)
{
  if (a)
  {
    Py_ssize_t m = static_cast<Py_ssize_t>(n);
    PyObject* t = PyTuple_New(m);
    for (Py_ssize_t i = 0; i < m; i++)
    {
      PyObject* o = svtkPythonArgs::BuildValue(a[i]);
      PyTuple_SET_ITEM(t, i, o);
    }
    return t;
  }

  Py_INCREF(Py_None);
  return Py_None;
}

#define SVTK_PYTHON_BUILD_TUPLE(T)                                                                  \
  PyObject* svtkPythonArgs::BuildTuple(const T* a, size_t n) { return svtkPythonBuildTuple(a, n); }

SVTK_PYTHON_BUILD_TUPLE(bool)
SVTK_PYTHON_BUILD_TUPLE(float)
SVTK_PYTHON_BUILD_TUPLE(double)
SVTK_PYTHON_BUILD_TUPLE(signed char)
SVTK_PYTHON_BUILD_TUPLE(unsigned char)
SVTK_PYTHON_BUILD_TUPLE(short)
SVTK_PYTHON_BUILD_TUPLE(unsigned short)
SVTK_PYTHON_BUILD_TUPLE(int)
SVTK_PYTHON_BUILD_TUPLE(unsigned int)
SVTK_PYTHON_BUILD_TUPLE(long)
SVTK_PYTHON_BUILD_TUPLE(unsigned long)
SVTK_PYTHON_BUILD_TUPLE(long long)
SVTK_PYTHON_BUILD_TUPLE(unsigned long long)
SVTK_PYTHON_BUILD_TUPLE(std::string)
SVTK_PYTHON_BUILD_TUPLE(svtkUnicodeString)

//--------------------------------------------------------------------

PyObject* svtkPythonArgs::BuildEnumValue(int val, const char* enumname)
{
  PyTypeObject* pytype = svtkPythonUtil::FindEnum(enumname);
  if (!pytype)
  {
    std::string errstring = "cannot build unknown enum ";
    errstring += enumname;
    PyErr_SetString(PyExc_TypeError, errstring.c_str());
    return nullptr;
  }

  return PySVTKEnum_New(pytype, val);
}

//--------------------------------------------------------------------
// If "self" is a class, get real "self" from arg list
PyObject* svtkPythonArgs::GetSelfFromFirstArg(PyObject* self, PyObject* args)
{
  if (PyType_Check(self))
  {
    PyTypeObject* pytype = (PyTypeObject*)self;
    if (PyTuple_GET_SIZE(args) > 0)
    {
      self = PyTuple_GET_ITEM(args, 0);
      if (PyObject_TypeCheck(self, pytype))
      {
        return self;
      }
    }

    char buf[256];
    snprintf(
      buf, sizeof(buf), "unbound method requires a %.200s as the first argument", pytype->tp_name);
    PyErr_SetString(PyExc_TypeError, buf);
    return nullptr;
  }

  PyErr_SetString(PyExc_TypeError, "unbound method requires a svtkobject");
  return nullptr;
}

//--------------------------------------------------------------------
// Define the GetArg methods for getting objects

PyObject* svtkPythonArgs::GetArgAsPythonObject(bool& valid)
{
  PyObject* o = PyTuple_GET_ITEM(this->Args, this->I++);
  valid = true;
  return o;
}

PyObject* svtkPythonArgs::GetArgAsPythonObject(PyObject* o, bool& valid)
{
  valid = true;
  return o;
}

svtkObjectBase* svtkPythonArgs::GetArgAsSVTKObject(const char* classname, bool& valid)
{
  PyObject* o = PyTuple_GET_ITEM(this->Args, this->I++);
  svtkObjectBase* r = svtkPythonArgs::GetArgAsSVTKObject(o, classname, valid);
  if (!valid)
  {
    this->RefineArgTypeError(this->I - this->M - 1);
  }
  return r;
}

svtkObjectBase* svtkPythonArgs::GetArgAsSVTKObject(PyObject* o, const char* classname, bool& valid)
{
  svtkObjectBase* r = svtkPythonUtil::GetPointerFromObject(o, classname);
  valid = (r || o == Py_None);
  return r;
}

void* svtkPythonArgs::GetArgAsSpecialObject(const char* classname, PyObject** p)
{
  PyObject* o = PyTuple_GET_ITEM(this->Args, this->I++);
  void* r = svtkPythonArgs::GetArgAsSpecialObject(o, classname, p);
  if (r == nullptr)
  {
    this->RefineArgTypeError(this->I - this->M - 1);
  }
  return r;
}

void* svtkPythonArgs::GetArgAsSpecialObject(PyObject* o, const char* classname, PyObject** p)
{
  void* r = svtkPythonUtil::GetPointerFromSpecialObject(o, classname, p);
  return r;
}

int svtkPythonArgs::GetArgAsEnum(const char* enumname, bool& valid)
{
  PyObject* o = PyTuple_GET_ITEM(this->Args, this->I++);
  int i = svtkPythonArgs::GetArgAsEnum(o, enumname, valid);
  if (!valid)
  {
    this->RefineArgTypeError(this->I - this->M - 1);
  }
  return i;
}

int svtkPythonArgs::GetArgAsEnum(PyObject* o, const char* enumname, bool& valid)
{
  long i = 0;
  PyTypeObject* pytype = svtkPythonUtil::FindEnum(enumname);
  if (pytype && PyObject_TypeCheck(o, pytype))
  {
    i = PyInt_AsLong(o);
    valid = true;
  }
  else
  {
    std::string errstring = "expected enum ";
    errstring += enumname;
    errstring += ", got ";
    errstring += Py_TYPE(o)->tp_name;
    PyErr_SetString(PyExc_TypeError, errstring.c_str());
    valid = false;
  }
  return i;
}

//--------------------------------------------------------------------
// Define all the "GetValue" methods in the class.

#define SVTK_PYTHON_GET_ARG(T)                                                                      \
  bool svtkPythonArgs::GetValue(T& a)                                                               \
  {                                                                                                \
    PyObject* o = PyTuple_GET_ITEM(this->Args, this->I++);                                         \
    if (PySVTKReference_Check(o))                                                                   \
    {                                                                                              \
      o = PySVTKReference_GetValue(o);                                                              \
    }                                                                                              \
    if (svtkPythonGetValue(o, a))                                                                   \
    {                                                                                              \
      return true;                                                                                 \
    }                                                                                              \
    this->RefineArgTypeError(this->I - this->M - 1);                                               \
    return false;                                                                                  \
  }                                                                                                \
                                                                                                   \
  bool svtkPythonArgs::GetValue(PyObject* o, T& a) { return svtkPythonGetValue(o, a); }

SVTK_PYTHON_GET_ARG(const char*)
SVTK_PYTHON_GET_ARG(std::string)
SVTK_PYTHON_GET_ARG(svtkUnicodeString)
SVTK_PYTHON_GET_ARG(char)
SVTK_PYTHON_GET_ARG(bool)
SVTK_PYTHON_GET_ARG(float)
SVTK_PYTHON_GET_ARG(double)
SVTK_PYTHON_GET_ARG(signed char)
SVTK_PYTHON_GET_ARG(unsigned char)
SVTK_PYTHON_GET_ARG(short)
SVTK_PYTHON_GET_ARG(unsigned short)
SVTK_PYTHON_GET_ARG(int)
SVTK_PYTHON_GET_ARG(unsigned int)
SVTK_PYTHON_GET_ARG(long)
SVTK_PYTHON_GET_ARG(unsigned long)
SVTK_PYTHON_GET_ARG(long long)
SVTK_PYTHON_GET_ARG(unsigned long long)

//--------------------------------------------------------------------
// Define all the GetArray methods in the class.

#define SVTK_PYTHON_GET_ARRAY_ARG(T)                                                                \
  bool svtkPythonArgs::GetArray(T* a, size_t n)                                                     \
  {                                                                                                \
    PyObject* o = PyTuple_GET_ITEM(this->Args, this->I++);                                         \
    if (svtkPythonGetArray(o, a, n))                                                                \
    {                                                                                              \
      return true;                                                                                 \
    }                                                                                              \
    this->RefineArgTypeError(this->I - this->M - 1);                                               \
    return false;                                                                                  \
  }

SVTK_PYTHON_GET_ARRAY_ARG(bool)
SVTK_PYTHON_GET_ARRAY_ARG(float)
SVTK_PYTHON_GET_ARRAY_ARG(double)
SVTK_PYTHON_GET_ARRAY_ARG(char)
SVTK_PYTHON_GET_ARRAY_ARG(signed char)
SVTK_PYTHON_GET_ARRAY_ARG(unsigned char)
SVTK_PYTHON_GET_ARRAY_ARG(short)
SVTK_PYTHON_GET_ARRAY_ARG(unsigned short)
SVTK_PYTHON_GET_ARRAY_ARG(int)
SVTK_PYTHON_GET_ARRAY_ARG(unsigned int)
SVTK_PYTHON_GET_ARRAY_ARG(long)
SVTK_PYTHON_GET_ARRAY_ARG(unsigned long)
SVTK_PYTHON_GET_ARRAY_ARG(long long)
SVTK_PYTHON_GET_ARRAY_ARG(unsigned long long)
SVTK_PYTHON_GET_ARRAY_ARG(std::string)
SVTK_PYTHON_GET_ARRAY_ARG(svtkUnicodeString)

//--------------------------------------------------------------------
// Define all the GetNArray methods in the class.

#define SVTK_PYTHON_GET_NARRAY_ARG(T)                                                               \
  bool svtkPythonArgs::GetNArray(T* a, int ndim, const size_t* dims)                                \
  {                                                                                                \
    PyObject* o = PyTuple_GET_ITEM(this->Args, this->I++);                                         \
    if (svtkPythonGetNArray(o, a, ndim, dims))                                                      \
    {                                                                                              \
      return true;                                                                                 \
    }                                                                                              \
    this->RefineArgTypeError(this->I - this->M - 1);                                               \
    return false;                                                                                  \
  }

SVTK_PYTHON_GET_NARRAY_ARG(bool)
SVTK_PYTHON_GET_NARRAY_ARG(float)
SVTK_PYTHON_GET_NARRAY_ARG(double)
SVTK_PYTHON_GET_NARRAY_ARG(char)
SVTK_PYTHON_GET_NARRAY_ARG(signed char)
SVTK_PYTHON_GET_NARRAY_ARG(unsigned char)
SVTK_PYTHON_GET_NARRAY_ARG(short)
SVTK_PYTHON_GET_NARRAY_ARG(unsigned short)
SVTK_PYTHON_GET_NARRAY_ARG(int)
SVTK_PYTHON_GET_NARRAY_ARG(unsigned int)
SVTK_PYTHON_GET_NARRAY_ARG(long)
SVTK_PYTHON_GET_NARRAY_ARG(unsigned long)
SVTK_PYTHON_GET_NARRAY_ARG(long long)
SVTK_PYTHON_GET_NARRAY_ARG(unsigned long long)

//--------------------------------------------------------------------
// Define the special function pointer GetValue method

bool svtkPythonArgs::GetFunction(PyObject* arg, PyObject*& o)
{
  o = arg;
  if (o == Py_None || PyCallable_Check(o))
  {
    return true;
  }
  PyErr_SetString(PyExc_TypeError, "a callable object is required");
  return false;
}

bool svtkPythonArgs::GetFunction(PyObject*& o)
{
  PyObject* arg = PyTuple_GET_ITEM(this->Args, this->I++);
  return svtkPythonArgs::GetFunction(arg, o);
}

//--------------------------------------------------------------------
// Define the void pointer GetValue method

#define SVTK_PYTHON_GET_BUFFER(T, btype)                                                            \
  bool svtkPythonArgs::GetBuffer(T*& a, Py_buffer* buf)                                             \
  {                                                                                                \
    PyObject* o = PyTuple_GET_ITEM(this->Args, this->I++);                                         \
    void* v;                                                                                       \
    if (svtkPythonGetValue(o, v, buf, btype))                                                       \
    {                                                                                              \
      a = static_cast<T*>(v);                                                                      \
      return true;                                                                                 \
    }                                                                                              \
    this->RefineArgTypeError(this->I - this->M - 1);                                               \
    return false;                                                                                  \
  }                                                                                                \
                                                                                                   \
  bool svtkPythonArgs::GetBuffer(const T*& a, Py_buffer* buf)                                       \
  {                                                                                                \
    PyObject* o = PyTuple_GET_ITEM(this->Args, this->I++);                                         \
    const void* v;                                                                                 \
    if (svtkPythonGetValue(o, v, buf, btype))                                                       \
    {                                                                                              \
      a = static_cast<const T*>(v);                                                                \
      return true;                                                                                 \
    }                                                                                              \
    this->RefineArgTypeError(this->I - this->M - 1);                                               \
    return false;                                                                                  \
  }                                                                                                \
                                                                                                   \
  bool svtkPythonArgs::GetBuffer(PyObject* o, T*& a, Py_buffer* buf)                                \
  {                                                                                                \
    void* v;                                                                                       \
    if (svtkPythonGetValue(o, v, buf, btype))                                                       \
    {                                                                                              \
      a = static_cast<T*>(v);                                                                      \
      return true;                                                                                 \
    }                                                                                              \
    return false;                                                                                  \
  }                                                                                                \
                                                                                                   \
  bool svtkPythonArgs::GetBuffer(PyObject* o, const T*& a, Py_buffer* buf)                          \
  {                                                                                                \
    const void* v;                                                                                 \
    if (svtkPythonGetValue(o, v, buf, btype))                                                       \
    {                                                                                              \
      a = static_cast<const T*>(v);                                                                \
      return true;                                                                                 \
    }                                                                                              \
    return false;                                                                                  \
  }

SVTK_PYTHON_GET_BUFFER(void, '\0')
SVTK_PYTHON_GET_BUFFER(float, 'f')
SVTK_PYTHON_GET_BUFFER(double, 'd')
SVTK_PYTHON_GET_BUFFER(bool, '\?')
SVTK_PYTHON_GET_BUFFER(char, 'c')
SVTK_PYTHON_GET_BUFFER(signed char, 'b')
SVTK_PYTHON_GET_BUFFER(unsigned char, 'B')
SVTK_PYTHON_GET_BUFFER(short, 'h')
SVTK_PYTHON_GET_BUFFER(unsigned short, 'H')
SVTK_PYTHON_GET_BUFFER(int, 'i')
SVTK_PYTHON_GET_BUFFER(unsigned int, 'I')
SVTK_PYTHON_GET_BUFFER(long, 'l')
SVTK_PYTHON_GET_BUFFER(unsigned long, 'L')
SVTK_PYTHON_GET_BUFFER(long long, 'q')
SVTK_PYTHON_GET_BUFFER(unsigned long long, 'Q')

//--------------------------------------------------------------------
// Define all the SetArgValue methods for setting reference args

#define SVTK_PYTHON_SET_ARG(T)                                                                      \
  bool svtkPythonArgs::SetArgValue(int i, T a)                                                      \
  {                                                                                                \
    if (this->M + i < this->N)                                                                     \
    {                                                                                              \
      PyObject* m = PyTuple_GET_ITEM(this->Args, this->M + i);                                     \
      PyObject* o = svtkPythonArgs::BuildValue(a);                                                  \
      int r = PySVTKReference_SetValue(m, o);                                                       \
      if (r == 0)                                                                                  \
      {                                                                                            \
        return true;                                                                               \
      }                                                                                            \
      this->RefineArgTypeError(i);                                                                 \
      return false;                                                                                \
    }                                                                                              \
    return true;                                                                                   \
  }

#define SVTK_PYTHON_SET_ARGN(T)                                                                     \
  bool svtkPythonArgs::SetArgValue(int i, const T* a, size_t n)                                     \
  {                                                                                                \
    if (this->M + i < this->N)                                                                     \
    {                                                                                              \
      PyObject* m = PyTuple_GET_ITEM(this->Args, this->M + i);                                     \
      PyObject* o = svtkPythonArgs::BuildTuple(a, n);                                               \
      int r = PySVTKReference_SetValue(m, o);                                                       \
      if (r == 0)                                                                                  \
      {                                                                                            \
        return true;                                                                               \
      }                                                                                            \
      this->RefineArgTypeError(i);                                                                 \
      return false;                                                                                \
    }                                                                                              \
    return true;                                                                                   \
  }

SVTK_PYTHON_SET_ARG(const std::string&)
SVTK_PYTHON_SET_ARG(const svtkUnicodeString&)
SVTK_PYTHON_SET_ARG(char)
SVTK_PYTHON_SET_ARG(bool)
SVTK_PYTHON_SET_ARG(float)
SVTK_PYTHON_SET_ARG(double)
SVTK_PYTHON_SET_ARG(signed char)
SVTK_PYTHON_SET_ARG(unsigned char)
SVTK_PYTHON_SET_ARG(short)
SVTK_PYTHON_SET_ARG(unsigned short)
SVTK_PYTHON_SET_ARG(int)
SVTK_PYTHON_SET_ARG(unsigned int)
SVTK_PYTHON_SET_ARG(long)
SVTK_PYTHON_SET_ARG(unsigned long)
SVTK_PYTHON_SET_ARG(long long)
SVTK_PYTHON_SET_ARG(unsigned long long)
SVTK_PYTHON_SET_ARGN(bool)
SVTK_PYTHON_SET_ARGN(float)
SVTK_PYTHON_SET_ARGN(double)
SVTK_PYTHON_SET_ARGN(signed char)
SVTK_PYTHON_SET_ARGN(unsigned char)
SVTK_PYTHON_SET_ARGN(short)
SVTK_PYTHON_SET_ARGN(unsigned short)
SVTK_PYTHON_SET_ARGN(int)
SVTK_PYTHON_SET_ARGN(unsigned int)
SVTK_PYTHON_SET_ARGN(long)
SVTK_PYTHON_SET_ARGN(unsigned long)
SVTK_PYTHON_SET_ARGN(long long)
SVTK_PYTHON_SET_ARGN(unsigned long long)

//--------------------------------------------------------------------
// Define all the SetArgValue methods for setting array args

#define SVTK_PYTHON_SET_ARRAY_ARG(T)                                                                \
  bool svtkPythonArgs::SetArray(int i, const T* a, size_t n)                                        \
  {                                                                                                \
    if (this->M + i < this->N)                                                                     \
    {                                                                                              \
      PyObject* o = PyTuple_GET_ITEM(this->Args, this->M + i);                                     \
      if (svtkPythonSetArray(o, a, n))                                                              \
      {                                                                                            \
        return true;                                                                               \
      }                                                                                            \
      this->RefineArgTypeError(i);                                                                 \
      return false;                                                                                \
    }                                                                                              \
    return true;                                                                                   \
  }

SVTK_PYTHON_SET_ARRAY_ARG(bool)
SVTK_PYTHON_SET_ARRAY_ARG(float)
SVTK_PYTHON_SET_ARRAY_ARG(double)
SVTK_PYTHON_SET_ARRAY_ARG(char)
SVTK_PYTHON_SET_ARRAY_ARG(signed char)
SVTK_PYTHON_SET_ARRAY_ARG(unsigned char)
SVTK_PYTHON_SET_ARRAY_ARG(short)
SVTK_PYTHON_SET_ARRAY_ARG(unsigned short)
SVTK_PYTHON_SET_ARRAY_ARG(int)
SVTK_PYTHON_SET_ARRAY_ARG(unsigned int)
SVTK_PYTHON_SET_ARRAY_ARG(long)
SVTK_PYTHON_SET_ARRAY_ARG(unsigned long)
SVTK_PYTHON_SET_ARRAY_ARG(long long)
SVTK_PYTHON_SET_ARRAY_ARG(unsigned long long)

//--------------------------------------------------------------------
// Define all the SetArgValue methods for setting multi-dim array args

#define SVTK_PYTHON_SET_NARRAY_ARG(T)                                                               \
  bool svtkPythonArgs::SetNArray(int i, const T* a, int ndim, const size_t* dims)                   \
  {                                                                                                \
    if (this->M + i < this->N)                                                                     \
    {                                                                                              \
      PyObject* o = PyTuple_GET_ITEM(this->Args, this->M + i);                                     \
      if (svtkPythonSetNArray(o, a, ndim, dims))                                                    \
      {                                                                                            \
        return true;                                                                               \
      }                                                                                            \
      this->RefineArgTypeError(i);                                                                 \
      return false;                                                                                \
    }                                                                                              \
    return true;                                                                                   \
  }

SVTK_PYTHON_SET_NARRAY_ARG(bool)
SVTK_PYTHON_SET_NARRAY_ARG(float)
SVTK_PYTHON_SET_NARRAY_ARG(double)
SVTK_PYTHON_SET_NARRAY_ARG(char)
SVTK_PYTHON_SET_NARRAY_ARG(signed char)
SVTK_PYTHON_SET_NARRAY_ARG(unsigned char)
SVTK_PYTHON_SET_NARRAY_ARG(short)
SVTK_PYTHON_SET_NARRAY_ARG(unsigned short)
SVTK_PYTHON_SET_NARRAY_ARG(int)
SVTK_PYTHON_SET_NARRAY_ARG(unsigned int)
SVTK_PYTHON_SET_NARRAY_ARG(long)
SVTK_PYTHON_SET_NARRAY_ARG(unsigned long)
SVTK_PYTHON_SET_NARRAY_ARG(long long)
SVTK_PYTHON_SET_NARRAY_ARG(unsigned long long)

//--------------------------------------------------------------------
// Replace the contents of an argument, arg[:] = seq
bool svtkPythonArgs::SetContents(int i, PyObject* seq)
{
  if (this->M + i < this->N)
  {
    PyObject* o = PyTuple_GET_ITEM(this->Args, this->M + i);
    Py_ssize_t l = PySequence_Size(o);
    if (l >= 0 && PySequence_SetSlice(o, 0, l, seq) != -1)
    {
      return true;
    }
    this->RefineArgTypeError(i);
    return false;
  }
  return true;
}

//--------------------------------------------------------------------
// Raise an exception about incorrect arg count.
bool svtkPythonArgs::ArgCountError(Py_ssize_t m, Py_ssize_t n)
{
  char text[256];
  const char* name = this->MethodName;
  Py_ssize_t nargs = this->N;

  snprintf(text, sizeof(text),
    "%.200s%s takes %s %" PY_FORMAT_SIZE_T "d argument%s (%" PY_FORMAT_SIZE_T "d given)",
    (name ? name : "function"), (name ? "()" : ""),
    ((m == n) ? "exactly" : ((nargs < m) ? "at least" : "at most")), ((nargs < m) ? m : n),
    ((((nargs < m) ? m : n)) == 1 ? "" : "s"), nargs);
  PyErr_SetString(PyExc_TypeError, text);
  return false;
}

//--------------------------------------------------------------------
// Static method to write an arg count error.
bool svtkPythonArgs::ArgCountError(Py_ssize_t n, const char* name)
{
  char text[256];

  snprintf(text, sizeof(text), "no overloads of %.200s%s take %" PY_FORMAT_SIZE_T "d argument%s",
    (name ? name : "function"), (name ? "()" : ""), n, (n == 1 ? "" : "s"));
  PyErr_SetString(PyExc_TypeError, text);
  return false;
}

//--------------------------------------------------------------------
// Static method to raise an exception on a failed precondition.
bool svtkPythonArgs::PrecondError(const char* ctext)
{
  char text[256];

  snprintf(text, sizeof(text), "expects %.200s", ctext);
  PyErr_SetString(PyExc_ValueError, text);
  return false;
}

//--------------------------------------------------------------------
// Raise an exception about pure virtual method call
bool svtkPythonArgs::PureVirtualError()
{
  char text[256];

  snprintf(text, sizeof(text), "pure virtual method %.200s() was called", this->MethodName);
  PyErr_SetString(PyExc_TypeError, text);
  return false;
}

//--------------------------------------------------------------------
// Refine an error by saying what argument it is for
bool svtkPythonArgs::RefineArgTypeError(Py_ssize_t i)
{
  if (PyErr_ExceptionMatches(PyExc_TypeError) || PyErr_ExceptionMatches(PyExc_ValueError) ||
    PyErr_ExceptionMatches(PyExc_OverflowError))
  {
    PyObject* exc;
    PyObject *val, *newval;
    PyObject* frame;

    PyErr_Fetch(&exc, &val, &frame);

#ifdef SVTK_PY3K
    const char* cp = "";
    if (val && !PyUnicode_Check(val))
    {
      Py_DECREF(val);
      val = 0;
    }
    newval = PyUnicode_FromFormat(
      "%s argument %" PY_FORMAT_SIZE_T "d: %V", this->MethodName, i + 1, val, cp);
#else
    const char* cp = "";
    if (val && PyString_Check(val))
    {
      cp = PyString_AsString(val);
    }
    newval =
      PyString_FromFormat("%s argument %" PY_FORMAT_SIZE_T "d: %s", this->MethodName, i + 1, cp);
#endif

    Py_XDECREF(val);
    PyErr_Restore(exc, newval, frame);
  }
  return false;
}

//--------------------------------------------------------------------
// Raise a type error for a sequence arg of wrong type or size.
bool svtkPythonSequenceError(PyObject* o, size_t n, size_t m)
{
  char text[80];
  if (m == n)
  {
    snprintf(text, sizeof(text), "expected a sequence of %lld value%s, got %s",
      static_cast<long long>(n), ((n == 1) ? "" : "s"), Py_TYPE(o)->tp_name);
  }
  else
  {
    snprintf(text, sizeof(text), "expected a sequence of %lld value%s, got %lld values",
      static_cast<long long>(n), ((n == 1) ? "" : "s"), static_cast<long long>(m));
  }
  PyErr_SetString(PyExc_TypeError, text);
  return false;
}

//--------------------------------------------------------------------
// Checking size of array arg.
size_t svtkPythonArgs::GetArgSize(int i)
{
  size_t size = 0;
  if (this->M + i < this->N)
  {
    PyObject* o = PyTuple_GET_ITEM(this->Args, this->M + i);
    if (PySequence_Check(o))
    {
      size = PySequence_Size(o);
    }
  }
  return size;
}

//--------------------------------------------------------------------
// Checking size of string arg.
size_t svtkPythonArgs::GetStringSize(int i)
{
  size_t size = 0;
  if (this->M + i < this->N)
  {
    PyObject* o = PyTuple_GET_ITEM(this->Args, this->M + i);
    size = svtkPythonGetStringSize(o);
    if (size == 0 && PySequence_Check(o))
    {
      size = PySequence_Size(o);
    }
  }
  return size;
}

//--------------------------------------------------------------------
// Check if 'm' equals 'n', and report an error for arg i if not.
bool svtkPythonArgs::CheckSizeHint(int i, size_t m, size_t n)
{
  if (this->M + i < this->N)
  {
    if (m != n)
    {
      PyObject* o = PyTuple_GET_ITEM(this->Args, this->M + i);
      return svtkPythonSequenceError(o, n, m);
    }
  }
  return true;
}

//--------------------------------------------------------------------
// Use stack space for small arrays, heap for large arrays.
template <class T>
svtkPythonArgs::Array<T>::Array(size_t n)
  : Pointer(nullptr)
{
  if (n > basicsize)
  {
    this->Pointer = new T[n];
  }
  else if (n != 0)
  {
    this->Pointer = this->Storage;
  }
}

// Instantiate the Array class template over all types:
svtkPythonArgsTemplateMacro(template class SVTKWRAPPINGPYTHONCORE_EXPORT svtkPythonArgs::Array);
