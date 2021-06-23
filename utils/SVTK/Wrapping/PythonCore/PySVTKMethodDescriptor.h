/*=========================================================================

  Program:   Visualization Toolkit
  Module:    PySVTKMethodDescriptor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-----------------------------------------------------------------------
  The PySVTKMethodDescriptor was created in July 2015 by David Gobbi.

  Python's built-in method descriptor can only be used for non-static
  method calls.  SVTK, however, has many methods where one signature of
  the method is static and another signature of the method is not. In
  order to wrap these methods, a custom method descriptor is needed.
-----------------------------------------------------------------------*/

#ifndef PySVTKMethodDescriptor_h
#define PySVTKMethodDescriptor_h

#include "svtkPython.h"
#include "svtkSystemIncludes.h"
#include "svtkWrappingPythonCoreModule.h" // For export macro

extern SVTKWRAPPINGPYTHONCORE_EXPORT PyTypeObject PySVTKMethodDescriptor_Type;

#define PySVTKMethodDescriptor_Check(obj) (Py_TYPE(obj) == &PySVTKMethodDescriptor_Type)

extern "C"
{
  // Create a new method descriptor from a PyMethodDef.
  SVTKWRAPPINGPYTHONCORE_EXPORT
  PyObject* PySVTKMethodDescriptor_New(PyTypeObject* cls, PyMethodDef* meth);
}

#endif
