/*=========================================================================

  Program:   Visualization Toolkit
  Module:    PySVTKSpecialObject.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-----------------------------------------------------------------------
  The PySVTKSpecialObject was created in Feb 2001 by David Gobbi.
  The PySVTKSpecialType class was created in April 2010 by David Gobbi.
-----------------------------------------------------------------------*/

#ifndef PySVTKSpecialObject_h
#define PySVTKSpecialObject_h

#include "svtkPython.h"
#include "svtkSystemIncludes.h"
#include "svtkWrappingPythonCoreModule.h" // For export macro

// This for objects not derived from svtkObjectBase

// Prototypes for per-type copy, delete, and print funcs

// copy the object and return the copy
typedef void* (*svtkcopyfunc)(const void*);

// Because the PyTypeObject can't hold all the typing information that we
// need, we use this PySVTKSpecialType class to hold a bit of extra info.
class SVTKWRAPPINGPYTHONCORE_EXPORT PySVTKSpecialType
{
public:
  PySVTKSpecialType()
    : py_type(nullptr)
    , svtk_methods(nullptr)
    , svtk_constructors(nullptr)
    , svtk_copy(nullptr)
  {
  }

  PySVTKSpecialType(
    PyTypeObject* typeobj, PyMethodDef* cmethods, PyMethodDef* ccons, svtkcopyfunc copyfunc);

  // general information
  PyTypeObject* py_type;
  PyMethodDef* svtk_methods;
  PyMethodDef* svtk_constructors;
  // copy an object
  svtkcopyfunc svtk_copy;
};

// The PySVTKSpecialObject is very lightweight.  All special SVTK types
// that are wrapped in SVTK use this struct, they do not define their
// own structs.
struct PySVTKSpecialObject
{
  PyObject_HEAD PySVTKSpecialType* svtk_info;
  void* svtk_ptr;
  long svtk_hash;
};

extern "C"
{
  SVTKWRAPPINGPYTHONCORE_EXPORT
  PyTypeObject* PySVTKSpecialType_Add(
    PyTypeObject* pytype, PyMethodDef* methods, PyMethodDef* constructors, svtkcopyfunc copyfunc);

  SVTKWRAPPINGPYTHONCORE_EXPORT
  PyObject* PySVTKSpecialObject_New(const char* classname, void* ptr);

  SVTKWRAPPINGPYTHONCORE_EXPORT
  PyObject* PySVTKSpecialObject_CopyNew(const char* classname, const void* ptr);

  SVTKWRAPPINGPYTHONCORE_EXPORT
  PyObject* PySVTKSpecialObject_Repr(PyObject* self);

  SVTKWRAPPINGPYTHONCORE_EXPORT
  PyObject* PySVTKSpecialObject_SequenceString(PyObject* self);
}

#endif
