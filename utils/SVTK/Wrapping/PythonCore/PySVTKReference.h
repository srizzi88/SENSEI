/*=========================================================================

  Program:   Visualization Toolkit
  Module:    PySVTKReference.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-----------------------------------------------------------------------
  The PySVTKReference was created in Sep 2010 by David Gobbi.

  This class is a proxy for python int and float, it allows these objects
  to be passed to SVTK methods that require a ref to a numeric type.
-----------------------------------------------------------------------*/

#ifndef PySVTKReference_h
#define PySVTKReference_h

#include "svtkPython.h"
#include "svtkSystemIncludes.h"
#include "svtkWrappingPythonCoreModule.h" // For export macro

// The PySVTKReference is a wrapper around a PyObject of
// type int or float.
struct PySVTKReference
{
  PyObject_HEAD PyObject* value;
};

extern SVTKWRAPPINGPYTHONCORE_EXPORT PyTypeObject PySVTKReference_Type;
extern SVTKWRAPPINGPYTHONCORE_EXPORT PyTypeObject PySVTKNumberReference_Type;
extern SVTKWRAPPINGPYTHONCORE_EXPORT PyTypeObject PySVTKStringReference_Type;
extern SVTKWRAPPINGPYTHONCORE_EXPORT PyTypeObject PySVTKTupleReference_Type;

#define PySVTKReference_Check(obj) PyObject_TypeCheck(obj, &PySVTKReference_Type)

extern "C"
{
  // Set the value held by a mutable object.  It steals the reference
  // of the provided value.  Only float, long, and int are allowed.
  // A return value of -1 indicates than an error occurred.
  SVTKWRAPPINGPYTHONCORE_EXPORT
  int PySVTKReference_SetValue(PyObject* self, PyObject* val);

  // Get the value held by a mutable object.  A borrowed reference
  // is returned.
  SVTKWRAPPINGPYTHONCORE_EXPORT
  PyObject* PySVTKReference_GetValue(PyObject* self);
}

#endif
