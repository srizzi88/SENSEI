/*=========================================================================

  Program:   Visualization Toolkit
  Module:    PySVTKSpecialObject.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-----------------------------------------------------------------------
  The PySVTKSpecialObject was created in Feb 2001 by David Gobbi.
  It was substantially updated in April 2010 by David Gobbi.

  A PySVTKSpecialObject is a python object that represents an object
  that belongs to one of the special classes in SVTK, that is, classes
  that are not derived from svtkObjectBase.  Unlike svtkObjects, these
  special objects are not reference counted: a PySVTKSpecialObject
  always contains its own copy of the C++ object.

  The PySVTKSpecialType is a simple structure that contains information
  about the PySVTKSpecialObject type that cannot be stored in python's
  PyTypeObject struct.  Each PySVTKSpecialObject contains a pointer to
  its PySVTKSpecialType. The PySVTKSpecialTypes are also stored in a map
  in svtkPythonUtil.cxx, so that they can be lookup up by name.
-----------------------------------------------------------------------*/

#include "PySVTKSpecialObject.h"
#include "PySVTKMethodDescriptor.h"
#include "svtkPythonUtil.h"

#include <sstream>

// Silence warning like
// "dereferencing type-punned pointer will break strict-aliasing rules"
// it happens because this kind of expression: (long *)&ptr
#if defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif

//--------------------------------------------------------------------
PySVTKSpecialType::PySVTKSpecialType(
  PyTypeObject* typeobj, PyMethodDef* cmethods, PyMethodDef* ccons, svtkcopyfunc copyfunc)
{
  this->py_type = typeobj;
  this->svtk_methods = cmethods;
  this->svtk_constructors = ccons;
  this->svtk_copy = copyfunc;
}

//--------------------------------------------------------------------
// Object protocol

//--------------------------------------------------------------------
PyObject* PySVTKSpecialObject_Repr(PyObject* self)
{
  PySVTKSpecialObject* obj = (PySVTKSpecialObject*)self;
  PyTypeObject* type = Py_TYPE(self);
  const char* name = Py_TYPE(self)->tp_name;

  while (type->tp_base && !type->tp_str)
  {
    type = type->tp_base;
  }

  // use str() if available
  PyObject* s = nullptr;
  if (type->tp_str && type->tp_str != (&PyBaseObject_Type)->tp_str)
  {
    PyObject* t = type->tp_str(self);
    if (t == nullptr)
    {
      Py_XDECREF(s);
      s = nullptr;
    }
    else
    {
#ifdef SVTK_PY3K
      s = PyString_FromFormat("(%.80s)%S", name, t);
#else
      s = PyString_FromFormat("(%.80s)%s", name, PyString_AsString(t));
#endif
    }
  }
  // otherwise just print address of object
  else if (obj->svtk_ptr)
  {
    s = PyString_FromFormat("(%.80s)%p", name, obj->svtk_ptr);
  }

  return s;
}

//--------------------------------------------------------------------
PyObject* PySVTKSpecialObject_SequenceString(PyObject* self)
{
  Py_ssize_t n, i;
  PyObject* s = nullptr;
  PyObject *t, *o, *comma;
  const char* bracket = "[...]";

  if (Py_TYPE(self)->tp_as_sequence && Py_TYPE(self)->tp_as_sequence->sq_item != nullptr &&
    Py_TYPE(self)->tp_as_sequence->sq_ass_item == nullptr)
  {
    bracket = "(...)";
  }

  i = Py_ReprEnter(self);
  if (i < 0)
  {
    return nullptr;
  }
  else if (i > 0)
  {
    return PyString_FromString(bracket);
  }

  n = PySequence_Size(self);
  if (n >= 0)
  {
    comma = PyString_FromString(", ");
    s = PyString_FromStringAndSize(bracket, 1);

    for (i = 0; i < n && s != nullptr; i++)
    {
      if (i > 0)
      {
#ifdef SVTK_PY3K
        PyObject* tmp = PyUnicode_Concat(s, comma);
        Py_DECREF(s);
        s = tmp;
#else
        PyString_Concat(&s, comma);
#endif
      }
      o = PySequence_GetItem(self, i);
      t = nullptr;
      if (o)
      {
        t = PyObject_Repr(o);
        Py_DECREF(o);
      }
      if (t)
      {
#ifdef SVTK_PY3K
        PyObject* tmp = PyUnicode_Concat(s, t);
        Py_DECREF(s);
        Py_DECREF(t);
        s = tmp;
#else
        PyString_ConcatAndDel(&s, t);
#endif
      }
      else
      {
        Py_DECREF(s);
        s = nullptr;
      }
      n = PySequence_Size(self);
    }

    if (s)
    {
#ifdef SVTK_PY3K
      PyObject* tmp1 = PyString_FromStringAndSize(&bracket[4], 1);
      PyObject* tmp2 = PyUnicode_Concat(s, tmp1);
      Py_DECREF(s);
      Py_DECREF(tmp1);
      s = tmp2;
#else
      PyString_ConcatAndDel(&s, PyString_FromStringAndSize(&bracket[4], 1));
#endif
    }

    Py_DECREF(comma);
  }

  Py_ReprLeave(self);

  return s;
}

//--------------------------------------------------------------------
// C API

//--------------------------------------------------------------------
// Create a new python object from the pointer to a C++ object
PyObject* PySVTKSpecialObject_New(const char* classname, void* ptr)
{
  // would be nice if "info" could be passed instead if "classname",
  // but this way of doing things is more dynamic if less efficient
  PySVTKSpecialType* info = svtkPythonUtil::FindSpecialType(classname);

  PySVTKSpecialObject* self = PyObject_New(PySVTKSpecialObject, info->py_type);

  self->svtk_info = info;
  self->svtk_ptr = ptr;
  self->svtk_hash = -1;

  return (PyObject*)self;
}

//--------------------------------------------------------------------
// Create a new python object via the copy constructor of the C++ object
PyObject* PySVTKSpecialObject_CopyNew(const char* classname, const void* ptr)
{
  PySVTKSpecialType* info = svtkPythonUtil::FindSpecialType(classname);

  if (info == nullptr)
  {
    return PyErr_Format(PyExc_ValueError, "cannot create object of unknown type \"%s\"", classname);
  }
  else if (info->svtk_copy == nullptr)
  {
    return PyErr_Format(
      PyExc_ValueError, "no copy constructor for object of type \"%s\"", classname);
  }

  PySVTKSpecialObject* self = PyObject_New(PySVTKSpecialObject, info->py_type);

  self->svtk_info = info;
  self->svtk_ptr = info->svtk_copy(ptr);
  self->svtk_hash = -1;

  return (PyObject*)self;
}

//--------------------------------------------------------------------
// Add a special type, add methods and members to its type object.
// A return value of nullptr signifies that it was already added.
PyTypeObject* PySVTKSpecialType_Add(
  PyTypeObject* pytype, PyMethodDef* methods, PyMethodDef* constructors, svtkcopyfunc copyfunc)
{
  // Check whether the type is already in the map (use classname as key),
  // and return it if so.  If not, then add it to the map.
  pytype = svtkPythonUtil::AddSpecialTypeToMap(pytype, methods, constructors, copyfunc);

  // If type object already has a dict, we're done
  if (pytype->tp_dict)
  {
    return pytype;
  }

  // Create the dict
  pytype->tp_dict = PyDict_New();

  // Add all of the methods
  for (PyMethodDef* meth = methods; meth && meth->ml_name; meth++)
  {
    PyObject* func = PySVTKMethodDescriptor_New(pytype, meth);
    PyDict_SetItemString(pytype->tp_dict, meth->ml_name, func);
    Py_DECREF(func);
  }

  return pytype;
}
