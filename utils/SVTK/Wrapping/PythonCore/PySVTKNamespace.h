/*=========================================================================

  Program:   Visualization Toolkit
  Module:    PySVTKNamespace.h

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

#ifndef PySVTKNamespace_h
#define PySVTKNamespace_h

#include "svtkPython.h"
#include "svtkSystemIncludes.h"
#include "svtkWrappingPythonCoreModule.h" // For export macro

extern SVTKWRAPPINGPYTHONCORE_EXPORT PyTypeObject PySVTKNamespace_Type;

#define PySVTKNamespace_Check(obj) (Py_TYPE(obj) == &PySVTKNamespace_Type)

extern "C"
{
  SVTKWRAPPINGPYTHONCORE_EXPORT
  PyObject* PySVTKNamespace_New(const char* name);

  SVTKWRAPPINGPYTHONCORE_EXPORT
  PyObject* PySVTKNamespace_GetDict(PyObject* self);

  SVTKWRAPPINGPYTHONCORE_EXPORT
  const char* PySVTKNamespace_GetName(PyObject* self);
}

#endif
