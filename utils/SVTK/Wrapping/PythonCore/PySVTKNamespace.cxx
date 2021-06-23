/*=========================================================================

  Program:   Visualization Toolkit
  Module:    PySVTKNamespace.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-----------------------------------------------------------------------
  The PySVTKNamespace was created in Nov 2014 by David Gobbi.

  This is a PyModule subclass for wrapping C++ namespaces.
-----------------------------------------------------------------------*/

#include "PySVTKNamespace.h"
#include "svtkPythonUtil.h"

// Silence warning like
// "dereferencing type-punned pointer will break strict-aliasing rules"
// it happens because this kind of expression: (long *)&ptr
#if defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif

//--------------------------------------------------------------------

static const char* PySVTKNamespace_Doc = "A python module that wraps a C++ namespace.\n";

//--------------------------------------------------------------------
static void PySVTKNamespace_Delete(PyObject* op)
{
  // remove from the map so that there is no dangling reference
  svtkPythonUtil::RemoveNamespaceFromMap(op);
  // call the superclass destructor
  PySVTKNamespace_Type.tp_base->tp_dealloc(op);
}

//--------------------------------------------------------------------
// clang-format off
PyTypeObject PySVTKNamespace_Type = {
  PyVarObject_HEAD_INIT(&PyType_Type, 0)
  "svtkmodules.svtkCommonCore.namespace", // tp_name
  0,                  // tp_basicsize
  0,                  // tp_itemsize
  PySVTKNamespace_Delete, // tp_dealloc
#if PY_VERSION_HEX >= 0x03080000
  0,                  // tp_vectorcall_offset
#else
  nullptr,            // tp_print
#endif
  nullptr,            // tp_getattr
  nullptr,            // tp_setattr
  nullptr,            // tp_compare
  nullptr,            // tp_repr
  nullptr,            // tp_as_number
  nullptr,            // tp_as_sequence
  nullptr,            // tp_as_mapping
  nullptr,            // tp_hash
  nullptr,            // tp_call
  nullptr,            // tp_string
  nullptr,            // tp_getattro
  nullptr,            // tp_setattro
  nullptr,            // tp_as_buffer
  Py_TPFLAGS_DEFAULT, // tp_flags
  PySVTKNamespace_Doc, // tp_doc
  nullptr,            // tp_traverse
  nullptr,            // tp_clear
  nullptr,            // tp_richcompare
  0,                  // tp_weaklistoffset
  nullptr,            // tp_iter
  nullptr,            // tp_iternext
  nullptr,            // tp_methods
  nullptr,            // tp_members
  nullptr,            // tp_getset
  &PyModule_Type,     // tp_base
  nullptr,            // tp_dict
  nullptr,            // tp_descr_get
  nullptr,            // tp_descr_set
  0,                  // tp_dictoffset
  nullptr,            // tp_init
  nullptr,            // tp_alloc
  nullptr,            // tp_new
  nullptr,            // tp_free
  nullptr,            // tp_is_gc
  nullptr,            // tp_bases
  nullptr,            // tp_mro
  nullptr,            // tp_cache
  nullptr,            // tp_subclasses
  nullptr,            // tp_weaklist
  SVTK_WRAP_PYTHON_SUPPRESS_UNINITIALIZED };
// clang-format on

//--------------------------------------------------------------------
PyObject* PySVTKNamespace_New(const char* name)
{
  // first check to see if this namespace exists
  PyObject* self = svtkPythonUtil::FindNamespace(name);
  if (self)
  {
    Py_INCREF(self);
  }
  else
  {
    // make sure python has readied the type object
    PyType_Ready(&PySVTKNamespace_Type);
    // call the allocator provided by python for this type
    self = PySVTKNamespace_Type.tp_alloc(&PySVTKNamespace_Type, 0);
    // call the superclass init function
    PyObject* args = PyTuple_New(1);
    PyTuple_SET_ITEM(args, 0, PyString_FromString(name));
    PySVTKNamespace_Type.tp_base->tp_init(self, args, nullptr);
    Py_DECREF(args);
    // remember the object for later reference
    svtkPythonUtil::AddNamespaceToMap(self);
  }
  return self;
}

//--------------------------------------------------------------------
PyObject* PySVTKNamespace_GetDict(PyObject* self)
{
  return PyModule_GetDict(self);
}

//--------------------------------------------------------------------
const char* PySVTKNamespace_GetName(PyObject* self)
{
  return PyModule_GetName(self);
}
